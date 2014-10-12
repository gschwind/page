/*
 * page.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
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

using namespace std;

namespace page {

long int const ClientEventMask = (StructureNotifyMask | PropertyChangeMask);

time_t const page_t::default_wait{1000000000L / 120L};

page_t::page_t(int argc, char ** argv) :
		viewport_outputs()
{

	page_areas = nullptr;
	use_internal_compositor = true;
	replace_wm = false;
	char const * conf_file_name = 0;

	_need_render = false;

	/** parse command line **/

	int k = 1;
	while(k < argc) {
		string x = argv[k];
		if(x == "--replace") {
			replace_wm = true;
		} else {
			conf_file_name = argv[k];
		}
		++k;
	}

	process_mode = PROCESS_NORMAL;
	key_press_mode = KEY_PRESS_NORMAL;

	cnx = new display_t();
	rnd = nullptr;

	set_window_cursor(cnx->root(), default_cursor);

	running = false;

	/* load configurations, from lower priority to high one */

	/* load default configuration */
	conf.merge_from_file_if_exist(string(DATA_DIR "/page/page.conf"));

	/* load homedir configuration */
	{
		char const * chome = getenv("HOME");
		if(chome != nullptr) {
			string xhome = chome;
			string file = xhome + "/.page.conf";
			conf.merge_from_file_if_exist(file);
		}
	}

	/* load file in arguments if provided */
	if (conf_file_name != nullptr) {
		string s(conf_file_name);
		conf.merge_from_file_if_exist(s);
	}

	default_window_pop = nullptr;

	page_base_dir = conf.get_string("default", "theme_dir");

	_last_focus_time = 0;
	_last_button_press = 0;

	theme = nullptr;
	rpage = nullptr;

	_client_focused.push_front(nullptr);

	find_key_from_string(conf.get_string("default", "bind_page_quit"), bind_page_quit);
	find_key_from_string(conf.get_string("default", "bind_toggle_fullscreen"), bind_toggle_fullscreen);
	find_key_from_string(conf.get_string("default", "bind_debug_1"), bind_debug_1);
	find_key_from_string(conf.get_string("default", "bind_debug_2"), bind_debug_2);
	find_key_from_string(conf.get_string("default", "bind_debug_3"), bind_debug_3);
	find_key_from_string(conf.get_string("default", "bind_debug_4"), bind_debug_4);

}

page_t::~page_t() {

	XFreeCursor(cnx->dpy(), xc_left_ptr);
	XFreeCursor(cnx->dpy(), xc_fleur);
	XFreeCursor(cnx->dpy(), xc_bottom_left_corner);
	XFreeCursor(cnx->dpy(), xc_bottom_righ_corner);
	XFreeCursor(cnx->dpy(), xc_bottom_side);
	XFreeCursor(cnx->dpy(), xc_left_side);
	XFreeCursor(cnx->dpy(), xc_right_side);
	XFreeCursor(cnx->dpy(), xc_top_right_corner);
	XFreeCursor(cnx->dpy(), xc_top_left_corner);
	XFreeCursor(cnx->dpy(), xc_top_side);

	delete rpage;

	pfm.reset();
	pn0.reset();
	ps.reset();
	pat.reset();
	menu.reset();


	/* get all childs excluding this */
	auto childs = get_all_childs();
	for (auto &i : childs) {

		if(typeid(*i) == typeid(client_managed_t)) {
			client_managed_t * mw = dynamic_cast<client_managed_t *>(i);
			cnx->reparentwindow(mw->orig(), cnx->root(), 0, 0);
		}

		delete i;
	}
	childs.clear();
	/* all client should be in childs, while we already destroyed all child, just clear this list */
	clients.clear();

	if(page_areas != nullptr) {
		delete page_areas;
		page_areas = nullptr;
	}

	if (theme != nullptr)
		delete theme;
	if (rnd != nullptr)
		delete rnd;

	// cleanup cairo, for valgrind happiness.
	cairo_debug_reset_static_data();

	if(cnx != nullptr) {
		/** clean up properies defined by Window Manager **/
		cnx->delete_property(cnx->root(), _NET_SUPPORTED);
		cnx->delete_property(cnx->root(), _NET_CLIENT_LIST);
		cnx->delete_property(cnx->root(), _NET_CLIENT_LIST_STACKING);
		cnx->delete_property(cnx->root(), _NET_ACTIVE_WINDOW);
		cnx->delete_property(cnx->root(), _NET_NUMBER_OF_DESKTOPS);
		cnx->delete_property(cnx->root(), _NET_DESKTOP_GEOMETRY);
		cnx->delete_property(cnx->root(), _NET_DESKTOP_VIEWPORT);
		cnx->delete_property(cnx->root(), _NET_CURRENT_DESKTOP);
		cnx->delete_property(cnx->root(), _NET_DESKTOP_NAMES);
		cnx->delete_property(cnx->root(), _NET_WORKAREA);
		cnx->delete_property(cnx->root(), _NET_SUPPORTING_WM_CHECK);
		cnx->delete_property(cnx->root(), _NET_SHOWING_DESKTOP);

		delete cnx;
	}

}

void page_t::run() {

	/* check for required page extension */
	check_x11_extension();
	/* Before doing anything, trying to register wm and cm */
	create_identity_window();
	register_wm();
	register_cm();

	/** initialise theme **/
	theme = new simple2_theme_t(cnx, conf);

	/* start listen root event before anything each event will be stored to right run later */
	XSelectInput(cnx->dpy(), cnx->root(), ROOT_EVENT_MASK);

	/** load compositor if requested **/
	if (use_internal_compositor) {
		/** try to start compositor, if fail, just ignore it **/
		try {
			rnd = new compositor_t(cnx, damage_event, xshape_event, xrandr_event);
			rnd->renderable_clear();
			//rnd->renderable_add(this);
		} catch (...) {
			rnd = nullptr;
		}
	}

	/**
	 * listen RRCrtcChangeNotifyMask for possible change in screen layout.
	 **/
	XRRSelectInput(cnx->dpy(), cnx->root(), RRCrtcChangeNotifyMask);

	update_viewport_layout();

	/* init page render */
	rpage = new renderable_page_t(cnx, theme, _root_position.w,
			_root_position.h);

	/* create and add popups (overlay) */
	pfm = ptr<popup_frame_move_t>{new popup_frame_move_t(theme)};
	pn0 = ptr<popup_notebook0_t>{new popup_notebook0_t(theme)};
	ps = ptr<popup_split_t>{new popup_split_t(theme)};
	pat = ptr<popup_alt_tab_t>{new popup_alt_tab_t(cnx, theme)};
	menu = ptr<dropdown_menu_t>{new dropdown_menu_t(cnx, theme)};

	xc_left_ptr = XCreateFontCursor(cnx->dpy(), XC_left_ptr);
	xc_fleur = XCreateFontCursor(cnx->dpy(), XC_fleur);

	xc_bottom_left_corner = XCreateFontCursor(cnx->dpy(), XC_bottom_left_corner);
	xc_bottom_righ_corner = XCreateFontCursor(cnx->dpy(), XC_bottom_right_corner);
	xc_bottom_side = XCreateFontCursor(cnx->dpy(), XC_bottom_side);

	xc_left_side = XCreateFontCursor(cnx->dpy(), XC_left_side);
	xc_right_side = XCreateFontCursor(cnx->dpy(), XC_right_side);

	xc_top_right_corner = XCreateFontCursor(cnx->dpy(), XC_top_right_corner);
	xc_top_left_corner = XCreateFontCursor(cnx->dpy(), XC_top_left_corner);
	xc_top_side = XCreateFontCursor(cnx->dpy(), XC_top_side);

	XDefineCursor(cnx->dpy(), cnx->root(), xc_left_ptr);

	default_window_pop = 0;

	default_window_pop = get_another_notebook();
	if (default_window_pop == 0)
		throw std::runtime_error("very bad error");

	if (default_window_pop != 0)
		default_window_pop->set_default(true);

	update_net_supported();

	/* update number of desktop */
	int32_t number_of_desktop = 1;
	cnx->change_property(cnx->root(), _NET_NUMBER_OF_DESKTOPS,
			CARDINAL, 32, &number_of_desktop, 1);

	/* define desktop geometry */
	set_desktop_geometry(_root_position.w, _root_position.h);

	/* set viewport */
	long viewport[2] = { 0, 0 };
	cnx->change_property(cnx->root(), _NET_DESKTOP_VIEWPORT,
			CARDINAL, 32, viewport, 2);

	/* set current desktop */
	long current_desktop = 0;
	cnx->change_property(cnx->root(), _NET_CURRENT_DESKTOP, CARDINAL,
			32, &current_desktop, 1);

	long showing_desktop = 0;
	cnx->change_property(cnx->root(), _NET_SHOWING_DESKTOP, CARDINAL,
			32, &showing_desktop, 1);

	char const desktop_name[10] = "NoName";
	cnx->change_property(cnx->root(), _NET_DESKTOP_NAMES,
			UTF8_STRING, 8, desktop_name, (strlen(desktop_name) + 2));

	XIconSize icon_size;
	icon_size.min_width = 16;
	icon_size.min_height = 16;
	icon_size.max_width = 16;
	icon_size.max_height = 16;
	icon_size.width_inc = 1;
	icon_size.height_inc = 1;
	XSetIconSizes(cnx->dpy(), cnx->root(), &icon_size, 1);

	/* setup _NET_ACTIVE_WINDOW */
	set_focus(0, 0);

	scan();
	update_windows_stack();

	long workarea[4];
	workarea[0] = 0;
	workarea[1] = 0;
	workarea[2] = _root_position.w;
	workarea[3] = _root_position.h;
	cnx->change_property(cnx->root(), _NET_WORKAREA, CARDINAL, 32,
			workarea, 4);

	rpage->add_damaged(_root_position);

	update_keymap();
	update_grabkey();

	/**
	 * This grab will freeze input for all client, all mouse button, until
	 * we choose what to do with them with XAllowEvents. we can choose to keep
	 * grabbing events or release event and allow further processing by other clients.
	 **/
	XGrabButton(cnx->dpy(), AnyButton, AnyModifier, cnx->root(), False,
	ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
	GrabModeSync, GrabModeAsync, None, None);


	_max_wait = default_wait;
	_next_frame.get_time();

	fd_set fds_read;
	fd_set fds_intr;

	int max = cnx->fd();

	if (rnd != nullptr) {
		rnd->render();
		rnd->xflush();
	}

	update_allocation();
	XFlush(cnx->dpy());
	running = true;
	while (running) {

		FD_ZERO(&fds_read);
		FD_ZERO(&fds_intr);

		FD_SET(cnx->fd(), &fds_read);
		FD_SET(cnx->fd(), &fds_intr);

		/**
		 * wait for data in both X11 connection streams (compositor and page)
		 **/
		timespec max_wait = _max_wait;
		int nfd = pselect(max + 1, &fds_read, NULL, &fds_intr, &max_wait,
		NULL);

		while (cnx->has_pending_events()) {
			if(rnd != nullptr) {
				rnd->process_event(*(cnx->front_event()));
			}

			process_event(*(cnx->front_event()));
			cnx->pop_event();
		}

		if (rnd != nullptr) {
			/** limit FPS **/
			time_t cur_tic;
			cur_tic.get_time();
			if (cur_tic > _next_frame) {
				rnd->clear_renderable();
				vector<ptr<renderable_t>> ret;
				prepare_render(ret, cur_tic);
				rnd->push_back_renderable(ret);
				rnd->render();
				XSync(cnx->dpy(), False);
				cur_tic.get_time();

				/** slow down frame if render is slow **/
				_next_frame = cur_tic + default_wait;
				_max_wait = default_wait;
			} else {
				_max_wait = _next_frame - cur_tic;
			}
		}
	}
}

client_managed_t * page_t::manage(Atom net_wm_type, ptr<client_properties_t> c) {
	cnx->add_to_save_set(c->id());
	/* set border to zero */
	XSetWindowBorder(cnx->dpy(), c->id(), 0);
	/* assign window to desktop 0 */
	c->set_net_wm_desktop(0);
	client_managed_t * mw = new client_managed_t{net_wm_type, c, theme};
	add_client(mw);

	printf("managing : '%s'\n", mw->title().c_str());
	return mw;
}

void page_t::unmanage(client_managed_t * mw) {
	/* if window is in move/resize/notebook move, do cleanup */
	cleanup_grab(mw);

	printf("unmanaging : '%s'\n", mw->title().c_str());

	if (has_key(fullscreen_client_to_viewport, mw)) {
		fullscreen_data_t & data = fullscreen_client_to_viewport[mw];

		viewport_t * v = data.viewport;
		fullscreen_client_to_viewport.erase(mw);
		v->_is_visible = true;

		/* map all notebook window */
		auto ns = get_notebooks(v);
		for (auto i : ns) {
			i->map_all();
		}
	}

	detach(mw);

	/* if managed window have active clients */
	list<tree_t *> subclient = mw->childs();
	for(auto i: subclient) {
		client_base_t * c = dynamic_cast<client_base_t *>(i);
		if(c != nullptr) {
			insert_in_tree_using_transient_for(c);
		}
	}

	/** if the window is destroyed, this not work, see fix on destroy **/
	_client_focused.remove(mw);

	/* as recommended by EWMH delete _NET_WM_STATE when window become unmanaged */
	mw->net_wm_state_delete();
	mw->wm_state_delete();

	clients.erase(mw->orig());
	update_client_list();
	update_allocation();
	rpage->mark_durty();

	delete mw;

}

void page_t::scan() {

	unsigned int num;
	Window d1, d2, *wins = 0;

	cnx->grab();
	cnx->fetch_pending_events();

	if (XQueryTree(cnx->dpy(), cnx->root(), &d1, &d2, &wins, &num)) {

		for (unsigned i = 0; i < num; ++i) {
			Window w = wins[i];

			ptr<client_properties_t> c{new client_properties_t(cnx, w)};
			if (!c->read_window_attributes()) {
				continue;
			}

			if(c->wa()->_class == InputOnly) {
				continue;
			}

			c->read_all_properties();

			if (c->wa()->map_state != IsUnmapped) {
				onmap(w);
			} else {
				/**
				 * if the window is not map check if previous windows manager has set WM_STATE to iconic
				 * if this is the case, that mean that is a managed window, otherwise it is a WithDrwn window
				 **/
				if (c->wm_state() != nullptr) {
					if (c->wm_state()->state == IconicState) {
						onmap(w);
					}
				}
			}
		}
		XFree(wins);
	}

	update_client_list();
	update_allocation();
	update_windows_stack();

	cnx->ungrab();

}

void page_t::update_net_supported() {
	vector<long> supported_list;

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
	supported_list.push_back(A(_NET_WM_OPAQUE_REGION));

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

	cnx->change_property(cnx->root(), _NET_SUPPORTED, ATOM, 32, &supported_list[0],
			supported_list.size());

}

void page_t::update_client_list() {
	set<Window> s_client_list;
	set<Window> s_client_list_stack;
	list<client_managed_t *> lst = get_managed_windows();

	for (auto i : lst) {
		s_client_list.insert(i->orig());
		s_client_list_stack.insert(i->orig());
	}

	vector<Window> client_list(s_client_list.begin(), s_client_list.end());
	vector<Window> client_list_stack(s_client_list_stack.begin(),
			s_client_list_stack.end());

	cnx->change_property(cnx->root(), _NET_CLIENT_LIST_STACKING,
			WINDOW, 32, &client_list_stack[0], client_list_stack.size());
	cnx->change_property(cnx->root(), _NET_CLIENT_LIST, WINDOW, 32,
			&client_list[0], client_list.size());

}

void page_t::process_event(XKeyEvent const & e) {

	if(_last_focus_time > e.time) {
		_last_focus_time = e.time;
	}

	printf("%s key = %d, mod4 = %s, mod1 = %s\n",
			e.type == KeyPress ? "KeyPress" : "KeyRelease",
			e.keycode,
			e.state & Mod4Mask ? "true" : "false",
			e.state & Mod1Mask ? "true" : "false");


	/* get KeyCode for Unmodified Key */
	KeySym k = keymap->get(e.keycode);

	if (k == 0)
		return;

	if(e.type == KeyPress) {

		if(k == bind_page_quit.ks and (e.state & bind_page_quit.mod)) {
			running = false;
		}

		if(k == bind_toggle_fullscreen.ks and (e.state & bind_toggle_fullscreen.mod)) {
			if(not _client_focused.empty()) {
				if(_client_focused.front() != nullptr) {
					toggle_fullscreen(_client_focused.front());
				}
			}
		}

		if(k == bind_debug_1.ks and (e.state & bind_debug_1.mod)) {
			if(rnd->show_fps()) {
				rnd->set_show_fps(false);
			} else {
				rnd->set_show_fps(true);
			}
		}

		if(k == bind_debug_2.ks and (e.state & bind_debug_2.mod)) {
			if(rnd->show_damaged()) {
				rnd->set_show_damaged(false);
			} else {
				rnd->set_show_damaged(true);
			}
		}

		if(k == bind_debug_3.ks and (e.state & bind_debug_3.mod)) {
			if(rnd->show_opac()) {
				rnd->set_show_opac(false);
			} else {
				rnd->set_show_opac(true);
			}
		}

		if(k == bind_debug_4.ks and (e.state & bind_debug_4.mod)) {
			update_windows_stack();
			print_tree(0);
			list<client_managed_t *> lst = get_managed_windows();
			for (auto i : lst) {
				switch (i->get_type()) {
				case MANAGED_NOTEBOOK:
					cout << "[" << i->orig() << "] notebook : " << i->title()
							<< endl;
					break;
				case MANAGED_FLOATING:
					cout << "[" << i->orig() << "] floating : " << i->title()
							<< endl;
					break;
				case MANAGED_FULLSCREEN:
					cout << "[" << i->orig() << "] fullscreen : " << i->title()
							<< endl;
					break;
				}
			}
		}


		if (k == XK_Tab and ((e.state & 0x0f) == Mod1Mask)) {

			list<client_managed_t *> managed_window = get_managed_windows();

			if (key_press_mode == KEY_PRESS_NORMAL and not managed_window.empty()) {

				/* Grab keyboard */
				XGrabKeyboard(e.display, cnx->root(), False, GrabModeAsync, GrabModeAsync,
						e.time);

				/** Continue to play event as usual (Alt+Tab is in Sync mode) **/
				XAllowEvents(e.display, AsyncKeyboard, e.time);

				key_press_mode = KEY_PRESS_ALT_TAB;
				key_mode_data.selected = _client_focused.front();

				int sel = 0;

				vector<cycle_window_entry_t *> v;
				int s = 0;

				for (auto i : managed_window) {
					icon64 * icon = new icon64(*i);
					cycle_window_entry_t * cy = new cycle_window_entry_t(i, i->title(), icon);
					v.push_back(cy);
					if (i == _client_focused.front()) {
						sel = s;
					}
					++s;
				}

				pat->update_window(v, sel);

				viewport_t * viewport = viewport_outputs.begin()->second;

				int y = v.size() / 4 + 1;

				pat->move_resize(
						i_rect(
								viewport->_raw_aera.x
										+ (viewport->_raw_aera.w - 80 * 4) / 2,
								viewport->_raw_aera.y
										+ (viewport->_raw_aera.h - y * 80) / 2,
								80 * 4, y * 80));
				pat->show();



			} else {
				XAllowEvents(e.display, ReplayKeyboard, e.time);
			}


			pat->select_next();
			_need_render = true;

		}



	} else {
		/** here we guess Mod1 is bound to Alt **/
		if ((XK_Alt_L == k or XK_Alt_R == k)
				and key_press_mode == KEY_PRESS_ALT_TAB) {

			/** Ungrab Keyboard **/
			XUngrabKeyboard(e.display, e.time);

			key_press_mode = KEY_PRESS_NORMAL;

			/**
			 * do not use dynamic_cast because managed window can be already
			 * destroyed.
			 **/
			client_managed_t * mw =
					reinterpret_cast<client_managed_t *>(pat->get_selected());
			set_focus(mw, e.time);

			pat->hide();
		}
	}

}

/* Button event make page to grab pointer */
void page_t::process_event_press(XButtonEvent const & e) {

	client_managed_t * mw = find_managed_window_with(e.window);

	switch (process_mode) {
	case PROCESS_NORMAL:

		if (e.window == cnx->root() && e.subwindow == None && e.button == Button1) {

			update_page_areas();

			page_event_t * b = 0;
			for (vector<page_event_t>::iterator i = page_areas->begin();
					i != page_areas->end(); ++i) {
				//printf("box = %s => %s\n", (i)->position.to_string().c_str(), typeid(*i).name());
				if ((*i).position.is_inside(e.x, e.y)) {
					b = &(*i);
					break;
				}
			}

			if (b != nullptr) {

				if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT) {
					process_mode = PROCESS_NOTEBOOK_GRAB;
					mode_data_notebook.c = const_cast<client_managed_t*>(b->clt);
					mode_data_notebook.from = const_cast<notebook_t*>(b->nbk);
					mode_data_notebook.ns = nullptr;
					mode_data_notebook.zone = SELECT_NONE;
					mode_data_notebook.ev = *b;

					pn0->move_resize(mode_data_notebook.from->tab_area);
					pn0->update_window(mode_data_notebook.c);
					rpage->add_damaged(mode_data_notebook.from->allocation());
				} else if (b->type == PAGE_EVENT_NOTEBOOK_CLOSE) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = 0;
					mode_data_notebook.from = const_cast<notebook_t*>(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_HSPLIT) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = 0;
					mode_data_notebook.from = const_cast<notebook_t*>(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_VSPLIT) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = 0;
					mode_data_notebook.from = const_cast<notebook_t*>(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_MARK) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = 0;
					mode_data_notebook.from = const_cast<notebook_t*>(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT_CLOSE) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = const_cast<client_managed_t*>(b->clt);
					mode_data_notebook.from = const_cast<notebook_t*>(b->nbk);
					mode_data_notebook.ns = 0;
				} else if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT_UNBIND) {
					process_mode = PROCESS_NOTEBOOK_BUTTON_PRESS;
					mode_data_notebook.c = const_cast<client_managed_t*>(b->clt);
					mode_data_notebook.from = const_cast<notebook_t*>(b->nbk);
					mode_data_notebook.ns = 0;

				} else if (b->type == PAGE_EVENT_SPLIT) {

					process_mode = PROCESS_SPLIT_GRAB;

					mode_data_split.split_ratio = b->spt->split();
					mode_data_split.split = const_cast<split_t*>(b->spt);
					mode_data_split.slider_area =
							mode_data_split.split->get_split_bar_area();

					/* show split overlay */
					ps->set_current_split(b->spt);
					ps->set_position(mode_data_split.split_ratio);
					ps->move_resize(mode_data_split.split->allocation());
					ps->show();
				} else if (b->type == PAGE_EVENT_NOTEBOOK_MENU) {
					process_mode = PROCESS_NOTEBOOK_MENU;
					mode_data_notebook_menu.from = const_cast<notebook_t*>(b->nbk);
					mode_data_notebook_menu.b = b->position;

					list<client_managed_t *> managed_window = mode_data_notebook_menu.from->get_clients();

					int sel = 0;

					vector<dropdown_menu_entry_t *> v;
					int s = 0;

					for (auto i : managed_window) {
						dropdown_menu_entry_t * cy = new dropdown_menu_entry_t(i, i->icon(), i->title());
						v.push_back(cy);
						if (i == _client_focused.front()) {
							sel = s;
						}
						++s;
					}
					menu->update_window(v, sel);
					menu->move(mode_data_notebook_menu.from->allocation().x, mode_data_notebook_menu.from->allocation().y + 20);
					menu->show();
					//rpage->add_damaged(mode_data_notebook.from->allocation());
				}
			}

		}


		if (mw != nullptr) {

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
				mode_data_floating.button = Button1;

				//pfm->move_resize(mw->get_base_position());
				//pfm->update_window(mw, mw->title());
				//pfm->show();

				if ((e.state & ControlMask)) {
					process_mode = PROCESS_FLOATING_RESIZE;
					mode_data_floating.mode = RESIZE_BOTTOM_RIGHT;
					XDefineCursor(cnx->dpy(), mw->base(), xc_bottom_righ_corner);
					XDefineCursor(cnx->dpy(), mw->orig(), xc_bottom_righ_corner);
				} else {
					safe_raise_window(mw);
					process_mode = PROCESS_FLOATING_MOVE;
					XDefineCursor(cnx->dpy(), mw->base(), xc_fleur);
					XDefineCursor(cnx->dpy(), mw->orig(), xc_fleur);
				}


			} else if (mw->is(MANAGED_FLOATING) and e.button == Button1
					and e.subwindow != mw->orig()) {
				mw->update_floating_areas();
				auto const * l = mw->floating_areas();
				floating_event_t const * b = 0;
				for (auto &i : (*l)) {
					if(i.position.is_inside(e.x, e.y)) {
						b = &i;
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
					mode_data_floating.button = Button1;

					if (b->type == FLOATING_EVENT_CLOSE) {

						mode_data_floating.f = mw;
						process_mode = PROCESS_FLOATING_CLOSE;

					} else if (b->type == FLOATING_EVENT_BIND) {

						mode_data_bind.c = mw;
						mode_data_bind.ns = 0;
						mode_data_bind.zone = SELECT_NONE;

						pn0->move_resize(mode_data_bind.c->get_base_position());
						pn0->update_window(mw);

						process_mode = PROCESS_FLOATING_BIND;

					} else if (b->type == FLOATING_EVENT_TITLE) {

						//pfm->move_resize(mw->get_base_position());
						//pfm->update_window(mw, mw->title());
						//pfm->show();

						safe_raise_window(mw);
						process_mode = PROCESS_FLOATING_MOVE;
						XDefineCursor(cnx->dpy(), mw->base(), xc_fleur);
					} else {

						//pfm->move_resize(mw->get_base_position());
						//pfm->update_window(mw, mw->title());
						//pfm->show();

						if (b->type == FLOATING_EVENT_GRIP_TOP) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_TOP;
							XDefineCursor(cnx->dpy(), mw->base(), xc_top_side);
						} else if (b->type == FLOATING_EVENT_GRIP_BOTTOM) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_BOTTOM;
							XDefineCursor(cnx->dpy(), mw->base(), xc_bottom_side);
						} else if (b->type == FLOATING_EVENT_GRIP_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_LEFT;
							XDefineCursor(cnx->dpy(), mw->base(), xc_left_side);
						} else if (b->type == FLOATING_EVENT_GRIP_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_RIGHT;
							XDefineCursor(cnx->dpy(), mw->base(), xc_right_side);
						} else if (b->type == FLOATING_EVENT_GRIP_TOP_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_TOP_LEFT;
							XDefineCursor(cnx->dpy(), mw->base(), xc_top_left_corner);
						} else if (b->type == FLOATING_EVENT_GRIP_TOP_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_TOP_RIGHT;
							XDefineCursor(cnx->dpy(), mw->base(), xc_top_right_corner);
						} else if (b->type == FLOATING_EVENT_GRIP_BOTTOM_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_BOTTOM_LEFT;
							XDefineCursor(cnx->dpy(), mw->base(), xc_bottom_left_corner);
						} else if (b->type == FLOATING_EVENT_GRIP_BOTTOM_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_BOTTOM_RIGHT;
							XDefineCursor(cnx->dpy(), mw->base(), xc_bottom_righ_corner);
						} else {
							safe_raise_window(mw);
							process_mode = PROCESS_FLOATING_MOVE;
							XDefineCursor(cnx->dpy(), mw->base(), xc_fleur);
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
					pn0->update_window(mw);
					pn0->show();
					pn0->move_resize(v->_raw_aera);
				}
			} else if (mw->is(MANAGED_NOTEBOOK) and e.button == (Button1)
					and (e.state & (Mod1Mask))) {

				notebook_t * n = find_parent_notebook_for(mw);

				process_mode = PROCESS_NOTEBOOK_GRAB;
				mode_data_notebook.c = mw;
				mode_data_notebook.from = n;
				mode_data_notebook.ns = 0;
				mode_data_notebook.zone = SELECT_NONE;

				pn0->move_resize(mode_data_notebook.from->tab_area);
				pn0->update_window(mw);

				//mode_data_notebook.from->set_selected(mode_data_notebook.c);
				rpage->add_damaged(mode_data_notebook.from->allocation());

			}
		}

		break;
	default:
		break;
	}


	/**
	 * if no change happenned to process mode
	 * We allow events (remove the grab), and focus those window.
	 **/
	if (process_mode == PROCESS_NORMAL) {
		XAllowEvents(cnx->dpy(), ReplayPointer, CurrentTime);
		client_managed_t * mw = find_managed_window_with(e.window);
		if (mw != nullptr) {
			set_focus(mw, e.time);
		}
	} else {
		/* Do not replay events, grab them and process them until Release Button */
		XAllowEvents(cnx->dpy(), AsyncPointer, e.time);
	}

}

void page_t::process_event_release(XButtonEvent const & e) {

	switch (process_mode) {
	case PROCESS_NORMAL:
		fprintf(stderr, "Warning: release and normal mode are incompatible\n");
		break;
	case PROCESS_SPLIT_GRAB:
		if (e.button == Button1) {
			process_mode = PROCESS_NORMAL;
			ps->hide();
			rnd->add_damaged(mode_data_split.split->allocation());
			mode_data_split.split->set_split(mode_data_split.split_ratio);
			rpage->add_damaged(mode_data_split.split->allocation());
			mode_data_split.reset();

		}
		break;
	case PROCESS_NOTEBOOK_GRAB:
		if (e.button == Button1) {
			process_mode = PROCESS_NORMAL;

			pn0->hide();

			if (mode_data_notebook.zone == SELECT_TAB
					&& mode_data_notebook.ns != 0
					&& mode_data_notebook.ns != mode_data_notebook.from) {
				detach(mode_data_notebook.c);
				insert_window_in_notebook(mode_data_notebook.c,
						mode_data_notebook.ns, true);
			} else if (mode_data_notebook.zone == SELECT_TOP
					&& mode_data_notebook.ns != 0) {
				split_top(mode_data_notebook.ns, mode_data_notebook.c);
			} else if (mode_data_notebook.zone == SELECT_LEFT
					&& mode_data_notebook.ns != 0) {
				split_left(mode_data_notebook.ns, mode_data_notebook.c);
			} else if (mode_data_notebook.zone == SELECT_BOTTOM
					&& mode_data_notebook.ns != 0) {
				split_bottom(mode_data_notebook.ns, mode_data_notebook.c);
			} else if (mode_data_notebook.zone == SELECT_RIGHT
					&& mode_data_notebook.ns != 0) {
				split_right(mode_data_notebook.ns, mode_data_notebook.c);
			} else if (mode_data_notebook.zone == SELECT_CENTER
					&& mode_data_notebook.ns != 0) {
				unbind_window(mode_data_notebook.c);
			} else {

				if(_client_focused.empty()) {
					set_focus(mode_data_notebook.c, e.time);
					mode_data_notebook.from->set_selected(mode_data_notebook.c);
				} else {
					if(mode_data_notebook.from->get_selected() == mode_data_notebook.c
							and _client_focused.front() == mode_data_notebook.c) {
						/** focus root **/
						set_focus(nullptr, e.time);
						/** iconify **/
						mode_data_notebook.from->iconify_client(mode_data_notebook.c);
					} else {
						set_focus(mode_data_notebook.c, e.time);
						mode_data_notebook.from->set_selected(mode_data_notebook.c);
					}
				}

			}

			/* Automatically close empty notebook (disabled) */
//			if (mode_data_notebook.from->_clients.empty()
//					&& mode_data_notebook.from->parent() != 0) {
//				notebook_close(mode_data_notebook.from);
//				update_allocation();
//			}
			set_focus(mode_data_notebook.c, e.time);
			if (mode_data_notebook.from != nullptr)
				rpage->add_damaged(mode_data_notebook.from->allocation());
			if (mode_data_notebook.ns != nullptr)
				rpage->add_damaged(mode_data_notebook.ns->allocation());

			mode_data_notebook.reset();
		}
		break;
	case PROCESS_NOTEBOOK_BUTTON_PRESS:
		if (e.button == Button1) {
			process_mode = PROCESS_NORMAL;

			{
				page_event_t * b = 0;
				for (auto &i : *page_areas) {
					if (i.position.is_inside(e.x, e.y)) {
						b = &i;
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
						if (default_window_pop != 0) {
							default_window_pop->set_default(false);
							rpage->add_damaged(
									default_window_pop->allocation());
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

			mode_data_notebook.reset();

		}
		break;
	case PROCESS_FLOATING_MOVE:
		if (e.button == Button1) {
			pfm->hide();

			XUndefineCursor(cnx->dpy(), mode_data_floating.f->base());
			XUndefineCursor(cnx->dpy(), mode_data_floating.f->orig());

			mode_data_floating.f->set_floating_wished_position(
					mode_data_floating.final_position);
			mode_data_floating.f->reconfigure();

			set_focus(mode_data_floating.f, e.time);

			process_mode = PROCESS_NORMAL;
			mode_data_floating.reset();
		}
		break;
	case PROCESS_FLOATING_MOVE_BY_CLIENT:
		if (e.button == mode_data_floating.button) {
			pfm->hide();

			XUngrabPointer(cnx->dpy(), e.time);

			mode_data_floating.f->set_floating_wished_position(
					mode_data_floating.final_position);
			mode_data_floating.f->reconfigure();

			set_focus(mode_data_floating.f, e.time);

			process_mode = PROCESS_NORMAL;
			mode_data_floating.reset();
		}
		break;
	case PROCESS_FLOATING_RESIZE:
		if (e.button == Button1) {
			pfm->hide();

			XUndefineCursor(cnx->dpy(), mode_data_floating.f->base());
			XUndefineCursor(cnx->dpy(), mode_data_floating.f->orig());

			mode_data_floating.f->set_floating_wished_position(
					mode_data_floating.final_position);
			mode_data_floating.f->reconfigure();

			set_focus(mode_data_floating.f, e.time);
			process_mode = PROCESS_NORMAL;

			mode_data_floating.reset();
		}
		break;
	case PROCESS_FLOATING_RESIZE_BY_CLIENT:
		if (e.button == mode_data_floating.button) {
			//pfm->hide();

			XUngrabPointer(cnx->dpy(), e.time);

			mode_data_floating.f->set_floating_wished_position(
					mode_data_floating.final_position);
			mode_data_floating.f->reconfigure();

			set_focus(mode_data_floating.f, e.time);

			process_mode = PROCESS_NORMAL;
			mode_data_floating.reset();
		}
		break;
	case PROCESS_FLOATING_CLOSE:

		if (e.button == Button1) {
			client_managed_t * mw = mode_data_floating.f;
			mw->delete_window(e.time);
			/* cleanup */
			process_mode = PROCESS_NORMAL;
			mode_data_floating.reset();
		}

		break;

	case PROCESS_FLOATING_BIND:
		if (e.button == Button1) {
			process_mode = PROCESS_NORMAL;

			pn0->hide();

			set_focus(mode_data_bind.c, e.time);

			if (mode_data_bind.zone == SELECT_TAB && mode_data_bind.ns != 0) {
				mode_data_bind.c->set_managed_type(MANAGED_NOTEBOOK);
				insert_window_in_notebook(mode_data_bind.c, mode_data_bind.ns,
						true);

				safe_raise_window(mode_data_bind.c);
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
				bind_window(mode_data_bind.c, true);
			}

			process_mode = PROCESS_NORMAL;
			mode_data_bind.reset();

		}

		break;

	case PROCESS_FULLSCREEN_MOVE:

		if (e.button == Button1) {

			process_mode = PROCESS_NORMAL;

			viewport_t * v = find_mouse_viewport(e.x_root, e.y_root);

			if (v != 0) {
				if (v != mode_data_fullscreen.v) {
					pn0->move_resize(v->_raw_aera);
					mode_data_fullscreen.v = v;
				}
			}

			pn0->hide();

			if (mode_data_fullscreen.v != nullptr
					and mode_data_fullscreen.mw != nullptr) {
				// check if we realy need to move
				if (mode_data_fullscreen.v
						!= fullscreen_client_to_viewport[mode_data_fullscreen.mw].viewport) {
					// finally fullscreen the client.
					fullscreen(mode_data_fullscreen.mw, mode_data_fullscreen.v);
				}
			}

			mode_data_fullscreen.reset();

		}

		break;
	case PROCESS_NOTEBOOK_MENU:
		if (e.button == Button1) {
			if(mode_data_notebook_menu.b.is_inside(e.x, e.y) and not mode_data_notebook_menu.active_grab) {
				mode_data_notebook_menu.active_grab = true;
				XGrabPointer(cnx->dpy(),cnx->root(), True, ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None, e.time);
			} else {
				if (mode_data_notebook_menu.active_grab) {
					XUngrabPointer(cnx->dpy(), e.time);
					mode_data_notebook_menu.active_grab = false;
				}

				process_mode = PROCESS_NORMAL;
				if (menu->position().is_inside(e.x_root, e.y_root)) {
					int selx = (int) floor(
							(e.y_root - menu->position().y) / 24.0);
					menu->set_selected(selx);
					mode_data_notebook_menu.from->set_selected(
							const_cast<client_managed_t*>(menu->get_selected()));

					rpage->add_damaged(
							mode_data_notebook_menu.from->allocation());
				}
				mode_data_notebook_menu.from = nullptr;
				menu->hide();
				rnd->add_damaged(menu->get_opaque_region());
			}
		}
		break;
	default:
		if (e.button == Button1) {
			process_mode = PROCESS_NORMAL;
		}
		break;
	}
}

void page_t::process_event(XMotionEvent const & e) {

	if(_last_focus_time > e.time) {
		_last_focus_time = e.time;
	}

	XEvent ev;
	i_rect old_area;
	i_rect new_position;
	static int count = 0;
	count++;
	switch (process_mode) {
	case PROCESS_NORMAL:
		fprintf(stderr, "Warning: This case should not happen %s:%d\n", __FILE__, __LINE__);
		break;
	case PROCESS_SPLIT_GRAB:

		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy(), Button1MotionMask, &ev));

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


		ps->set_position(mode_data_split.split_ratio);
		_need_render = true;

		break;
	case PROCESS_NOTEBOOK_GRAB:
		{
		/* Get latest know motion event */
		ev.xmotion = e;
		while (XCheckMaskEvent(cnx->dpy(), Button1MotionMask, &ev))
			continue;

		/* do not start drag&drop for small move */
		if (!mode_data_notebook.ev.position.is_inside(ev.xmotion.x_root, ev.xmotion.y_root)) {
			if(!pn0->is_visible())
				pn0->show();
		}

		++count;

		auto ln = get_notebooks();
		for (auto i : ln) {
			if (i->tab_area.is_inside(ev.xmotion.x_root, ev.xmotion.y_root)) {
				if (mode_data_notebook.zone != SELECT_TAB
						|| mode_data_notebook.ns != i) {
					mode_data_notebook.zone = SELECT_TAB;
					mode_data_notebook.ns = i;
					update_popup_position(pn0, i->tab_area);
				}
				break;
			} else if (i->right_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				if (mode_data_notebook.zone != SELECT_RIGHT
						|| mode_data_notebook.ns != i) {
					mode_data_notebook.zone = SELECT_RIGHT;
					mode_data_notebook.ns = i;
					update_popup_position(pn0, i->popup_right_area);
				}
				break;
			} else if (i->top_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				if (mode_data_notebook.zone != SELECT_TOP
						|| mode_data_notebook.ns != i) {
					mode_data_notebook.zone = SELECT_TOP;
					mode_data_notebook.ns = i;
					update_popup_position(pn0, i->popup_top_area);
				}
				break;
			} else if (i->bottom_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				if (mode_data_notebook.zone != SELECT_BOTTOM
						|| mode_data_notebook.ns != i) {
					mode_data_notebook.zone = SELECT_BOTTOM;
					mode_data_notebook.ns = i;
					update_popup_position(pn0, i->popup_bottom_area);
				}
				break;
			} else if (i->left_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				if (mode_data_notebook.zone != SELECT_LEFT
						|| mode_data_notebook.ns != i) {
					mode_data_notebook.zone = SELECT_LEFT;
					mode_data_notebook.ns = i;
					update_popup_position(pn0, i->popup_left_area);
				}
				break;
			} else if (i->popup_center_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				if (mode_data_notebook.zone != SELECT_CENTER
						|| mode_data_notebook.ns != i) {
					mode_data_notebook.zone = SELECT_CENTER;
					mode_data_notebook.ns = i;
					update_popup_position(pn0, i->popup_center_area);
				}
				break;
			}
		}
	}

		break;
	case PROCESS_FLOATING_MOVE: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy(), Button1MotionMask, &ev));


		rnd->add_damaged(mode_data_floating.f->visible_area());

		/* compute new window position */
		i_rect new_position = mode_data_floating.original_position;
		new_position.x += e.x_root - mode_data_floating.x_root;
		new_position.y += e.y_root - mode_data_floating.y_root;
		mode_data_floating.final_position = new_position;
		mode_data_floating.f->set_floating_wished_position(new_position);
		mode_data_floating.f->reconfigure();

		_need_render = true;
		break;
	}
	case PROCESS_FLOATING_MOVE_BY_CLIENT: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy(), ButtonMotionMask, &ev));

		rnd->add_damaged(mode_data_floating.f->visible_area());

		/* compute new window position */
		i_rect new_position = mode_data_floating.original_position;
		new_position.x += e.x_root - mode_data_floating.x_root;
		new_position.y += e.y_root - mode_data_floating.y_root;
		mode_data_floating.final_position = new_position;
		mode_data_floating.f->set_floating_wished_position(new_position);
		mode_data_floating.f->reconfigure();

		_need_render = true;
		break;
	}
	case PROCESS_FLOATING_RESIZE: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy(), Button1MotionMask, &ev));

		rnd->add_damaged(mode_data_floating.f->visible_area());

		i_rect size = mode_data_floating.original_position;

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

		/* apply normal hints */
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

		i_rect popup_new_position = size;
		popup_new_position.x -= theme->floating.margin.left;
		popup_new_position.y -= theme->floating.margin.top;
		popup_new_position.w += theme->floating.margin.left + theme->floating.margin.right;
		popup_new_position.h += theme->floating.margin.top + theme->floating.margin.bottom;

		update_popup_position(pfm, popup_new_position);
		_need_render = true;
		break;
	}
	case PROCESS_FLOATING_RESIZE_BY_CLIENT: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx->dpy(), ButtonMotionMask, &ev));

		rnd->add_damaged(mode_data_floating.f->visible_area());

		i_rect size = mode_data_floating.original_position;

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
		//mode_data_floating.f->reconfigure();

		i_rect popup_new_position = size;
		popup_new_position.x -= theme->floating.margin.left;
		popup_new_position.y -= theme->floating.margin.top;
		popup_new_position.w += theme->floating.margin.left + theme->floating.margin.right;
		popup_new_position.h += theme->floating.margin.top + theme->floating.margin.bottom;

		update_popup_position(pfm, popup_new_position);
		_need_render = true;
		break;
	}
	case PROCESS_FLOATING_CLOSE:
		break;
	case PROCESS_FLOATING_BIND: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while (XCheckMaskEvent(cnx->dpy(), Button1MotionMask, &ev))
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

		auto ln = get_notebooks();

		for(auto i : ln) {
			if (i->tab_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				if (mode_data_bind.zone != SELECT_TAB
						|| mode_data_bind.ns != i) {
					mode_data_bind.zone = SELECT_TAB;
					mode_data_bind.ns = i;
					update_popup_position(pn0,
							i->tab_area);
				}
				break;
			} else if (i->right_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				if (mode_data_bind.zone != SELECT_RIGHT
						|| mode_data_bind.ns != i) {
					mode_data_bind.zone = SELECT_RIGHT;
					mode_data_bind.ns = i;
					update_popup_position(pn0,
							i->popup_right_area);
				}
				break;
			} else if (i->top_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				if (mode_data_bind.zone != SELECT_TOP
						|| mode_data_bind.ns != i) {
					mode_data_bind.zone = SELECT_TOP;
					mode_data_bind.ns = i;
					update_popup_position(pn0,
							i->popup_top_area);
				}
				break;
			} else if (i->bottom_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				if (mode_data_bind.zone != SELECT_BOTTOM
						|| mode_data_bind.ns != i) {
					mode_data_bind.zone = SELECT_BOTTOM;
					mode_data_bind.ns = i;
					update_popup_position(pn0,
							i->popup_bottom_area);
				}
				break;
			} else if (i->left_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				if (mode_data_bind.zone != SELECT_LEFT
						|| mode_data_bind.ns != i) {
					mode_data_bind.zone = SELECT_LEFT;
					mode_data_bind.ns = i;
					update_popup_position(pn0,
							i->popup_left_area);
				}
				break;
			} else if (i->popup_center_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				if (mode_data_bind.zone != SELECT_CENTER
						|| mode_data_bind.ns != i) {
					mode_data_bind.zone = SELECT_CENTER;
					mode_data_bind.ns = i;
					update_popup_position(pn0,
							i->popup_center_area);
				}
				break;
			}
		}
	}

		break;
	case PROCESS_FULLSCREEN_MOVE: {
		/* get lastest know motion event */
		ev.xmotion = e;
		while (XCheckMaskEvent(cnx->dpy(), Button1MotionMask, &ev))
			continue;

		viewport_t * v = find_mouse_viewport(ev.xmotion.x_root,
				ev.xmotion.y_root);

		if (v != nullptr) {
			if (v != mode_data_fullscreen.v) {
				pn0->move_resize(v->_raw_aera);
				mode_data_fullscreen.v = v;
			}
		}

		break;
	}
	case PROCESS_NOTEBOOK_MENU:
		/** TODO: select menu **/
	{
		int selx = (int) floor((e.y_root - menu->position().y) / 24.0);
		menu->set_selected(selx);
		rnd->need_render();
	}
		break;
	default:
		fprintf(stderr, "Warning: Unknown process_mode\n");
		break;
	}
}

void page_t::process_event(XCirculateEvent const & e) {

}

void page_t::process_event(XConfigureEvent const & e) {

	if(e.send_event == True)
		return;

	client_base_t * c = find_client(e.window);

	if(c != nullptr) {
		/** damage corresponding area **/
		if(e.event == cnx->root()) {
			rnd->add_damaged(c->visible_area());
		}

		c->process_event(e);

		/** damage corresponding area **/
		if(e.event == cnx->root()) {
			rnd->add_damaged(c->visible_area());
		}
	}



}

/* track all created window */
void page_t::process_event(XCreateWindowEvent const & e) {

}

void page_t::process_event(XDestroyWindowEvent const & e) {
	if(e.send_event == True)
		return;

	client_base_t * c = find_client(e.window);
	if (c != nullptr) {
		if(typeid(*c) == typeid(client_managed_t)) {
			cout << "WARNING: client destroyed a window without sending synthetic unmap" << endl;
			cout << "Sent Event:" << (e.send_event?"true":"false") << endl;
			client_managed_t * mw = dynamic_cast<client_managed_t *>(c);
			unmanage(mw);
		} else {
			cleanup_client(c);
		}
	}
}

void page_t::process_event(XGravityEvent const & e) {
	/* Ignore it, never happen ? */
}

void page_t::process_event(XMapEvent const & e) {

	/* if map event does not occur within root, ignore it */
	if (e.event != cnx->root())
		return;

	onmap(e.window);

}

void page_t::process_event(XReparentEvent const & e) {
	//printf("Reparent window: %lu, parent: %lu, overide: %d, send_event: %d\n",
	//		e.window, e.parent, e.override_redirect, e.send_event);
	if(e.send_event == True)
		return;
	/* Reparent the root window ? hu :/ */
	if(e.window == cnx->root())
		return;

	/* If reparent occure on managed windows and new parent is an unknown window then unmanage */
	client_managed_t * mw = find_managed_window_with(e.window);
	if (mw != nullptr) {
		if (e.window == mw->orig() and e.parent != mw->base()) {
			/* unmanage the window */
			unmanage(mw);
		}
	}

	/* if a unmanaged window leave the root window for any reason, this client is forgoten */
	client_not_managed_t * uw = dynamic_cast<client_not_managed_t *>(find_client_with(e.window));
	if(uw != nullptr and e.parent != cnx->root()) {
		cleanup_client(uw);
	}

}

void page_t::process_event(XUnmapEvent const & e) {

	/* if client is managed */
	client_base_t * c = find_client(e.window);

	/**
	 * Client must send a fake unmap event if he want get back the window.
	 * (i.e. he want that we unmanage it.
	 */
	if (c != nullptr) {

		rnd->add_damaged(c->visible_area());

		if(e.send_event == True and typeid(*c) == typeid(client_managed_t)) {
			client_managed_t * mw = dynamic_cast<client_managed_t *>(c);
			cnx->reparentwindow(mw->orig(), cnx->root(), 0.0, 0.0);
			unmanage(mw);
		} else if (e.send_event == True or typeid(*c) == typeid(client_not_managed_t)) {
			cleanup_client(c);
		}
	}
}

void page_t::process_event(XCirculateRequestEvent const & e) {
	/* will happpen ? */
	client_base_t * c = find_client_with(e.window);
	if (c != 0) {
		if (e.place == PlaceOnTop) {
			safe_raise_window(c);
		} else if (e.place == PlaceOnBottom) {
			cnx->lower_window(e.window);
		}
	}
}

void page_t::process_event(XConfigureRequestEvent const & e) {

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


	client_base_t * c = find_client(e.window);

	if (c != nullptr) {

		rnd->add_damaged(c->visible_area());

		if(typeid(*c) == typeid(client_managed_t)) {

			client_managed_t * mw = dynamic_cast<client_managed_t *>(c);

			if (mw->lock()) {

				if ((e.value_mask & (CWX | CWY | CWWidth | CWHeight)) != 0) {

					i_rect old_size = mw->get_floating_wished_position();
					/** compute floating size **/
					i_rect new_size = mw->get_floating_wished_position();

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

					//printf("new_size = %s\n", new_size.to_string().c_str());

					if ((e.value_mask & (CWX)) and (e.value_mask & (CWY))
							and e.x == 0 and e.y == 0
							and !viewport_outputs.empty()) {
						viewport_t * v = viewport_outputs.begin()->second;
						i_rect b = v->raw_area();
						/* place on center */
						new_size.x = (b.w - new_size.w) / 2 + b.x;
						new_size.y = (b.h - new_size.h) / 2 + b.y;
					}

					unsigned int final_width = new_size.w;
					unsigned int final_height = new_size.h;

					compute_client_size_with_constraint(mw->orig(),
							(unsigned) new_size.w, (unsigned) new_size.h,
							final_width, final_height);

					new_size.w = final_width;
					new_size.h = final_height;

					//printf("new_size = %s\n", new_size.to_string().c_str());

					if (new_size != old_size) {
						/** only affect floating windows **/
						mw->set_floating_wished_position(new_size);
						mw->reconfigure();
						rnd->add_damaged(mw->base_position());
					}
				}

				mw->unlock();
			}

		} else {
			/** validate configure when window is not managed **/
			ackwoledge_configure_request(e);
		}

	} else {
		/** validate configure when window is not managed **/
		ackwoledge_configure_request(e);
	}

}

void page_t::ackwoledge_configure_request(XConfigureRequestEvent const & e) {
	static unsigned int CONFIGURE_MASK =
			(CWX | CWY | CWHeight | CWWidth | CWBorderWidth);

	//printf("ackwoledge_configure_request ");

	int i = 0;
	uint32_t value[5] = {0};
	uint32_t mask = 0;
	if(e.value_mask & CWX) {
		mask |= XCB_CONFIG_WINDOW_X;
		value[i++] = e.x;
		//printf("x = %d ", e.x);
	}

	if(e.value_mask & CWY) {
		mask |= XCB_CONFIG_WINDOW_Y;
		value[i++] = e.y;
		//printf("y = %d ", e.y);
	}

	if(e.value_mask & CWWidth) {
		mask |= XCB_CONFIG_WINDOW_WIDTH;
		value[i++] = e.width;
		//printf("w = %d ", e.width);
	}

	if(e.value_mask & CWHeight) {
		mask |= XCB_CONFIG_WINDOW_HEIGHT;
		value[i++] = e.height;
		//printf("h = %d ", e.height);
	}

	if(e.value_mask & CWBorderWidth) {
		mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
		value[i++] = e.border_width;
		//printf("border = %d ", e.border_width);
	}

	//printf("\n");

	xcb_configure_window(cnx->xcb(), e.window, mask, value);

}

void page_t::process_event(XMapRequestEvent const & e) {

	if (e.send_event == True or e.parent != cnx->root()) {
		XMapWindow(e.display, e.window);
		return;
	}

	onmap(e.window);

}

void page_t::process_event(XPropertyEvent const & e) {

	if(e.window == cnx->root())
		return;

	client_base_t * c = find_client(e.window);

	if(c != nullptr) {
		if(e.atom == A(WM_NAME)) {
			c->update_wm_name();
		} else if (e.atom == A(WM_ICON_NAME)) {
			c->update_wm_icon_name();
		} else if (e.atom == A(WM_NORMAL_HINTS)) {
			c->update_wm_normal_hints();
		} else if (e.atom == A(WM_HINTS)) {
			c->update_wm_hints();
		} else if (e.atom == A(WM_CLASS)) {
			c->update_wm_class();
		} else if (e.atom == A(WM_TRANSIENT_FOR)) {
			c->update_wm_transient_for();
		} else if (e.atom == A(WM_PROTOCOLS)) {
			c->update_wm_protocols();
		} else if (e.atom == A(WM_COLORMAP_WINDOWS)) {
			c->update_wm_colormap_windows();
		} else if (e.atom == A(WM_CLIENT_MACHINE)) {
			c->update_wm_client_machine();
		} else if (e.atom == A(WM_STATE)) {
			c->update_wm_state();
		} else if (e.atom == A(_NET_WM_NAME)) {
			c->update_net_wm_name();
		} else if (e.atom == A(_NET_WM_VISIBLE_NAME)) {
			c->update_net_wm_visible_name();
		} else if (e.atom == A(_NET_WM_ICON_NAME)) {
			c->update_net_wm_icon_name();
		} else if (e.atom == A(_NET_WM_VISIBLE_ICON_NAME)) {
			c->update_net_wm_visible_icon_name();
		} else if (e.atom == A(_NET_WM_DESKTOP)) {
			c->update_net_wm_desktop();
		} else if (e.atom == A(_NET_WM_WINDOW_TYPE)) {
			c->update_net_wm_window_type();
		} else if (e.atom == A(_NET_WM_STATE)) {
			c->update_net_wm_state();
		} else if (e.atom == A(_NET_WM_ALLOWED_ACTIONS)) {
			c->update_net_wm_allowed_actions();
		} else if (e.atom == A(_NET_WM_STRUT)) {
			c->update_net_wm_struct();
		} else if (e.atom == A(_NET_WM_STRUT_PARTIAL)) {
			c->update_net_wm_struct_partial();
		} else if (e.atom == A(_NET_WM_ICON_GEOMETRY)) {
			c->update_net_wm_icon_geometry();
		} else if (e.atom == A(_NET_WM_ICON)) {
			c->update_net_wm_icon();
		} else if (e.atom == A(_NET_WM_PID)) {
			c->update_net_wm_pid();
		} else if (e.atom == A(_NET_WM_USER_TIME)) {
			c->update_net_wm_user_time();
		} else if (e.atom == A(_NET_WM_USER_TIME_WINDOW)) {
			c->update_net_wm_user_time_window();
		} else if (e.atom == A(_NET_FRAME_EXTENTS)) {
			c->update_net_frame_extents();
		} else if (e.atom == A(_NET_WM_OPAQUE_REGION)) {
			c->update_net_wm_opaque_region();
		} else if (e.atom == A(_NET_WM_BYPASS_COMPOSITOR)) {
			c->update_net_wm_bypass_compositor();
		}  else if (e.atom == A(_MOTIF_WM_HINTS)) {
			c->update_motif_hints();
		}

	}

	Window x = e.window;
	client_managed_t * mw = find_managed_window_with(e.window);

	if (e.atom == A(_NET_WM_USER_TIME)) {

	} else if (e.atom == A(_NET_WM_NAME) || e.atom == A(WM_NAME)) {


		if (mw != nullptr) {
			mw->update_title();

			if (mw->is(MANAGED_NOTEBOOK)) {
				notebook_t * n = find_parent_notebook_for(mw);
				rpage->add_damaged(n->allocation());
			}

			if (mw->is(MANAGED_FLOATING)) {
				mw->expose();
			}
		}

	} else if (e.atom == A(_NET_WM_STRUT_PARTIAL)) {
		for(map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin(); i != viewport_outputs.end(); ++i) {
			compute_viewport_allocation(*(i->second));
		}
		rpage->mark_durty();
	} else if (e.atom == A(_NET_WM_STRUT)) {
		//x->mark_durty_net_wm_partial_struct();
		//rpage->mark_durty();
	} else if (e.atom == A(_NET_WM_ICON)) {
		if (mw != 0)
			mw->update_icon();
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
		if (mw != nullptr) {

			if (mw->is(MANAGED_NOTEBOOK)) {
				find_parent_notebook_for(mw)->update_client_position(mw);
			}

			/* apply normal hint to floating window */
			i_rect new_size = mw->get_wished_position();
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
		if (c != 0) {
			safe_update_transient_for(c);
			update_windows_stack();
		}
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

	char * name = XGetAtomName(cnx->dpy(), e.message_type);
	printf("ClientMessage type = %s\n", name);
	XFree(name);

	Window w = e.window;
	if (w == None)
		return;

	client_managed_t * mw = find_managed_window_with(e.window);

	if (e.message_type == A(_NET_ACTIVE_WINDOW)) {
		if (mw != nullptr) {

			if (mw->lock()) {
				if (e.data.l[1] == CurrentTime) {
					printf("Invalid focus request ... but stealing focus\n");
					set_focus(mw, CurrentTime);
				} else {
					set_focus(mw, e.data.l[1]);
				}
				mw->unlock();
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
		if (mw != nullptr) {
			if (mw->lock()) {
				if (mw->is(MANAGED_FLOATING) and e.data.l[0] == IconicState) {
					bind_window(mw, false);
				} else if (mw->is(
						MANAGED_NOTEBOOK) and e.data.l[0] == IconicState) {
					notebook_t * n = dynamic_cast<notebook_t *>(mw->parent());
					n->iconify_client(mw);
					rpage->add_damaged(n->allocation());
					rnd->need_render();
				}
				mw->unlock();
			}
		}

	} else if (e.message_type == A(PAGE_QUIT)) {
		running = false;
	} else if (e.message_type == A(WM_PROTOCOLS)) {

	} else if (e.message_type == A(_NET_CLOSE_WINDOW)) {
		if(mw != nullptr) {
			mw->delete_window(e.data.l[0]);
		}
	} else if (e.message_type == A(_NET_REQUEST_FRAME_EXTENTS)) {

	} else if (e.message_type == A(_NET_WM_MOVERESIZE)) {
		if (mw != nullptr) {
			if (mw->is(MANAGED_FLOATING) and process_mode == PROCESS_NORMAL) {

				if (mw->lock()) {
					Cursor xc = None;

					long x_root = e.data.l[0];
					long y_root = e.data.l[1];
					long direction = e.data.l[2];
					long button = e.data.l[3];
					long source = e.data.l[4];

					mode_data_floating.x_offset = x_root
							- mw->get_base_position().x;
					mode_data_floating.y_offset = y_root
							- mw->get_base_position().y;
					;
					mode_data_floating.x_root = x_root;
					mode_data_floating.y_root = y_root;
					mode_data_floating.f = mw;
					mode_data_floating.original_position =
							mw->get_floating_wished_position();
					mode_data_floating.final_position =
							mw->get_floating_wished_position();
					mode_data_floating.popup_original_position =
							mw->get_base_position();
					mode_data_floating.button = button;

					if (direction == _NET_WM_MOVERESIZE_MOVE) {
						safe_raise_window(mw);
						process_mode = PROCESS_FLOATING_MOVE_BY_CLIENT;
						xc = xc_fleur;
					} else {

						if (direction == _NET_WM_MOVERESIZE_SIZE_TOP) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_TOP;
							xc = xc_top_side;
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_BOTTOM) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_BOTTOM;
							xc = xc_bottom_side;
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_LEFT;
							xc = xc_left_side;
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_RIGHT;
							xc = xc_right_side;
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_TOPLEFT) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_TOP_LEFT;
							xc = xc_top_left_corner;
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_TOPRIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_TOP_RIGHT;
							xc = xc_top_right_corner;
						} else if (direction
								== _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_BOTTOM_LEFT;
							xc = xc_bottom_left_corner;
						} else if (direction
								== _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_BOTTOM_RIGHT;
							xc = xc_bottom_righ_corner;
						} else {
							safe_raise_window(mw);
							process_mode = PROCESS_FLOATING_MOVE_BY_CLIENT;
							xc = xc_fleur;
						}
					}

					if (process_mode != PROCESS_NORMAL) {
						XGrabPointer(cnx->dpy(), cnx->root(), False,
						ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
						GrabModeAsync,
						GrabModeAsync, None, xc, CurrentTime);
					}

					mw->unlock();

				}
			}
		}
	}
}

void page_t::process_event(XDamageNotifyEvent const & e) {
	/**
	 * Try to render if any damage event is encountered. But limit general
	 * rendering to 60 fps.
	 **/
	time_t cur_tic;
	cur_tic.get_time();
	rnd->clear_renderable();
	vector<ptr<renderable_t>> ret;
	prepare_render(ret, cur_tic);
	rnd->push_back_renderable(ret);
	rnd->render();

	cur_tic.get_time();
	/** slow down frame if render is slow **/
	_next_frame = cur_tic + default_wait;
	_max_wait = default_wait;
}

void page_t::fullscreen(client_managed_t * mw, viewport_t * v) {

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
		data.revert_notebook = find_parent_notebook_for(mw);
		page_assert(data.revert_notebook != nullptr);
		data.revert_type = MANAGED_NOTEBOOK;
		if (v == nullptr) {
			data.viewport = find_viewport_for(data.revert_notebook);
		} else {
			data.viewport = v;
		}
		page_assert(data.viewport != nullptr);
		detach(mw);
	} else if (mw->is(MANAGED_FLOATING)) {
		detach(mw);
		data.revert_type = MANAGED_FLOATING;
		data.revert_notebook = nullptr;
		if (v == nullptr) {
			// TODO: find a free viewport
			data.viewport = viewport_outputs.begin()->second;
		} else {
			data.viewport = v;
		}
	}

	// unfullscreen client that already use this screen
	for (auto &x : fullscreen_client_to_viewport) {
		if (x.second.viewport == mode_data_fullscreen.v) {
			unfullscreen(
					dynamic_cast<client_managed_t *>(x.first));
			break;
		}
	}

	data.viewport->_is_visible = false;

	/* unmap all notebook window */
	list<notebook_t *> ns = get_notebooks(data.viewport);
	for(auto i : ns) {
		i->unmap_all();
	}

	mw->set_managed_type(MANAGED_FULLSCREEN);
	mw->set_parent(this);
	fullscreen_client_to_viewport[mw] = data;
	mw->set_notebook_wished_position(data.viewport->_raw_aera);
	mw->reconfigure();
	mw->normalize();
	update_windows_stack();
}

void page_t::unfullscreen(client_managed_t * mw) {
	/* WARNING: Call order is important, change it with caution */
	mw->net_wm_state_remove(_NET_WM_STATE_FULLSCREEN);
	if(!has_key(fullscreen_client_to_viewport, mw))
		return;

	fullscreen_data_t & data = fullscreen_client_to_viewport[mw];

	viewport_t * v = data.viewport;

	if (data.revert_type == MANAGED_NOTEBOOK) {
		notebook_t * old = data.revert_notebook;
		mw->set_managed_type(MANAGED_NOTEBOOK);
		insert_window_in_notebook(mw, old, true);
		old->activate_client(mw);
		mw->reconfigure();
	} else {
		mw->set_managed_type(MANAGED_FLOATING);
		insert_in_tree_using_transient_for(mw);
		mw->reconfigure();
	}

	fullscreen_client_to_viewport.erase(mw);
	v->_is_visible = true;

	/* map all notebook window */
	auto ns = get_notebooks(v);
	for (auto i : ns) {
		i->map_all();
	}

	update_allocation();
	rpage->mark_durty();

}

void page_t::toggle_fullscreen(client_managed_t * c) {
	if(c->is(MANAGED_FULLSCREEN))
		unfullscreen(c);
	else
		fullscreen(c);
}


void page_t::process_event(XEvent const & e) {

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
	} else if (e.type == damage_event + XDamageNotify) {
		process_event(reinterpret_cast<XDamageNotifyEvent const &>(e));
	} else if (e.type == Expose) {
		client_managed_t * mw = find_managed_window_with(e.xexpose.window);
		if (mw != nullptr) {
			if (mw->is(MANAGED_FLOATING)) {
				mw->expose();
				//rnd->add_damaged(mw->base_position());
			}
		}

	} else if (e.type == xrandr_event + RRNotify) {
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

		if (ev.subtype == RRNotify_CrtcChange) {
			update_viewport_layout();
			update_allocation();

			theme->update();

			delete rpage;
			rpage = new renderable_page_t(cnx, theme, _root_position.w,
					_root_position.h);
			rpage->show();

		}

		XGrabButton(cnx->dpy(), AnyButton, AnyModifier, cnx->root(), False,
				ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
				GrabModeSync, GrabModeAsync, None, None);

		/** put rpage behind all managed windows **/
		update_windows_stack();


	} else if (e.type == xrandr_event + RRScreenChangeNotify) {
		/** do nothing **/
	} else if (e.type == SelectionClear) {
		running = false;
	} else if (e.type == xshape_event + ShapeNotify) {
		XShapeEvent * se = (XShapeEvent *)&e;
		if(se->kind == ShapeBounding) {
			Window w = se->window;
			client_base_t * c = find_client(w);
			if(c != nullptr) {
				c->update_shape();
			}
		}
	} else if (e.type == FocusOut) {
		//printf("FocusOut #%lu\n", e.xfocus.window);
	} else if (e.type == FocusIn) {
		//printf("FocusIn #%lu\n", e.xfocus.window);
	} else if (e.type == MappingNotify) {
		update_keymap();
		update_grabkey();
	} else {

	}

}

void page_t::insert_window_in_notebook(client_managed_t * x, notebook_t * n,
		bool prefer_activate) {
	page_assert(x != nullptr);
	page_assert(find_parent_notebook_for(x) == nullptr);
	if (n == nullptr)
		n = default_window_pop;
	page_assert(n != nullptr);
	x->set_managed_type(MANAGED_NOTEBOOK);
	n->add_client(x, prefer_activate);
	rpage->add_damaged(n->allocation());

}

/* update viewport and childs allocation */
void page_t::update_allocation() {
	for(auto & i : viewport_outputs) {
		compute_viewport_allocation(*(i.second));
	}
}

void page_t::set_default_pop(notebook_t * x) {
	if(default_window_pop != nullptr) {
		default_window_pop->set_default(false);
		rpage->add_damaged(default_window_pop->allocation());
	}
	default_window_pop = x;
	default_window_pop->set_default(true);
	rpage->add_damaged(default_window_pop->allocation());
}

/** If tfocus == CurrentTime, still the focus ... it's a known issue of X11 protocol + ICCCM */
void page_t::set_focus(client_managed_t * new_focus, Time tfocus) {

	//if (new_focus != nullptr)
	//	cout << "try focus [" << new_focus->title() << "] " << tfocus << endl;

	/** ignore focus if time is too old **/
	if(tfocus <= _last_focus_time and tfocus != CurrentTime)
		return;

	if(tfocus != CurrentTime)
		_last_focus_time = tfocus;

	client_managed_t * old_focus = nullptr;

	/** NULL pointer is always in the list **/
	old_focus = _client_focused.front();

	if (old_focus != nullptr) {
		/**
		 * update _NET_WM_STATE and grab button mode and mark decoration
		 * durty (for floating window only)
		 **/
		old_focus->set_focus_state(false);

		/**
		 * if this is a notebook, mark the area for updates.
		 **/
		if (old_focus->is(MANAGED_NOTEBOOK)) {
			notebook_t * n = find_parent_notebook_for(old_focus);
			if (n != nullptr) {
				rpage->add_damaged(n->allocation());
			}
		}
	}

	/**
	 * put this managed window in front of list
	 **/
	_client_focused.remove(new_focus);
	_client_focused.push_front(new_focus);

	if(new_focus == nullptr) {
		XSetInputFocus(cnx->dpy(), cnx->root(), RevertToNone, _last_focus_time);
		return;
	}

	/**
	 * raise the newly focused window at top, in respect of transient for.
	 **/
	safe_raise_window(new_focus);

	/**
	 * if this is a notebook, mark the area for updates and activated the
	 * notebook
	 **/
	if (new_focus->is(MANAGED_NOTEBOOK)) {
		notebook_t * n = find_parent_notebook_for(new_focus);
		if (n != nullptr) {
			n->activate_client(new_focus);
			rpage->add_damaged(n->allocation());
		}
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

display_t * page_t::get_xconnection() {
	return cnx;
}

void page_t::split(notebook_t * nbk, split_type_e type) {
	split_t * split = new split_t(type, theme);
	nbk->parent()->replace(nbk, split);
	notebook_t * n = new notebook_t(theme);
	split->set_pack0(nbk);
	split->set_pack1(n);
	rpage->add_damaged(split->allocation());
}

void page_t::split_left(notebook_t * nbk, client_managed_t * c) {
	page_component_t * parent = nbk->parent();
	notebook_t * n = new notebook_t(theme);
	split_t * split = new split_t(VERTICAL_SPLIT, theme);
	parent->replace(nbk, split);
	split->set_pack0(n);
	split->set_pack1(nbk);
	detach(c);
	insert_window_in_notebook(c, n, true);
	update_allocation();
	rpage->add_damaged(split->allocation());
}

void page_t::split_right(notebook_t * nbk, client_managed_t * c) {
	page_component_t * parent = nbk->parent();
	notebook_t * n = new notebook_t(theme);
	split_t * split = new split_t(VERTICAL_SPLIT, theme);
	parent->replace(nbk, split);
	split->set_pack0(nbk);
	split->set_pack1(n);
	detach(c);
	insert_window_in_notebook(c, n, true);
	update_allocation();
	rpage->add_damaged(split->allocation());
}

void page_t::split_top(notebook_t * nbk, client_managed_t * c) {
	page_component_t * parent = nbk->parent();
	notebook_t * n = new notebook_t(theme);
	split_t * split = new split_t(HORIZONTAL_SPLIT, theme);
	parent->replace(nbk, split);
	split->set_pack0(n);
	split->set_pack1(nbk);
	detach(c);
	insert_window_in_notebook(c, n, true);
	update_allocation();
	rpage->add_damaged(split->allocation());
}

void page_t::split_bottom(notebook_t * nbk, client_managed_t * c) {
	page_component_t * parent = nbk->parent();
	notebook_t * n = new notebook_t(theme);
	split_t * split = new split_t(HORIZONTAL_SPLIT, theme);
	parent->replace(nbk, split);
	split->set_pack0(nbk);
	split->set_pack1(n);
	detach(c);
	insert_window_in_notebook(c, n, true);
	update_allocation();
	rpage->add_damaged(split->allocation());
}

void page_t::notebook_close(notebook_t * src) {

	split_t * ths = dynamic_cast<split_t *>(src->parent());

	/* if parent is viewport then we cannot close current notebook */
	if(ths == nullptr)
		return;

	page_assert(src == ths->get_pack0() or src == ths->get_pack1());

	page_component_t * dst = (src == ths->get_pack0()) ? ths->get_pack1() : ths->get_pack0();

	/* if notebook is default_pop, select another one */
	if (default_window_pop == src) {
		/* prefer a nearest new notebook */
		default_window_pop = get_another_notebook(dst, src);
		/* else try a global one */
		if(default_window_pop == nullptr) {
			default_window_pop = get_another_notebook(this, src);
		}
		page_assert(default_window_pop != nullptr);
		default_window_pop->set_default(true);
		rpage->add_damaged(default_window_pop->allocation());
	}

	/* move all windows from src to default_window_pop */
	auto windows = src->get_clients();
	for(auto i : windows) {
		detach(i);
		insert_window_in_notebook(i, 0, false);
	}

	/* if a fullscreen window want revert to this notebook,
	 * change it to default_window_pop */
	for (auto & i : fullscreen_client_to_viewport) {
		if (i.second.revert_notebook == src) {
			i.second.revert_notebook = default_window_pop;
		}
	}

	page_assert(ths->parent() != nullptr);

	detach(src);
	/* remove this split from tree */
	ths->parent()->replace(ths, dst);
	rpage->add_damaged(ths->allocation());

	/* cleanup */
	delete src;
	delete ths;

}

void page_t::update_popup_position(ptr<popup_notebook0_t> p,
		i_rect & position) {
	rnd->add_damaged(p->position());
	p->move_resize(position);
	rnd->add_damaged(p->position());
}

void page_t::update_popup_position(ptr<popup_frame_move_t> p,
		i_rect & position) {
	rnd->add_damaged(p->position());
	p->move_resize(position);
	rnd->add_damaged(p->position());
}


/*
 * Compute the usable desktop area and dock allocation.
 */
void page_t::compute_viewport_allocation(viewport_t & v) {

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

	int xtop = v._raw_aera.y;
	int xleft = v._raw_aera.x;
	int xright = _root_position.w - v._raw_aera.x - v._raw_aera.w;
	int xbottom = _root_position.h - v._raw_aera.y - v._raw_aera.h;


	for(auto & j : clients) {
		if (j.second->net_wm_struct_partial() != nullptr) {
			auto const & ps = *(j.second->net_wm_struct_partial());

			if (ps[PS_LEFT] > 0) {
				/* check if raw area intersect current viewport */
				i_rect b(0, ps[PS_LEFT_START_Y], ps[PS_LEFT],
						ps[PS_LEFT_END_Y] - ps[PS_LEFT_START_Y] + 1);
				i_rect x = v._raw_aera & b;
				if (!x.is_null()) {
					xleft = std::max(xleft, ps[PS_LEFT]);
				}
			}

			if (ps[PS_RIGHT] > 0) {
				/* check if raw area intersect current viewport */
				i_rect b(_root_position.w - ps[PS_RIGHT],
						ps[PS_RIGHT_START_Y], ps[PS_RIGHT],
						ps[PS_RIGHT_END_Y] - ps[PS_RIGHT_START_Y] + 1);
				i_rect x = v._raw_aera & b;
				if (!x.is_null()) {
					xright = std::max(xright, ps[PS_RIGHT]);
				}
			}

			if (ps[PS_TOP] > 0) {
				/* check if raw area intersect current viewport */
				i_rect b(ps[PS_TOP_START_X], 0,
						ps[PS_TOP_END_X] - ps[PS_TOP_START_X] + 1, ps[PS_TOP]);
				i_rect x = v._raw_aera & b;
				if (!x.is_null()) {
					xtop = std::max(xtop, ps[PS_TOP]);
				}
			}

			if (ps[PS_BOTTOM] > 0) {
				/* check if raw area intersect current viewport */
				i_rect b(ps[PS_BOTTOM_START_X],
						_root_position.h - ps[PS_BOTTOM],
						ps[PS_BOTTOM_END_X] - ps[PS_BOTTOM_START_X] + 1,
						ps[PS_BOTTOM]);
				i_rect x = v._raw_aera & b;
				if (!x.is_null()) {
					xbottom = std::max(xbottom, ps[PS_BOTTOM]);
				}
			}
		}
	}

	i_rect final_size;

	final_size.x = xleft;
	final_size.w = _root_position.w - xright - xleft;
	final_size.y = xtop;
	final_size.h = _root_position.h - xbottom - xtop;

	v.set_allocation(final_size);

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

	auto name = cnx->get_atom_name(state_properties);
	printf("_NET_WM_STATE %s %s from %lu\n", action, name.get(), c);

	client_managed_t * mw = find_managed_window_with(c);

	if(mw == nullptr)
		return;

	if (mw->is(MANAGED_NOTEBOOK)) {

		if (state_properties == A(_NET_WM_STATE_FULLSCREEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				break;
			case _NET_WM_STATE_ADD:
				fullscreen(mw);
				break;
			case _NET_WM_STATE_TOGGLE:
				toggle_fullscreen(mw);
				break;
			}
			update_allocation();
		} else if (state_properties == A(_NET_WM_STATE_HIDDEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
			{
				notebook_t * n = dynamic_cast<notebook_t*>(mw->parent());
				if(n != nullptr)
					n->activate_client(mw);
			}

				break;
			case _NET_WM_STATE_ADD:
				mw->iconify();
				break;
			case _NET_WM_STATE_TOGGLE:
				/** IWMH say ignore it ? **/
			default:
				break;
			}
		}
	} else if (mw->is(MANAGED_FLOATING)) {

		if (state_properties == A(_NET_WM_STATE_FULLSCREEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				break;
			case _NET_WM_STATE_ADD:
				fullscreen(mw);
				break;
			case _NET_WM_STATE_TOGGLE:
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
				/** I ignore it **/
				break;
			case _NET_WM_STATE_TOGGLE:
				/** IWMH say ignore it ? **/
			default:
				break;
			}
		}
	} else if (mw->is(MANAGED_FULLSCREEN)) {
		if (state_properties == A(_NET_WM_STATE_FULLSCREEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				unfullscreen(mw);
				break;
			case _NET_WM_STATE_ADD:
				break;
			case _NET_WM_STATE_TOGGLE:
				toggle_fullscreen(mw);
				break;
			}
			update_allocation();
		} else if (state_properties == A(_NET_WM_STATE_HIDDEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				break;
			case _NET_WM_STATE_ADD:
				break;
			case _NET_WM_STATE_TOGGLE:
			default:
				break;
			}
		}
	}
}

void page_t::insert_in_tree_using_transient_for(client_base_t * c) {
	detach(c);
	client_base_t * transient_for = get_transient_for(c);
	if (transient_for != nullptr) {
		transient_for->add_subclient(c);
	} else {
		root_subclients.push_back(c);
		c->set_parent(this);
	}
}

client_base_t * page_t::get_transient_for(client_base_t * c) {
	client_base_t * transient_for = nullptr;
	if(c != nullptr) {
		if(c->wm_transient_for() != nullptr) {
			transient_for = find_client_with(*(c->wm_transient_for()));
			if(transient_for == nullptr)
				printf("Warning transient for an unknown client\n");
		}
	}
	return transient_for;
}

void page_t::detach(tree_t * t) {
	remove(t);
	list<tree_t *> elements = get_all_childs();
	for(auto &i : elements) {
		i->remove(t);
	}
	t->set_parent(nullptr);
}

void page_t::safe_raise_window(client_base_t * c) {
	if(process_mode != PROCESS_NORMAL)
		return;
	c->raise_child();
	/** apply change **/
	update_windows_stack();
}

void page_t::compute_client_size_with_constraint(Window c,
		unsigned int wished_width, unsigned int wished_height,
		unsigned int & width, unsigned int & height) {

	/* default size if no size_hints is provided */
	width = wished_width;
	height = wished_height;

	client_base_t * _c = find_client_with(c);

	if (_c == nullptr)
		return;

	if (_c->wm_normal_hints() == nullptr)
		return;

	::page::compute_client_size_with_constraint(*(_c->wm_normal_hints()),
			wished_width, wished_height, width, height);

}

void page_t::bind_window(client_managed_t * mw, bool activate) {
	/* update database */
	cout << "bind: " << mw->title() << endl;
	detach(mw);
	insert_window_in_notebook(mw, 0, activate);
	safe_raise_window(mw);
	update_client_list();
}

void page_t::unbind_window(client_managed_t * mw) {
	detach(mw);
	/* update database */
	mw->set_managed_type(MANAGED_FLOATING);
	mw->set_parent(this);
	safe_update_transient_for(mw);
	mw->expose();
	mw->normalize();
	safe_raise_window(mw);
	update_client_list();
	update_windows_stack();
}

void page_t::grab_pointer() {
	/* Grab Pointer no other client will get mouse event */
	if (XGrabPointer(cnx->dpy(), cnx->root(), False,
			(ButtonPressMask | ButtonReleaseMask
					| PointerMotionMask),
			GrabModeAsync, GrabModeAsync, None, None,
			CurrentTime) != GrabSuccess) {
		/* bad news */
		throw std::runtime_error("fail to grab pointer");
	}

}

void page_t::cleanup_grab(client_managed_t * mw) {

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
	case PROCESS_FLOATING_MOVE_BY_CLIENT:
	case PROCESS_FLOATING_RESIZE_BY_CLIENT:
		if (mode_data_floating.f == mw) {
			process_mode = PROCESS_NORMAL;

			pfm->hide();

			mode_data_floating.mode = RESIZE_NONE;
			mode_data_floating.x_offset = 0;
			mode_data_floating.y_offset = 0;
			mode_data_floating.x_root = 0;
			mode_data_floating.y_root = 0;
			mode_data_floating.original_position = i_rect();
			mode_data_floating.f = 0;
			mode_data_floating.popup_original_position = i_rect();
			mode_data_floating.final_position = i_rect();

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
	case PROCESS_NOTEBOOK_MENU:
		menu->hide();
		rnd->add_damaged(menu->get_opaque_region());
		process_mode = PROCESS_NORMAL;
		mode_data_notebook_menu.from = nullptr;
		break;
	}
}

/* look for a notebook in tree base, that is deferent from nbk */
notebook_t * page_t::get_another_notebook(tree_t * base, tree_t * nbk) {
	list<notebook_t *> l;

	if (base == 0) {
		l = get_notebooks();
	} else {
		l = get_notebooks(base);
	}

	if (!l.empty()) {
		if (l.front() != nbk)
			return l.front();
		if (l.back() != nbk)
			return l.back();
	}

	return nullptr;

}

list<notebook_t *> page_t::get_notebooks(tree_t * base) {
	if(base == nullptr) {
		return filter_class<notebook_t>(get_all_childs());
	} else {
		return filter_class<notebook_t>(base->get_all_childs());
	}
}

notebook_t * page_t::find_parent_notebook_for(client_managed_t * mw) {
	return dynamic_cast<notebook_t*>(mw->parent());
}

viewport_t * page_t::find_viewport_for(notebook_t * n) {
	tree_t * x = n;
	while (x != nullptr) {
		if (typeid(*x) == typeid(viewport_t))
			return dynamic_cast<viewport_t *>(x);
		x = (x->parent());
	}
	return nullptr;
}

void page_t::set_window_cursor(Window w, Cursor c) {
	XSetWindowAttributes swa;
	swa.cursor = c;
	XChangeWindowAttributes(cnx->dpy(), w, CWCursor, &swa);
}

void page_t::update_windows_stack() {

	list<tree_t *> childs = this->get_all_childs();
	list<client_base_t *> clients = filter_class<client_base_t>(childs);

	list<Window> final_order;
	for(auto i: clients) {
		final_order.push_back(i->base());
	}

	if(rnd != 0) {
		final_order.push_back(rnd->get_composite_overlay());
	}

	final_order.reverse();

	XRaiseWindow(cnx->dpy(), final_order.front());
	/**
	 * convert list to C array, see std::vector API.
	 **/
	vector<Window> v_order(final_order.begin(), final_order.end());
	XRestackWindows(cnx->dpy(), &v_order[0], v_order.size());

}

void page_t::update_viewport_layout() {

	/** update root size infos **/

	XWindowAttributes rwa;
	if(!XGetWindowAttributes(cnx->dpy(), cnx->root(), &rwa)) {
		throw std::runtime_error("FATAL: cannot read root window attributes\n");
	}

	_root_position = i_rect(rwa.x, rwa.y, rwa.width, rwa.height);
	set_desktop_geometry(_root_position.w, _root_position.h);
	_allocation = _root_position;

	/** store the newer layout, to cleanup obsolet viewport **/
	map<RRCrtc, viewport_t *> new_layout;

	XRRScreenResources * resources = XRRGetScreenResourcesCurrent(
			cnx->dpy(), cnx->root());

	for (int i = 0; i < resources->ncrtc; ++i) {
		XRRCrtcInfo * info = XRRGetCrtcInfo(cnx->dpy(), resources,
				resources->crtcs[i]);

		/** if the CRTC has at less one output bound **/
		if(info->noutput > 0) {
			i_rect area(info->x, info->y, info->width, info->height);
			/** if this crtc do not has a viewport **/
			if (!has_key(viewport_outputs, resources->crtcs[i])) {
				/** then create a new one, and store it in new_layout **/
				viewport_t * v = new viewport_t(theme, area);
				v->set_parent(this);
				new_layout[resources->crtcs[i]] = v;
			} else {
				/** update allocation, and store it to new_layout **/
				viewport_outputs[resources->crtcs[i]]->set_raw_area(area);
				new_layout[resources->crtcs[i]] = viewport_outputs[resources->crtcs[i]];
			}
			compute_viewport_allocation(*(new_layout[resources->crtcs[i]]));
		}
		XRRFreeCrtcInfo(info);
	}
	XRRFreeScreenResources(resources);

	if(new_layout.size() < 1) {
		/** fallback to one screen **/
		i_rect area(rwa.x, rwa.y, rwa.width, rwa.height);
		/** if this crtc do not has a viewport **/
		if (!has_key(viewport_outputs, (XID)None)) {
			/** then create a new one, and store it in new_layout **/
			viewport_t * v = new viewport_t(theme, area);
			v->set_parent(this);
			new_layout[None] = v;
		} else {
			/** update allocation, and store it to new_layout **/
			viewport_outputs[None]->set_raw_area(area);
			new_layout[None] = viewport_outputs[None];
		}
		compute_viewport_allocation(*(new_layout[0]));
	}

	/** clean up old layout **/
	for (auto & i : viewport_outputs) {
		if (!has_key(new_layout, i.first)) {
			/** destroy this viewport **/
			remove_viewport(i.second);
			destroy_viewport(i.second);
		}
	}

	viewport_outputs = new_layout;
}

void page_t::remove_viewport(viewport_t * v) {

	/* remove fullscreened clients if needed */

	// unfullscreen client that already use this screen
	for (auto &x : fullscreen_client_to_viewport) {
		if (x.second.viewport == v) {
			unfullscreen(dynamic_cast<client_managed_t *>(x.first));
			break;
		}
	}

	/* Transfer clients to a valid notebook */
	list<notebook_t *> nbks = get_notebooks(v);
	for (auto i : nbks) {
		if (default_window_pop == i)
			default_window_pop = get_another_notebook(i);
		default_window_pop->set_default(true);
		rpage->add_damaged(default_window_pop->allocation());
		list<client_managed_t *> lc = i->get_clients();
		for (auto j : lc) {
			default_window_pop->add_client(j, false);
		}
	}
}

void page_t::destroy_viewport(viewport_t * v) {
	list<tree_t *> lst = v->get_all_childs();
	for (auto i : lst) {
		if (typeid(client_base_t) != typeid(*i)) {
			delete i;
		}
	}
	delete v;
}

/**
 * this function will check if a window must be managed or not.
 * If a window have to be managed, this function manage this window, if not
 * The function create unmanaged window.
 **/
void page_t::onmap(Window w) {

	if (rnd != nullptr) {
		if (w == rnd->get_composite_overlay())
			return;
	}

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
	cnx->grab();
	cnx->fetch_pending_events();

	if(cnx->check_for_destroyed_window(w)) {
		printf("do not manage %lu because it wil be destoyed\n", w);
		cnx->ungrab();
		return;
	}

	if(cnx->check_for_unmap_window(w)) {
		printf("do not manage %lu because it wil be unmapped\n", w);
		cnx->ungrab();
		return;
	}

	if(cnx->check_for_reparent_window(w)) {
		printf("do not manage %lu because it wil be reparented\n", w);
		cnx->ungrab();
		return;
	}

	client_base_t * c = find_client_with(w);

	if (c != nullptr) {
		/** client already existing **/
		c->read_all_properties();
		safe_update_transient_for(c);
		update_windows_stack();

		client_managed_t * mw = dynamic_cast<client_managed_t*>(c);
		if (mw != nullptr) {
			mw->reconfigure();
		}

	} else {
		try {
			ptr<client_properties_t> props(new client_properties_t(cnx, w));
			if (props->read_window_attributes()) {
				if(props->wa()->_class != InputOnly) {
					props->read_all_properties();

				//props->print_window_attributes();
				//props->print_properties();

				Atom type = props->type();

				if (!props->wa()->override_redirect) {
					if (type == A(_NET_WM_WINDOW_TYPE_DESKTOP)) {
						create_managed_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_DOCK)) {
						create_dock_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLBAR)) {
						create_managed_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_MENU)) {
						create_managed_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_UTILITY)) {
						create_managed_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_SPLASH)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_DIALOG)) {
						create_managed_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_POPUP_MENU)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLTIP)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_COMBO)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_DND)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
						create_managed_window(props, type);
					}
				} else {
					if (type == A(_NET_WM_WINDOW_TYPE_DESKTOP)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_DOCK)) {
						create_dock_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLBAR)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_MENU)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_UTILITY)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_SPLASH)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_DIALOG)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_POPUP_MENU)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLTIP)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_COMBO)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_DND)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
						create_unmanaged_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
						create_unmanaged_window(props, type);
					}
				}

			}
		}

		} catch (...) {
			c = nullptr;
		}

	}

	cnx->ungrab();

}


void page_t::create_managed_window(shared_ptr<client_properties_t> c, Atom type) {

	try {
		client_managed_t * mw = new client_managed_t(type, c, theme);
		add_client(mw);

		manage_managed_window(mw, type);

	} catch (...) {
		printf("Error while creating managed window\n");
	}
}

void page_t::manage_managed_window(client_managed_t * mw, Atom type) {

	try {

		safe_update_transient_for(mw);
		mw->raise_child(nullptr);
		update_client_list();
		update_windows_stack();

		/* HACK OLD FASHION FULLSCREEN */
		if (mw->geometry()->x == 0 and mw->geometry()->y == 0 and mw->geometry()->width == _allocation.w
				and mw->geometry()->height == _allocation.h
				and mw->type() == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
			mw->net_wm_state_add(_NET_WM_STATE_FULLSCREEN);
		}

		if (mw->is_fullscreen()) {
			mw->normalize();
			fullscreen(mw);
			mw->reconfigure();
			Time time = 0;
			if (get_safe_net_wm_user_time(mw, time)) {
				set_focus(mw, time);
			} else {
				mw->set_focus_state(false);
			}

		} else if ((type == A(_NET_WM_WINDOW_TYPE_NORMAL)
				or type == A(_NET_WM_WINDOW_TYPE_DESKTOP))
				and get_transient_for(mw) == nullptr
				and mw->has_motif_border()) {

			/** select if the client want to apear mapped or iconic **/
			bool activate = true;

			/**
			 * first try if previous vm has put this window in IconicState, then
			 * Check if the client have a prefered initiale state.
			 **/
			if (mw->wm_state() != nullptr) {
				if (mw->wm_state()->state == IconicState) {
					activate = false;
				}
			} else {
				if (mw->wm_hints() != nullptr) {
					if (mw->wm_hints()->initial_state == IconicState) {
						activate = false;
					}
				}
			}

			if(mw->net_wm_state() != nullptr) {
				if(has_key(*mw->net_wm_state(), static_cast<xcb_atom_t>(A(_NET_WM_STATE_HIDDEN)))) {
					activate = false;
				}
			}

			bind_window(mw, activate);

			mw->reconfigure();

		} else {
			mw->normalize();
			mw->reconfigure();
			Time time = 0;
			if (get_safe_net_wm_user_time(mw, time)) {
				set_focus(mw, time);
			} else {
				mw->set_focus_state(false);
			}
		}

	} catch (...) {
		printf("Error while creating managed window\n");
	}
}

void page_t::create_unmanaged_window(shared_ptr<client_properties_t> c, Atom type) {
	try {
		client_not_managed_t * uw = new client_not_managed_t(type, c);
		add_client(uw);
		uw->map();
		safe_update_transient_for(uw);
		safe_raise_window(uw);
		rnd->add_damaged(uw->visible_area());
		update_windows_stack();
		_need_render = true;
	} catch (...) {
		printf("Error while creating unmanaged window\n");
	}
}

void page_t::create_dock_window(shared_ptr<client_properties_t> c, Atom type) {
	client_not_managed_t * uw = new client_not_managed_t(type, c);
	add_client(uw);
	uw->map();
	attach_dock(uw);
	safe_raise_window(uw);
	update_allocation();
}

viewport_t * page_t::find_mouse_viewport(int x, int y) {
	for (auto i : viewport_outputs) {
		if (i.second->_raw_aera.is_inside(x, y))
			return i.second;
	}
	return 0;
}

/**
 * Read user time with proper fallback
 * @return: true if successfully find usertime, otherwise false.
 * @output time: if time is found time is set to the found value.
 * @input c: a window client handler.
 **/
bool page_t::get_safe_net_wm_user_time(client_base_t * c, Time & time) {

	bool has_time = false;
	Window time_window;

	if (c->net_wm_user_time() != nullptr) {
		time = *(c->net_wm_user_time());
		has_time = true;
	} else {
		/* if no time window try to go on referenced window */
		if (c->net_wm_user_time_window() != nullptr) {

			Time * xtime = cnx->read_net_wm_user_time(
					*(c->net_wm_user_time_window()));
			if (xtime != nullptr) {
				if (*xtime != 0) {
					time = *(xtime);
					has_time = true;
				}
				delete xtime;
			}
		}
	}

	return has_time;

}

list<client_managed_t *> page_t::get_managed_windows() {
	return filter_class<client_managed_t>(get_all_childs());
}

client_managed_t * page_t::find_managed_window_with(Window w) {
	client_base_t * c = find_client_with(w);
	if (c != nullptr) {
		return dynamic_cast<client_managed_t *>(c);
	} else {
		return nullptr;
	}
}

/**
 * This will remove a client from tree and destroy related data. This
 * function do not send any X11 request.
 **/
void page_t::cleanup_client(client_base_t * c) {

	if(typeid(*c) == typeid(client_managed_t)) {
		client_managed_t * mw = dynamic_cast<client_managed_t*>(c);
		unmanage(mw);
	}

	remove_client(c);
	update_client_list();
	rpage->mark_durty();

}

void page_t::safe_update_transient_for(client_base_t * c) {
	if (typeid(*c) == typeid(client_managed_t)) {
		client_managed_t * mw = dynamic_cast<client_managed_t*>(c);
		if (mw->is(MANAGED_FLOATING)) {
			insert_in_tree_using_transient_for(mw);
		} else if (mw->is(MANAGED_NOTEBOOK)) {
			/* DO NOTHING */
		} else if (mw->is(MANAGED_FULLSCREEN)) {
			/* DO NOTHING */
		}
	} else if (typeid(*c) == typeid(client_not_managed_t)) {
		client_not_managed_t * uw = dynamic_cast<client_not_managed_t*>(c);

		if (uw->type() == A(_NET_WM_WINDOW_TYPE_TOOLTIP)) {
			detach(uw);
			tooltips.push_back(uw);
			uw->set_parent(this);
		} else if (uw->type() == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
			detach(uw);
			notifications.push_back(uw);
			uw->set_parent(this);
		} else if (uw->net_wm_state() != nullptr
				and has_key(*(uw->net_wm_state()), static_cast<xcb_atom_t>(A(_NET_WM_STATE_ABOVE)))) {
			detach(uw);
			above.push_back(uw);
			uw->set_parent(this);
		} else if (uw->net_wm_state() != nullptr
				and has_key(*(uw->net_wm_state()), static_cast<xcb_atom_t>(A(_NET_WM_STATE_BELOW)))) {
			detach(uw);
			below.push_back(uw);
			uw->set_parent(this);
		} else {
			insert_in_tree_using_transient_for(uw);
		}
	}
}

void page_t::update_page_areas() {

	if (page_areas != 0) {
		delete page_areas;
	}

	list<tree_t *> l = get_all_childs();
	list<tree_t const *> lc(l.begin(), l.end());
	page_areas = compute_page_areas(lc);
}

void page_t::set_desktop_geometry(long width, long height) {
	/* define desktop geometry */
	long desktop_geometry[2];
	desktop_geometry[0] = width;
	desktop_geometry[1] = height;
	cnx->change_property(cnx->root(), _NET_DESKTOP_GEOMETRY,
			CARDINAL, 32, desktop_geometry, 2);
}

client_base_t * page_t::find_client_with(Window w) {
	for(auto &i: clients) {
		if(i.second->has_window(w)) {
			return i.second;
		}
	}
	return nullptr;
}

client_base_t * page_t::find_client(Window w) {
	auto i = clients.find(w);
	if (i != clients.end()) {
		return i->second;
	}
	return nullptr;
}

void page_t::remove_client(client_base_t * c) {
	clients.erase(c->orig());
	tree_t * parent = c->parent();
	if (parent != nullptr) {
		if (typeid(*parent) == typeid(notebook_t)) {
			notebook_t * nb = dynamic_cast<notebook_t*>(parent);
			rpage->add_damaged(nb->allocation());
		}
	}
	detach(c);
	list<tree_t *> subclient = c->childs();
	for(auto i: subclient) {
		client_base_t * c = dynamic_cast<client_base_t *>(i);
		if(c != nullptr) {
			insert_in_tree_using_transient_for(c);
		}
	}
	delete c;
}

void page_t::add_client(client_base_t * c) {
	auto i = clients.find(c->orig());
	if(i != clients.end() and c != i->second) {
		auto x = i->second;
		detach(x);
		delete x;
		clients.erase(i);
	}
	clients[c->orig()] = c;
}

list<tree_t *> page_t::childs() const {
	list<tree_t *> ret;
	for (auto &i : viewport_outputs) {
		if (i.second != nullptr) {
			ret.push_back(i.second);
		}
	}

	for(auto x: below) {
		ret.push_back(x);
	}

	for(auto x: docks) {
		ret.push_back(x);
	}

	for(auto x: fullscreen_client_to_viewport) {
		ret.push_back(x.first);
	}

	for(auto x: root_subclients) {
		ret.push_back(x);
	}

	for(auto x: tooltips) {
		ret.push_back(x);
	}

	for(auto x: notifications) {
		ret.push_back(x);
	}

	for(auto x: above) {
		ret.push_back(x);
	}


	return ret;
}

string page_t::get_node_name() const {
	return _get_node_name<'P'>();
}

void page_t::replace(page_component_t * src, page_component_t * by) {
	printf("Unexpectected use of page::replace function\n");
}

void page_t::raise_child(tree_t * t) {

	if(t == nullptr)
		return;

	/** TODO: raise a viewport **/

	/* do nothing, not needed at this level */
	client_base_t * x = dynamic_cast<client_base_t *>(t);
	if(has_key(root_subclients, x)) {
		root_subclients.remove(x);
		root_subclients.push_back(x);
	}

	client_not_managed_t * y = dynamic_cast<client_not_managed_t *>(t);
	if(has_key(tooltips, y)) {
		tooltips.remove(y);
		tooltips.push_back(y);
	}

	if(has_key(notifications, y)) {
		notifications.remove(y);
		notifications.push_back(y);
	}

	if(has_key(docks, y)) {
		docks.remove(y);
		docks.push_back(y);
	}

	if(has_key(above, y)) {
		above.remove(y);
		above.push_back(y);
	}

	if(has_key(below, y)) {
		below.remove(y);
		below.push_back(y);
	}
}

void page_t::remove(tree_t * t) {

	for(auto &i: viewport_outputs) {
		if(i.second == t) {
			cout << "WARNING: using page::remove to remove viewport is not recommended, use page::remove_viewport instead" << endl;
			remove_viewport(i.second);
			return;
		}
	}

	root_subclients.remove(dynamic_cast<client_base_t *>(t));
	tooltips.remove(dynamic_cast<client_not_managed_t *>(t));
	notifications.remove(dynamic_cast<client_not_managed_t *>(t));
	fullscreen_client_to_viewport.erase(dynamic_cast<client_managed_t *>(t));
	docks.remove(dynamic_cast<client_not_managed_t*>(t));
	above.remove(dynamic_cast<client_not_managed_t*>(t));
	below.remove(dynamic_cast<client_not_managed_t*>(t));

}

void page_t::create_identity_window() {
	/* create an invisible window to identify page */
	identity_window = XCreateSimpleWindow(cnx->dpy(), cnx->root(), -100,
			-100, 1, 1, 0, 0, 0);
	std::string name("page");
	cnx->change_property(identity_window, _NET_WM_NAME, UTF8_STRING, 8, name.c_str(),
			name.length() + 1);
	cnx->change_property(identity_window, _NET_SUPPORTING_WM_CHECK, WINDOW, 32,
			&identity_window, 1);
	cnx->change_property(cnx->root(), _NET_SUPPORTING_WM_CHECK,
			WINDOW, 32, &identity_window, 1);
	long pid = getpid();
	cnx->change_property(identity_window, _NET_WM_PID, CARDINAL, 32, &pid, 1);
	XSelectInput(cnx->dpy(), identity_window, StructureNotifyMask | PropertyChangeMask);
}

void page_t::register_wm() {
	if (!cnx->register_wm(replace_wm, identity_window)) {
		throw runtime_error("Cannot register window manager");
	}
}

void page_t::register_cm() {
	if (use_internal_compositor) {
		if (!cnx->register_cm(identity_window)) {
			throw runtime_error("Cannot register composite manager");
		}
	}
}

void page_t::check_x11_extension() {
	if (!cnx->check_shape_extension(&xshape_opcode, &xshape_event,
			&xshape_error)) {
		throw std::runtime_error(SHAPENAME " extension is not supported");
	}

	if (!cnx->check_randr_extension(&xrandr_opcode, &xrandr_event,
			&xrandr_error)) {
		throw std::runtime_error(RANDR_NAME " extension is not supported");
	}

	if (use_internal_compositor) {
		if (!cnx->check_composite_extension(&composite_opcode, &composite_event,
				&composite_error)) {
			throw std::runtime_error("X Server doesn't support Composite 0.4");
		}

		if (!cnx->check_damage_extension(&damage_opcode, &damage_event,
				&damage_error)) {
			throw std::runtime_error("Damage extension is not supported");
		}

		if (!cnx->check_shape_extension(&xshape_opcode, &xshape_event,
				&xshape_error)) {
			throw std::runtime_error(SHAPENAME " extension is not supported");
		}

		if (!cnx->check_randr_extension(&xrandr_opcode, &xrandr_event,
				&xrandr_error)) {
			throw std::runtime_error(RANDR_NAME " extension is not supported");
		}
	}
}




/**
 * Update grab keys aware of current keymap
 */
void page_t::update_grabkey() {

	assert(keymap != nullptr);

	/** ungrab all previews key **/
	XUngrabKey(cnx->dpy(), AnyKey, AnyModifier, cnx->root());

	int kc = 0;

	if ((kc = keymap->find_keysim(bind_debug_1.ks))) {
		XGrabKey(cnx->dpy(), kc, bind_debug_1.mod, cnx->root(),
		True, GrabModeAsync, GrabModeAsync);
		if(keymap->numlock_mod_mask() != 0) {
			XGrabKey(cnx->dpy(), kc, bind_debug_1.mod | keymap->numlock_mod_mask(), cnx->root(),
			True, GrabModeAsync, GrabModeAsync);
		}
	}

	if ((kc = keymap->find_keysim(bind_debug_2.ks))) {
		XGrabKey(cnx->dpy(), kc, bind_debug_2.mod, cnx->root(),
		True, GrabModeAsync, GrabModeAsync);
		if(keymap->numlock_mod_mask() != 0) {
			XGrabKey(cnx->dpy(), kc, bind_debug_2.mod | keymap->numlock_mod_mask(), cnx->root(),
			True, GrabModeAsync, GrabModeAsync);
		}
	}

	if ((kc = keymap->find_keysim(bind_debug_3.ks))) {
		XGrabKey(cnx->dpy(), kc, bind_debug_3.mod, cnx->root(),
		True, GrabModeAsync, GrabModeAsync);
		if(keymap->numlock_mod_mask() != 0) {
			XGrabKey(cnx->dpy(), kc, bind_debug_3.mod | keymap->numlock_mod_mask(), cnx->root(),
			True, GrabModeAsync, GrabModeAsync);
		}
	}

	if ((kc = keymap->find_keysim(bind_debug_4.ks))) {
		XGrabKey(cnx->dpy(), kc, bind_debug_4.mod, cnx->root(),
		True, GrabModeAsync, GrabModeAsync);
		if(keymap->numlock_mod_mask() != 0) {
			XGrabKey(cnx->dpy(), kc, bind_debug_4.mod | keymap->numlock_mod_mask(), cnx->root(),
			True, GrabModeAsync, GrabModeAsync);
		}
	}

	/* quit page */
	if ((kc = keymap->find_keysim(bind_page_quit.ks))) {
		printf("bind page quit => kc: %d mod: %02x, keysym: %04x\n", kc, (unsigned int)bind_page_quit.mod, (unsigned int)bind_page_quit.ks);
		XGrabKey(cnx->dpy(), kc, bind_page_quit.mod, cnx->root(),
		True, GrabModeAsync, GrabModeAsync);
		if(keymap->numlock_mod_mask() != 0) {
			XGrabKey(cnx->dpy(), kc, bind_page_quit.mod | keymap->numlock_mod_mask(), cnx->root(),
			True, GrabModeAsync, GrabModeAsync);
		}
	}

	/* toggle fullscreen */
	if ((kc = keymap->find_keysim(bind_toggle_fullscreen.ks))) {
		printf("bind toggle fullscreen => kc: %d mod: %02x, keysym: %04x\n", kc, (unsigned int)bind_toggle_fullscreen.mod, (unsigned int)bind_toggle_fullscreen.ks);
		XGrabKey(cnx->dpy(), kc, bind_toggle_fullscreen.mod, cnx->root(),
		True, GrabModeAsync, GrabModeAsync);
		if(keymap->numlock_mod_mask() != 0) {
			XGrabKey(cnx->dpy(), kc, bind_toggle_fullscreen.mod | keymap->numlock_mod_mask(), cnx->root(),
			True, GrabModeAsync, GrabModeAsync);
		}
	}

	/* Alt-Tab */
	if ((kc = keymap->find_keysim(XK_Tab))) {
		XGrabKey(cnx->dpy(), kc, Mod1Mask, cnx->root(),
		False, GrabModeAsync, GrabModeSync);
		if(keymap->numlock_mod_mask() != 0) {
			XGrabKey(cnx->dpy(), kc, Mod1Mask | keymap->numlock_mod_mask(), cnx->root(),
			True, GrabModeAsync, GrabModeAsync);
		}
	}


}

void page_t::update_keymap() {
	if(keymap != nullptr) {
		delete keymap;
	}
	keymap = new keymap_t(cnx->dpy());
}

void page_t::prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time) {

	rnd->add_damaged(rpage->get_damaged());
	rpage->repair_damaged(get_all_childs());

	/**
	 * first get renderable and then repair it to get proper
	 * damaged area
	 **/
	out += dynamic_pointer_cast<renderable_t>(rpage->prepare_render());
	rpage->repair_damaged(get_all_childs());

	tree_t::_prepare_render(out, time);

	/** Add all possible popups **/
	if(pat->is_visible()) {
		out.push_back(pat);
	}

	if(ps->is_visible()) {
		out.push_back(ps);
	}

	if(pn0->is_visible()) {
		out.push_back(pn0);
	}

	if(pfm->is_visible()) {
		out.push_back(pfm);
	}

	if(menu->is_visible()) {
		out.push_back(menu);
	}

	_need_render = false;

}


vector<page_event_t> * page_t::compute_page_areas(
		list<tree_t const *> const & page) const {

	vector<page_event_t> * ret = new vector<page_event_t>();

	for (list<tree_t const *>::const_iterator i = page.begin();
			i != page.end(); ++i) {

		if(dynamic_cast<split_t const *>(*i)) {
			split_t const * s = dynamic_cast<split_t const *>(*i);
			page_event_t bsplit(PAGE_EVENT_SPLIT);
			bsplit.position = s->compute_split_bar_location();

			if(s->type() == VERTICAL_SPLIT) {
				bsplit.position.w += theme->notebook.margin.right + theme->notebook.margin.left;
				bsplit.position.x -= theme->notebook.margin.right;
			} else {
				bsplit.position.h += theme->notebook.margin.bottom;
				bsplit.position.y -= theme->notebook.margin.bottom;
			}

			bsplit.spt = s;
			ret->push_back(bsplit);
		} else if (dynamic_cast<notebook_t const *>(*i)) {
			notebook_t const * n = dynamic_cast<notebook_t const *>(*i);
			n->compute_areas_for_notebook(ret);
		} else if (dynamic_cast<viewport_base_t const *>(*i)) {
			/** nothing to do **/
		}
	}

	return ret;
}

page_component_t * page_t::parent() const {
	return nullptr;
}

void page_t::set_parent(tree_t * parent) {
	throw exception_t("page::page_t can't have parent nor tree_t parent");
}

void page_t::set_parent(page_component_t * parent) {
	throw exception_t("page::page_t can't have parent");
}

void page_t::set_allocation(i_rect const & r) {
	_allocation = r;
}

}
