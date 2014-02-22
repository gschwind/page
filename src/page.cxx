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

	page_areas = 0;

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
	key_press_mode = KEY_PRESS_NORMAL;

	cnx = new xconnection_t();

	rnd = 0;

	default_cursor = XCreateFontCursor(cnx->dpy, XC_arrow);

//	cursor_fleur = XCreateFontCursor(cnx->dpy, XC_fleur);

	set_window_cursor(cnx->get_root_window(), default_cursor);

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
		string s(conf_file_name);
		conf.merge_from_file_if_exist(s);
	}

	default_window_pop = 0;

	page_base_dir = conf.get_string("default", "theme_dir");

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


	if(pfm != 0)
		delete pfm;
	if(pn0 != 0)
		delete pn0;
	if(ps != 0)
		delete ps;
	if(pat != 0)
		delete pat;

	set<managed_window_t *>::iterator i = managed_window.begin();
	while(i != managed_window.end()) {
		Window orig = (*i)->orig();
		delete *i;
		/**
		 * Map managed window to allow next window manager to not confuse
		 * this window with WithDraw window.
		 **/
		XMapWindow(cnx->dpy, orig);
		++i;
	}
	managed_window.clear();

	set<unmanaged_window_t *>::iterator j = unmanaged_window.begin();
	while(j != unmanaged_window.end()) {
		delete *j;
		++j;
	}
	unmanaged_window.clear();

	{
		map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin();
		while (i != viewport_outputs.end()) {
			destroy_viewport(i->second);
			++i;
		}
	}

	if(page_areas != 0) {
		delete page_areas;
	}

	// cleanup cairo, for valgrind happiness.
	cairo_debug_reset_static_data();

	delete cnx;

}

void page_t::run() {

	init_xprop(cnx->dpy);

//	printf("root size: %d,%d\n", cnx->get_root_size().w, cnx->get_root_size().h);

	/* create an invisible window to identify page */
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

	/** initialise theme **/
	theme = new simple2_theme_t(cnx, conf);

	/**
	 * listen RRCrtcChangeNotifyMask for possible change in screen layout.
	 **/
	XRRSelectInput(cnx->dpy, cnx->get_root_window(), RRCrtcChangeNotifyMask);

	update_viewport_layout();

	/* init page render */
	rpage = new renderable_page_t(cnx, theme,
			_root_position.w, _root_position.h);

	/* create and add popups (overlay) */
	pfm = new popup_frame_move_t(cnx, theme);
	pn0 = new popup_notebook0_t(cnx, theme);
	ps = new popup_split_t(cnx, theme);
	pat = new popup_alt_tab_t(cnx, theme);

	default_window_pop = 0;

	default_window_pop = get_another_notebook();
	if(default_window_pop == 0)
		throw std::runtime_error("very bad error");

	if(default_window_pop != 0)
		default_window_pop->set_default(true);

	update_net_supported();

	/* update number of desktop */
	int32_t number_of_desktop = 1;
	cnx->change_property(cnx->get_root_window(), _NET_NUMBER_OF_DESKTOPS,
			CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&number_of_desktop), 1);

	/* define desktop geometry */
	set_desktop_geometry(_root_position.w, _root_position.h);

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
	set_focus(0, 0);

	scan();
	update_windows_stack();

	long workarea[4];
	workarea[0] = 0;
	workarea[1] = 0;
	workarea[2] = _root_position.w;
	workarea[3] = _root_position.h;
	cnx->change_property(cnx->get_root_window(), _NET_WORKAREA, CARDINAL,
			32, PropModeReplace, reinterpret_cast<unsigned char*>(workarea), 4);

	rpage->add_damaged(_root_position);

	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_f), Mod4Mask, cnx->get_root_window(),
			True, GrabModeAsync, GrabModeAsync);
	/* quit page */
	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_q), Mod4Mask, cnx->get_root_window(),
			True, GrabModeAsync, GrabModeAsync);

	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_r), Mod4Mask, cnx->get_root_window(),
			True, GrabModeAsync, GrabModeAsync);

	/* print state info */
	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_s), Mod4Mask,
			cnx->get_root_window(),
			True, GrabModeAsync, GrabModeAsync);

	/* Alt-Tab */
	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_Tab), Mod1Mask,
			cnx->get_root_window(),
			False, GrabModeAsync, GrabModeSync);

	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_w), Mod4Mask,
			cnx->get_root_window(),
			True, GrabModeAsync, GrabModeAsync);

	XGrabKey(cnx->dpy, XKeysymToKeycode(cnx->dpy, XK_z), Mod4Mask,
			cnx->get_root_window(),
			True, GrabModeAsync, GrabModeAsync);

	/**
	 * This grab will freeze input for all client, all mouse button, until
	 * we choose what to do with them with XAllowEvents. we can choose to keep
	 * grabbing events or release event and allow further processing by other clients.
	 **/
	XGrabButton(cnx->dpy, AnyButton, AnyModifier, rpage->id(), False,
			ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
			GrabModeSync, GrabModeAsync, None, None);

	timespec _max_wait;
	time_t const default_wait = 1000000000L/60L;
	time_t max_wait = default_wait;
	time_t next_frame;

	next_frame.get_time();

	fd_set fds_read;
	fd_set fds_intr;

	int max = cnx->fd();

	if (rnd != 0) {
		max = cnx->fd() > rnd->fd() ? cnx->fd() : rnd->fd();
	}

	update_allocation();
	XFlush(cnx->dpy);
	running = true;
	while (running) {

		time_t cur_tic;
		cur_tic.get_time();

		if(cur_tic > next_frame) {
			next_frame = cur_tic + default_wait;
			max_wait = default_wait;
			if (rnd != 0) {
				rnd->render();
				rnd->xflush();
			}
		} else {
			max_wait = next_frame - cur_tic;
		}

		FD_ZERO(&fds_read);
		FD_ZERO(&fds_intr);

		FD_SET(cnx->fd(), &fds_read);
		FD_SET(cnx->fd(), &fds_intr);

		/** listen for compositor events **/
		if (rnd != 0) {
			FD_SET(rnd->fd(), &fds_read);
			FD_SET(rnd->fd(), &fds_intr);
		}

		/**
		 * wait for data in both X11 connection streams (compositor and page)
		 **/
		_max_wait = max_wait;
		//printf("%ld %ld\n", _max_wait.tv_sec, _max_wait.tv_nsec);
		int nfd = pselect(max + 1, &fds_read, NULL, &fds_intr, &_max_wait, NULL);

		while (XPending(cnx->dpy)) {
			XEvent ev;
			XNextEvent(cnx->dpy, &ev);
			process_event(ev);
		}

		rpage->repair_damaged(childs());
		XFlush(cnx->dpy);

		if (rnd != 0) {
			rnd->process_events();
			rnd->xflush();
		}

	}
}

managed_window_t * page_t::manage(managed_window_type_e type, Atom net_wm_type, Window w, XWindowAttributes const & wa) {

	cnx->add_to_save_set(w);

	/* set border to zero */
	XWindowChanges wc;
	wc.border_width = 0;
	XConfigureWindow(cnx->dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(cnx->dpy, w, 0);

	/* assign window to desktop 0 */
	if (!cnx->read_net_wm_desktop(w)) {
		long int net_wm_desktop = 0;
		cnx->change_property(w, _NET_WM_DESKTOP,
				CARDINAL, 32, PropModeReplace,
				(unsigned char *) &net_wm_desktop, 1);

	}

	managed_window_t * mw = new managed_window_t(cnx, type, net_wm_type, w, wa, theme);
	managed_window.insert(mw);

	//Show_All_Props2(cnx->dpy, w);

	//printf("WM_TYPE: %s\n", cnx->get_atom_name(net_wm_type).c_str());

	return mw;

}

void page_t::unmanage(managed_window_t * mw) {

	//printf("unmanage %lu\n", mw->orig());

	if (has_key(fullscreen_client_to_viewport, mw)) {
		unfullscreen(mw);
	}

	/** if the window is destroyed, this not work, see fix on destroy **/
	if (mw == _client_focused.front()) {
		_client_focused.remove(mw);

		/* Do not try to refocus */
//		if (_client_focused.front() != 0) {
//			Time t;
//			if (get_safe_net_wm_user_time(_client_focused.front()->orig(), t)) {
//				set_focus(_client_focused.front(), t);
//			}
//		}

	} else {
		_client_focused.remove(mw);
	}

	/* as recommended by EWMH delete _NET_WM_STATE when window become unmanaged */
	mw->net_wm_state_delete();
	mw->wm_state_delete();

	/* if window is in move/resize/notebook move, do cleanup */
	cleanup_grab(mw);

	/** try to remove it from tree **/
	remove_window_from_tree(mw);

	destroy_managed_window(mw);
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

			XWindowAttributes wa;
			if(!XGetWindowAttributes(cnx->dpy, wins[i], &wa))
				continue;

			update_transient_for(w);

			if (wa.map_state != IsUnmapped) {
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
	update_allocation();
//	printf("return %s\n", __PRETTY_FUNCTION__);
}

void page_t::update_net_supported() {
	std::list<Atom> supported_list;

	supported_list.push_back(A(_NET_WM_NAME));
	supported_list.push_back(A(_NET_WM_USER_TIME));
	supported_list.push_back(A(_NET_WM_USER_TIME_WINDOW));
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

	cnx->change_property(cnx->get_root_window(), _NET_CLIENT_LIST_STACKING,
			WINDOW, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&client_list_stack[0]), client_list_stack.size());
	cnx->change_property(cnx->get_root_window(), _NET_CLIENT_LIST, WINDOW,
			32, PropModeReplace, reinterpret_cast<unsigned char *>(&client_list[0]),
			client_list.size());

}

void page_t::process_event(XKeyEvent const & e) {

	if (e.type == KeyPress) {
		fprintf(stderr, "KeyPress key = %d, mod4 = %s, mod1 = %s\n", e.keycode,
				e.state & Mod4Mask ? "true" : "false",
				e.state & Mod1Mask ? "true" : "false");
	} else {
		fprintf(stderr, "KeyRelease key = %d, mod4 = %s, mod1 = %s\n",
				e.keycode, e.state & Mod4Mask ? "true" : "false",
				e.state & Mod1Mask ? "true" : "false");
	}

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
						(*i)->title());
				break;
			case MANAGED_FLOATING:
				printf("[%lu] floating : %s\n", (*i)->orig(),
						(*i)->title());
				break;
			case MANAGED_FULLSCREEN:
				printf("[%lu] fullscreen : %s\n", (*i)->orig(),
						(*i)->title());
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
		rpage->add_damaged(_root_position);
	}

	if (XK_z == k[0] and e.type == KeyPress and (e.state & Mod4Mask)) {
		if(rnd != 0) {
			if(rnd->get_render_mode() == compositor_t::COMPOSITOR_MODE_AUTO) {
				rnd->set_render_mode(compositor_t::COMPOSITOR_MODE_MANAGED);
			} else {
				rnd->set_render_mode(compositor_t::COMPOSITOR_MODE_AUTO);
			}
		}
	}

	if (XK_Tab == k[0] && e.type == KeyPress && ((e.state & 0x0f) == Mod1Mask)) {

		if (key_press_mode == KEY_PRESS_NORMAL and not managed_window.empty()) {

			/* Grab keyboard */
			XGrabKeyboard(e.display, cnx->get_root_window(), False, GrabModeAsync, GrabModeAsync,
					e.time);

			/** Continue to play event as usual (Alt+Tab is in Sync mode) **/
			XAllowEvents(e.display, AsyncKeyboard, e.time);

			key_press_mode = KEY_PRESS_ALT_TAB;
			key_mode_data.selected = _client_focused.front();

			int sel = 0;

			vector<cycle_window_entry_t *> v;
			int s = 0;
			for(set<managed_window_t *>::iterator i = managed_window.begin();
					i != managed_window.end(); ++i) {
				window_icon_handler_t * icon = new window_icon_handler_t(cnx, (*i)->orig(), 64, 64);
				cycle_window_entry_t * cy = new cycle_window_entry_t(*i, icon);
				v.push_back(cy);

				if(*i == _client_focused.front()) {
					sel = s;
				}

				++s;

			}

			pat->update_window(v, sel);

			viewport_t * viewport = viewport_outputs.begin()->second;

			int y = v.size() / 4 + 1;

			pat->move_resize(
					box_int_t(
							viewport->raw_aera.x
									+ (viewport->raw_aera.w - 80 * 4) / 2,
							viewport->raw_aera.y
									+ (viewport->raw_aera.h - y * 80) / 2,
							80 * 4, y * 80));
			pat->show();

		} else {
			XAllowEvents(e.display, ReplayKeyboard, e.time);
		}


		pat->select_next();
		pat->mark_durty();
		pat->expose();
		pat->mark_durty();

	}



	if ((XK_Alt_L == k[0] or XK_Alt_R == k[0]) && e.type == KeyRelease
			&& key_press_mode == KEY_PRESS_ALT_TAB) {

		/** Ungrab Keyboard **/
		XUngrabKeyboard(e.display, e.time);

		key_press_mode = KEY_PRESS_NORMAL;

		/**
		 * do not use dynamic_cast because managed window can be already
		 * destroyed.
		 **/
		managed_window_t * mw = reinterpret_cast<managed_window_t *> (pat->get_selected());
		set_focus(mw, e.time);

		pat->hide();
	}

	XFree(k);

}

/* Button event make page to grab pointer */
void page_t::process_event_press(XButtonEvent const & e) {
	//fprintf(stderr, "Xpress event, window = %lu, root = %lu, subwindow = %lu, pos = (%d,%d), time = %lu\n",
	//		e.window, e.root, e.subwindow, e.x_root, e.y_root, e.time);
	managed_window_t * mw = find_managed_window_with(e.window);

	switch (process_mode) {
	case PROCESS_NORMAL:

		if (e.window == rpage->id() && e.subwindow == None && e.button == Button1) {

			update_page_areas();

			page_event_t * b = 0;
			for (vector<page_event_t>::iterator i = page_areas->begin();
					i != page_areas->end(); ++i) {
				//printf("box = %s => %s\n", (*i)->position.to_string().c_str(), typeid(**i).name());
				if ((*i).position.is_inside(e.x, e.y)) {
					b = &(*i);
					break;
				}
			}

			if (b != 0) {

				if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT) {
					process_mode = PROCESS_NOTEBOOK_GRAB;
					mode_data_notebook.c = _upgrade(b->clt);
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;
					mode_data_notebook.zone = SELECT_NONE;

					pn0->move_resize(mode_data_notebook.from->tab_area);
					pn0->update_window(mode_data_notebook.c->orig(),
							mode_data_notebook.c->title());
					set_focus(mode_data_notebook.c, e.time);
					rpage->add_damaged(mode_data_notebook.from->allocation());
				} else if (b->type == PAGE_EVENT_NOTEBOOK_CLOSE) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = 0;
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_HSPLIT) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = 0;
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_VSPLIT) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = 0;
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_MARK) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = 0;
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT_CLOSE) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = _upgrade(b->clt);
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT_UNBIND) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = _upgrade(b->clt);
					mode_data_notebook.from = _upgrade(b->nbk);
					mode_data_notebook.ns = 0;

				} else if (b->type == PAGE_EVENT_SPLIT) {

					process_mode = PROCESS_SPLIT_GRAB;

					mode_data_split.split_ratio = b->spt->split();
					mode_data_split.split = _upgrade(b->spt);
					mode_data_split.slider_area =
							mode_data_split.split->get_split_bar_area();

					/* show split overlay */
					ps->move_resize(mode_data_split.slider_area);
					ps->show();
					ps->expose();
				}
			}

		}


		if (mw != 0) {

			if (mw->is(MANAGED_FLOATING) and e.button == Button1
					and (e.state & (Mod1Mask | ControlMask))) {

				mode_data_floating.x_offset = e.x;
				mode_data_floating.y_offset = e.y;
				mode_data_floating.x_root = e.x_root;
				mode_data_floating.y_root = e.y_root;
				mode_data_floating.f = mw;
				mode_data_floating.original_position = mw->get_floating_wished_position();
				mode_data_floating.final_position = mw->get_floating_wished_position();
				mode_data_floating.popup_original_position = mw->get_base_position();

//				pfm->move_resize(mw->get_base_position());
//				pfm->update_window(mw->orig(), mw->title());
//				pfm->show();

				if ((e.state & ControlMask)) {
					process_mode = PROCESS_FLOATING_RESIZE;
					mode_data_floating.mode = RESIZE_BOTTOM_RIGHT;
				} else {
					safe_raise_window(mw->orig());
					process_mode = PROCESS_FLOATING_MOVE;
				}


			} else if (mw->is(MANAGED_FLOATING) and e.button == Button1
					and e.subwindow != mw->orig()) {

				mw->update_floating_areas();
				vector<floating_event_t> const * l = mw->floating_areas();

				floating_event_t const * b = 0;
				for (vector<floating_event_t>::const_iterator i = l->begin();
						i != l->end(); ++i) {
					//printf("box = %s => %s\n", (*i)->position.to_string().c_str(), "test");
					if((*i).position.is_inside(e.x, e.y)) {
						b = &(*i);
						break;
					}
				}


				if (b != 0) {

					mode_data_floating.x_offset = e.x;
					mode_data_floating.y_offset = e.y;
					mode_data_floating.x_root = e.x_root;
					mode_data_floating.y_root = e.y_root;
					mode_data_floating.f = mw;
					mode_data_floating.original_position = mw->get_floating_wished_position();
					mode_data_floating.final_position = mw->get_floating_wished_position();
					mode_data_floating.popup_original_position = mw->get_base_position();

					if (b->type == FLOATING_EVENT_CLOSE) {

						mode_data_floating.f = mw;
						process_mode = PROCESS_FLOATING_CLOSE;

					} else if (b->type == FLOATING_EVENT_BIND) {

						mode_data_bind.c = mw;
						mode_data_bind.ns = 0;
						mode_data_bind.zone = SELECT_NONE;

						pn0->move_resize(mode_data_bind.c->get_base_position());
						pn0->update_window(mw->orig(), mw->title());

						process_mode = PROCESS_FLOATING_BIND;

					} else if (b->type == FLOATING_EVENT_TITLE) {

//						pfm->move_resize(mw->get_base_position());
//						pfm->update_window(mw->orig(), mw->title());
//						pfm->show();

						safe_raise_window(mw->orig());
						process_mode = PROCESS_FLOATING_MOVE;
					} else {

//						pfm->move_resize(mw->get_base_position());
//						pfm->update_window(mw->orig(), mw->title());
//						pfm->show();

						if (b->type == FLOATING_EVENT_GRIP_TOP) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_TOP;
						} else if (b->type == FLOATING_EVENT_GRIP_BOTTOM) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_BOTTOM;
						} else if (b->type == FLOATING_EVENT_GRIP_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_LEFT;
						} else if (b->type == FLOATING_EVENT_GRIP_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_RIGHT;
						} else if (b->type == FLOATING_EVENT_GRIP_TOP_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_TOP_LEFT;
						} else if (b->type == FLOATING_EVENT_GRIP_TOP_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_TOP_RIGHT;
						} else if (b->type == FLOATING_EVENT_GRIP_BOTTOM_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_BOTTOM_LEFT;
						} else if (b->type == FLOATING_EVENT_GRIP_BOTTOM_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_BOTTOM_RIGHT;
						} else {
							safe_raise_window(mw->orig());
							process_mode = PROCESS_FLOATING_MOVE;
						}
					}

				}

			} else if (mw->is(MANAGED_FULLSCREEN) and e.button == (Button1)
					and (e.state & (Mod1Mask))) {
				fprintf(stderr, "start FULLSCREEN MOVE\n");
				/** start moving fullscreen window **/
				viewport_t * v = find_mouse_viewport(e.x_root, e.y_root);

				if (v != 0) {
					fprintf(stderr, "start FULLSCREEN MOVE\n");
					process_mode = PROCESS_FULLSCREEN_MOVE;
					mode_data_fullscreen.mw = mw;
					mode_data_fullscreen.v = v;
					pn0->update_window(mw->orig(), mw->title());
					pn0->show();
					pn0->move_resize(v->raw_aera);
					pn0->expose();
				}
			} else if (mw->is(MANAGED_NOTEBOOK) and e.button == (Button1)
					and (e.state & (Mod1Mask))) {

				notebook_t * n = find_notebook_for(mw);

				process_mode = PROCESS_NOTEBOOK_GRAB;
				mode_data_notebook.c = mw;
				mode_data_notebook.from = n;
				mode_data_notebook.ns = 0;
				mode_data_notebook.zone = SELECT_NONE;

				pn0->move_resize(mode_data_notebook.from->tab_area);
				pn0->update_window(mw->orig(), mw->title());

				mode_data_notebook.from->set_selected(mode_data_notebook.c);
				rpage->add_damaged(mode_data_notebook.from->allocation());

			}
		}

		break;
	default:
		break;
	}


	if (process_mode == PROCESS_NORMAL) {

		XAllowEvents(cnx->dpy, ReplayPointer, CurrentTime);

		/**
		 * This focus is anoying because, passive grab can the
		 * focus itself.
		 **/
//		if(mw == 0)
//			mw = find_managed_window_with(e.subwindow);
//
		managed_window_t * mw = find_managed_window_with(e.window);
		if (mw != 0) {
			set_focus(mw, e.time);
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

//		fprintf(stderr,
//				"XXXXGrab event, window = %lu, root = %lu, subwindow = %lu, pos = (%d,%d), time = %lu\n",
//				e.window, e.root, e.subwindow, e.x_root, e.y_root, e.time);
//
	}

}

void page_t::process_event_release(XButtonEvent const & e) {
	//fprintf(stderr, "Xrelease event, window = %lu, root = %lu, subwindow = %lu, pos = (%d,%d)\n",
	//		e.window, e.root, e.subwindow, e.x_root, e.y_root);

	if (e.button == Button1) {
		switch (process_mode) {
		case PROCESS_NORMAL:
			fprintf(stderr, "DDDDDDDDDDD\n");
			break;
		case PROCESS_SPLIT_GRAB:

			process_mode = PROCESS_NORMAL;

			ps->hide();

			mode_data_split.split->set_split(mode_data_split.split_ratio);
			rpage->add_damaged(mode_data_split.split->allocation());

			mode_data_split.split = 0;
			mode_data_split.slider_area = box_int_t();
			mode_data_split.split_ratio = 0.5;

			break;
		case PROCESS_NOTEBOOK_GRAB:

			process_mode = PROCESS_NORMAL;

			pn0->hide();

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

			/* Automatically close empty notebook (disabled) */
//			if (mode_data_notebook.from->_clients.empty()
//					&& mode_data_notebook.from->parent() != 0) {
//				notebook_close(mode_data_notebook.from);
//				update_allocation();
//			}

			set_focus(mode_data_notebook.c, e.time);
			if (mode_data_notebook.from != 0)
				rpage->add_damaged(mode_data_notebook.from->allocation());
			if (mode_data_notebook.ns != 0)
				rpage->add_damaged(mode_data_notebook.ns->allocation());

			mode_data_notebook.start_x = 0;
			mode_data_notebook.start_y = 0;
			mode_data_notebook.zone = SELECT_NONE;
			mode_data_notebook.c = 0;
			mode_data_notebook.from = 0;
			mode_data_notebook.ns = 0;

			break;
		case PROCESS_NOTEBOOK_BUTTON_PRESS:
			process_mode = PROCESS_NORMAL;

			{
				page_event_t * b = 0;
				for (vector<page_event_t>::iterator i = page_areas->begin();
						i != page_areas->end(); ++i) {
					if ((*i).position.is_inside(e.x, e.y)) {
						b = &(*i);
						break;
					}
				}

				if (b != 0) {
					if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT) {
						/** do noting **/
					} else if (b->type == PAGE_EVENT_NOTEBOOK_CLOSE) {
						notebook_close(mode_data_notebook.from);
					} else if (b->type == PAGE_EVENT_NOTEBOOK_HSPLIT) {
						split(mode_data_notebook.from, HORIZONTAL_SPLIT);
					} else if (b->type == PAGE_EVENT_NOTEBOOK_VSPLIT) {
						split(mode_data_notebook.from, VERTICAL_SPLIT);
					} else if (b->type == PAGE_EVENT_NOTEBOOK_MARK) {
						if(default_window_pop != 0) {
							default_window_pop->set_default(false);
							rpage->add_damaged(default_window_pop->allocation());
						}
						default_window_pop = mode_data_notebook.from;
						default_window_pop->set_default(true);
						rpage->add_damaged(default_window_pop->allocation());
					} else if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT_CLOSE) {
						mode_data_notebook.c->delete_window(e.time);
					} else if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT_UNBIND) {
						unbind_window(mode_data_notebook.c);
					} else if (b->type == PAGE_EVENT_SPLIT) {
						/** do nothing **/
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
		case PROCESS_FLOATING_MOVE:

			//pfm->hide();

			mode_data_floating.f->set_floating_wished_position(
					mode_data_floating.final_position);
			mode_data_floating.f->reconfigure();

			set_focus(mode_data_floating.f, e.time);

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

			//pfm->hide();

			mode_data_floating.f->set_floating_wished_position(
					mode_data_floating.final_position);
			mode_data_floating.f->reconfigure();

			set_focus(mode_data_floating.f, e.time);
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
			mw->delete_window(e.time);

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

			set_focus(mode_data_bind.c, e.time);

			if (mode_data_bind.zone == SELECT_TAB && mode_data_bind.ns != 0) {
				mode_data_bind.c->set_managed_type(MANAGED_NOTEBOOK);
				insert_window_in_tree(mode_data_bind.c, mode_data_bind.ns,
						true);

				safe_raise_window(mode_data_bind.c->orig());
				rpage->add_damaged(mode_data_bind.ns->allocation());
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

			process_mode = PROCESS_NORMAL;

			mode_data_bind.start_x = 0;
			mode_data_bind.start_y = 0;
			mode_data_bind.zone = SELECT_NONE;
			mode_data_bind.c = 0;
			mode_data_bind.ns = 0;

			break;
		}

		case PROCESS_FULLSCREEN_MOVE:  {

			process_mode = PROCESS_NORMAL;

			viewport_t * v = find_mouse_viewport(e.x_root, e.y_root);

			if (v != 0) {
				if (v != mode_data_fullscreen.v) {
					pn0->move_resize(v->raw_aera);
					mode_data_fullscreen.v = v;
				}
			}

			pn0->hide();

			if (mode_data_fullscreen.v != 0 and mode_data_fullscreen.mw != 0) {
				if (mode_data_fullscreen.v->fullscreen_client
						!= mode_data_fullscreen.mw) {
					unfullscreen(mode_data_fullscreen.mw);
					fullscreen(mode_data_fullscreen.mw, mode_data_fullscreen.v);
				}
			}

			mode_data_fullscreen.v = 0;
			mode_data_fullscreen.mw = 0;

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
		fprintf(stderr, "Warning: This case should not happen %s:%d\n", __FILE__, __LINE__);
		break;
	case PROCESS_SPLIT_GRAB:

		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev));

		if (mode_data_split.split->get_split_type() == VERTICAL_SPLIT) {
			mode_data_split.split_ratio = (ev.xmotion.x
					- mode_data_split.split->allocation().x)
					/ (double) (mode_data_split.split->allocation().w);
		} else {
			mode_data_split.split_ratio = (ev.xmotion.y
					- mode_data_split.split->allocation().y)
					/ (double) (mode_data_split.split->allocation().h);
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
		/* Get latest know motion event */
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
		}

		++count;

		vector<notebook_t *> ln;
		get_notebooks(ln);
		for(vector<notebook_t *>::iterator i = ln.begin(); i != ln.end(); ++i) {
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
	case PROCESS_FLOATING_MOVE: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev));

		/* compute new window position */
		box_int_t new_position = mode_data_floating.original_position;
		new_position.x += e.x_root - mode_data_floating.x_root;
		new_position.y += e.y_root - mode_data_floating.y_root;
		mode_data_floating.final_position = new_position;
		mode_data_floating.f->set_floating_wished_position(new_position);
		mode_data_floating.f->reconfigure();

//		box_int_t popup_new_position = mode_data_floating.popup_original_position;
//		popup_new_position.x += e.x_root - mode_data_floating.x_root;
//		popup_new_position.y += e.y_root - mode_data_floating.y_root;
//		update_popup_position(pfm, popup_new_position);

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
		if(size.w > _root_position.w - 100)
			size.w = _root_position.w - 100;
		if(size.h > _root_position.h - 100)
			size.h = _root_position.h - 100;

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

		mode_data_floating.f->set_floating_wished_position(size);
		mode_data_floating.f->reconfigure();

//		box_int_t popup_new_position = size;
//		popup_new_position.x -= theme->floating_margin.left;
//		popup_new_position.y -= theme->floating_margin.top;
//		popup_new_position.w += theme->floating_margin.left + theme->floating_margin.right;
//		popup_new_position.h += theme->floating_margin.top + theme->floating_margin.bottom;
//
//		update_popup_position(pfm, popup_new_position);

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
//			if(!pn1->is_visible())
//				pn1->show();
		}

		++count;

//		pn1->move(ev.xmotion.x_root + 10, ev.xmotion.y_root);

		vector<notebook_t *> ln;
		get_notebooks(ln);
		for (vector<notebook_t *>::iterator i = ln.begin(); i != ln.end(); ++i) {
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
	case PROCESS_FULLSCREEN_MOVE: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while (XCheckMaskEvent(cnx->dpy, Button1MotionMask, &ev))
			continue;

		viewport_t * v = find_mouse_viewport(ev.xmotion.x_root,
				ev.xmotion.y_root);

		if (v != 0) {
			if (v != mode_data_fullscreen.v) {
				pn0->move_resize(v->raw_aera);
				pn0->expose();
				mode_data_fullscreen.v = v;
			}

		}

		break;
	}

	default:
		fprintf(stderr, "Warning: Unknown process_mode\n");
		break;
	}
}

void page_t::process_event(XCirculateEvent const & e) {

}

void page_t::process_event(XConfigureEvent const & e) {

}

/* track all created window */
void page_t::process_event(XCreateWindowEvent const & e) {

}

void page_t::process_event(XDestroyWindowEvent const & e) {

	managed_window_t * mw = find_managed_window_with(e.window);
	if(mw != 0) {
		unmanage(mw);
	}

	unmanaged_window_t * uw = find_unmanaged_window_with(e.window);
	if(uw != 0) {
		delete uw;
		unmanaged_window.erase(uw);
	}

//	if(!_client_focused.empty()) {
//		if(_client_focused.front() != 0) {
//			set_focus(_client_focused.front(), CurrentTime);
//		}
//	}

	cleanup_transient_for_for_window(e.window);

	update_client_list();
	rpage->mark_durty();

}

void page_t::process_event(XGravityEvent const & e) {
	/* Ignore it, never happen ? */
}

void page_t::process_event(XMapEvent const & e) {

	/* if map event does not occur within root, ignore it */
	if (e.event != cnx->get_root_window())
		return;

	/** usefull for windows statink **/
	update_transient_for(e.window);

	if(e.override_redirect) {
		update_windows_stack();
		return;
	}

	if (onmap(e.window)) {
		rpage->mark_durty();
		update_client_list();
	}

	update_windows_stack();

}

void page_t::process_event(XReparentEvent const & e) {
	if(e.send_event == True)
		return;
	if(e.window == cnx->get_root_window())
		return;

}

void page_t::process_event(XUnmapEvent const & e) {
	//printf("Unmap event %lu is send event = %d\n", e.window, e.send_event);

	Window x = e.window;

	/**
	 * Filter own unmap.
	 **/
//	bool expected_event = cnx->find_pending_event(event_t(e.serial, e.type));
//	if (expected_event)
//		return;

	/* if client is managed */

	managed_window_t * mw = find_managed_window_with(e.window);
	if(mw != 0 and e.send_event == True) {
		unmanage(mw);
		cleanup_transient_for_for_window(x);
		update_client_list();
	}

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

		if ((e.value_mask & (CWX | CWY | CWWidth | CWHeight)) != 0) {

			/** compute floating size **/
			box_int_t new_size = mw->get_floating_wished_position();

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

			printf("new_size = %s\n", new_size.to_string().c_str());

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

			compute_client_size_with_constraint(mw->orig(),
					(unsigned) new_size.w, (unsigned) new_size.h, final_width,
					final_height);

			new_size.w = final_width;
			new_size.h = final_height;

			printf("new_size = %s\n", new_size.to_string().c_str());

			/** only affect floating windows **/
			mw->set_floating_wished_position(new_size);
			mw->reconfigure();
		}

	} else {

//		if (e.value_mask & CWX)
//			printf("xxhas x: %d\n", e.x);
//		if (e.value_mask & CWY)
//			printf("xxhas y: %d\n", e.y);
//		if (e.value_mask & CWWidth)
//			printf("xxhas width: %d\n", e.width);
//		if (e.value_mask & CWHeight)
//			printf("xxhas height: %d\n", e.height);
//		if (e.value_mask & CWSibling)
//			printf("xxhas sibling: %lu\n", e.above);
//		if (e.value_mask & CWStackMode)
//			printf("xxhas stack mode: %d\n", e.detail);
//		if (e.value_mask & CWBorderWidth)
//			printf("xxhas border: %d\n", e.border_width);

		/** validate configure when window is not managed **/
		ackwoledge_configure_request(e);
	}

}

void page_t::ackwoledge_configure_request(XConfigureRequestEvent const & e) {
	static unsigned int CONFIGURE_MASK =
			(CWX | CWY | CWHeight | CWWidth | CWBorderWidth);

	XWindowChanges ev;
	ev.x = e.x;
	ev.y = e.y;
	ev.width = e.width;
	ev.height = e.height;
	ev.sibling = e.above;
	ev.stack_mode = e.detail;
	ev.border_width = e.border_width;

	XConfigureWindow(e.display, e.window,
			((unsigned int) e.value_mask) & CONFIGURE_MASK, &ev);

}

void page_t::process_event(XMapRequestEvent const & e) {

	if (e.send_event || e.parent != cnx->get_root_window()) {
		XMapWindow(e.display, e.window);
		return;
	}

	if (onmap(e.window)) {
		rpage->mark_durty();
		update_client_list();
		update_windows_stack();
	} else {
		XMapWindow(e.display, e.window);
	}

}

void page_t::process_event(XPropertyEvent const & e) {
	//printf("Atom Name = \"%s\"\n", A_name(e.atom).c_str());

	if(e.window == cnx->get_root_window())
		return;

	Window x = e.window;
	managed_window_t * mw = find_managed_window_with(e.window);

	if (e.atom == A(_NET_WM_USER_TIME)) {

	} else if (e.atom == A(_NET_WM_NAME) || e.atom == A(WM_NAME)) {
		if (mw != 0) {
			mw->mark_title_durty();

			if (mw->is(MANAGED_NOTEBOOK)) {
				notebook_t * n = find_notebook_for(mw);
				rpage->add_damaged(n->allocation());
			}

			if (mw->is(MANAGED_FLOATING)) {
				mw->expose();
			}
		}

	} else if (e.atom == A(_NET_WM_STRUT_PARTIAL)) {
		//printf("UPDATE PARTIAL STRUCT\n");
		for(map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin(); i != viewport_outputs.end(); ++i) {
			fix_allocation(*(i->second));
		}
		rpage->mark_durty();
	} else if (e.atom == A(_NET_WM_STRUT)) {
		//x->mark_durty_net_wm_partial_struct();
		//rpage->mark_durty();
	} else if (e.atom == A(_NET_WM_ICON)) {
		if (mw != 0)
			mw->mark_icon_durty();
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

			/* apply normal hint to floating window */
			box_int_t new_size = mw->get_wished_position();
			unsigned int final_width = new_size.w;
			unsigned int final_height = new_size.h;
			compute_client_size_with_constraint(mw->orig(),
					(unsigned) new_size.w, (unsigned) new_size.h, final_width,
					final_height);
			new_size.w = final_width;
			new_size.h = final_height;
			mw->set_floating_wished_position(new_size);
			mw->reconfigure();

		}
	} else if (e.atom == A(WM_PROTOCOLS)) {
	} else if (e.atom == A(WM_TRANSIENT_FOR)) {
//		printf("TRANSIENT_FOR = #%ld\n", x->transient_for());

		update_transient_for(x);
		update_windows_stack();

	} else if (e.atom == A(WM_HINTS)) {
	} else if (e.atom == A(_NET_WM_STATE)) {
		/* this event are generated by page */
		/* change of net_wm_state must be requested by client message */
	} else if (e.atom == A(WM_STATE)) {
		/** this is set by page ... don't read it **/
	} else if (e.atom == A(_NET_WM_DESKTOP)) {
		/* this set by page in most case */
		//x->read_net_wm_desktop();
	}
}

void page_t::process_event(XClientMessageEvent const & e) {

	Window w = e.window;
	if(w == 0)
		return;

	managed_window_t * mw = find_managed_window_with(e.window);

	if (e.message_type == A(_NET_ACTIVE_WINDOW)) {
		if (mw != 0) {
			if(e.data.l[1] == CurrentTime) {
				printf("Invalid focus request\n");
			} else {
				set_focus(mw, e.data.l[1]);
			}
		}
	} else if (e.message_type == A(_NET_WM_STATE)) {

		/* process first request */
		process_net_vm_state_client_message(w, e.data.l[0], e.data.l[1]);
		/* process second request */
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

		/** When window want to become iconic, just bind them **/
		if (mw != 0) {
			if (mw->is(MANAGED_FLOATING) and e.data.l[0] == IconicState) {
				bind_window(mw);
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
	//printf("Should not append\n");
	//printf("Damage area %dx%d+%d+%d\n", e.area.width, e.area.height, e.area.x, e.area.y);
}

void page_t::fullscreen(managed_window_t * mw, viewport_t * v) {

	mw->net_wm_state_add(_NET_WM_STATE_FULLSCREEN);
	if(mw->is(MANAGED_FULLSCREEN))
		return;

	/* WARNING: Call order is important, change it with caution */
	fullscreen_data_t data;


	if(mw->is(MANAGED_NOTEBOOK)) {
		/**
		 * if the current window is managed in notebook:
		 *
		 * 1. search for the current notebook,
		 * 2. search the viewport for this notebook, and use it as default
		 *    fullscreen host or use the first available viewport.
		 **/
		data.revert_notebook = find_notebook_for(mw);
		page_assert(data.revert_notebook != 0);
		data.revert_type = MANAGED_NOTEBOOK;
		if (v == 0) {
			data.viewport = find_viewport_for(data.revert_notebook);
		} else {
			data.viewport = v;
		}
		page_assert(data.viewport != 0);
		remove_window_from_tree(mw);
	} else if (mw->is(MANAGED_FLOATING)) {
		data.revert_type = MANAGED_FLOATING;
		data.revert_notebook = 0;
		if (v == 0) {
			data.viewport = viewport_outputs.begin()->second;
		} else {
			data.viewport = v;
		}
	}


	if (data.viewport->fullscreen_client != 0) {
		unfullscreen(data.viewport->fullscreen_client);
	}

	data.viewport->_is_visible = false;

	/* unmap all notebook window */
	vector<notebook_t *> ns;
	get_notebooks(data.viewport, ns);
	for(vector<notebook_t *>::iterator i = ns.begin(); i != ns.end(); ++i) {
		(*i)->unmap_all();
	}

	mw->set_managed_type(MANAGED_FULLSCREEN);
	data.viewport->fullscreen_client = mw;
	fullscreen_client_to_viewport[mw] = data;
	mw->set_notebook_wished_position(data.viewport->raw_aera);
	mw->reconfigure();
	//printf("FULLSCREEN TO %dx%d+%d+%d\n", data.viewport->raw_aera.w, data.viewport->raw_aera.h, data.viewport->raw_aera.x, data.viewport->raw_aera.y);
	mw->normalize();
	safe_raise_window(mw->orig());
}

void page_t::unfullscreen(managed_window_t * mw) {
	/* WARNING: Call order is important, change it with caution */
	mw->net_wm_state_remove(_NET_WM_STATE_FULLSCREEN);
	if(!has_key(fullscreen_client_to_viewport, mw))
		return;

	fullscreen_data_t & data = fullscreen_client_to_viewport[mw];

	viewport_t * v = data.viewport;

	if (data.revert_type == MANAGED_NOTEBOOK) {
		notebook_t * old = data.revert_notebook;
		mw->set_managed_type(MANAGED_NOTEBOOK);
		insert_window_in_tree(mw, old, true);
		old->activate_client(mw);
		mw->reconfigure();
	} else {
		mw->set_managed_type(MANAGED_FLOATING);
		mw->reconfigure();
	}

	v->fullscreen_client = 0;
	fullscreen_client_to_viewport.erase(mw);
	v->_is_visible = true;

	/* map all notebook window */
	vector<notebook_t *> ns;
	get_notebooks(v, ns);
	for (vector<notebook_t *>::iterator i = ns.begin(); i != ns.end(); ++i) {
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
//				x_event_name[e.type], e.xmap.event, e.xmap.window, "none");
//	} else if (e.type == DestroyNotify) {
//		fprintf(stderr, "#%08lu %s: event = %lu, win = %lu, name=\"%s\"\n", e.xdestroywindow.serial,
//				x_event_name[e.type], e.xdestroywindow.event,
//				e.xdestroywindow.window, "none");
//	} else if (e.type == MapRequest) {
//		fprintf(stderr, "#%08lu %s: parent = %lu, win = %lu, name=\"%s\"\n", e.xmaprequest.serial,
//				x_event_name[e.type], e.xmaprequest.parent,
//				e.xmaprequest.window, "none");
//	} else if (e.type == UnmapNotify) {
//		fprintf(stderr, "#%08lu %s: event = %lu, win = %lu, name=\"%s\"\n", e.xunmap.serial,
//				x_event_name[e.type], e.xunmap.event, e.xunmap.window, "none");
//	} else if (e.type == CreateNotify) {
//		fprintf(stderr, "#%08lu %s: parent = %lu, win = %lu, name=\"%s\"\n", e.xcreatewindow.serial,
//				x_event_name[e.type], e.xcreatewindow.parent,
//				e.xcreatewindow.window, "none");
//	} else if (e.type < LASTEvent && e.type > 0) {
//		fprintf(stderr, "#%08lu %s: win: #%lu, name=\"%s\"\n", e.xany.serial, x_event_name[e.type],
//				e.xany.window, "none");
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
		if (mw != 0) {
			if (mw->is(MANAGED_FLOATING)) {
				mw->expose();
			}
		}

		if (e.xexpose.window == rpage->id()) {
			rpage->expose(
					box_int_t(e.xexpose.x, e.xexpose.y, e.xexpose.width,
							e.xexpose.height));
		} else if (e.xexpose.window == pfm->id()) {
			pfm->expose();
		} else if (e.xexpose.window == ps->id()) {
			ps->expose();
		} else if (e.xexpose.window == pat->id()) {
			pat->expose();
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

		//printf("RRNotify : %s\n", s_subtype);

		if (ev.subtype == RRNotify_CrtcChange) {
			update_viewport_layout();
			update_allocation();

			theme->update();

			delete rpage;
			rpage = new renderable_page_t(cnx, theme, _root_position.w,
					_root_position.h);
			rpage->show();

		}

		XGrabButton(cnx->dpy, AnyButton, AnyModifier, rpage->id(), False,
				ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
				GrabModeSync, GrabModeAsync, None, None);

		/** put rpage behind all managed windows **/
		update_windows_stack();


	} else if (e.type == cnx->xrandr_event + RRScreenChangeNotify) {
		/** do nothing **/
	} else if (e.type == SelectionClear) {
		//printf("SelectionClear\n");
		running = false;
	} else if (e.type == cnx->xshape_event + ShapeNotify) {
		XShapeEvent * se = (XShapeEvent *)&e;
		if(se->kind == ShapeClip) {
			//Window w = se->window;
		}
	} else if (e.type == FocusOut) {
		//printf("FocusOut %lu\n", e.xfocus.window);
	} else if (e.type == FocusIn) {
		//printf("FocusIn %lu\n", e.xfocus.window);
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

void page_t::insert_window_in_tree(managed_window_t * x, notebook_t * n, bool prefer_activate) {
	page_assert(x != 0);
	page_assert(find_notebook_for(x) == 0);
	if (n == 0)
		n = default_window_pop;
	page_assert(n != 0);
	x->set_managed_type(MANAGED_NOTEBOOK);
	n->add_client(x, prefer_activate);
	rpage->add_damaged(n->allocation());

}

void page_t::remove_window_from_tree(managed_window_t * x) {
	notebook_t * n = find_notebook_for(x);
	if (n != 0) {
		n->remove_client(x);
		rpage->add_damaged(n->allocation());
	}
}

void page_t::iconify_client(managed_window_t * x) {
	vector<notebook_t *> lst;
	get_notebooks(lst);

	for (vector<notebook_t *>::iterator i = lst.begin(); i != lst.end(); ++i) {
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
	if(default_window_pop != 0) {
		default_window_pop->set_default(false);
		rpage->add_damaged(default_window_pop->allocation());
	}
	default_window_pop = x;
	default_window_pop->set_default(true);
	rpage->add_damaged(default_window_pop->allocation());
}

void page_t::set_focus(managed_window_t * new_focus, Time tfocus) {

	/** ignore focus if time is too old **/
	if(tfocus <= _last_focus_time and tfocus != CurrentTime)
		return;

	/**
	 * Try to focus unknown window, this can happen on alt-tab when
	 * selected window is destroyed
	 **/
	if(not has_key(managed_window, new_focus)) {
		return;
	}

	if(tfocus != CurrentTime)
		_last_focus_time = tfocus;

	managed_window_t * old_focus = 0;

	/** NULL pointer is always in the list **/
	old_focus = _client_focused.front();

	if (old_focus != 0) {
		/**
		 * update _NET_WM_STATE and grab button mode and mark decoration
		 * durty (for floating window only)
		 **/
		old_focus->set_focus_state(false);

		/**
		 * if this is a notebook, mark the area for updates.
		 **/
		if (old_focus->is(MANAGED_NOTEBOOK)) {
			notebook_t * n = find_notebook_for(old_focus);
			page_assert(n != 0);
			rpage->add_damaged(n->allocation());
		}
	}

	/**
	 * put this managed window in front of list
	 **/
	_client_focused.remove(new_focus);
	_client_focused.push_front(new_focus);

	if(new_focus == 0)
		return;

	/**
	 * raise the newly focused window at top, in respect of transient for.
	 **/
	safe_raise_window(new_focus->orig());

	/**
	 * if this is a notebook, mark the area for updates and activated the
	 * notebook
	 **/
	if(new_focus->is(MANAGED_NOTEBOOK)) {
		notebook_t * n = find_notebook_for(new_focus);
		page_assert(n != 0);
		n->activate_client(new_focus);
		rpage->add_damaged(n->allocation());
	}


	/**
	 * change root window properties to the current focused window.
	 **/
	cnx->write_net_active_window(new_focus->orig());

	/**
	 * update the focus status
	 **/
	new_focus->focus(_last_focus_time);


}

xconnection_t * page_t::get_xconnection() {
	return cnx;
}

void page_t::split(notebook_t * nbk, split_type_e type) {
	split_t * split = new_split(type);
	nbk->parent()->replace(nbk, split);
	split->replace(0, nbk);
	notebook_t * n = new_notebook();
	split->replace(0, n);

	update_allocation();
	rpage->add_damaged(split->allocation());

	print_tree();

}

void page_t::split_left(notebook_t * nbk, managed_window_t * c) {
	tree_t * parent = nbk->parent();
	notebook_t * n = new notebook_t(theme);
	split_t * split = new split_t(VERTICAL_SPLIT, theme, n, nbk);
	parent->replace(nbk, split);
	insert_window_in_tree(c, n, true);
	update_allocation();
	rpage->add_damaged(split->allocation());

	print_tree();

}

void page_t::split_right(notebook_t * nbk, managed_window_t * c) {
	tree_t * parent = nbk->parent();
	notebook_t * n = new notebook_t(theme);
	split_t * split = new split_t(VERTICAL_SPLIT, theme, nbk, n);
	parent->replace(nbk, split);
	insert_window_in_tree(c, n, true);
	update_allocation();
	rpage->add_damaged(split->allocation());

	print_tree();

}

void page_t::split_top(notebook_t * nbk, managed_window_t * c) {
	tree_t * parent = nbk->parent();
	notebook_t * n = new notebook_t(theme);
	split_t * split = new split_t(HORIZONTAL_SPLIT, theme, n, nbk);
	parent->replace(nbk, split);
	insert_window_in_tree(c, n, true);
	update_allocation();
	rpage->add_damaged(split->allocation());

	print_tree();

}

void page_t::split_bottom(notebook_t * nbk, managed_window_t * c) {
	tree_t * parent = nbk->parent();
	notebook_t * n = new notebook_t(theme);
	split_t * split = new split_t(HORIZONTAL_SPLIT, theme, nbk, n);
	parent->replace(nbk, split);
	insert_window_in_tree(c, n, true);
	update_allocation();
	rpage->add_damaged(split->allocation());

	print_tree();

}

void page_t::notebook_close(notebook_t * src) {

	printf("start close notebook\n");
	printf("current notebook stase\n");
	print_tree();


	split_t * ths = dynamic_cast<split_t *>(src->parent());

	/* if parent is viewport return */
	if(ths == 0)
		return;

	tree_t * dst = (src == ths->get_pack0()) ? ths->get_pack1() : ths->get_pack0();


	page_assert(src == ths->get_pack0() || src == ths->get_pack1());

	/* if notebook is default_pop, select another one */
	if (default_window_pop == src) {
		/* if notebook list is empty we probably did something wrong */
		default_window_pop = get_another_notebook(ths, src);
		page_assert(default_window_pop != 0);
		default_window_pop->set_default(true);
		rpage->add_damaged(default_window_pop->allocation());
		/* put it back temporary since destroy will remove it */
	}


	/* move all windows from src to default_window_pop */
	list<managed_window_t *> windows = src->get_clients();
	for(list<managed_window_t *>::iterator i = windows.begin(); i != windows.end(); ++i) {
		remove_window_from_tree((*i));
		insert_window_in_tree((*i), 0, false);
	}

	/* if full want revert to this notebook, update it */
	for(map<managed_window_t *, fullscreen_data_t>::iterator i = fullscreen_client_to_viewport.begin();
			i != fullscreen_client_to_viewport.end(); ++i) {
		if(i->second.revert_notebook == src) {
			i->second.revert_notebook = default_window_pop;
		}
	}


	page_assert(ths->parent() != 0);
	/* remove this split from tree */
	ths->parent()->replace(ths, dst);

	rpage->add_damaged(ths->allocation());

	/* cleanup */
	destroy(src);
	destroy(ths);

	printf("new state for tree\n");
	print_tree();
	printf("finish close_notenook\n");

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


/*
 * Compute the usable desktop area and dock allocation.
 */
void page_t::fix_allocation(viewport_t & v) {
	//printf("fix_allocation %dx%d+%d+%d\n", v.raw_aera.w, v.raw_aera.h,
	//		v.raw_aera.x, v.raw_aera.y);

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
	long xright = _root_position.w - v.raw_aera.x - v.raw_aera.w;
	long xbottom = _root_position.h - v.raw_aera.y - v.raw_aera.h;

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
				box_int_t b(_root_position.w - ps[PS_RIGHT],
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
						_root_position.h - ps[PS_BOTTOM],
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
	final_size.w = _root_position.w - xright - xleft;
	final_size.y = xtop;
	final_size.h = _root_position.h - xbottom - xtop;

	v.set_effective_area(final_size);

	//printf("subarea %dx%d+%d+%d\n", v.effective_aera.w, v.effective_aera.h,
	//		v.effective_aera.x, v.effective_aera.y);
}

split_t * page_t::new_split(split_type_e type) {
	split_t * x = new split_t(type, theme);
	printf("create %s\n", x->get_node_name().c_str());
	return x;
}

notebook_t * page_t::new_notebook() {
	notebook_t * x = new notebook_t(theme);
	printf("create %s\n", x->get_node_name().c_str());
	return x;
}

void page_t::destroy(split_t * x) {
	printf("destroy %s\n", x->get_node_name().c_str());
	delete x;
}

void page_t::destroy(notebook_t * x) {
	printf("destroy %s\n", x->get_node_name().c_str());
	delete x;
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
	//printf("_NET_WM_STATE %s %s from %lu\n", action, name.c_str(), c);

	managed_window_t * mw = find_managed_window_with(c);
	if(mw == 0)
		return;

	if (mw->is(MANAGED_NOTEBOOK)) {

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
				//printf("SET normal\n");
				unfullscreen(mw);
				break;
			case _NET_WM_STATE_ADD:
				//printf("SET fullscreen\n");
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
	}
}

void page_t::update_transient_for(Window w) {

	map<Window, Window>::iterator x = transient_for_cache.find(w);

	/** if there is an old transient for, clear childs **/
	if(x != transient_for_cache.end()) {
		transient_for_tree[x->second].remove(w);
	}

	/** read newer transient for **/
	Window transient_for;
	if (!cnx->read_wm_transient_for(w, &transient_for)) {
		transient_for = None;
	}

	/** store it in cache **/
	transient_for_cache[w] = transient_for;

	/** update the logical tree of windows **/
	transient_for_tree[transient_for].push_back(w);

}

void page_t::cleanup_transient_for_for_window(Window w) {
	map<Window, Window>::iterator x = transient_for_cache.find(w);

	/** if there is an old transient for, clear childs **/
	if(x != transient_for_cache.end()) {
		transient_for_tree[x->second].remove(w);
	}

	transient_for_cache.erase(w);

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
	while(not has_key(already_raise, w_next)) {
		already_raise.insert(w_next);

		map<Window, Window>::iterator x = transient_for_cache.find(w_next);

		if(x == transient_for_cache.end()) {
			update_transient_for(w_next);
		}

		x = transient_for_cache.find(w_next);

		transient_for_tree[x->second].remove(w_next);
		transient_for_tree[x->second].push_back(w_next);

		if (x != transient_for_cache.end()) {
			w_next = x->second;
		} else {
			break;
		}
	}

	/** apply change **/
	update_windows_stack();

}

void page_t::destroy_managed_window(managed_window_t * mw) {
	managed_window.erase(mw);
	fullscreen_client_to_viewport.erase(mw);
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

	::page::compute_client_size_with_constraint(size_hints, wished_width, wished_height,
			width, height);

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
			if (has_key(transient_for_tree, cur.second)) {
				list<Window> childs = transient_for_tree[cur.second];
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
			//printf("    ");
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
	update_client_list();

}

void page_t::unbind_window(managed_window_t * mw) {

	remove_window_from_tree(mw);

	/* update database */
	mw->set_managed_type(MANAGED_FLOATING);
	mw->expose();
	mw->normalize();
	safe_raise_window(mw->orig());
	update_client_list();

}

void page_t::grab_pointer() {
	/* Grab Pointer no other client will get mouse event */
	if (XGrabPointer(cnx->dpy, cnx->get_root_window(), False,
			(ButtonPressMask | ButtonReleaseMask
					| PointerMotionMask),
			GrabModeAsync, GrabModeAsync, None, None,
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

			mode_data_notebook.start_x = 0;
			mode_data_notebook.start_y = 0;
			mode_data_notebook.zone = SELECT_NONE;
			mode_data_notebook.c = 0;
			mode_data_notebook.from = 0;
			mode_data_notebook.ns = 0;

		}
		break;

	case PROCESS_FLOATING_MOVE:
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

			mode_data_bind.start_x = 0;
			mode_data_bind.start_y = 0;
			mode_data_bind.zone = SELECT_NONE;
			mode_data_bind.c = 0;
			mode_data_bind.ns = 0;

		}
		break;
	case PROCESS_SPLIT_GRAB:
		break;
	case PROCESS_FULLSCREEN_MOVE:
		if(mode_data_fullscreen.mw == mw) {
			mode_data_fullscreen.mw = 0;
			mode_data_fullscreen.v = 0;
			pn0->hide();
			process_mode = PROCESS_NORMAL;
		}
		break;
	}
}

/* look for a notebook in tree base, that is deferent from nbk */
notebook_t * page_t::get_another_notebook(tree_t * base, tree_t * nbk) {
	vector<notebook_t *> l;

	if (base == 0) {
		get_notebooks(l);
	} else {
		get_notebooks(base, l);
	}

	if (!l.empty()) {
		if (l.front() != nbk)
			return l.front();
		if (l.back() != nbk)
			return l.back();
	}

	return 0;

}

void page_t::get_notebooks(vector<notebook_t *> & l) {
	for(map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin(); i != viewport_outputs.end(); ++i) {
		get_notebooks(i->second, l);
	}
}

void page_t::get_splits(vector<split_t *> & l) {
	for(map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin(); i != viewport_outputs.end(); ++i) {
		get_splits(i->second, l);
	}
}

notebook_t * page_t::find_notebook_for(managed_window_t * mw) {
	vector<notebook_t *> lst;
	get_notebooks(lst);
	for(vector<notebook_t *>::iterator i = lst.begin(); i != lst.end(); ++i) {
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
		x = (x->parent());
	}
	return 0;
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
			if (has_key(transient_for_tree, cur)) {
				list<Window> childs = transient_for_tree[cur];
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

	final_order.remove(ps->id());
	final_order.push_back(ps->id());

	final_order.remove(pat->id());
	final_order.push_back(pat->id());

	final_order.remove(rpage->id());
	final_order.push_front(rpage->id());

	final_order.reverse();

	/**
	 * convert list to C array, see std::vector API.
	 **/
	vector<Window> v_order(final_order.begin(), final_order.end());
	XRestackWindows(cnx->dpy, &v_order[0], v_order.size());
}

void page_t::update_viewport_layout() {

	/** update root size infos **/

	XWindowAttributes rwa;
	if(!XGetWindowAttributes(cnx->dpy, DefaultRootWindow(cnx->dpy), &rwa)) {
		throw std::runtime_error("FATAL: cannot read root window attributes\n");
	}

	_root_position = box_t<int>(rwa.x, rwa.y, rwa.width, rwa.height);
	set_desktop_geometry(_root_position.w, _root_position.h);

	/** store the newer layout, to cleanup obsolet viewport **/
	map<RRCrtc, viewport_t *> new_layout;

	XRRScreenResources * resources = XRRGetScreenResourcesCurrent(
			cnx->dpy, cnx->get_root_window());

	for (int i = 0; i < resources->ncrtc; ++i) {
		XRRCrtcInfo * info = XRRGetCrtcInfo(cnx->dpy, resources,
				resources->crtcs[i]);
		//printf(
		//		"CrtcInfo: width = %d, height = %d, x = %d, y = %d, noutputs = %d\n",
		//		info->width, info->height, info->x, info->y, info->noutput);

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
		/** fallback to one screen **/
		box_t<int> area(rwa.x, rwa.y, rwa.width, rwa.height);
		/** if this crtc do not has a viewport **/
		if (!has_key(viewport_outputs, (XID)None)) {
			/** then create a new one, and store it in new_layout **/
			new_layout[None] = new viewport_t(theme, area);
		} else {
			/** update allocation, and store it to new_layout **/
			viewport_outputs[None]->set_raw_area(area);
			new_layout[None] = viewport_outputs[None];
		}
		fix_allocation(*(new_layout[0]));
	}

	/** clean up old layout **/
	map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin();
	while(i != viewport_outputs.end()) {
		if(!has_key(new_layout, i->first)) {
			/** destroy this viewport **/
			remove_viewport(i->second);
			destroy_viewport(i->second);
		}
		++i;
	}

	viewport_outputs = new_layout;
}

void  page_t::remove_viewport(viewport_t * v) {

	/* remove fullscreened clients if needed */
	if (v->fullscreen_client != 0) {
		unfullscreen(v->fullscreen_client);
	}

	/* Transfer clients to a valid notebook */
	vector<notebook_t *> nbks;
	get_notebooks(v, nbks);
	for (vector<notebook_t *>::iterator i = nbks.begin(); i != nbks.end();
			++i) {
		if (default_window_pop == *i)
			default_window_pop = get_another_notebook(*i);
		default_window_pop->set_default(true);
		rpage->add_damaged(default_window_pop->allocation());
		list<managed_window_t *> lc = (*i)->get_clients();
		for (list<managed_window_t *>::iterator i = lc.begin();
				i != lc.end(); ++i) {
			default_window_pop->add_client(*i, false);
		}
	}


}

void  page_t::destroy_viewport(viewport_t * v) {
	vector<tree_t *> lst;
	v->get_childs(lst);
	for (vector<tree_t *>::iterator i = lst.begin(); i != lst.end();
			++i) {
		delete *i;
	}
	delete v;
}

/**
 * this function will check if a window must be managed or not.
 * If a window have to be managed, this function manage this window, if not
 * The function create unmanaged window.
 **/
bool page_t::onmap(Window w) {

	if (find_managed_window_with(w))
		return false;
	if (find_unmanaged_window_with(w))
		return false;

	/**
	 * XSync here is mandatory.
	 *
	 * Because:
	 *
	 * client create window, then resize it, then map it.
	 *
	 * resize generate ConfigureRequest, page, reply to it with
	 * ackwoledge_configure_request since the window is not mapped, then
	 * Page receive MapRequest, but if we do not XSync,
	 * ackwoledge_configure_request will resize the window AFTER the moment we
	 * managed this window and the window have not the right default size.
	 *
	 **/
	XSync(cnx->dpy, False);

	XWindowAttributes wa;
	if(not XGetWindowAttributes(cnx->dpy, w, &wa))
		return false;
	if(wa.c_class == InputOnly)
		return false;

	//printf("XX size = %dx%d+%d+%d\n", wa.width, wa.height, wa.x, wa.y);

	Atom type = find_net_wm_type(w, wa.override_redirect);

	if(type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
		Window tf = None;
		if(cnx->read_wm_transient_for(w, &tf)) {
			if(tf != None) {
				type = A(_NET_WM_WINDOW_TYPE_UTILITY);
			}
		}
	}

//	/** MOTIF HACK **/
//	{
//		motif_wm_hints_t motif_hints;
//		if (cnx->read_motif_wm_hints(w, &motif_hints)) {
//			bool is_fullscreen = false;
//
//			list<Atom> state;
//			cnx->read_net_wm_state(w, &state);
//			list<Atom>::iterator x = find(state.begin(), state.end(),
//					A(_NET_WM_STATE_FULLSCREEN));
//			is_fullscreen = (x != state.end());
//
//
//			if (motif_hints.flags & MWM_HINTS_DECORATIONS) {
//				if (not (motif_hints.decorations & MWM_DECOR_BORDER)
//						and not ((motif_hints.decorations & MWM_DECOR_ALL))
//				/* fullscreen windows has no border but are not TOOLTIP */
//				and not is_fullscreen) {
//					type = A(_NET_WM_WINDOW_TYPE_TOOLTIP);
//				}
//
//			}
//		}
//
//	}


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

	if (!wa.override_redirect) {
		if (type == A(_NET_WM_WINDOW_TYPE_DESKTOP)) {
			create_managed_window(w, type, wa);
			return true;
		} else if (type == A(_NET_WM_WINDOW_TYPE_DOCK)) {
			create_unmanaged_window(w, type);
			update_allocation();
		} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLBAR)) {
			create_managed_window(w, type, wa);
			return true;
		} else if (type == A(_NET_WM_WINDOW_TYPE_MENU)) {
			create_managed_window(w, type, wa);
			return true;
		} else if (type == A(_NET_WM_WINDOW_TYPE_UTILITY)) {
			create_managed_window(w, type, wa);
			return true;
		} else if (type == A(_NET_WM_WINDOW_TYPE_SPLASH)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DIALOG)) {
			create_managed_window(w, type, wa);
			return true;
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
			create_managed_window(w, type, wa);
			return true;
		}
	} else {
		if (type == A(_NET_WM_WINDOW_TYPE_DESKTOP)) {
			create_unmanaged_window(w, type);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DOCK)) {
			create_unmanaged_window(w, type);
			update_allocation();
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

	return false;

}


void page_t::create_managed_window(Window w, Atom type, XWindowAttributes const & wa) {

	//printf("manage window %lu\n", w);

	/* make managed window floating when they have parent */
	if (type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
		Window tf = None;
		if (cnx->read_wm_transient_for(w, &tf)) {
			if (tf != None and tf != cnx->get_root_window()) {
				type = A(_NET_WM_WINDOW_TYPE_UTILITY);
			}
		}
	}

	managed_window_t * mw;
	if((type == A(_NET_WM_WINDOW_TYPE_NORMAL)
			|| type == A(_NET_WM_WINDOW_TYPE_DESKTOP))
			&& !cnx->read_wm_transient_for(w)
			&& cnx->motif_has_border(w)) {

		mw = manage(MANAGED_NOTEBOOK, type, w, wa);
		insert_window_in_tree(mw, 0, true);


		/** TODO function **/
		Time time = 0;
		if(get_safe_net_wm_user_time(w, time)) {
			set_focus(mw, time);
		}

	} else {
		mw = manage(MANAGED_FLOATING, type, w, wa);
		mw->normalize();

		Time time = 0;
		if(get_safe_net_wm_user_time(w, time)) {
			set_focus(mw, time);
		}

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

Atom page_t::find_net_wm_type(Window w, bool override_redirect) {

	list<Atom> net_wm_window_type;

	if(!cnx->read_net_wm_window_type(w, &net_wm_window_type)) {
		/**
		 * Fallback from ICCCM.
		 **/

		if(!override_redirect) {
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
		//printf("Check for %s\n", cnx->get_atom_name(*i).c_str());
		if(has_key(known_type, *i))
			return *i;
		++i;
	}

	/* should never happen */
	return None;

}


viewport_t * page_t::find_mouse_viewport(int x, int y) {
	map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin();
	while (i != viewport_outputs.end()) {
		if (i->second->raw_aera.is_inside(x, y))
			return i->second;
		++i;
	}
	return 0;
}

/**
 * Read user time with proper fallback
 * @return: true if successfully find usertime, otherwise false.
 * @output time: if time is found time is set to the found value.
 * @input w: X11 Window ID.
 **/
bool page_t::get_safe_net_wm_user_time(Window w, Time & time) {
	/** TODO function **/
	bool has_time = false;
	Window time_window;

	if (cnx->read_net_wm_user_time(w, &time)) {
		has_time = true;
	} else {
		if (cnx->read_net_wm_user_time_window(w, &time_window)) {
			has_time = cnx->read_net_wm_user_time(time_window, &time);
		}
	}

	return has_time;
}

void page_t::get_notebooks(tree_t * base, vector<notebook_t *> & l) {
	stack<tree_t *> st;
	st.push(base);
	while (not st.empty()) {
		vector<tree_t *> childs = st.top()->get_direct_childs();
		notebook_t * n = dynamic_cast<notebook_t *>(st.top());
		if (n != 0) {
			l.push_back(n);
		}
		st.pop();

		for (vector<tree_t *>::iterator i = childs.begin(); i != childs.end();
				++i) {
			st.push(*i);
		}
	}
}

void page_t::get_splits(tree_t * base, vector<split_t *> & l) {
	vector<tree_t *> lt;
	base->get_childs(lt);
	for (vector<tree_t *>::iterator i = lt.begin(); i != lt.end(); ++i) {
		split_t * s = dynamic_cast<split_t *>(*i);
		if(s != 0) {
			l.push_back(s);
		}
	}
}

}
