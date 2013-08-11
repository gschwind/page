/*
 * page.cxx
 *
 * copyright (2010) Benoit Gschwind
 *
 */

/* According to POSIX.1-2001 */
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <X11/keysymdef.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrandr.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <stdlib.h>
#include <cstring>

#include <sstream>
#include <limits>
#include <stdint.h>
#include <stdexcept>
#include <set>
#include <stack>
#include <vector>
#include <typeinfo>

#include "page.hxx"

/* ICCCM definition */
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

/** xprop exec src **/
extern "C"	int __main(int argc, char ** argv);
extern "C" void Show_All_Props2 (Display * _dpy, Window _w);
extern "C" void init_xprop(Display * __dpy);

using namespace std;

namespace page {

long int const ClientEventMask = (StructureNotifyMask | PropertyChangeMask);

page_t::page_t(int argc, char ** argv) : viewport_outputs() {

	use_internal_compositor = true;
	char const * conf_file_name = 0;

	/** parse command line **/

	int k = 1;
	while(k < argc) {
		string x = argv[k];
		if(x == "--disable-compositor") {
			use_internal_compositor = false;
		} else {
			conf_file_name = argv[k];
		}
		++k;
	}

	process_mode = PROCESS_NORMAL;

	cnx = new xconnection_t();

	rnd = 0;

	cursor = XCreateFontCursor(cnx->dpy, XC_arrow);
	cursor_fleur = XCreateFontCursor(cnx->dpy, XC_fleur);

	set_window_cursor(cnx->get_root_window(), cursor);

	running = false;

	/* load configurations, from lower priority to high one */

	/* load default configuration */
	conf.merge_from_file_if_exist(string(DATA_DIR "/page/page.conf"));

	/* load homedir configuration */
	{
		char const * chome = getenv("HOME");
		if(chome != NULL) {
			string xhome = chome;
			string file = xhome + "/.page.conf";
			conf.merge_from_file_if_exist(file);
		}
	}

	/* load file in arguments if provided */
	if (conf_file_name != 0) {
		conf.merge_from_file_if_exist(string(conf_file_name));
	}

	default_window_pop = 0;

	page_base_dir = conf.get_string("default", "theme_dir");

	font = conf.get_string("default", "font_file");

	font_bold = conf.get_string("default", "font_bold_file");

	_last_focus_time = 0;
	_last_button_press = 0;

	theme = 0;

	rpage = 0;

	_client_focused.push_front(0);


}

page_t::~page_t() {

	delete rpage;
	if (theme != 0)
		delete theme;
	if (rnd != 0)
		delete rnd;
	delete cnx;

}

void page_t::run() {

	init_xprop(cnx->dpy);

//	printf("root size: %d,%d\n", cnx->get_root_size().w, cnx->get_root_size().h);

	/* create an invisile window to identify page */
	wm_window = XCreateSimpleWindow(cnx->dpy, cnx->get_root_window(), -100, -100, 1, 1, 0, 0,
			0);
	std::string name("page");
	cnx->change_property(wm_window, _NET_WM_NAME, UTF8_STRING, 8,
	PropModeReplace, reinterpret_cast<unsigned char const *>(name.c_str()),
			name.length() + 1);
	cnx->change_property(wm_window, _NET_SUPPORTING_WM_CHECK, WINDOW, 32,
			PropModeReplace, reinterpret_cast<unsigned char *>(&wm_window), 1);
	cnx->change_property(cnx->get_root_window(), _NET_SUPPORTING_WM_CHECK,
			WINDOW, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&wm_window), 1);

	long pid = getpid();

	cnx->change_property(wm_window, _NET_WM_PID, CARDINAL,
			32, (PropModeReplace),
			reinterpret_cast<unsigned char const *>(&pid), 1);

	cnx->select_input(wm_window, StructureNotifyMask);


	if(!cnx->register_wm(true, wm_window)) {
		printf("Cannot register window manager\n");
		return;
	}

	/** load compositor if requested **/
	if (use_internal_compositor) {
		/** try to start compositor, if fail, just ignore it **/
		try {
			rnd = new compositor_t();
		} catch (...) {
			rnd = 0;
		}
	}

	theme = new simple_theme_t(cnx, conf);

	/* init page render */
	rpage = new renderable_page_t(cnx, theme,
			cnx->get_root_size().w, cnx->get_root_size().h);

	/* create and add popups (overlay) */
	pfm = new popup_frame_move_t(cnx, theme);
	pn0 = new popup_notebook0_t(cnx, theme);
	pn1 = new popup_notebook1_t(cnx, theme->get_default_font());
	ps = new popup_split_t(cnx);


	default_window_pop = 0;


	/**
	 * listen RRCrtcChangeNotifyMask for possible change in screen layout.
	 **/
	XRRSelectInput(cnx->dpy, cnx->get_root_window(), RRCrtcChangeNotifyMask);

	rr_update_viewport_layout();

	default_window_pop = get_another_notebook();
	if(default_window_pop == 0)
		throw std::runtime_error("very bad error");

	update_net_supported();

	/* update number of desktop */
	int32_t number_of_desktop = 1;
	cnx->change_property(cnx->get_root_window(), _NET_NUMBER_OF_DESKTOPS,
			CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&number_of_desktop), 1);

	/* define desktop geometry */
	long desktop_geometry[2];
	desktop_geometry[0] = cnx->get_root_size().w;
	desktop_geometry[1] = cnx->get_root_size().h;
	cnx->change_property(cnx->get_root_window(), _NET_DESKTOP_GEOMETRY,
			CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(desktop_geometry), 2);

	/* set viewport */
	long viewport[2] = { 0, 0 };
	cnx->change_property(cnx->get_root_window(), _NET_DESKTOP_VIEWPORT,
			CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(viewport), 2);

	/* set current desktop */
	long current_desktop = 0;
	cnx->change_property(cnx->get_root_window(), _NET_CURRENT_DESKTOP,
			CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&current_desktop), 1);

	long showing_desktop = 0;
	cnx->change_property(cnx->get_root_window(), _NET_SHOWING_DESKTOP,
			CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&showing_desktop), 1);

	char const desktop_name[10] = "NoName";
	cnx->change_property(cnx->get_root_window(), _NET_DESKTOP_NAMES,
			UTF8_STRING, 8, PropModeReplace,
			(unsigned char *) desktop_name, (int) (strlen(desktop_name) + 2));

	XIconSize icon_size;
	icon_size.min_width = 16;
	icon_size.min_height = 16;
	icon_size.max_width = 16;
	icon_size.max_height = 16;
	icon_size.width_inc = 1;
	icon_size.height_inc = 1;
	XSetIconSizes(cnx->dpy, cnx->get_root_window(), &icon_size, 1);

	/* setup _NET_ACTIVE_WINDOW */
	set_focus(0, 0, false);

	/* add page event handler */
	cnx->add_event_handler(this);

	scan();
	update_windows_stack();

	long workarea[4];
	workarea[0] = 0;
	workarea[1] = 0;
	workarea[2] = cnx->get_root_size().w;
	workarea[3] = cnx->get_root_size().h;
	cnx->change_property(cnx->get_root_window(), _NET_WORKAREA, CARDINAL,
			32, PropModeReplace, reinterpret_cast<unsigned char*>(workarea), 4);

	rpage->mark_durty();

	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_f), Mod4Mask, cnx->get_root_window(),
			True, GrabModeAsync, GrabModeAsync);
	/* quit page */
	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_q), Mod4Mask, cnx->get_root_window(),
			True, GrabModeAsync, GrabModeAsync);

	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_r), Mod4Mask, cnx->get_root_window(),
			True, GrabModeAsync, GrabModeAsync);

	/* print state info */
	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_s), Mod4Mask, cnx->get_root_window(),
			True, GrabModeAsync, GrabModeAsync);
	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_Tab), Mod1Mask, cnx->get_root_window(),
			True, GrabModeAsync, GrabModeAsync);

	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_w), Mod4Mask, cnx->get_root_window(),
			True, GrabModeAsync, GrabModeAsync);

	/**
	 * This grab will freeze input for all client, all mouse button, until
	 * we choose what to do with them with XAllowEvents. we can choose to keep
	 * grabbing events or release event and allow futher processing by other clients.
	 **/
	XGrabButton(cnx->dpy, AnyButton, AnyModifier, rpage->id(), False,
			ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
			GrabModeSync, GrabModeAsync, None, None);

	timespec max_wait;
	fd_set fds_read;
	fd_set fds_intr;

	/**
	 * wait for a maximum of 15 ms
	 * i.e about 60 times per second.
	 **/
	max_wait.tv_sec = 0;
	max_wait.tv_nsec = 15000000;

	int max = cnx->fd();

	if (rnd != 0) {
		max = cnx->fd() > rnd->get_connection_fd() ?
				cnx->fd() : rnd->get_connection_fd();
	}

	update_allocation();
	XFlush(cnx->dpy);
	running = true;
	while (running) {

//		FD_ZERO(&fds_read);
//		FD_ZERO(&fds_intr);
//
//		FD_SET(cnx->fd(), &fds_read);
//		FD_SET(cnx->fd(), &fds_intr);
//
//		/** listen for compositor events **/
//		if (rnd != 0) {
//			FD_SET(rnd->get_connection_fd(), &fds_read);
//			FD_SET(rnd->get_connection_fd(), &fds_intr);
//		}
//
//		/**
//		 * wait for data in both X11 connection streams (compositor and page)
//		 **/
//		int nfd = pselect(max + 1, &fds_read, NULL, &fds_intr, &max_wait, NULL);

		while (cnx->process_check_event())
			continue;
		rpage->render_if_needed(list_values(viewport_outputs));
		XFlush(cnx->dpy);

		if (rnd != 0) {
			rnd->process_events();
			rnd->xflush();
		}

		/** wait for 15 millisecond (~60 fps) **/
		usleep(15000);

	}
}

managed_window_t * page_t::manage(managed_window_type_e type, Atom net_wm_type, Window w) {
	printf("manage \n");
	cnx->add_to_save_set(w);
	//w->set_managed(true);
	//w->read_all();
	//w->set_default_action();
	//w->write_net_wm_state();

	/* set border to zero */
	XWindowChanges wc;
	wc.border_width = 0;
	XConfigureWindow(cnx->dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(cnx->dpy, w, 0);

	//w->write_net_frame_extents();

	/* assign window to desktop 0 */
	if (!cnx->read_net_wm_desktop(w)) {
		long int net_wm_desktop = 0;
		cnx->change_property(w, _NET_WM_DESKTOP,
				CARDINAL, 32, PropModeReplace,
				(unsigned char *) &net_wm_desktop, 1);

	}

	managed_window_t * mw = new managed_window_t(cnx, type, net_wm_type, w,
			theme->get_theme_layout());
	managed_window.insert(mw);

	Show_All_Props2(cnx->dpy, w);

	printf("WM_TYPE: %s\n", cnx->get_atom_name(net_wm_type).c_str());

	return mw;

}

void page_t::unmanage(managed_window_t * mw) {
//	printf("unamage\n");

	if(mw == _client_focused.front()) {
		_client_focused.remove(mw);
		if(_client_focused.front() != 0) {
			if(_client_focused.front()->is(MANAGED_NOTEBOOK)) {
				activate_client(_client_focused.front());
			} else {
				//set_focus(_client_focused.front(), true);
			}
		}

	} else {
		_client_focused.remove(mw);
	}

	if (has_key(fullscreen_client_to_viewport, mw))
		unfullscreen(mw);

	/* if window is in move/resize/notebook move, do cleanup */
	cleanup_grab(mw);

	Window base = mw->base();

	if (mw->is(MANAGED_NOTEBOOK)) {
		remove_window_from_tree(mw);
	}

	destroy_managed_window(mw);
	XDestroyWindow(cnx->dpy, base);
	update_client_list();
}

void page_t::scan() {
//	printf("call %s\n", __PRETTY_FUNCTION__);
	unsigned int num;
	Window d1, d2, *wins = 0;

	/* start listen root event before anything each event will be stored to right run later */
	cnx->select_input(cnx->get_root_window(),
	SubstructureNotifyMask | SubstructureRedirectMask | PropertyChangeMask);

	if (XQueryTree(cnx->dpy, cnx->get_root_window(), &d1, &d2, &wins, &num)) {

		for (unsigned i = 0; i < num; ++i) {
			Window w = wins[i];

			if(!cnx->get_window_attributes(w)->is_valid)
				continue;
			update_transient_for(w);

			if (cnx->get_window_attributes(w)->map_state != IsUnmapped) {
				onmap(w);
			} else {
				/**
				 * if the window is not map check if previous windows manager has set WM_STATE to iconic
				 * if this is the case, that mean that is a managed window, otherwise it is a WithDrwn window
				 **/
				long state = 0;
				if (cnx->read_wm_state(w, &state)) {
					if (state == IconicState) {
						onmap(w);
					}
				}
			}
		}
		XFree(wins);
	}

	update_client_list();
//	printf("return %s\n", __PRETTY_FUNCTION__);
}

void page_t::update_net_supported() {
	std::list<Atom> supported_list;

	supported_list.push_back(A(_NET_WM_NAME));
	supported_list.push_back(A(_NET_WM_USER_TIME));
	supported_list.push_back(A(_NET_CLIENT_LIST));
	supported_list.push_back(A(_NET_CLIENT_LIST_STACKING));
	supported_list.push_back(A(_NET_WM_STRUT_PARTIAL));
	supported_list.push_back(A(_NET_NUMBER_OF_DESKTOPS));
	supported_list.push_back(A(_NET_DESKTOP_GEOMETRY));
	supported_list.push_back(A(_NET_DESKTOP_VIEWPORT));
	supported_list.push_back(A(_NET_CURRENT_DESKTOP));
	supported_list.push_back(A(_NET_ACTIVE_WINDOW));
	supported_list.push_back(A(_NET_WM_STATE_FULLSCREEN));
	supported_list.push_back(A(_NET_WM_STATE_FOCUSED));
	supported_list.push_back(A(_NET_WM_STATE_DEMANDS_ATTENTION));
	supported_list.push_back(A(_NET_WM_STATE_HIDDEN));
	supported_list.push_back(A(_NET_REQUEST_FRAME_EXTENTS));
	supported_list.push_back(A(_NET_FRAME_EXTENTS));

	supported_list.push_back(A(_NET_WM_ALLOWED_ACTIONS));
	supported_list.push_back(A(_NET_WM_ACTION_FULLSCREEN));
	supported_list.push_back(A(_NET_WM_ACTION_ABOVE));
	supported_list.push_back(A(_NET_WM_ACTION_BELOW));
	supported_list.push_back(A(_NET_WM_ACTION_CHANGE_DESKTOP));
	supported_list.push_back(A(_NET_WM_ACTION_CLOSE));
	supported_list.push_back(A(_NET_WM_ACTION_FULLSCREEN));
	supported_list.push_back(A(_NET_WM_ACTION_MAXIMIZE_HORZ));
	supported_list.push_back(A(_NET_WM_ACTION_MAXIMIZE_VERT));
	supported_list.push_back(A(_NET_WM_ACTION_MINIMIZE));
	supported_list.push_back(A(_NET_WM_ACTION_MOVE));
	supported_list.push_back(A(_NET_WM_ACTION_RESIZE));
	supported_list.push_back(A(_NET_WM_ACTION_SHADE));
	supported_list.push_back(A(_NET_WM_ACTION_STICK));

	supported_list.push_back(A(_NET_WM_WINDOW_TYPE));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_COMBO));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_DESKTOP));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_DIALOG));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_DND));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_DOCK));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_MENU));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_NORMAL));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_NOTIFICATION));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_POPUP_MENU));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_SPLASH));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_TOOLBAR));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_TOOLTIP));
	supported_list.push_back(A(_NET_WM_WINDOW_TYPE_UTILITY));

	supported_list.push_back(A(_NET_SUPPORTING_WM_CHECK));

	long * list = new long[supported_list.size()];
	int k = 0;
	for (std::list<Atom>::iterator i = supported_list.begin();
			i != supported_list.end(); ++i, ++k) {
		list[k] = *i;
	}

	cnx->change_property(cnx->get_root_window(), _NET_SUPPORTED, ATOM, 32,
			PropModeReplace, reinterpret_cast<unsigned char *>(list), k);

	delete[] list;

}

void page_t::update_client_list() {


	set<Window> s_client_list;
	set<Window> s_client_list_stack;

	list<managed_window_t *> lst;
	get_managed_windows(lst);

	for (list<managed_window_t *>::iterator i =
			lst.begin();
			i != lst.end(); ++i) {
		s_client_list.insert((*i)->orig());
		s_client_list_stack.insert((*i)->orig());
	}

	vector<Window> client_list(s_client_list.begin(), s_client_list.end());
	vector<Window> client_list_stack(s_client_list_stack.begin(), s_client_list_stack.end());

//	for (map<Window, managed_window_t *>::iterator i =
//			orig_window_to_notebook_window.begin();
//			i != orig_window_to_notebook_window.end(); ++i) {
//		client_list.push_back(i->first);
//		client_list_stack.push_back(i->first);
//	}
//
//	for (map<Window, managed_window_t *>::iterator i =
//			orig_window_to_floating_window.begin();
//			i != orig_window_to_floating_window.end(); ++i) {
//		client_list.push_back(i->first);
//		client_list_stack.push_back(i->first);
//	}

	cnx->change_property(cnx->get_root_window(), _NET_CLIENT_LIST_STACKING,
			WINDOW, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&client_list_stack[0]), client_list_stack.size());
	cnx->change_property(cnx->get_root_window(), _NET_CLIENT_LIST, WINDOW,
			32, PropModeReplace, reinterpret_cast<unsigned char *>(&client_list[0]),
			client_list.size());

}

/* inspired from dwm */
bool page_t::get_text_prop(Window w, atom_e atom, std::string & text) {
	//char **list = NULL;
	XTextProperty name;
	cnx->get_text_property(w, &name, atom);
	if (!name.nitems)
		return false;
	text = (char const *) name.value;
	return true;
}

void page_t::process_event(XKeyEvent const & e) {
	int n;
	KeySym * k = XGetKeyboardMapping(cnx->dpy, e.keycode, 1, &n);

	if (k == 0)
		return;
	if (n == 0) {
		XFree(k);
		return;
	}

//	printf("key : %x\n", (unsigned) k[0]);

	if (XK_f == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
//		if (client_focused != 0) {
//			if (has_key(orig_window_to_notebook_window, client_focused))
//				toggle_fullscreen(orig_window_to_notebook_window[client_focused]);
//		}
	}

	if (XK_q == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
		running = false;
	}

	if (XK_w == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
		//print_tree_windows();

		list<managed_window_t *> lst;
		get_managed_windows(lst);
		for (list<managed_window_t *>::iterator i = lst.begin(); i != lst.end();
				++i) {
			switch ((*i)->get_type()) {
			case MANAGED_NOTEBOOK:
				printf("[%lu] notebook : %s\n", (*i)->orig(),
						(*i)->get_title().c_str());
				break;
			case MANAGED_FLOATING:
				printf("[%lu] floating : %s\n", (*i)->orig(),
						(*i)->get_title().c_str());
				break;
			case MANAGED_FULLSCREEN:
				printf("[%lu] fullscreen : %s\n", (*i)->orig(),
						(*i)->get_title().c_str());
				break;
			}
		}

		//printf("fast_region_surf = %g (%.2f)\n", rnd->fast_region_surf, rnd->fast_region_surf / (rnd->fast_region_surf + rnd->slow_region_surf) * 100.0);
		//printf("slow_region_surf = %g (%.2f)\n", rnd->slow_region_surf, rnd->slow_region_surf / (rnd->fast_region_surf + rnd->slow_region_surf) * 100.0);


	}

	if (XK_s == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
		/* free short cut */
	}

	if (XK_r == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
		printf("rerender background\n");
		rpage->mark_durty();
	}

	if (XK_Tab == k[0] && e.type == KeyPress && (e.state & Mod1Mask)) {

		if(!managed_window.empty()) {
			managed_window_t * mw = _client_focused.front();
			set<managed_window_t *>::iterator x = managed_window.find(mw);
			if(x == managed_window.end()) {
				activate_client(*managed_window.begin());
			} else {
				++x;
				if(x == managed_window.end()) {
					activate_client(*managed_window.begin());
				} else {
					activate_client(*x);
				}
			}
		}
	}

	XFree(k);

}

/* Button event make page to grab pointer */
void page_t::process_event_press(XButtonEvent const & e) {
	fprintf(stderr, "Xpress event, window = %lu, root = %lu, subwindow = %lu, pos = (%d,%d), time = %lu\n",
			e.window, e.root, e.subwindow, e.x_root, e.y_root, e.time);
	managed_window_t * mw = find_managed_window_with(e.window);

	switch (process_mode) {
	case PROCESS_NORMAL:

		if (e.window == rpage->id() && e.subwindow == None && e.button == Button1) {
		/* split and notebook are mutually exclusive */
			if (!check_for_start_split(e)) {
				mode_data_notebook.start_x = e.x_root;
				mode_data_notebook.start_y = e.y_root;
				check_for_start_notebook(e);
			}
		}

		if (mw != 0) {
			if (mw->is(MANAGED_FLOATING)
					&& (e.subwindow == (None)
			|| (e.state & (Mod1Mask)) || (e.state & (ControlMask))) && e.button == Button1) {

				box_int_t size = mw->get_base_position();

				box_int_t close_position =
						theme->get_theme_layout()->compute_floating_close_position(
								size);
				box_int_t dock_position =
						theme->get_theme_layout()->compute_floating_bind_position(
								size);


				theme_layout_t const * layout = theme->get_theme_layout();

				/* TODO: NEED TO FIX "- 20" in top margin */
				box_int_t resize_position_top_left(size.x, size.y,
						layout->floating_margin.left,
						layout->floating_margin.top - 20);
				box_int_t resize_position_top(
						size.x + layout->floating_margin.left, size.y,
						size.w - layout->floating_margin.left
								- layout->floating_margin.right,
						layout->floating_margin.top - 20);
				box_int_t resize_position_top_right(
						size.x + size.w - layout->floating_margin.right, size.y,
						layout->floating_margin.right,
						layout->floating_margin.top - 20);

				box_int_t resize_position_left(size.x,
						size.y + layout->floating_margin.top,
						layout->floating_margin.left,
						size.h - layout->floating_margin.top
								- layout->floating_margin.bottom);
				box_int_t resize_position_right(
						size.x + size.w - layout->floating_margin.right,
						size.y + layout->floating_margin.top,
						layout->floating_margin.right,
						size.h - layout->floating_margin.top
								- layout->floating_margin.bottom);

				box_int_t resize_position_bottom_left(size.x,
						size.y + size.h - layout->floating_margin.bottom,
						layout->floating_margin.left,
						layout->floating_margin.bottom);
				box_int_t resize_position_bottom(
						size.x + layout->floating_margin.left,
						size.y + size.h - layout->floating_margin.bottom,
						size.w - layout->floating_margin.left
								- layout->floating_margin.right,
						layout->floating_margin.bottom);
				box_int_t resize_position_bottom_right(
						size.x + size.w - layout->floating_margin.right,
						size.y + size.h - layout->floating_margin.bottom,
						layout->floating_margin.right,
						layout->floating_margin.bottom);


				/* click on close button ? */
				if (close_position.is_inside(e.x_root, e.y_root)) {
					mode_data_floating.f = mw;
					process_mode = PROCESS_FLOATING_CLOSE;
				} else if (dock_position.is_inside(e.x_root, e.y_root)) {

					mode_data_bind.c = mw;
					mode_data_bind.ns = 0;
					mode_data_bind.zone = SELECT_NONE;

					pn0->move_resize(mode_data_bind.c->get_base_position());
					pn0->update_window(mw->orig());

					pn1->update_data(mw->get_icon()->get_cairo_surface(), mw->get_title());
					pn1->move(e.x_root, e.y_root);

					process_mode = PROCESS_FLOATING_BIND;

				} else {
					mode_data_floating.x_offset = e.x;
					mode_data_floating.y_offset = e.y;
					mode_data_floating.x_root = e.x_root;
					mode_data_floating.y_root = e.y_root;
					mode_data_floating.f = mw;
					mode_data_floating.original_position = mw->get_wished_position();
					mode_data_floating.final_position = mw->get_wished_position();
					mode_data_floating.popup_original_position = mw->get_base_position();

					pfm->move_resize(mw->get_base_position());
					pfm->show();

					if ((e.state & ControlMask)) {
						process_mode = PROCESS_FLOATING_RESIZE;
						mode_data_floating.mode = RESIZE_BOTTOM_RIGHT;
					} else if (resize_position_top_left.is_inside(e.x_root, e.y_root)) {
						process_mode = PROCESS_FLOATING_RESIZE;
						mode_data_floating.mode = RESIZE_TOP_LEFT;
					} else if (resize_position_top.is_inside(e.x_root, e.y_root)) {
						process_mode = PROCESS_FLOATING_RESIZE;
						mode_data_floating.mode = RESIZE_TOP;
					} else if (resize_position_top_right.is_inside(e.x_root, e.y_root)) {
						process_mode = PROCESS_FLOATING_RESIZE;
						mode_data_floating.mode = RESIZE_TOP_RIGHT;
					} else if (resize_position_left.is_inside(e.x_root, e.y_root)) {
						process_mode = PROCESS_FLOATING_RESIZE;
						mode_data_floating.mode = RESIZE_LEFT;
					} else if (resize_position_right.is_inside(e.x_root, e.y_root)) {
						process_mode = PROCESS_FLOATING_RESIZE;
						mode_data_floating.mode = RESIZE_RIGHT;
					} else if (resize_position_bottom_left.is_inside(e.x_root, e.y_root)) {
						process_mode = PROCESS_FLOATING_RESIZE;
						mode_data_floating.mode = RESIZE_BOTTOM_LEFT;
					} else if (resize_position_bottom.is_inside(e.x_root, e.y_root)) {
						process_mode = PROCESS_FLOATING_RESIZE;
						mode_data_floating.mode = RESIZE_BOTTOM;
					} else if (resize_position_bottom_right.is_inside(e.x_root, e.y_root)) {
						process_mode = PROCESS_FLOATING_RESIZE;
						mode_data_floating.mode = RESIZE_BOTTOM_RIGHT;
					} else {
						process_mode = PROCESS_FLOATING_GRAB;
					}
				}

			}
		}

		break;
	default:
		break;
	}


	if (process_mode == PROCESS_NORMAL) {

		XAllowEvents(cnx->dpy, ReplayPointer, CurrentTime);
		XFlush(cnx->dpy);

		/**
		 * This focus is anoying because, passive grab can the
		 * focus itself.
		 **/
//		if(mw == 0)
//			mw = find_managed_window_with(e.subwindow);
//
		managed_window_t * mw = find_managed_window_with(e.window);
		if (mw != 0) {
			set_focus(mw, e.time, true);
		}

//		fprintf(stderr,
//				"UnGrab event, window = %lu, root = %lu, subwindow = %lu, pos = (%d,%d), time = %lu\n",
//				e.window, e.root, e.subwindow, e.x_root, e.y_root, e.time);

	} else {
		/**
		 * if no change in process mode, focus the window under the cursor
		 * and replay events for the window this window.
		 **/
		/**
		 * keep pointer events for page.
		 * It's like we XGrabButton with GrabModeASync
		 **/

		XAllowEvents(cnx->dpy, AsyncPointer, e.time);
		XFlush(cnx->dpy);

//		fprintf(stderr,
//				"XXXXGrab event, window = %lu, root = %lu, subwindow = %lu, pos = (%d,%d), time = %lu\n",
//				e.window, e.root, e.subwindow, e.x_root, e.y_root, e.time);
//
	}

}

void page_t::process_event_release(XButtonEvent const & e) {
	fprintf(stderr, "Xrelease event, window = %lu, root = %lu, subwindow = %lu, pos = (%d,%d)\n",
			e.window, e.root, e.subwindow, e.x_root, e.y_root);

	if (e.button == Button1) {
		switch (process_mode) {
		case PROCESS_NORMAL:
			fprintf(stderr, "DDDDDDDDDDD\n");
			break;
		case PROCESS_SPLIT_GRAB:

			process_mode = PROCESS_NORMAL;

			ps->hide();

			mode_data_split.split->set_split(mode_data_split.split_ratio);
			rpage->mark_durty();

			mode_data_split.split = 0;
			mode_data_split.slider_area = box_int_t();
			mode_data_split.split_ratio = 0.5;

			break;
		case PROCESS_NOTEBOOK_GRAB:

			process_mode = PROCESS_NORMAL;

			pn0->hide();
			pn1->hide();

			if (mode_data_notebook.zone == SELECT_TAB
					&& mode_data_notebook.ns != 0
					&& mode_data_notebook.ns != mode_data_notebook.from) {
				remove_window_from_tree(mode_data_notebook.c);
				insert_window_in_tree(mode_data_notebook.c,
						mode_data_notebook.ns, true);
			} else if (mode_data_notebook.zone == SELECT_TOP
					&& mode_data_notebook.ns != 0) {
				remove_window_from_tree(mode_data_notebook.c);
				split_top(mode_data_notebook.ns, mode_data_notebook.c);
			} else if (mode_data_notebook.zone == SELECT_LEFT
					&& mode_data_notebook.ns != 0) {
				remove_window_from_tree(mode_data_notebook.c);
				split_left(mode_data_notebook.ns, mode_data_notebook.c);
			} else if (mode_data_notebook.zone == SELECT_BOTTOM
					&& mode_data_notebook.ns != 0) {
				remove_window_from_tree(mode_data_notebook.c);
				split_bottom(mode_data_notebook.ns, mode_data_notebook.c);
			} else if (mode_data_notebook.zone == SELECT_RIGHT
					&& mode_data_notebook.ns != 0) {
				remove_window_from_tree(mode_data_notebook.c);
				split_right(mode_data_notebook.ns, mode_data_notebook.c);
			} else if (mode_data_notebook.zone == SELECT_CENTER
					&& mode_data_notebook.ns != 0) {
				unbind_window(mode_data_notebook.c);
			} else {
				mode_data_notebook.from->set_selected(mode_data_notebook.c);
			}

			/* automaticaly close empty notebook */
			if (mode_data_notebook.from->_clients.empty()
					&& mode_data_notebook.from->_parent != 0) {
				notebook_close(mode_data_notebook.from);
				update_allocation();
			}

			set_focus(mode_data_notebook.c, e.time, false);
			rpage->mark_durty();

			mode_data_notebook.start_x = 0;
			mode_data_notebook.start_y = 0;
			mode_data_notebook.zone = SELECT_NONE;
			mode_data_notebook.c = 0;
			mode_data_notebook.from = 0;
			mode_data_notebook.ns = 0;

			break;
		case PROCESS_NOTEBOOK_BUTTON_PRESS:
			process_mode = PROCESS_NORMAL;

			if (mode_data_notebook.c != 0) {
				mode_data_notebook.from->update_close_area();
				if (mode_data_notebook.from->close_client_area.is_inside(
						e.x_root, e.y_root)) {
					mode_data_notebook.c->delete_window(e.time);
				} else if (mode_data_notebook.from->undck_client_area.is_inside(
						e.x_root, e.y_root)) {
					unbind_window(mode_data_notebook.c);
				}
			} else {
				if (mode_data_notebook.from != 0) {
					if (mode_data_notebook.from->button_close.is_inside(e.x,
							e.y)) {
						notebook_close(mode_data_notebook.from);
						rpage->mark_durty();
					} else if (mode_data_notebook.from->button_vsplit.is_inside(
							e.x, e.y)) {
						split(mode_data_notebook.from, VERTICAL_SPLIT);
						update_allocation();
						rpage->mark_durty();
					} else if (mode_data_notebook.from->button_hsplit.is_inside(
							e.x, e.y)) {
						split(mode_data_notebook.from, HORIZONTAL_SPLIT);
						update_allocation();
						rpage->mark_durty();
					} else if (mode_data_notebook.from->button_pop.is_inside(
							e.x, e.y)) {
						default_window_pop = mode_data_notebook.from;
						update_allocation();
						rpage->mark_durty();
					}
				}
			}

			mode_data_notebook.start_x = 0;
			mode_data_notebook.start_y = 0;
			mode_data_notebook.zone = SELECT_NONE;
			mode_data_notebook.c = 0;
			mode_data_notebook.from = 0;
			mode_data_notebook.ns = 0;

			break;
		case PROCESS_FLOATING_GRAB:

			pfm->hide();

			mode_data_floating.f->set_wished_position(
					mode_data_floating.final_position);

			set_focus(mode_data_floating.f, e.time, false);
			theme->render_floating(mode_data_floating.f,
					is_focussed(mode_data_floating.f));

			process_mode = PROCESS_NORMAL;

			mode_data_floating.mode = RESIZE_NONE;
			mode_data_floating.x_offset = 0;
			mode_data_floating.y_offset = 0;
			mode_data_floating.x_root = 0;
			mode_data_floating.y_root = 0;
			mode_data_floating.original_position = box_int_t();
			mode_data_floating.f = 0;
			mode_data_floating.popup_original_position = box_int_t();
			mode_data_floating.final_position = box_int_t();

			break;
		case PROCESS_FLOATING_RESIZE:

			pfm->hide();

			mode_data_floating.f->set_wished_position(
					mode_data_floating.final_position);
			theme->render_floating(mode_data_floating.f,
					is_focussed(mode_data_floating.f));

			set_focus(mode_data_floating.f, e.time, false);
			process_mode = PROCESS_NORMAL;

			mode_data_floating.mode = RESIZE_NONE;
			mode_data_floating.x_offset = 0;
			mode_data_floating.y_offset = 0;
			mode_data_floating.x_root = 0;
			mode_data_floating.y_root = 0;
			mode_data_floating.original_position = box_int_t();
			mode_data_floating.f = 0;
			mode_data_floating.popup_original_position = box_int_t();
			mode_data_floating.final_position = box_int_t();

			break;
		case PROCESS_FLOATING_CLOSE: {
			managed_window_t * mw = mode_data_floating.f;
			box_int_t size = mw->get_base_position();
			box_int_t close_position =
					theme->get_theme_layout()->compute_floating_close_position(
							size);
			/* click on close button ? */
			if (close_position.is_inside(e.x_root, e.y_root)) {
				mode_data_floating.f = mw;
				mw->delete_window(e.time);
			}

			/* cleanup */
			process_mode = PROCESS_NORMAL;

			mode_data_floating.mode = RESIZE_NONE;
			mode_data_floating.x_offset = 0;
			mode_data_floating.y_offset = 0;
			mode_data_floating.x_root = 0;
			mode_data_floating.y_root = 0;
			mode_data_floating.original_position = box_int_t();
			mode_data_floating.f = 0;
			mode_data_floating.popup_original_position = box_int_t();
			mode_data_floating.final_position = box_int_t();

			break;
		}

		case PROCESS_FLOATING_BIND: {

			process_mode = PROCESS_NORMAL;

			pn0->hide();
			pn1->hide();

			if (mode_data_bind.zone == SELECT_TAB && mode_data_bind.ns != 0) {
				mode_data_bind.c->set_managed_type(MANAGED_NOTEBOOK);
				insert_window_in_tree(mode_data_bind.c, mode_data_bind.ns,
						true);

				safe_raise_window(mode_data_bind.c->orig());
				rpage->mark_durty();
				update_client_list();

			} else if (mode_data_bind.zone == SELECT_TOP
					&& mode_data_bind.ns != 0) {
				mode_data_bind.c->set_managed_type(MANAGED_NOTEBOOK);
				split_top(mode_data_bind.ns, mode_data_bind.c);
			} else if (mode_data_bind.zone == SELECT_LEFT
					&& mode_data_bind.ns != 0) {
				mode_data_bind.c->set_managed_type(MANAGED_NOTEBOOK);
				split_left(mode_data_bind.ns, mode_data_bind.c);
			} else if (mode_data_bind.zone == SELECT_BOTTOM
					&& mode_data_bind.ns != 0) {
				mode_data_bind.c->set_managed_type(MANAGED_NOTEBOOK);
				split_bottom(mode_data_bind.ns, mode_data_bind.c);
			} else if (mode_data_bind.zone == SELECT_RIGHT
					&& mode_data_bind.ns != 0) {
				mode_data_bind.c->set_managed_type(MANAGED_NOTEBOOK);
				split_right(mode_data_bind.ns, mode_data_bind.c);
			} else {
				bind_window(mode_data_bind.c);
			}

			set_focus(mode_data_bind.c, e.time, false);
			rpage->mark_durty();

			process_mode = PROCESS_NORMAL;

			mode_data_bind.start_x = 0;
			mode_data_bind.start_y = 0;
			mode_data_bind.zone = SELECT_NONE;
			mode_data_bind.c = 0;
			mode_data_bind.ns = 0;

			break;
		}

		default:
			process_mode = PROCESS_NORMAL;
			break;
		}
	}

}

void page_t::process_event(XMotionEvent const & e) {

	XEvent ev;
	box_int_t old_area;
	box_int_t new_position;
	static int count = 0;
	count++;
	switch (process_mode) {
	case PROCESS_NORMAL:
		fprintf(stderr, "EEEEEEEEEE\n");
		/* should not happen */
		break;
	case PROCESS_SPLIT_GRAB:

		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev));

		if (mode_data_split.split->get_split_type() == VERTICAL_SPLIT) {
			mode_data_split.split_ratio = (ev.xmotion.x
					- mode_data_split.split->_allocation.x)
					/ (double) (mode_data_split.split->_allocation.w);
		} else {
			mode_data_split.split_ratio = (ev.xmotion.y
					- mode_data_split.split->_allocation.y)
					/ (double) (mode_data_split.split->_allocation.h);
		}

		if (mode_data_split.split_ratio > 0.95)
			mode_data_split.split_ratio = 0.95;
		if (mode_data_split.split_ratio < 0.05)
			mode_data_split.split_ratio = 0.05;

		/* Render slider with quite complex render method to avoid flickering */
		old_area = mode_data_split.slider_area;
		mode_data_split.split->compute_split_location(
				mode_data_split.split_ratio, mode_data_split.slider_area.x,
				mode_data_split.slider_area.y);


		ps->move(mode_data_split.slider_area.x, mode_data_split.slider_area.y);

		break;
	case PROCESS_NOTEBOOK_GRAB:
		{
		/* get lastest know motion event */
		ev.xmotion = e;
		while (XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev))
			continue;

		/* do not start drag&drop for small move */
		if (ev.xmotion.x_root < mode_data_notebook.start_x - 5
				|| ev.xmotion.x_root > mode_data_notebook.start_x + 5
				|| ev.xmotion.y_root < mode_data_notebook.start_y - 5
				|| ev.xmotion.y_root > mode_data_notebook.start_y + 5
				|| !mode_data_notebook.from->tab_area.is_inside(
						ev.xmotion.x_root, ev.xmotion.y_root)) {

			if(!pn0->is_visible())
				pn0->show();
			if(!pn1->is_visible())
				pn1->show();

		}

		++count;

		pn1->move(ev.xmotion.x_root + 10, ev.xmotion.y_root);

		list<notebook_t *> ln;
		get_notebooks(ln);
		for(list<notebook_t *>::iterator i = ln.begin(); i != ln.end(); ++i) {
			if ((*i)->tab_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("tab\n");
				if (mode_data_notebook.zone != SELECT_TAB
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_TAB;
					mode_data_notebook.ns = (*i);
					update_popup_position(pn0,
							(*i)->tab_area);
				}
				break;
			} else if ((*i)->right_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("right\n");
				if (mode_data_notebook.zone != SELECT_RIGHT
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_RIGHT;
					mode_data_notebook.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_right_area);
				}
				break;
			} else if ((*i)->top_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("top\n");
				if (mode_data_notebook.zone != SELECT_TOP
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_TOP;
					mode_data_notebook.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_top_area);
				}
				break;
			} else if ((*i)->bottom_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("bottom\n");
				if (mode_data_notebook.zone != SELECT_BOTTOM
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_BOTTOM;
					mode_data_notebook.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_bottom_area);
				}
				break;
			} else if ((*i)->left_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("left\n");
				if (mode_data_notebook.zone != SELECT_LEFT
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_LEFT;
					mode_data_notebook.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_left_area);
				}
				break;
			} else if ((*i)->popup_center_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("center\n");
				if (mode_data_notebook.zone != SELECT_CENTER
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_CENTER;
					mode_data_notebook.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_center_area);
				}
				break;
			}
		}
	}

		break;
	case PROCESS_FLOATING_GRAB: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev));

		/* compute new window position */
		box_int_t new_position = mode_data_floating.original_position;
		new_position.x += e.x_root - mode_data_floating.x_root;
		new_position.y += e.y_root - mode_data_floating.y_root;
		mode_data_floating.final_position = new_position;

		box_int_t popup_new_position = mode_data_floating.popup_original_position;
		popup_new_position.x += e.x_root - mode_data_floating.x_root;
		popup_new_position.y += e.y_root - mode_data_floating.y_root;
		update_popup_position(pfm, popup_new_position);

		break;
	}
	case PROCESS_FLOATING_RESIZE: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev));
		box_int_t size = mode_data_floating.original_position;

		if(mode_data_floating.mode == RESIZE_TOP_LEFT) {
			size.w -= e.x_root - mode_data_floating.x_root;
			size.h -= e.y_root - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_TOP) {
			size.h -= e.y_root - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_TOP_RIGHT) {
			size.w += e.x_root - mode_data_floating.x_root;
			size.h -= e.y_root - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_LEFT) {
			size.w -= e.x_root - mode_data_floating.x_root;
		} else if (mode_data_floating.mode == RESIZE_RIGHT) {
			size.w += e.x_root - mode_data_floating.x_root;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM_LEFT) {
			size.w -= e.x_root - mode_data_floating.x_root;
			size.h += e.y_root - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM) {
			size.h += e.y_root - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM_RIGHT) {
			size.w += e.x_root - mode_data_floating.x_root;
			size.h += e.y_root - mode_data_floating.y_root;
		}

		/* apply mornal hints */
		unsigned int final_width = size.w;
		unsigned int final_height = size.h;
		compute_client_size_with_constraint(mode_data_floating.f->orig(),
				(unsigned) size.w, (unsigned) size.h, final_width,
				final_height);
		size.w = final_width;
		size.h = final_height;

		if(size.h < 1)
			size.h = 1;
		if(size.w < 1)
			size.w = 1;

		/* do not allow to large windows */
		if(size.w > cnx->get_root_size().w - 100)
			size.w = cnx->get_root_size().w - 100;
		if(size.h > cnx->get_root_size().h - 100)
			size.h = cnx->get_root_size().h - 100;

		int x_diff = 0;
		int y_diff = 0;

		if(mode_data_floating.mode == RESIZE_TOP_LEFT) {
			x_diff = mode_data_floating.original_position.w - size.w;
			y_diff = mode_data_floating.original_position.h - size.h;
		} else if (mode_data_floating.mode == RESIZE_TOP) {
			y_diff = mode_data_floating.original_position.h - size.h;
		} else if (mode_data_floating.mode == RESIZE_TOP_RIGHT) {
			y_diff = mode_data_floating.original_position.h - size.h;
		} else if (mode_data_floating.mode == RESIZE_LEFT) {
			x_diff = mode_data_floating.original_position.w - size.w;
		} else if (mode_data_floating.mode == RESIZE_RIGHT) {

		} else if (mode_data_floating.mode == RESIZE_BOTTOM_LEFT) {
			x_diff = mode_data_floating.original_position.w - size.w;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM) {

		} else if (mode_data_floating.mode == RESIZE_BOTTOM_RIGHT) {

		}

		size.x += x_diff;
		size.y += y_diff;
		mode_data_floating.final_position = size;

		box_int_t popup_new_position = size;
		popup_new_position.x -= theme->get_theme_layout()->floating_margin.left;
		popup_new_position.y -= theme->get_theme_layout()->floating_margin.top;
		popup_new_position.w += theme->get_theme_layout()->floating_margin.left + theme->get_theme_layout()->floating_margin.right;
		popup_new_position.h += theme->get_theme_layout()->floating_margin.top + theme->get_theme_layout()->floating_margin.bottom;

		update_popup_position(pfm, popup_new_position);

		break;
	}
	case PROCESS_FLOATING_CLOSE:
		break;
	case PROCESS_FLOATING_BIND: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while (XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev))
			continue;

		/* do not start drag&drop for small move */
		if (ev.xmotion.x_root < mode_data_bind.start_x - 5
				|| ev.xmotion.x_root > mode_data_bind.start_x + 5
				|| ev.xmotion.y_root < mode_data_bind.start_y - 5
				|| ev.xmotion.y_root > mode_data_bind.start_y + 5) {

			if(!pn0->is_visible())
				pn0->show();
			if(!pn1->is_visible())
				pn1->show();
		}

		++count;

		pn1->move(ev.xmotion.x_root + 10, ev.xmotion.y_root);

		list<notebook_t *> ln;
		get_notebooks(ln);
		for (list<notebook_t *>::iterator i = ln.begin(); i != ln.end(); ++i) {
			if ((*i)->tab_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("tab\n");
				if (mode_data_bind.zone != SELECT_TAB
						|| mode_data_bind.ns != (*i)) {
					mode_data_bind.zone = SELECT_TAB;
					mode_data_bind.ns = (*i);
					update_popup_position(pn0,
							(*i)->tab_area);
				}
				break;
			} else if ((*i)->right_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("right\n");
				if (mode_data_bind.zone != SELECT_RIGHT
						|| mode_data_bind.ns != (*i)) {
					mode_data_bind.zone = SELECT_RIGHT;
					mode_data_bind.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_right_area);
				}
				break;
			} else if ((*i)->top_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("top\n");
				if (mode_data_bind.zone != SELECT_TOP
						|| mode_data_bind.ns != (*i)) {
					mode_data_bind.zone = SELECT_TOP;
					mode_data_bind.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_top_area);
				}
				break;
			} else if ((*i)->bottom_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("bottom\n");
				if (mode_data_bind.zone != SELECT_BOTTOM
						|| mode_data_bind.ns != (*i)) {
					mode_data_bind.zone = SELECT_BOTTOM;
					mode_data_bind.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_bottom_area);
				}
				break;
			} else if ((*i)->left_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("left\n");
				if (mode_data_bind.zone != SELECT_LEFT
						|| mode_data_bind.ns != (*i)) {
					mode_data_bind.zone = SELECT_LEFT;
					mode_data_bind.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_left_area);
				}
				break;
			} else if ((*i)->popup_center_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("center\n");
				if (mode_data_bind.zone != SELECT_CENTER
						|| mode_data_bind.ns != (*i)) {
					mode_data_bind.zone = SELECT_CENTER;
					mode_data_bind.ns = (*i);
					update_popup_position(pn0,
							(*i)->popup_center_area);
				}
				break;
			}
		}
	}

		break;
	default:
		break;
	}
}

void page_t::process_event(XCirculateEvent const & e) {


// this will be handled in configure events
//	Window w = e.window;
//	std::map<Window, renderable_Window>::iterator x = window_to_renderable_context.find(w);
//	if(x != window_to_renderable_context.end()) {
//		if (e.place == PlaceOnTop) {
//			rnd->raise(x->second);
//		} else if (e.place == PlaceOnBottom) {
//			rnd->lower(x->second);
//		}
//	}
}

void page_t::process_event(XConfigureEvent const & e) {
//	printf("Configure %dx%d+%d+%d above:%lu, event:%lu, window:%lu, send_event: %s \n", e.width,
//			e.height, e.x, e.y, e.above, e.event, e.window, (e.send_event == True)?"true":"false");
	if(e.send_event == True)
		return;

	if(e.window == cnx->get_root_window()) {
		update_allocation();
		rpage->move_resize(cnx->get_root_size());
		return;
	}

	Window w = e.window;

	/* enforce the valid position */
	managed_window_t * mw = find_managed_window_with(e.window);
	if (mw != 0) {
		box_int_t x(e.x, e.y, e.width, e.height);

		if(mw->orig() == w) {
			if(!mw->check_orig_position(x))
				mw->reconfigure();
		}

		if(mw->base() == w) {
			if(!mw->check_base_position(x))
				mw->reconfigure();
		}

		if(mw->is(MANAGED_FLOATING))
			theme->render_floating(mw, is_focussed(mw));

	}

}

/* track all created window */
void page_t::process_event(XCreateWindowEvent const & e) {
//	printf("Create window %lu\n", e.window);
	/* Is-it fake event? */
	if (e.send_event == True)
		return;

	//if(rnd != 0)
	//	rnd->process_event(e);

	/* Does it come from root window */
	if (e.parent != cnx->get_root_window())
		return;
	if(e.window == cnx->get_root_window())
		return;

	/* check for already created window, if the case that mean floating window */
	Window w = e.window;

	cnx->grab();

	if (!cnx->get_window_attributes(w)->is_valid) {
		cnx->ungrab();
		return;
	}

	//w->add_select_input(PropertyChangeMask | StructureNotifyMask);
	update_transient_for(w);
	//w->default_position = w->get_size();

	cnx->ungrab();

}

void page_t::process_event(XDestroyWindowEvent const & e) {

	Window c = e.window;

	managed_window_t * mw = find_managed_window_with(e.window);
	if(mw != 0) {
		unmanage(mw);
	}

	unmanaged_window_t * uw = find_unmanaged_window_with(e.window);
	if(uw != 0) {
		delete uw;
		unmanaged_window.erase(uw);
	}

	update_client_list();
	destroy(c);
	rpage->mark_durty();

}

void page_t::process_event(XGravityEvent const & e) {
	/* Ignore it, never happen ? */
}

void page_t::process_event(XMapEvent const & e) {

	/* find/create window handler */
	Window x = e.window;

	/* if map event does not occur within root, ignore it */
	if (e.event != cnx->get_root_window())
		return;

	/**
	 * Read window attributes, if fail, it's probably because
	 * the window is already destroyed.
	 **/
	if(not cnx->get_window_attributes(x)->is_valid)
		return;

	/** usefull for windows statink **/
	update_transient_for(x);

	managed_window_t * wm;
	if((wm = find_managed_window_with(e.window)) != 0) {
		//set_focus(wm, true);
		if(wm == _client_focused.front())
			wm->focus(_last_focus_time);
		wm->reconfigure();
		if(wm->is(MANAGED_FLOATING))
			theme->render_floating(wm, is_focussed(wm));
	}

	onmap(x);

	update_windows_stack();

}

void page_t::process_event(XReparentEvent const & e) {
	if(e.send_event == True)
		return;
	if(e.window == cnx->get_root_window())
		return;

	/* TODO: track reparent */
	//Window x = e.window;

}

void page_t::process_event(XUnmapEvent const & e) {

	Window x = e.window;

	bool expected_event = cnx->find_pending_event(event_t(e.serial, e.type));
	if (expected_event)
		return;

	/* if client is managed */

	managed_window_t * mw = find_managed_window_with(e.window);
	if(mw != 0) {
		cnx->reparentwindow(mw->orig(), cnx->get_root_window(), 0, 0);
		unmanage(mw);
	}

	//x->set_managed(false);
	//x->write_wm_state(WithdrawnState);

	clear_sibbling_child(x);

	update_client_list();

}

void page_t::process_event(XCirculateRequestEvent const & e) {
	/* will happpen ? */
	Window x = e.window;
	if (x) {
		if (e.place == PlaceOnTop) {
			safe_raise_window(x);
		} else if (e.place == PlaceOnBottom) {
			cnx->lower_window(e.window);
		}
	}
}

void page_t::process_event(XConfigureRequestEvent const & e) {
//	printf("ConfigureRequest %dx%d+%d+%d above:%lu, mode:%x, window:%lu \n",
//			e.width, e.height, e.x, e.y, e.above, e.detail, e.window);

//	printf("name = %s\n", c->get_title().c_str());
//
//	printf("send event: %s\n", e.send_event ? "true" : "false");
//
//	if (e.value_mask & CWX)
//		printf("has x: %d\n", e.x);
//	if (e.value_mask & CWY)
//		printf("has y: %d\n", e.y);
//	if (e.value_mask & CWWidth)
//		printf("has width: %d\n", e.width);
//	if (e.value_mask & CWHeight)
//		printf("has height: %d\n", e.height);
//	if (e.value_mask & CWSibling)
//		printf("has sibling: %lu\n", e.above);
//	if (e.value_mask & CWStackMode)
//		printf("has stack mode: %d\n", e.detail);
//	if (e.value_mask & CWBorderWidth)
//		printf("has border: %d\n", e.border_width);

	managed_window_t * mw = find_managed_window_with(e.window);

	if (mw != 0) {

		switch (mw->get_type()) {
		case MANAGED_NOTEBOOK:
		case MANAGED_FULLSCREEN:
			/** do not allow resize **/
			if ((e.value_mask & (CWX | CWY | CWWidth | CWHeight)) != 0){
				mw->fake_configure();
			}

			break;
		case MANAGED_FLOATING:

			/** resize floating **/
			box_int_t new_size = mw->get_wished_position();

			if (e.value_mask & CWX) {
				new_size.x = e.x;
			}

			if (e.value_mask & CWY) {
				new_size.y = e.y;
			}

			if (e.value_mask & CWWidth) {
				new_size.w = e.width;
			}

			if (e.value_mask & CWHeight) {
				new_size.h = e.height;
			}

			if ((e.value_mask & (CWX)) and (e.value_mask & (CWY)) and e.x == 0
					and e.y == 0 and !viewport_outputs.empty()) {
				viewport_t * v = viewport_outputs.begin()->second;
				box_int_t b = v->get_absolute_extend();
				/* place on center */
				new_size.x = (b.w - new_size.w) / 2 + b.x;
				new_size.y = (b.h - new_size.h) / 2 + b.y;
			}

			unsigned int final_width = new_size.w;
			unsigned int final_height = new_size.h;

			compute_client_size_with_constraint(mw->orig(), (unsigned) new_size.w,
					(unsigned) new_size.h, final_width, final_height);

			new_size.w = final_width;
			new_size.h = final_height;

			mw->set_wished_position(new_size);

			break;
		}
	} else {
		/** validate configure when window is not managed **/
		ackwoledge_configure_request(e);
	}

}

void page_t::ackwoledge_configure_request(XConfigureRequestEvent const & e) {
	static unsigned long CONFIGURE_MASK =
			(CWX | CWY | CWHeight | CWWidth | CWBorderWidth);

	XWindowChanges ev;
	ev.x = e.x;
	ev.y = e.y;
	ev.width = e.width;
	ev.height = e.height;
	ev.sibling = e.above;
	ev.stack_mode = e.detail;
	ev.border_width = e.border_width;

	XConfigureWindow(e.display, e.window, e.value_mask & CONFIGURE_MASK, &ev);

}

void page_t::process_event(XMapRequestEvent const & e) {
	fprintf(stderr, "Maprequest\n");
	/* everything will be done in MapEvent */
	Window a = e.window;

	onmap(a);
	cnx->map_window(a);

}

void page_t::process_event(XPropertyEvent const & e) {
	//printf("Atom Name = \"%s\"\n", A_name(e.atom).c_str());

	if(e.window == cnx->get_root_window())
		return;

	Window x = e.window;
	managed_window_t * mw = find_managed_window_with(e.window);

	if (e.atom == A(_NET_WM_USER_TIME)) {
		//if (_last_focus_time < x->get_net_user_time())
		//	_last_focus_time = x->get_net_user_time();
	} else if (e.atom == A(_NET_WM_NAME)) {
		if (mw != 0) {
			if (mw->is(MANAGED_NOTEBOOK)) {
				rpage->mark_durty();
			}

			if (mw->is(MANAGED_FLOATING)) {
				theme->render_floating(mw, is_focussed(mw));
			}
		}

	} else if (e.atom == A(WM_NAME)) {
		if (mw != 0) {
			if (mw->is(MANAGED_NOTEBOOK)) {
				rpage->mark_durty();
			}

			if (mw->is(MANAGED_FLOATING)) {
				theme->render_floating(mw, is_focussed(mw));
			}
		}
	} else if (e.atom == A(_NET_WM_STRUT_PARTIAL)) {
		printf("UPDATE PARTIAL STRUCT\n");
		for(map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin(); i != viewport_outputs.end(); ++i) {
			fix_allocation(*(i->second));
		}
		rpage->mark_durty();
	} else if (e.atom == A(_NET_WM_STRUT)) {
		//x->mark_durty_net_wm_partial_struct();
		//rpage->mark_durty();
	} else if (e.atom == A(_NET_WM_ICON)) {
		/* TODO: durty notebook */
	} else if (e.atom == A(_NET_WM_WINDOW_TYPE)) {
		/* window type must be set on map, I guess it should never change ? */
		/* update cache */

		//window_t::page_window_type_e old = x->get_window_type();
		//x->read_transient_for();
		//x->find_window_type();
		/* I do not see something in ICCCM */
		//if(x->get_window_type() == window_t::PAGE_NORMAL_WINDOW_TYPE && old != window_t::PAGE_NORMAL_WINDOW_TYPE) {
		//	manage_notebook(x);
		//}
	} else if (e.atom == A(WM_NORMAL_HINTS)) {
		if (mw != 0) {

			if (mw->is(MANAGED_NOTEBOOK)) {
				find_notebook_for(mw)->update_client_position(mw);
			}

			if (mw->is(MANAGED_FLOATING)) {

				/* apply normal hint to floating window */
				box_int_t new_size = mw->get_wished_position();
				unsigned int final_width = new_size.w;
				unsigned int final_height = new_size.h;
				compute_client_size_with_constraint(mw->orig(),
						(unsigned) new_size.w, (unsigned) new_size.h,
						final_width, final_height);
				new_size.w = final_width;
				new_size.h = final_height;
				mw->set_wished_position(new_size);
				theme->render_floating(mw, is_focussed(mw));

			}
		}
	} else if (e.atom == A(WM_PROTOCOLS)) {
	} else if (e.atom == A(WM_TRANSIENT_FOR)) {
//		printf("TRANSIENT_FOR = #%ld\n", x->transient_for());

		update_transient_for(x);
		update_windows_stack();

	} else if (e.atom == A(WM_HINTS)) {
	} else if (e.atom == A(_NET_WM_STATE)) {
	} else if (e.atom == A(WM_STATE)) {
		/** this is set by page ... don't read it **/
	} else if (e.atom == A(_NET_WM_DESKTOP)) {
		/* this set by page in most case */
		//x->read_net_wm_desktop();
	}
}

void page_t::process_event(XClientMessageEvent const & e) {
//	printf("Entering in %s on %lu\n", __PRETTY_FUNCTION__, e.window);

	//printf("%lu\n", ev->xclient.message_type);
//	char * name = A_name(e.message_type);
//	printf("Atom Name = \"%s\"\n", name);
//	XFree(name);



	Window w = e.window;
	if(w == 0)
		return;

	managed_window_t * mw = find_managed_window_with(e.window);

	if (e.message_type == A(_NET_ACTIVE_WINDOW)) {
		//printf("request to activate %lu\n", e.window);

		if (mw != 0) {
			if (mw->is(MANAGED_NOTEBOOK)) {
				activate_client(mw);
			}

			if (mw->is(MANAGED_FLOATING)) {
				mw->normalize();
				//set_focus(mw, false);
			}
		}
	} else if (e.message_type == A(_NET_WM_STATE)) {

		process_net_vm_state_client_message(w, e.data.l[0], e.data.l[1]);
		process_net_vm_state_client_message(w, e.data.l[0], e.data.l[2]);

		for (int i = 1; i < 3; ++i) {
			if (std::find(supported_list.begin(), supported_list.end(),
					e.data.l[i]) != supported_list.end()) {
				switch (e.data.l[0]) {
				case _NET_WM_STATE_REMOVE:
					//w->unset_net_wm_state(e.data.l[i]);
					break;
				case _NET_WM_STATE_ADD:
					//w->set_net_wm_state(e.data.l[i]);
					break;
				case _NET_WM_STATE_TOGGLE:
					//w->toggle_net_wm_state(e.data.l[i]);
					break;
				}
			}
		}
	} else if (e.message_type == A(WM_CHANGE_STATE)) {
		/* client should send this message to go iconic */
		if (e.data.l[0] == IconicState) {
			//printf("Set to iconic %lu\n", w);
			if (mw != 0) {
				if (mw->is(MANAGED_NOTEBOOK))
					iconify_client (mw);
			}
		}

	} else if (e.message_type == A(PAGE_QUIT)) {
		running = false;
	} else if (e.message_type == A(WM_PROTOCOLS)) {
//		char * name = A_name(e.data.l[0]);
//		printf("PROTOCOL Atom Name = \"%s\"\n", name);
//		XFree(name);
	} else if (e.message_type == A(_NET_CLOSE_WINDOW)) {

		XEvent evx;
		evx.xclient.display = cnx->dpy;
		evx.xclient.type = ClientMessage;
		evx.xclient.format = 32;
		evx.xclient.message_type = A(WM_PROTOCOLS);
		evx.xclient.window = e.window;
		evx.xclient.data.l[0] = A(WM_DELETE_WINDOW);
		evx.xclient.data.l[1] = e.data.l[0];

		cnx->send_event(e.window, False, NoEventMask, &evx);
	} else if (e.message_type == A(_NET_REQUEST_FRAME_EXTENTS)) {
			//w->write_net_frame_extents();
	}
}

void page_t::process_event(XDamageNotifyEvent const & e) {
	printf("Should not append\n");
	//printf("Damage area %dx%d+%d+%d\n", e.area.width, e.area.height, e.area.x, e.area.y);
}

void page_t::fullscreen(managed_window_t * mw) {

	mw->net_wm_state_add(_NET_WM_STATE_FULLSCREEN);
	if(mw->is(MANAGED_FULLSCREEN))
		return;

	/* WARNING: Call order is important, change it with caution */
	fullscreen_data_t data;

	if(mw->is(MANAGED_NOTEBOOK)) {
		data.revert_notebook = find_notebook_for(mw);
		data.revert_type = MANAGED_NOTEBOOK;
		data.viewport = find_viewport_for(data.revert_notebook);
		assert(data.viewport != 0);
		remove_window_from_tree(mw);
	} else if (mw->is(MANAGED_FLOATING)) {
		data.revert_type = MANAGED_FLOATING;
		data.revert_notebook = 0;
		data.viewport = viewport_outputs.begin()->second;
	}


	if (data.viewport->fullscreen_client != 0) {
		unfullscreen(data.viewport->fullscreen_client);
	}

	data.viewport->_is_visible = false;

	/* unmap all notebook window */
	list<notebook_t *> ns;
	data.viewport->get_notebooks(ns);
	for(list<notebook_t *>::iterator i = ns.begin(); i != ns.end(); ++i) {
		(*i)->unmap_all();
	}

	mw->set_managed_type(MANAGED_FULLSCREEN);
	data.viewport->fullscreen_client = mw;
	fullscreen_client_to_viewport[mw] = data;
	mw->normalize();
	mw->set_wished_position(data.viewport->raw_aera);
	//set_focus(mw, false);
	safe_raise_window(mw->orig());
}

void page_t::unfullscreen(managed_window_t * mw) {
	/* WARNING: Call order is important, change it with caution */
	mw->net_wm_state_remove(_NET_WM_STATE_FULLSCREEN);
	if(!has_key(fullscreen_client_to_viewport, mw))
		return;

	fullscreen_data_t data = fullscreen_client_to_viewport[mw];

	viewport_t * v = data.viewport;

	if (data.revert_type == MANAGED_NOTEBOOK) {
		notebook_t * old = data.revert_notebook;
		if (!is_valid_notebook(old))
			old = default_window_pop;
		mw->set_managed_type(MANAGED_NOTEBOOK);
		insert_window_in_tree(mw, old, true);
		old->activate_client(mw);
		mw->reconfigure();
	} else {
		mw->set_managed_type(MANAGED_FLOATING);
		mw->reconfigure();
		theme->render_floating(mw, is_focussed(mw));
	}

	v->fullscreen_client = 0;
	fullscreen_client_to_viewport.erase(mw);
	v->_is_visible = true;

	/* map all notebook window */
	list<notebook_t *> ns;
	v->get_notebooks(ns);
	for (list<notebook_t *>::iterator i = ns.begin(); i != ns.end(); ++i) {
		(*i)->map_all();
	}

	update_allocation();
	rpage->mark_durty();

}

void page_t::toggle_fullscreen(managed_window_t * c) {
	//if(c->orig()->is_fullscreen())
	//	unfullscreen(c);
	//else
	//	fullscreen(c);
}

void page_t::print_window_attributes(Window w, XWindowAttributes & wa) {
//	printf(">>> Window: #%lu\n", w);
//	printf("> size: %dx%d+%d+%d\n", wa.width, wa.height, wa.x, wa.y);
//	printf("> border_width: %d\n", wa.border_width);
//	printf("> depth: %d\n", wa.depth);
//	printf("> visual #%p\n", wa.visual);
//	printf("> root: #%lu\n", wa.root);
//	if (wa.c_class == CopyFromParent) {
//		printf("> class: CopyFromParent\n");
//	} else if (wa.c_class == InputOutput) {
//		printf("> class: InputOutput\n");
//	} else if (wa.c_class == InputOnly) {
//		printf("> class: InputOnly\n");
//	} else {
//		printf("> class: Unknown\n");
//	}
//
//	if (wa.map_state == IsViewable) {
//		printf("> map_state: IsViewable\n");
//	} else if (wa.map_state == IsUnviewable) {
//		printf("> map_state: IsUnviewable\n");
//	} else if (wa.map_state == IsUnmapped) {
//		printf("> map_state: IsUnmapped\n");
//	} else {
//		printf("> map_state: Unknown\n");
//	}
//
//	printf("> bit_gravity: %d\n", wa.bit_gravity);
//	printf("> win_gravity: %d\n", wa.win_gravity);
//	printf("> backing_store: %dlx\n", wa.backing_store);
//	printf("> backing_planes: %lx\n", wa.backing_planes);
//	printf("> backing_pixel: %lx\n", wa.backing_pixel);
//	printf("> save_under: %d\n", wa.save_under);
//	printf("> colormap: ?\n");
//	printf("> all_event_masks: %08lx\n", wa.all_event_masks);
//	printf("> your_event_mask: %08lx\n", wa.your_event_mask);
//	printf("> do_not_propagate_mask: %08lx\n", wa.do_not_propagate_mask);
//	printf("> override_redirect: %d\n", wa.override_redirect);
//	printf("> screen: %p\n", wa.screen);
}

//std::string page_t::safe_get_window_name(Window w) {
//	Window x = get_window_t(w);
//	std::string wname = "unknown window";
//	if(x != 0) {
//		wname = x->get_title();
//	}
//
//	return wname;
//}

void page_t::process_event(XEvent const & e) {



//	if (e.type == MapNotify) {
//		fprintf(stderr, "#%08lu %s: event = %lu, win = %lu, name=\"%s\"\n", e.xmap.serial,
//				x_event_name[e.type], e.xmap.event, e.xmap.window, safe_get_window_name(e.xmap.window).c_str());
//	} else if (e.type == DestroyNotify) {
//		fprintf(stderr, "#%08lu %s: event = %lu, win = %lu, name=\"%s\"\n", e.xdestroywindow.serial,
//				x_event_name[e.type], e.xdestroywindow.event,
//				e.xdestroywindow.window, safe_get_window_name(e.xdestroywindow.window).c_str());
//	} else if (e.type == MapRequest) {
//		fprintf(stderr, "#%08lu %s: parent = %lu, win = %lu, name=\"%s\"\n", e.xmaprequest.serial,
//				x_event_name[e.type], e.xmaprequest.parent,
//				e.xmaprequest.window, safe_get_window_name(e.xmaprequest.window).c_str());
//	} else if (e.type == UnmapNotify) {
//		fprintf(stderr, "#%08lu %s: event = %lu, win = %lu, name=\"%s\"\n", e.xunmap.serial,
//				x_event_name[e.type], e.xunmap.event, e.xunmap.window, safe_get_window_name(e.xunmap.window).c_str());
//	} else if (e.type == CreateNotify) {
//		fprintf(stderr, "#%08lu %s: parent = %lu, win = %lu, name=\"%s\"\n", e.xcreatewindow.serial,
//				x_event_name[e.type], e.xcreatewindow.parent,
//				e.xcreatewindow.window, safe_get_window_name(e.xcreatewindow.window).c_str());
//	} else if (e.type < LASTEvent && e.type > 0) {
//		fprintf(stderr, "#%08lu %s: win: #%lu, name=\"%s\"\n", e.xany.serial, x_event_name[e.type],
//				e.xany.window, safe_get_window_name(e.xany.window).c_str());
//	}

//	fflush(stdout);

//	page.cnx->grab();
//
//	/* remove all destroyed windows */
//	XEvent tmp_ev;
//	while (XCheckTypedEvent(page.cnx->dpy, DestroyNotify, &tmp_ev)) {
//		page.process_event(e.xdestroywindow);
//	}

	if (e.type == ButtonPress) {
		process_event_press(e.xbutton);
	} else if (e.type == ButtonRelease) {
		process_event_release(e.xbutton);
	} else if (e.type == MotionNotify) {
		process_event(e.xmotion);
	} else if (e.type == KeyPress || e.type == KeyRelease) {
		process_event(e.xkey);
	} else if (e.type == CirculateNotify) {
		process_event(e.xcirculate);
	} else if (e.type == ConfigureNotify) {
		process_event(e.xconfigure);
	} else if (e.type == CreateNotify) {
		process_event(e.xcreatewindow);
	} else if (e.type == DestroyNotify) {
		process_event(e.xdestroywindow);
	} else if (e.type == GravityNotify) {
		process_event(e.xgravity);
	} else if (e.type == MapNotify) {
		process_event(e.xmap);
	} else if (e.type == ReparentNotify) {
		process_event(e.xreparent);
	} else if (e.type == UnmapNotify) {
		process_event(e.xunmap);
	} else if (e.type == CirculateRequest) {
		process_event(e.xcirculaterequest);
	} else if (e.type == ConfigureRequest) {
		process_event(e.xconfigurerequest);
	} else if (e.type == MapRequest) {
		process_event(e.xmaprequest);
	} else if (e.type == PropertyNotify) {
		process_event(e.xproperty);
	} else if (e.type == ClientMessage) {
		process_event(e.xclient);
	} else if (e.type == cnx->damage_event + XDamageNotify) {
		process_event(reinterpret_cast<XDamageNotifyEvent const &>(e));
	} else if (e.type == Expose) {
		managed_window_t * mw = find_managed_window_with(e.xexpose.window);
		if(mw != 0) {
			if(mw->is(MANAGED_FLOATING)) {
				theme->render_floating(mw, is_focussed(mw));
				mw->expose();
			}
		}

		if (e.xexpose.window == rpage->id()) {
			rpage->expose(list_values(viewport_outputs));
		} else if (e.xexpose.window == pn1->id()) {
			pn1->expose();
		} else if (e.xexpose.window == pfm->id()) {
			pfm->expose();
		} else if (e.xexpose.window == ps->id()) {
			ps->expose();
		}

	} else if (e.type == cnx->xrandr_event + RRNotify) {
		XRRNotifyEvent const &ev = reinterpret_cast<XRRNotifyEvent const &>(e);

		char const * s_subtype = "Unknown";

		switch(ev.subtype) {
		case RRNotify_CrtcChange:
			s_subtype = "RRNotify_CrtcChange";
			break;
		case RRNotify_OutputChange:
			s_subtype = "RRNotify_OutputChange";
			break;
		case RRNotify_OutputProperty:
			s_subtype = "RRNotify_OutputProperty";
			break;
		case RRNotify_ProviderChange:
			s_subtype = "RRNotify_ProviderChange";
			break;
		case RRNotify_ProviderProperty:
			s_subtype = "RRNotify_ProviderProperty";
			break;
		case RRNotify_ResourceChange:
			s_subtype = "RRNotify_ResourceChange";
			break;
		default:
			break;
		}

		printf("RRNotify : %s\n", s_subtype);

		if (ev.subtype == RRNotify_CrtcChange) {
			rr_update_viewport_layout();
			update_allocation();
		}

		p_window_attribute_t rwa = cnx->get_window_attributes(cnx->get_root_window());

		delete rpage;
		rpage = new renderable_page_t(cnx, theme,
				rwa->width, rwa->height);
		rpage->show();

		XGrabButton(cnx->dpy, AnyButton, AnyModifier, rpage->id(), False,
				ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
				GrabModeSync, GrabModeAsync, None, None);

		/** put rpage behind all managed windows **/
		update_windows_stack();


	} else if (e.type == cnx->xrandr_event + RRScreenChangeNotify) {
		XRRScreenChangeNotifyEvent const & ev = reinterpret_cast<XRRScreenChangeNotifyEvent const &>(e);

		printf("RRScreenChangeNotify heigth = %d, width = %d, mheight = %d, mwidth = %d\n", ev.height, ev.width, ev.mheight, ev.mwidth);

		p_window_attribute_t rwa = cnx->get_window_attributes(cnx->get_root_window());

		delete rpage;
		rpage = new renderable_page_t(cnx, theme,
				rwa->width, rwa->height);
		rpage->show();

		XGrabButton(cnx->dpy, AnyButton, AnyModifier, rpage->id(), False,
				ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
				GrabModeSync, GrabModeAsync, None, None);

		/** put rpage behind all managed windows **/
		update_windows_stack();

	} else if (e.type == SelectionClear) {
		printf("SelectionClear\n");
		running = false;
	} else if (e.type == cnx->xshape_event + ShapeNotify) {
		XShapeEvent * se = (XShapeEvent *)&e;
		if(se->kind == ShapeClip) {
			//Window w = se->window;
		}
	} else {
//		fprintf(stderr, "Not handled event:\n");
//		if (e.xany.type > 0 && e.xany.type < LASTEvent) {
//			fprintf(stderr, "#%lu type: %s, send_event: %u, window: %lu\n",
//					e.xany.serial, x_event_name[e.xany.type], e.xany.send_event,
//					e.xany.window);
//		} else {
//			fprintf(stderr, "#%lu type: %u, send_event: %u, window: %lu\n",
//					e.xany.serial, e.xany.type, e.xany.send_event,
//					e.xany.window);
//		}
	}

	//page.rnd->render_flush();

	//page.cnx->ungrab();

//	if (!page.cnx->is_not_grab()) {
//		fprintf(stderr, "SERVER IS GRAB WHERE IT SHOULDN'T");
//		exit(EXIT_FAILURE);
//	}

}

void page_t::activate_client(managed_window_t * x) {
	if(x == 0)
		return;
	notebook_t * n = find_notebook_for(x);
	if (n != 0) {
		n->activate_client(x);
		XFlush(cnx->dpy);
		//set_focus(x, false);
		rpage->mark_durty();
	} else {
		/* floating window or fullscreen window */
		//set_focus(x, false);
		//rnd->add_damage_area(x->get_base_position());
	}
}

void page_t::insert_window_in_tree(managed_window_t * x, notebook_t * n, bool prefer_activate) {
	assert(x != 0);
	assert(find_notebook_for(x) == 0);
	if (n == 0)
		n = default_window_pop;
	assert(n != 0);
	n->add_client(x, prefer_activate);

	rpage->mark_durty();

}

void page_t::remove_window_from_tree(managed_window_t * x) {
	assert(find_notebook_for(x) != 0);
	notebook_t * n = find_notebook_for(x);
	n->remove_client(x);
	rpage->mark_durty();
	//rnd->add_damage_area(n->get_absolute_extend());
	//rnd->add_damage_area(rpage->get_area());

}

void page_t::iconify_client(managed_window_t * x) {
	list<notebook_t *> lst;
	get_notebooks(lst);

	for (list<notebook_t *>::iterator i = lst.begin(); i != lst.end(); ++i) {
		(*i)->iconify_client(x);
	}
}

/* update viewport and childs allocation */
void page_t::update_allocation() {
	map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin();
	while (i != viewport_outputs.end()) {
		fix_allocation(*(i->second));
		i->second->reconfigure();
		++i;
	}
}

void page_t::set_default_pop(notebook_t * x) {
	default_window_pop = x;
	rpage->default_pop = default_window_pop;
}

void page_t::set_focus(managed_window_t * focus, Time tfocus, bool force_focus) {

	_last_focus_time = tfocus;

	managed_window_t * current_focus = 0;

	if(!_client_focused.empty()) {
		current_focus = _client_focused.front();
	}

	if(current_focus != focus && current_focus != 0) {
		current_focus->unset_focused();
		if(current_focus->is(MANAGED_FLOATING)) {
			theme->render_floating(current_focus, false);
		}
	}

	if(current_focus == focus && !force_focus)
		return;

	if(focus == 0)
		return;

	//printf("focus [%lu] %s\n", focus->orig, focus->get_title().c_str());
	//fflush(stdout);
	rpage->mark_durty();

	_client_focused.remove(focus);
	_client_focused.push_front(focus);

	/* focus the selected window */
	rpage->set_focuced_client(focus);
	safe_raise_window(focus->orig());

	focus->focus(_last_focus_time);
	if (focus->get_type() == MANAGED_FLOATING) {
		theme->render_floating(focus, true);
	}

	cnx->write_net_active_window(focus->orig());

}

xconnection_t * page_t::get_xconnection() {
	return cnx;
}

/* delete window handler */
void page_t::delete_window(Window x) {
//	assert(has_key(xwindow_to_window, x));
//	xwindow_to_window.erase(x);
//	delete x;
}

bool page_t::check_for_start_split(XButtonEvent const & e) {
	//printf("call %s\n", __FUNCTION__);
	split_t * x = 0;
	list<split_t *> lst;
	get_splits(lst);
	for(list<split_t *>::iterator i = lst.begin(); i != lst.end(); ++i) {
		if ((*i)->get_split_bar_area().is_inside(e.x_root, e.y_root)) {
			x = (*i);
			break;
		}
	}

	if (x != 0) {
		//printf("starting split\n");
		/* switch process mode */
		process_mode = PROCESS_SPLIT_GRAB;

		mode_data_split.split_ratio = x->get_split_ratio();
		mode_data_split.split = x;
		mode_data_split.slider_area = mode_data_split.split->get_split_bar_area();

		/* show split overlay */
		ps->move_resize(mode_data_split.slider_area);
		ps->show();
		ps->expose();

	}

	//printf("exit %s\n", __FUNCTION__);
	return x != 0;
}

bool page_t::check_for_start_notebook(XButtonEvent const & e) {
	//printf("call %s\n", __FUNCTION__);
	managed_window_t * c = 0;
	notebook_t * n = 0;
	list<notebook_t *> lst;
	get_notebooks(lst);

	for(list<notebook_t *>::iterator i = lst.begin(); i != lst.end(); ++i) {
		c = (*i)->find_client_tab(e.x_root, e.y_root);
		if (c) {
			n = *i;
			break;
		}
	}

	if (c != 0) {

		n->update_close_area();
		if (n->close_client_area.is_inside(e.x_root, e.y_root)) {

			/* apply change on button release */
			process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
			mode_data_notebook.c = c;
			mode_data_notebook.from = n;
			mode_data_notebook.ns = 0;

			/* TODO: on release */
			//c->delete_window(e.time);
		} else if (n->undck_client_area.is_inside(e.x_root, e.y_root)) {

			/* apply change on button release */
			process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
			mode_data_notebook.c = c;
			mode_data_notebook.from = n;
			mode_data_notebook.ns = 0;

			/* TODO: on release */
			//unbind_window(c);
		} else {

//			printf("starting notebook\n");
			process_mode = PROCESS_NOTEBOOK_GRAB;
			mode_data_notebook.c = c;
			mode_data_notebook.from = n;
			mode_data_notebook.ns = 0;
			mode_data_notebook.zone = SELECT_NONE;

			pn0->move_resize(mode_data_notebook.from->tab_area);
			pn0->update_window(c->orig());

			pn1->update_data(c->get_icon()->get_cairo_surface(), c->get_title());
			pn1->move(e.x_root, e.y_root);

			mode_data_notebook.from->set_selected(mode_data_notebook.c);
			rpage->mark_durty();

		}

	} else {

		notebook_t * x = 0;
		for (list<notebook_t *>::iterator i = lst.begin(); i != lst.end();
				++i) {
			notebook_t * n = (*i);
			if (n->button_close.is_inside(e.x, e.y)) {
				x = n;
				break;
			} else if (n->button_vsplit.is_inside(e.x, e.y)) {
				x = n;
				break;
			} else if (n->button_hsplit.is_inside(e.x, e.y)) {
				x = n;
				break;
			} else if (n->button_pop.is_inside(e.x, e.y)) {
				x = n;
				break;
			}
		}

		if (x != 0) {
			/* apply change on button release */
			process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
			mode_data_notebook.c = 0;
			mode_data_notebook.from = x;
			mode_data_notebook.ns = 0;

			/* TODO: in release button */
//			if (x->button_close.is_inside(e.x, e.y)) {
//				notebook_close(x);
//				rpage->mark_durty();
//				rnd->add_damage_area(cnx->get_root_size());
//			} else if (x->button_vsplit.is_inside(e.x, e.y)) {
//				split(x, VERTICAL_SPLIT);
//				update_allocation();
//				rpage->mark_durty();
//				rnd->add_damage_area(cnx->get_root_size());
//			} else if (x->button_hsplit.is_inside(e.x, e.y)) {
//				split(x, HORIZONTAL_SPLIT);
//				update_allocation();
//				rpage->mark_durty();
//				rnd->add_damage_area(cnx->get_root_size());
//			} else if (x->button_pop.is_inside(e.x, e.y)) {
//				default_window_pop = x;
//				rnd->add_damage_area(x->tab_area);
//				update_allocation();
//				rpage->mark_durty();
//				rnd->add_damage_area(cnx->get_root_size());
//			}
		}

	}
	//printf("exit %s\n", __FUNCTION__);
	return c != 0;
}

void page_t::split(notebook_t * nbk, split_type_e type) {
	//rnd->add_damage_area(nbk->_allocation);
	split_t * split = new_split(type);
	nbk->_parent->replace(nbk, split);
	split->replace(0, nbk);
	notebook_t * n = new_notebook();
	split->replace(0, n);

}

void page_t::split_left(notebook_t * nbk, managed_window_t * c) {
	//rnd->add_damage_area(nbk->_allocation);
	split_t * split = new_split(VERTICAL_SPLIT);
	notebook_t * n = new_notebook();
	nbk->_parent->replace(nbk, split);
	split->set_pack0(n);
	split->set_pack1(nbk);
	insert_window_in_tree(c, n, false);
}

void page_t::split_right(notebook_t * nbk, managed_window_t * c) {
	//rnd->add_damage_area(nbk->_allocation);
	split_t * split = new_split(VERTICAL_SPLIT);
	notebook_t * n = new_notebook();
	nbk->_parent->replace(nbk, split);
	split->set_pack0(nbk);
	split->set_pack1(n);
	insert_window_in_tree(c, n, false);
}

void page_t::split_top(notebook_t * nbk, managed_window_t * c) {
	//rnd->add_damage_area(nbk->_allocation);
	split_t * split = new_split(HORIZONTAL_SPLIT);
	notebook_t * n = new_notebook();
	nbk->_parent->replace(nbk, split);
	split->set_pack0(n);
	split->set_pack1(nbk);
	insert_window_in_tree(c, n, false);
}

void page_t::split_bottom(notebook_t * nbk, managed_window_t * c) {
	//rnd->add_damage_area(nbk->_allocation);
	split_t * split = new_split(HORIZONTAL_SPLIT);
	notebook_t * n = new_notebook();
	nbk->_parent->replace(nbk, split);
	split->set_pack0(nbk);
	split->set_pack1(n);
	insert_window_in_tree(c, n, false);
}

void page_t::notebook_close(notebook_t * src) {

	split_t * ths = dynamic_cast<split_t *>(src->_parent);

	/* if parent is viewport return */
	if(ths == 0)
		return;

	assert(src == ths->get_pack0() || src == ths->get_pack1());

	/* if notebook is default_pop, select another one */
	if (default_window_pop == src) {
		/* if notebook list is empty we probably did something wrong */
		default_window_pop = get_another_notebook(src);
		/* put it back temporary since destroy will remove it */
	}

	tree_t * dst = (src == ths->get_pack0()) ? ths->get_pack1() : ths->get_pack0();

	/* move all windows from src to default_window_pop */

	list<managed_window_t *> windows = src->get_clients();
	for(list<managed_window_t *>::iterator i = windows.begin(); i != windows.end(); ++i) {
		remove_window_from_tree((*i));
		insert_window_in_tree((*i), 0, false);
	}

	assert(ths->_parent != 0);
	/* remove this split from tree */
	ths->_parent->replace(ths, dst);

	/* cleanup */
	destroy(src);
	destroy(ths);
}

void page_t::update_popup_position(popup_notebook0_t * p,
		box_int_t & position) {
		p->move_resize(position);
		p->expose();
}

void page_t::update_popup_position(popup_frame_move_t * p, box_int_t & position) {
	p->move_resize(position);
	p->expose();
}


void page_t::fix_allocation(viewport_t & v) {
	printf("fix_allocation %dx%d+%d+%d\n", v.raw_aera.w, v.raw_aera.h,
			v.raw_aera.x, v.raw_aera.y);

	/* Partial struct content definition */
	enum {
		PS_LEFT = 0,
		PS_RIGHT = 1,
		PS_TOP = 2,
		PS_BOTTOM = 3,
		PS_LEFT_START_Y = 4,
		PS_LEFT_END_Y = 5,
		PS_RIGHT_START_Y = 6,
		PS_RIGHT_END_Y = 7,
		PS_TOP_START_X = 8,
		PS_TOP_END_X = 9,
		PS_BOTTOM_START_X = 10,
		PS_BOTTOM_END_X = 11,
	};

	long xtop = v.raw_aera.y;
	long xleft = v.raw_aera.x;
	long xright = cnx->get_root_size().w - v.raw_aera.x - v.raw_aera.w;
	long xbottom = cnx->get_root_size().h - v.raw_aera.y - v.raw_aera.h;

	set<unmanaged_window_t *>::iterator j = unmanaged_window.begin();
	while (j != unmanaged_window.end()) {
		vector<long> ps;
		if (cnx->read_net_wm_partial_struct(((*j)->orig), &ps)) {

			if (ps[PS_LEFT] > 0) {
				/* check if raw area intersect current viewport */
				box_int_t b(0, ps[PS_LEFT_START_Y], ps[PS_LEFT],
						ps[PS_LEFT_END_Y] - ps[PS_LEFT_START_Y] + 1);
				box_int_t x = v.raw_aera & b;
				if (!x.is_null()) {
					xleft = std::max(xleft, ps[PS_LEFT]);
				}
			}

			if (ps[PS_RIGHT] > 0) {
				/* check if raw area intersect current viewport */
				box_int_t b(cnx->get_root_size().w - ps[PS_RIGHT],
						ps[PS_RIGHT_START_Y], ps[PS_RIGHT],
						ps[PS_RIGHT_END_Y] - ps[PS_RIGHT_START_Y] + 1);
				box_int_t x = v.raw_aera & b;
				if (!x.is_null()) {
					xright = std::max(xright, ps[PS_RIGHT]);
				}
			}

			if (ps[PS_TOP] > 0) {
				/* check if raw area intersect current viewport */
				box_int_t b(ps[PS_TOP_START_X], 0,
						ps[PS_TOP_END_X] - ps[PS_TOP_START_X] + 1, ps[PS_TOP]);
				box_int_t x = v.raw_aera & b;
				if (!x.is_null()) {
					xtop = std::max(xtop, ps[PS_TOP]);
				}
			}

			if (ps[PS_BOTTOM] > 0) {
				/* check if raw area intersect current viewport */
				box_int_t b(ps[PS_BOTTOM_START_X],
						cnx->get_root_size().h - ps[PS_BOTTOM],
						ps[PS_BOTTOM_END_X] - ps[PS_BOTTOM_START_X] + 1,
						ps[PS_BOTTOM]);
				box_int_t x = v.raw_aera & b;
				if (!x.is_null()) {
					xbottom = std::max(xbottom, ps[PS_BOTTOM]);
				}
			}
		}
		++j;
	}

	box_int_t final_size;

	final_size.x = xleft;
	final_size.w = cnx->get_root_size().w - xright - xleft + 1;
	final_size.y = xtop;
	final_size.h = cnx->get_root_size().h - xbottom - xtop + 1;

	v.set_effective_area(final_size);

	printf("subarea %dx%d+%d+%d\n", v.effective_aera.w, v.effective_aera.h,
			v.effective_aera.x, v.effective_aera.y);
}

split_t * page_t::new_split(split_type_e type) {
	split_t * x = new split_t(type, theme->get_theme_layout());
	return x;
}

notebook_t * page_t::new_notebook() {
	notebook_t * x = new notebook_t(theme->get_theme_layout());
	return x;
}

void page_t::destroy(split_t * x) {
	delete x;
}

void page_t::destroy(notebook_t * x) {
	delete x;
}

void page_t::destroy(Window c) {

	clear_sibbling_child(c);
	delete_window(c);
}

void page_t::process_net_vm_state_client_message(Window c, long type, Atom state_properties) {
	if(state_properties == None)
		return;

	char const * action;

	switch(type) {
	case _NET_WM_STATE_REMOVE:
		action = "remove";
		break;
	case _NET_WM_STATE_ADD:
		action = "add";
		break;
	case _NET_WM_STATE_TOGGLE:
		action = "toggle";
		break;
	default:
		action = "unknown";
		break;
	}

	string name = cnx->get_atom_name(state_properties);
	printf("_NET_WM_STATE %s %s from %lu\n", action, name.c_str(), c);

	managed_window_t * mw = find_managed_window_with(c);
	if(mw == 0)
		return;

	if (mw->is(MANAGED_NOTEBOOK)) {

;

		if (state_properties == A(_NET_WM_STATE_FULLSCREEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				//printf("SET normal\n");
				break;
			case _NET_WM_STATE_ADD:
				//printf("SET fullscreen\n");
				fullscreen(mw);
				break;
			case _NET_WM_STATE_TOGGLE:
				//printf("SET toggle\n");
				toggle_fullscreen(mw);
				break;
			}
			update_allocation();
		} else if (state_properties == A(_NET_WM_STATE_HIDDEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				if (find_notebook_for(mw) != 0) {
					find_notebook_for(mw)->activate_client(mw);
				} else {
					//c->map();
				}
				break;
			case _NET_WM_STATE_ADD:
				if (find_notebook_for(mw) != 0) {
					find_notebook_for(mw)->iconify_client(mw);
				} else {
					//c->unmap();
				}
				break;
			case _NET_WM_STATE_TOGGLE:
				if (find_notebook_for(mw) != 0) {
//					if (cnx->get_wm_state(c) == IconicState) {
//						find_notebook_for(mw)->activate_client(mw);
//					} else {
//						find_notebook_for(mw)->iconify_client(mw);
//					}
				} else {
//					if (c->is_map()) {
//						c->unmap();
//					} else {
//						c->map();
//					}
				}
				break;
			default:
				break;
			}
		}
	} else if (mw->is(MANAGED_FLOATING)) {
		//char * name = A_name(state_properties);
		//printf("process wm_state %s %s\n", action, name);
		//XFree(name);

		if (state_properties == A(_NET_WM_STATE_FULLSCREEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				//printf("SET normal\n");
				break;
			case _NET_WM_STATE_ADD:
				//printf("SET fullscreen\n");
				fullscreen(mw);
				break;
			case _NET_WM_STATE_TOGGLE:
				//printf("SET toggle\n");
				toggle_fullscreen(mw);
				break;
			}
			update_allocation();
		} else if (state_properties == A(_NET_WM_STATE_HIDDEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				mw->normalize();
				break;
			case _NET_WM_STATE_ADD:
				mw->iconify();
				break;
			case _NET_WM_STATE_TOGGLE:
//				if (c->is_map()) {
//					mw->iconify();
//				} else {
//					mw->normalize();
//				}
				break;
			default:
				break;
			}
		}
	} else if (mw->is(MANAGED_FULLSCREEN)) {
		if (state_properties == A(_NET_WM_STATE_FULLSCREEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				printf("SET normal\n");
				unfullscreen(mw);
				break;
			case _NET_WM_STATE_ADD:
				printf("SET fullscreen\n");
				break;
			case _NET_WM_STATE_TOGGLE:
				printf("SET toggle\n");
				toggle_fullscreen(mw);
				break;
			}
			update_allocation();
		} else if (state_properties == A(_NET_WM_STATE_HIDDEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				mw->normalize();
				break;
			case _NET_WM_STATE_ADD:
				mw->iconify();
				break;
			case _NET_WM_STATE_TOGGLE:
//				if (c->is_map()) {
//					mw->iconify();
//				} else {
//					mw->normalize();
//				}
				break;
			default:
				break;
			}
		}
	}
}

void page_t::update_transient_for(Window w) {
	/* remove sibbling_child if needed */
	clear_sibbling_child(w);
	Window transiant_for = None;
	if (cnx->read_wm_transient_for(w, &transiant_for)) {
		transient_for_map[transiant_for].push_back(w);
	} else {
		transient_for_map[None].push_back(w);
	}
}

void page_t::safe_raise_window(Window w) {

	if(process_mode != PROCESS_NORMAL)
		return;

	Window c = find_client_window(w);
	if(c == 0)
		c = w;

	/**
	 * For each child list put the current client and it ancestor
	 * in back of the child list where they are.
	 **/
	set<Window> already_raise;
	already_raise.insert(None);
	Window w_next = w;
	while(already_raise.find(w_next) == already_raise.end()) {
		update_transient_for(w_next);
		already_raise.insert(w_next);
		if(!cnx->read_wm_transient_for(w_next, &w_next)) {
			w_next = None;
		}
	}

	/** apply change **/
	update_windows_stack();

}

void page_t::clear_sibbling_child(Window w) {
	for (map<Window, list<Window> >::iterator i = transient_for_map.begin();
			i != transient_for_map.end(); ++i) {
		i->second.remove(w);
	}
}

void page_t::destroy_managed_window(managed_window_t * mw) {
	clear_sibbling_child(mw->orig());
	managed_window.erase(mw);
	fullscreen_client_to_viewport.erase(mw);

	/* Fallback focus to last focused windows */
	if(_client_focused.front() == mw) {
		_client_focused.remove(mw);
		if(!_client_focused.empty()) {
			//set_focus(_client_focused.front(), true);
		}
	} else {
		_client_focused.remove(mw);
	}

	delete mw;
}

Window page_t::find_root_window(Window w) {
	managed_window_t * mw = find_managed_window_with(w);
	if(mw != 0)
		return mw->base();
	return 0;

}

Window page_t::find_client_window(Window w) {
	managed_window_t * mw = find_managed_window_with(w);
	if(mw != 0)
		return mw->orig();
	return None;

}

void page_t::compute_client_size_with_constraint(Window c,
		unsigned int wished_width, unsigned int wished_height, unsigned int & width,
		unsigned int & height) {

	/* default size if no size_hints is provided */
	width = wished_width;
	height = wished_height;

	XSizeHints size_hints;
	if(!cnx->read_wm_normal_hints(c, &size_hints)) {
		return;
	}

	/* default size if no size_hints is provided */
	width = wished_width;
	height = wished_height;

	if (size_hints.flags & PMaxSize) {
		if ((int)wished_width > size_hints.max_width)
			wished_width = size_hints.max_width;
		if ((int)wished_height > size_hints.max_height)
			wished_height = size_hints.max_height;
	}

	if (size_hints.flags & PBaseSize) {
		if ((int)wished_width < size_hints.base_width)
			wished_width = size_hints.base_width;
		if ((int)wished_height < size_hints.base_height)
			wished_height = size_hints.base_height;
	} else if (size_hints.flags & PMinSize) {
		if ((int)wished_width < size_hints.min_width)
			wished_width = size_hints.min_width;
		if ((int)wished_height < size_hints.min_height)
			wished_height = size_hints.min_height;
	}

	if (size_hints.flags & PAspect) {
		if (size_hints.flags & PBaseSize) {
			/**
			 * ICCCM say if base is set subtract base before aspect checking
			 * reference: ICCCM
			 **/
			if ((wished_width - size_hints.base_width) * size_hints.min_aspect.y
					< (wished_height - size_hints.base_height)
							* size_hints.min_aspect.x) {
				/* reduce h */
				wished_height = size_hints.base_height
						+ ((wished_width - size_hints.base_width)
								* size_hints.min_aspect.y)
								/ size_hints.min_aspect.x;

			} else if ((wished_width - size_hints.base_width)
					* size_hints.max_aspect.y
					> (wished_height - size_hints.base_height)
							* size_hints.max_aspect.x) {
				/* reduce w */
				wished_width = size_hints.base_width
						+ ((wished_height - size_hints.base_height)
								* size_hints.max_aspect.x)
								/ size_hints.max_aspect.y;
			}
		} else {
			if (wished_width * size_hints.min_aspect.y
					< wished_height * size_hints.min_aspect.x) {
				/* reduce h */
				wished_height = (wished_width * size_hints.min_aspect.y)
						/ size_hints.min_aspect.x;

			} else if (wished_width * size_hints.max_aspect.y
					> wished_height * size_hints.max_aspect.x) {
				/* reduce w */
				wished_width = (wished_height * size_hints.max_aspect.x)
						/ size_hints.max_aspect.y;
			}
		}

	}

	if (size_hints.flags & PResizeInc) {
		wished_width -=
				((wished_width - size_hints.base_width) % size_hints.width_inc);
		wished_height -= ((wished_height - size_hints.base_height)
				% size_hints.height_inc);
	}

	width = wished_width;
	height = wished_height;

}

void page_t::print_tree_windows() {
	printf("print tree \n");

	typedef pair<int, Window> item;

	set<Window> raised_window;
	list<item> window_stack;
	stack<item > nxt;

	nxt.push(item(0, None));

	while (!nxt.empty()) {
		item cur = nxt.top();
		nxt.pop();

		if (!has_key(raised_window, cur.second)) {
			raised_window.insert(cur.second);
			window_stack.push_back(cur);
			if (has_key(transient_for_map, cur.second)) {
				list<Window> childs = transient_for_map[cur.second];
				for (list<Window>::reverse_iterator j = childs.rbegin();
						j != childs.rend(); ++j) {
					nxt.push(item(cur.first + 1, *j));
				}
			}
		}
	}

	window_stack.pop_front();

	for(list<item>::iterator i = window_stack.begin(); i != window_stack.end(); ++i) {
		for(int k = 0; k < (*i).first; ++k) {
			printf("    ");
		}

		//Window w = (*i).second;
		//printf("%d [%lu] %s\n", (*i).first, w, w->get_title().c_str());
	}


}


void page_t::bind_window(managed_window_t * mw) {
	/* update database */
	mw->set_managed_type(MANAGED_NOTEBOOK);
	insert_window_in_tree(mw, 0, true);

	safe_raise_window(mw->orig());

	rpage->mark_durty();
	update_client_list();

}

void page_t::unbind_window(managed_window_t * mw) {

	remove_window_from_tree(mw);

	/* update database */
	mw->set_managed_type(MANAGED_FLOATING);
	theme->render_floating(mw, is_focussed(mw));
	mw->normalize();
	safe_raise_window(mw->orig());
	rpage->mark_durty();
	//rnd->add_damage_area(cnx->get_root_size());
	update_client_list();

}

void page_t::grab_pointer() {
	/* Grab Pointer no other client will get mouse event */
	if (XGrabPointer(cnx->dpy, cnx->get_root_window(), False,
			(ButtonPressMask | ButtonReleaseMask
					| PointerMotionMask),
			GrabModeAsync, GrabModeAsync, None, cursor_fleur,
			CurrentTime) != GrabSuccess) {
		/* bad news */
		throw std::runtime_error("fail to grab pointer");
	}
}

void page_t::cleanup_grab(managed_window_t * mw) {

	switch (process_mode) {
	case PROCESS_NORMAL:
		break;

	case PROCESS_NOTEBOOK_GRAB:
	case PROCESS_NOTEBOOK_BUTTON_PRESS:

		if (mode_data_notebook.c == mw) {
			mode_data_notebook.c = 0;
			process_mode = PROCESS_NORMAL;

			pn0->hide();
			pn1->hide();

			mode_data_notebook.start_x = 0;
			mode_data_notebook.start_y = 0;
			mode_data_notebook.zone = SELECT_NONE;
			mode_data_notebook.c = 0;
			mode_data_notebook.from = 0;
			mode_data_notebook.ns = 0;

			/* ev is button release
			 * so set the hidden focus parameter
			 */
			//XUngrabPointer(cnx->dpy, CurrentTime);
		}
		break;

	case PROCESS_FLOATING_GRAB:
	case PROCESS_FLOATING_RESIZE:
	case PROCESS_FLOATING_CLOSE:
		if (mode_data_floating.f == mw) {
			process_mode = PROCESS_NORMAL;

			pfm->hide();

			mode_data_floating.mode = RESIZE_NONE;
			mode_data_floating.x_offset = 0;
			mode_data_floating.y_offset = 0;
			mode_data_floating.x_root = 0;
			mode_data_floating.y_root = 0;
			mode_data_floating.original_position = box_int_t();
			mode_data_floating.f = 0;
			mode_data_floating.popup_original_position = box_int_t();
			mode_data_floating.final_position = box_int_t();

		}
		break;

	case PROCESS_FLOATING_BIND:
		if (mode_data_bind.c == mw) {
			mode_data_bind.c = 0;
			process_mode = PROCESS_NORMAL;


			pn0->hide();
			pn1->hide();

			mode_data_bind.start_x = 0;
			mode_data_bind.start_y = 0;
			mode_data_bind.zone = SELECT_NONE;
			mode_data_bind.c = 0;
			mode_data_bind.ns = 0;

		}
		break;
	default:
		break;
	}
}

notebook_t * page_t::get_another_notebook(tree_t * x) {
	list<notebook_t *> l;
	get_notebooks(l);

	if (!l.empty()) {
		if (l.front() != x)
			return l.front();
		if (l.back() != x)
			return l.back();
	}

	return 0;

}

void page_t::get_notebooks(list<notebook_t *> & l) {
	for(map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin(); i != viewport_outputs.end(); ++i) {
		i->second->get_notebooks(l);
	}
}

void page_t::get_splits(list<split_t *> & l) {
	for(map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin(); i != viewport_outputs.end(); ++i) {
		i->second->get_splits(l);
	}
}

notebook_t * page_t::find_notebook_for(managed_window_t * mw) {
	list<notebook_t *> lst;
	get_notebooks(lst);
	for(list<notebook_t *>::iterator i = lst.begin(); i != lst.end(); ++i) {
		if(has_key((*i)->get_clients(), mw)) {
			return *i;
		}
	}
	return 0;
}

void page_t::get_managed_windows(list<managed_window_t *> & l) {
	l.insert(l.end(), managed_window.begin(), managed_window.end());
}

managed_window_t * page_t::find_managed_window_with(Window w) {
	for (set<managed_window_t *>::iterator i = managed_window.begin();
			i != managed_window.end(); ++i) {
		if ((*i)->base() == w or (*i)->orig() == w or (*i)->deco() == w)
			return *i;
	}
	return 0;
}

unmanaged_window_t * page_t::find_unmanaged_window_with(Window w) {
	for (set<unmanaged_window_t *>::iterator i = unmanaged_window.begin();
			i != unmanaged_window.end(); ++i) {
		if ((*i)->orig == w)
			return *i;
	}
	return 0;
}

viewport_t * page_t::find_viewport_for(notebook_t * n) {
	tree_t * x = n;
	while (x != 0) {
		if (typeid(*x) == typeid(viewport_t))
			return dynamic_cast<viewport_t *>(x);
		x = (x->_parent);
	}
	return 0;
}

bool page_t::is_valid_notebook(notebook_t * n) {
	list<notebook_t *> l;
	get_notebooks(l);
	return has_key(l, n);
}

void page_t::set_window_cursor(Window w, Cursor c) {
	XSetWindowAttributes swa;
	swa.cursor = c;
	XChangeWindowAttributes(cnx->dpy, w, CWCursor, &swa);
}

bool page_t::is_focussed(managed_window_t * mw) {
	return _client_focused.empty() ? false : _client_focused.front() == mw;
}

void page_t::update_windows_stack() {
	/* recreate the stack order */
	set<Window> raised_window;
	list<Window> window_stack;
	stack<Window> nxt;
	nxt.push(None);
	while (!nxt.empty()) {
		Window cur = nxt.top();
		nxt.pop();

		if (!has_key(raised_window, cur)) {
			raised_window.insert(cur);
			window_stack.push_back(cur);
			if (has_key(transient_for_map, cur)) {
				list<Window> childs = transient_for_map[cur];
				for (list<Window>::reverse_iterator j = childs.rbegin();
						j != childs.rend(); ++j) {
					nxt.push(*j);
				}
			}
		}
	}

	/* remove the None window */
	window_stack.pop_front();

	list<Window> final_order;

	/* 1. raise window in tabs */
	for (list<Window>::iterator i = window_stack.begin();
			i != window_stack.end(); ++i) {
		managed_window_t * mw = find_managed_window_with((*i));
		if (mw != 0) {
			if (mw->is(MANAGED_NOTEBOOK)) {
				Window w = mw->base();
				final_order.remove(w);
				final_order.push_back(w);
			}
		}
	}

	/* 2. raise floating windows */
	for (list<Window>::iterator i = window_stack.begin();
			i != window_stack.end(); ++i) {
		managed_window_t * mw = find_managed_window_with((*i));
		if (mw != 0) {
			if (mw->is(MANAGED_FLOATING)) {
				Window w = mw->base();
				final_order.remove(w);
				final_order.push_back(w);
			}
		}
	}

	/* 3. raise docks */
	for (list<Window>::iterator i = window_stack.begin();
			i != window_stack.end(); ++i) {
		unmanaged_window_t * uw = find_unmanaged_window_with((*i));
		if (uw != 0) {
			if (uw->net_wm_type() == A(_NET_WM_WINDOW_TYPE_DOCK)) {
				Window w = (*i);
				final_order.remove(w);
				final_order.push_back(w);
			}
		}
	}

	/* 4. raise fullscreen window */
	set<Window> fullsceen_window;
	for (map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin();
			i != viewport_outputs.end(); ++i) {
		if (i->second->fullscreen_client != 0) {
			Window w = i->second->fullscreen_client->base();
			final_order.remove(w);
			final_order.push_back(w);
		}
	}

	/* 5. raise other windows */
	for (list<Window>::iterator i = window_stack.begin();
			i != window_stack.end(); ++i) {
		unmanaged_window_t * uw = find_unmanaged_window_with((*i));
		if (uw != 0) {
			if (uw->net_wm_type() != A(_NET_WM_WINDOW_TYPE_DOCK)
					&& (*i) != rpage->id()) {
				Window w = (*i);
				final_order.remove(w);
				final_order.push_back(w);
			}
		}
	}

	/* 6. raise notify windows */
	for (list<Window>::iterator i = window_stack.begin();
			i != window_stack.end(); ++i) {
		unmanaged_window_t * uw = find_unmanaged_window_with((*i));
		if (uw != 0) {
			if (uw->net_wm_type() == A(_NET_WM_WINDOW_TYPE_TOOLTIP)
					|| uw->net_wm_type() == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
				Window w = (*i);
				final_order.remove(w);
				final_order.push_back(w);
			}
		}
	}

	/* overlay */
	final_order.remove(pfm->id());
	final_order.push_back(pfm->id());

	final_order.remove(pn0->id());
	final_order.push_back(pn0->id());

	final_order.remove(pn1->id());
	final_order.push_back(pn1->id());

	final_order.remove(ps->id());
	final_order.push_back(ps->id());

	final_order.remove(rpage->id());
	final_order.push_front(rpage->id());

	final_order.reverse();

	/**
	 * convert list to C array, see std::vector API.
	 **/
	vector<Window> v_order(final_order.begin(), final_order.end());
	XRestackWindows(cnx->dpy, &v_order[0], v_order.size());
}

void page_t::rr_update_viewport_layout() {

	/** update root size infos **/

	/** store the newer layout, to cleanup obsolet viewport **/
	map<RRCrtc, viewport_t *> new_layout;

	XRRScreenResources * resources = XRRGetScreenResourcesCurrent(
			cnx->dpy, cnx->get_root_window());

	for (int i = 0; i < resources->ncrtc; ++i) {
		XRRCrtcInfo * info = XRRGetCrtcInfo(cnx->dpy, resources,
				resources->crtcs[i]);
		printf(
				"CrtcInfo: width = %d, height = %d, x = %d, y = %d, noutputs = %d\n",
				info->width, info->height, info->x, info->y, info->noutput);

		/** if the CRTC has at less one output bound **/
		if(info->noutput > 0) {
			box_t<int> area(info->x, info->y, info->width, info->height);
			/** if this crtc do not has a viewport **/
			if (!has_key(viewport_outputs, resources->crtcs[i])) {
				/** then create a new one, and store it in new_layout **/
				new_layout[resources->crtcs[i]] = new viewport_t(theme, area);
			} else {
				/** update allocation, and store it to new_layout **/
				viewport_outputs[resources->crtcs[i]]->set_raw_area(area);
				new_layout[resources->crtcs[i]] = viewport_outputs[resources->crtcs[i]];
			}
			fix_allocation(*(new_layout[resources->crtcs[i]]));
		}
		XRRFreeCrtcInfo(info);
	}
	XRRFreeScreenResources(resources);

	if(new_layout.size() < 1) {
		throw std::runtime_error("No output screen found\n");
	}

	/** clean up old layout **/
	map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin();
	while(i != viewport_outputs.end()) {
		if(!has_key(new_layout, i->first)) {
			/** destroy this viewport **/
			destroy_viewport(i->second);
		}
		++i;
	}

	viewport_outputs = new_layout;
}

void  page_t::destroy_viewport(viewport_t * v) {

	/* remove fullscreened clients if needed */
	if (v->fullscreen_client != 0) {
		unfullscreen(v->fullscreen_client);
	}

	/* Transfer clients to a valid notebook */
	list<notebook_t *> nbks;
	v->get_notebooks(nbks);
	for (list<notebook_t *>::iterator i = nbks.begin(); i != nbks.end();
			++i) {
		if (default_window_pop == *i)
			default_window_pop = get_another_notebook(*i);
		list<managed_window_t *> lc = (*i)->get_clients();
		for (list<managed_window_t *>::iterator i = lc.begin();
				i != lc.end(); ++i) {
			default_window_pop->add_client(*i, false);
		}
	}

	/* cleanup */
	{
		list<tree_t *> lst;
		v->get_childs(lst);
		for (list<tree_t *>::iterator i = lst.begin(); i != lst.end();
				++i) {
			delete *i;
		}
		delete v;
	}
}

//void page_t::manage_if_needed(Window x) {
//
//	if (find_managed_window_with(x))
//		return;
//
//	/* find the type of current window */
//	page_window_type_e type = find_window_type(cnx->dpy, x);
//
//	if(type == PAGE_OVERLAY_WINDOW_TYPE) {
//		x->map();
//		return;
//	} else if(type == PAGE_UNKNOW_WINDOW_TYPE) {
//		x->map();
//		return;
//	} else if (type == PAGE_NORMAL_WINDOW_TYPE) {
//		managed_window_t * fw = manage(MANAGED_NOTEBOOK, x);
//		insert_window_in_tree(fw, 0,
//				fw->orig()->get_initial_state() == NormalState
//						&& !fw->orig()->is_hidden());
//
//		if (x->is_fullscreen()) {
//			fullscreen(fw);
//		} else {
//			fw->reconfigure();
//		}
//
//	} else if (type == PAGE_FLOATING_WINDOW_TYPE) {
//		managed_window_t * fw = manage(MANAGED_FLOATING, x);
//		/* apply normal hint to floating window */
//		box_int_t new_size = fw->get_wished_position();
//
//		/* do not allow to large windows */
//		if (new_size.w
//				> (cnx->get_root_size().w
//						- theme->get_theme_layout()->floating_margin.left
//						- theme->get_theme_layout()->floating_margin.right))
//			new_size.w = cnx->get_root_size().w
//					- theme->get_theme_layout()->floating_margin.left
//					- theme->get_theme_layout()->floating_margin.right;
//		if (new_size.h
//				> cnx->get_root_size().h
//						- theme->get_theme_layout()->floating_margin.top
//						- theme->get_theme_layout()->floating_margin.bottom)
//			new_size.h = cnx->get_root_size().h
//					- theme->get_theme_layout()->floating_margin.top
//					- theme->get_theme_layout()->floating_margin.bottom;
//
//		unsigned int final_width = new_size.w;
//		unsigned int final_height = new_size.h;
//		compute_client_size_with_constraint(fw->orig(), (unsigned) new_size.w,
//				(unsigned) new_size.h, final_width, final_height);
//		new_size.w = final_width;
//		new_size.h = final_height;
//
//		fw->set_wished_position(new_size);
//		fw->normalize();
//
//		if (x->is_fullscreen()) {
//			fullscreen(fw);
//		} else {
//			fw->reconfigure();
//		}
//
//	} else if (type == PAGE_DOCK_TYPE) {
//		/** track positions and struct change **/
//		x->select_input(StructureNotifyMask | PropertyChangeMask);
//	}
//
//	rpage->mark_durty();
//	update_client_list();
//
//}

void page_t::onmap(Window w) {

	if (find_managed_window_with(w))
		return;
	if (find_unmanaged_window_with(w))
		return;
	if(cnx->get_window_attributes(w)->c_class == InputOnly)
		return;

	Atom type = find_net_wm_type(w);

	/** HACK FOR ECLIPSE **/
	{
		list<Atom> wm_state;
		xconnection_t::wm_class wm_class;
		if (cnx->read_wm_class(w, &wm_class) && cnx->read_net_wm_state(w, &wm_state)
				&& type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
			if (wm_class.res_name == "Eclipse") {
				list<Atom>::iterator x = find(wm_state.begin(), wm_state.end(),
						A(_NET_WM_STATE_SKIP_TASKBAR));
				if (x != wm_state.end()) {
					type = A(_NET_WM_WINDOW_TYPE_DND);
				}
			}
		}
	}

	if (!cnx->get_window_attributes(w)->override_redirect) {
		if (type == A(_NET_WM_WINDOW_TYPE_DESKTOP)) {
			create_managed_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DOCK)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLBAR)) {
			create_managed_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_MENU)) {
			create_managed_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_UTILITY)) {
			create_managed_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_SPLASH)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DIALOG)) {
			create_managed_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_POPUP_MENU)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLTIP)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_COMBO)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DND)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
			create_managed_window(w, type);
		}
	} else {
		if (type == A(_NET_WM_WINDOW_TYPE_DESKTOP)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DOCK)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLBAR)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_MENU)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_UTILITY)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_SPLASH)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DIALOG)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_POPUP_MENU)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLTIP)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_COMBO)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DND)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
			create_unmanaged_window(w, type);
		}
	}

	rpage->mark_durty();
	update_client_list();

}


void page_t::create_managed_window(Window w, Atom type) {

	managed_window_t * mw;
	if((type == A(_NET_WM_WINDOW_TYPE_NORMAL)
			|| type == A(_NET_WM_WINDOW_TYPE_DESKTOP))
			&& !cnx->read_wm_transient_for(w)) {

		mw = manage(MANAGED_NOTEBOOK, type, w);
		insert_window_in_tree(mw, 0, true);

	} else {
		mw = manage(MANAGED_FLOATING, type, w);
		mw->normalize();
	}

	if (mw->is_fullscreen()) {
		fullscreen(mw);
	} else {
		mw->reconfigure();
	}

}

void page_t::create_unmanaged_window(Window w, Atom type) {

	update_transient_for(w);

	unmanaged_window_t * uw = new unmanaged_window_t(cnx, w, type);
	unmanaged_window.insert(uw);
}

Atom page_t::find_net_wm_type(Window w) {

	list<Atom> net_wm_window_type;

	if(!cnx->read_net_wm_window_type(w, &net_wm_window_type)) {
		/**
		 * Fallback from ICCCM.
		 **/

		if(!cnx->get_window_attributes(w)->override_redirect) {
			/* Managed windows */
			if(!cnx->read_wm_transient_for(w)) {
				/**
				 * Extended ICCCM:
				 * _NET_WM_WINDOW_TYPE_NORMAL [...] Managed windows with neither
				 * _NET_WM_WINDOW_TYPE nor WM_TRANSIENT_FOR set MUST be taken
				 * as this type.
				 **/
				net_wm_window_type.push_back(A(_NET_WM_WINDOW_TYPE_NORMAL));
			} else {
				/**
				 * Extended ICCCM:
				 * _NET_WM_WINDOW_TYPE_DIALOG [...] If _NET_WM_WINDOW_TYPE is
				 * not set, then managed windows with WM_TRANSIENT_FOR set MUST
				 * be taken as this type.
				 **/
				net_wm_window_type.push_back(A(_NET_WM_WINDOW_TYPE_DIALOG));
			}

		} else {
			/**
			 * Override-redirected windows.
			 *
			 * Extended ICCCM:
			 * _NET_WM_WINDOW_TYPE_NORMAL [...] Override-redirect windows
			 * without _NET_WM_WINDOW_TYPE, must be taken as this type, whether
			 * or not they have WM_TRANSIENT_FOR set.
			 **/
			net_wm_window_type.push_back(A(_NET_WM_WINDOW_TYPE_NORMAL));
		}
	}

	/* always fall back to normal */
	net_wm_window_type.push_back(A(_NET_WM_WINDOW_TYPE_NORMAL));

	set<Atom> known_type;
	known_type.insert(A(_NET_CURRENT_DESKTOP));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_DESKTOP));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_DOCK));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_TOOLBAR));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_MENU));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_UTILITY));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_SPLASH));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_DIALOG));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_POPUP_MENU));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_TOOLTIP));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_NOTIFICATION));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_COMBO));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_DND));
	known_type.insert(A(_NET_WM_WINDOW_TYPE_NORMAL));

	/** find the first known window type **/
	std::list<Atom>::iterator i = net_wm_window_type.begin();
	while (i != net_wm_window_type.end()) {
		printf("Check for %s\n", cnx->get_atom_name(*i).c_str());
		if(has_key(known_type, *i))
			return *i;
		++i;
	}

	/* should never happen */
	return None;

}

}
