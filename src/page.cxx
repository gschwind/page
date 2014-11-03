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

#include <cairo.h>
#include <cairo-xlib.h>

#include <cstdlib>
#include <cstring>
#include <cassert>

#include <string>
#include <sstream>
#include <limits>
#include <stdint.h>
#include <stdexcept>
#include <set>
#include <stack>
#include <vector>
#include <typeinfo>
#include <memory>

#include "utils.hxx"

#include "renderable.hxx"
#include "key_desc.hxx"
#include "time.hxx"
#include "atoms.hxx"
#include "client_base.hxx"
#include "client_managed.hxx"
#include "client_not_managed.hxx"

#include "simple2_theme.hxx"

#include "notebook.hxx"
#include "desktop.hxx"
#include "split.hxx"
#include "page.hxx"

#include "popup_alt_tab.hxx"

/* ICCCM definition */
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

namespace page {

using namespace std;

long int const ClientEventMask = (StructureNotifyMask | PropertyChangeMask);

time_t const page_t::default_wait{1000000000L / 120L};

page_t::page_t(int argc, char ** argv)
{

	/** initialize the empty desktop **/
	_current_desktop = 0;
	for(unsigned k = 0; k < 4; ++k) {
		desktop_t * d = new desktop_t;
		d->set_parent(this);
		_desktop_list.push_back(d);
		d->hide();
	}

	_desktop_list[_current_desktop]->show();

	page_areas = nullptr;
	replace_wm = false;
	char const * conf_file_name = 0;

	_need_render = false;

	/** parse command line **/

	int k = 1;
	while(k < argc) {
		std::string x = argv[k];
		if(x == "--replace") {
			replace_wm = true;
		} else {
			conf_file_name = argv[k];
		}
		++k;
	}

	keymap = nullptr;
	process_mode = PROCESS_NORMAL;
	key_press_mode = KEY_PRESS_NORMAL;

	cnx = new display_t();
	rnd = nullptr;

	set_window_cursor(cnx->root(), cnx->xc_left_ptr);

	running = false;

	/* load configurations, from lower priority to high one */

	/* load default configuration */
	conf.merge_from_file_if_exist(std::string(DATA_DIR "/page/page.conf"));

	/* load homedir configuration */
	{
		char const * chome = getenv("HOME");
		if(chome != nullptr) {
			std::string xhome = chome;
			std::string file = xhome + "/.page.conf";
			conf.merge_from_file_if_exist(file);
		}
	}

	/* load file in arguments if provided */
	if (conf_file_name != nullptr) {
		std::string s(conf_file_name);
		conf.merge_from_file_if_exist(s);
	}

	page_base_dir = conf.get_string("default", "theme_dir");

	_last_focus_time = XCB_TIME_CURRENT_TIME;
	_last_button_press = XCB_TIME_CURRENT_TIME;

	theme = nullptr;
	rpage = nullptr;

	_client_focused.push_front(nullptr);

	find_key_from_string(conf.get_string("default", "bind_page_quit"), bind_page_quit);
	find_key_from_string(conf.get_string("default", "bind_toggle_fullscreen"), bind_toggle_fullscreen);
	find_key_from_string(conf.get_string("default", "bind_right_desktop"), bind_right_desktop);
	find_key_from_string(conf.get_string("default", "bind_left_desktop"), bind_left_desktop);


	find_key_from_string(conf.get_string("default", "bind_debug_1"), bind_debug_1);
	find_key_from_string(conf.get_string("default", "bind_debug_2"), bind_debug_2);
	find_key_from_string(conf.get_string("default", "bind_debug_3"), bind_debug_3);
	find_key_from_string(conf.get_string("default", "bind_debug_4"), bind_debug_4);

}

page_t::~page_t() {

	cnx->unload_cursors();

	delete rpage;

	pfm.reset();
	pn0.reset();
	ps.reset();
	pat.reset();
	menu.reset();
	client_menu.reset();

	for (auto i : filter_class<client_managed_t>(tree_t::get_all_children())) {
		cnx->reparentwindow(i->orig(), cnx->root(), 0, 0);
	}

	for (auto i : tree_t::get_all_children()) {
		delete i;
	}

	delete page_areas;
	delete theme;
	delete rnd;
	delete keymap;

	// cleanup cairo, for valgrind happiness.
	cairo_debug_reset_static_data();

	if(cnx != nullptr) {
		/** clean up properties defined by Window Manager **/
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
	cnx->check_x11_extension();
	/* Before doing anything, trying to register wm and cm */
	create_identity_window();
	register_wm();
	register_cm();

	/** initialise theme **/
	theme = new simple2_theme_t{cnx, conf};
	cmgr = new composite_surface_manager_t{cnx};

	/* start listen root event before anything each event will be stored to right run later */
	cnx->select_input(cnx->root(), ROOT_EVENT_MASK);

	rnd = new compositor_t(cnx, cmgr);

	/**
	 * listen RRCrtcChangeNotifyMask for possible change in screen layout.
	 **/
	xcb_randr_select_input(cnx->xcb(), cnx->root(), XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE);

	update_viewport_layout();

	/* init page render */
	rpage = new renderable_page_t(cnx, theme, _root_position.w,
			_root_position.h);

	/* create and add popups (overlay) */
	pfm = std::shared_ptr<popup_frame_move_t>{new popup_frame_move_t(theme)};
	pn0 = std::shared_ptr<popup_notebook0_t>{new popup_notebook0_t(theme)};
	ps = std::shared_ptr<popup_split_t>{new popup_split_t(theme)};
	pat = nullptr;
	menu = nullptr;

	cnx->load_cursors();
	set_window_cursor(cnx->root(), cnx->xc_left_ptr);

	update_net_supported();

	/* update number of desktop */
	uint32_t number_of_desktop = _desktop_list.size();
	cnx->change_property(cnx->root(), _NET_NUMBER_OF_DESKTOPS,
			CARDINAL, 32, &number_of_desktop, 1);

	/* define desktop geometry */
	set_desktop_geometry(_root_position.w, _root_position.h);

	/* set viewport */
	uint32_t viewport[2] = { 0, 0 };
	cnx->change_property(cnx->root(), _NET_DESKTOP_VIEWPORT,
			CARDINAL, 32, viewport, 2);

	update_current_desktop();

	/* page is not able to show desktop only windows */
	uint32_t showing_desktop = 0;
	cnx->change_property(cnx->root(), _NET_SHOWING_DESKTOP, CARDINAL,
			32, &showing_desktop, 1);


	std::vector<char> names_list;
	for(unsigned k = 0; k < _desktop_list.size(); ++k) {
		std::ostringstream os;
		os << "Desktop #" << k;
		std::string s{os.str()};
		names_list.insert(names_list.end(), s.begin(), s.end());
		names_list.push_back(0);
	}
	cnx->change_property(cnx->root(), _NET_DESKTOP_NAMES,
			UTF8_STRING, 8, &names_list[0], names_list.size());

	wm_icon_size_t icon_size;
	icon_size.min_width = 16;
	icon_size.min_height = 16;
	icon_size.max_width = 16;
	icon_size.max_height = 16;
	icon_size.width_inc = 16;
	icon_size.height_inc = 16;
	cnx->change_property(cnx->root(), WM_ICON_SIZE,
			CARDINAL, 32, &icon_size, 6);

	/* setup _NET_ACTIVE_WINDOW */
	set_focus(0, 0);

	scan();
	update_windows_stack();

	uint32_t workarea[4];
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
	xcb_grab_button(cnx->xcb(), false, cnx->root(),
			XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
					| XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_SYNC,
			XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY,
			XCB_MOD_MASK_ANY);


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
	xcb_flush(cnx->xcb());
	running = true;
	while (running) {

		xcb_flush(cnx->xcb());

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

		while(cnx->has_pending_events()) {
			cmgr->process_event(cnx->front_event());
			process_event(cnx->front_event());
			cnx->pop_event();
			xcb_flush(cnx->xcb());
		}

		/** render if no render occurred within previous 1/120 second **/
		time_t cur_tic;
		cur_tic.get_time();
		if (cur_tic > _next_frame) {
			render();
		} else {
			_max_wait = _next_frame - cur_tic;
		}

		xcb_flush(cnx->xcb());
	}
}

void page_t::unmanage(client_managed_t * mw) {
	/* if window is in move/resize/notebook move, do cleanup */
	cleanup_grab(mw);

	printf("unmanaging : '%s'\n", mw->title().c_str());

	if (has_key(_fullscreen_client_to_viewport, mw)) {
		fullscreen_data_t & data = _fullscreen_client_to_viewport[mw];
		if(not data.desktop->is_hidden())
			data.viewport->show();
		_fullscreen_client_to_viewport.erase(mw);
	}

	detach(mw);

	/* if managed window have active clients */
	for(auto i: mw->childs()) {
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

	update_client_list();
	update_allocation();
	rpage->mark_durty();

	delete mw;

}

void page_t::scan() {

	cnx->grab();
	cnx->fetch_pending_events();

	xcb_query_tree_cookie_t ck = xcb_query_tree(cnx->xcb(), cnx->root());
	xcb_query_tree_reply_t * r = xcb_query_tree_reply(cnx->xcb(), ck, 0);

	if(r == nullptr)
		throw exception_t("Cannot query tree");

	xcb_window_t * children = xcb_query_tree_children(r);
	unsigned n_children = xcb_query_tree_children_length(r);

	for (unsigned i = 0; i < n_children; ++i) {
		xcb_window_t w = children[i];

		std::shared_ptr<client_properties_t> c{new client_properties_t(cnx, w)};
		if (!c->read_window_attributes()) {
			continue;
		}

		if(c->wa()->_class == XCB_WINDOW_CLASS_INPUT_ONLY) {
			continue;
		}

		c->read_all_properties();

		if (c->wa()->map_state != XCB_MAP_STATE_UNMAPPED) {
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

	free(r);

	update_client_list();
	update_allocation();
	update_windows_stack();

	cnx->ungrab();

	print_state();

}

void page_t::update_net_supported() {
	std::vector<xcb_atom_t> supported_list;

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
	set<xcb_window_t> s_client_list;
	set<xcb_window_t> s_client_list_stack;
	std::vector<client_managed_t *> lst = get_managed_windows();

	for (auto i : lst) {
		s_client_list.insert(i->orig());
		s_client_list_stack.insert(i->orig());
	}

	std::vector<xcb_window_t> client_list(s_client_list.begin(), s_client_list.end());
	std::vector<xcb_window_t> client_list_stack(s_client_list_stack.begin(),
			s_client_list_stack.end());

	cnx->change_property(cnx->root(), _NET_CLIENT_LIST_STACKING,
			WINDOW, 32, &client_list_stack[0], client_list_stack.size());
	cnx->change_property(cnx->root(), _NET_CLIENT_LIST, WINDOW, 32,
			&client_list[0], client_list.size());

}

void page_t::process_event(xcb_key_press_event_t const * e) {

	if(_last_focus_time > e->time) {
		_last_focus_time = e->time;
	}

	printf("%s key = %d, mod4 = %s, mod1 = %s\n",
			e->response_type == XCB_KEY_PRESS ? "KeyPress" : "KeyRelease",
			e->detail,
			e->state & XCB_MOD_MASK_4 ? "true" : "false",
			e->state & XCB_MOD_MASK_1 ? "true" : "false");


	/* get KeyCode for Unmodified Key */
	xcb_keysym_t k = keymap->get(e->detail);

	if (k == 0)
		return;

	if(e->response_type == XCB_KEY_PRESS) {

		if(k == bind_page_quit.ks and (e->state & bind_page_quit.mod)) {
			running = false;
		}

		if(k == bind_toggle_fullscreen.ks and (e->state & bind_toggle_fullscreen.mod)) {
			if(not _client_focused.empty()) {
				if(_client_focused.front() != nullptr) {
					toggle_fullscreen(_client_focused.front());
				}
			}
		}

		if(k == bind_right_desktop.ks and (e->state & bind_right_desktop.mod)) {
			switch_to_desktop((_current_desktop + 1) % _desktop_list.size(), e->time);
		}

		if(k == bind_left_desktop.ks and (e->state & bind_left_desktop.mod)) {
			switch_to_desktop((_current_desktop - 1) % _desktop_list.size(), e->time);
		}


		if(k == bind_debug_1.ks and (e->state & bind_debug_1.mod)) {
			if(rnd->show_fps()) {
				rnd->set_show_fps(false);
			} else {
				rnd->set_show_fps(true);
			}
		}

		if(k == bind_debug_2.ks and (e->state & bind_debug_2.mod)) {
			if(rnd->show_damaged()) {
				rnd->set_show_damaged(false);
			} else {
				rnd->set_show_damaged(true);
			}
		}

		if(k == bind_debug_3.ks and (e->state & bind_debug_3.mod)) {
			if(rnd->show_opac()) {
				rnd->set_show_opac(false);
			} else {
				rnd->set_show_opac(true);
			}
		}

		if(k == bind_debug_4.ks and (e->state & bind_debug_4.mod)) {
			update_windows_stack();
			//print_tree(0);
			std::vector<client_managed_t *> lst = get_managed_windows();
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


		if (k == XK_Tab and ((e->state & 0x0f) == Mod1Mask)) {

			std::vector<client_managed_t *> managed_window = get_managed_windows();

			if (key_press_mode == KEY_PRESS_NORMAL and not managed_window.empty()) {

				/* Grab keyboard */
				/** TODO: check for success or failure **/
				xcb_grab_keyboard_unchecked(cnx->xcb(), false, cnx->root(), e->time, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

				/** Continue to play event as usual (Alt+Tab is in Sync mode) **/
				xcb_allow_events(cnx->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);

				key_press_mode = KEY_PRESS_ALT_TAB;
				key_mode_data.selected = _client_focused.front();

				int sel = 0;

				std::vector<std::shared_ptr<cycle_window_entry_t>> v;
				int s = 0;

				for (auto i : managed_window) {
					std::shared_ptr<icon64> icon{new icon64(*i)};
					std::shared_ptr<cycle_window_entry_t> cy{new cycle_window_entry_t{i, i->title(), icon}};
					v.push_back(cy);
					if (i == _client_focused.front()) {
						sel = s;
					}
					++s;
				}

				pat = std::shared_ptr<popup_alt_tab_t>{new popup_alt_tab_t{cnx, theme, v ,sel}};

				/** show it on all viewport ? **/
				viewport_t * viewport = _desktop_list[0]->get_any_viewport();

				int y = v.size() / 4 + 1;

				pat->move_resize(
						i_rect(
								viewport->raw_area().x
										+ (viewport->raw_area().w - 80 * 4) / 2,
								viewport->raw_area().y
										+ (viewport->raw_area().h - y * 80) / 2,
								80 * 4, y * 80));
				pat->show();



			} else {
				xcb_allow_events(cnx->xcb(), XCB_ALLOW_REPLAY_KEYBOARD, e->time);
			}

			pat->select_next();
			_need_render = true;

		}



	} else {
		/** here we guess Mod1 is bound to Alt **/
		if ((XK_Alt_L == k or XK_Alt_R == k)
				and key_press_mode == KEY_PRESS_ALT_TAB) {

			/** Ungrab Keyboard **/
			xcb_ungrab_keyboard(cnx->xcb(), e->time);

			key_press_mode = KEY_PRESS_NORMAL;

			/**
			 * do not use dynamic_cast because managed window can be already
			 * destroyed.
			 **/
			client_managed_t * mw = pat->get_selected();
			set_focus(mw, e->time);

			pat.reset();
		}
	}

}

/* Button event make page to grab pointer */
void page_t::process_event_press(xcb_button_press_event_t const * e) {

//	std::cout << "Button Event Press "
//			<< " event=" << e->event
//			<< " child=" << e->child
//			<< " root=" << e->root
//			<< " button=" << static_cast<int>(e->detail)
//			<< " mod1=" << (e->state & XCB_MOD_MASK_1 ? "true" : "false")
//			<< " mod2=" << (e->state & XCB_MOD_MASK_2 ? "true" : "false")
//			<< " mod3=" << (e->state & XCB_MOD_MASK_3 ? "true" : "false")
//			<< " mod4=" << (e->state & XCB_MOD_MASK_4 ? "true" : "false")
//			<< std::endl;


	client_managed_t * mw = find_managed_window_with(e->event);

	switch (process_mode) {
	case PROCESS_NORMAL:

		if (e->event == cnx->root()
				and e->child == XCB_NONE
				and e->detail == XCB_BUTTON_INDEX_1) {

			update_page_areas();

			page_event_t * b = nullptr;
			for (auto &i : *page_areas) {
				//printf("box = %s => %s\n", (i)->position.to_std::string().c_str(), typeid(*i).name());
				if (i.position.is_inside(e->root_x, e->root_y)) {
					b = &i;
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

					std::list<client_managed_t *> managed_window = mode_data_notebook_menu.from->get_clients();

					int sel = 0;

					std::vector<std::shared_ptr<notebook_dropdown_menu_t::item_t>> v;
					int s = 0;

					for (auto i : managed_window) {
						std::shared_ptr<notebook_dropdown_menu_t::item_t> cy{new notebook_dropdown_menu_t::item_t(i, i->icon(), i->title())};
						v.push_back(cy);
						if (i == _client_focused.front()) {
							sel = s;
						}
						++s;
					}

					int x = mode_data_notebook_menu.from->allocation().x;
					int y = mode_data_notebook_menu.from->allocation().y + theme->notebook.margin.top;

					menu = std::shared_ptr<notebook_dropdown_menu_t>{new notebook_dropdown_menu_t(cnx, theme, v, x, y, mode_data_notebook_menu.from->allocation().w)};
				}
			}

		}

		if (e->event == cnx->root()
				and e->child == XCB_NONE
				and e->detail == XCB_BUTTON_INDEX_3) {

			update_page_areas();

			page_event_t * b = nullptr;
			for (auto &i: *page_areas) {
				if (i.position.is_inside(e->event_x, e->event_y)) {
					b = &(i);
					break;
				}
			}

			if (b != nullptr) {
				if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT) {
					process_mode = PROCESS_NOTEBOOK_CLIENT_MENU;

					if(mode_data_notebook_client_menu.active_grab == false) {
						mode_data_notebook_client_menu.from = const_cast<notebook_t*>(b->nbk);
						mode_data_notebook_client_menu.client = const_cast<client_managed_t *>(b->clt);
						mode_data_notebook_client_menu.b = b->position;

						std::vector<std::shared_ptr<client_dropdown_menu_t::item_t>> v;
						for(unsigned k = 0; k < _desktop_list.size(); ++k) {
							std::ostringstream os;
							os << "Desktop #" << k;
							auto item = new client_dropdown_menu_t::item_t(k, nullptr, os.str());
							v.push_back(std::shared_ptr<client_dropdown_menu_t::item_t>{item});
						}

						int x = e->root_x;
						int y = e->root_y;

						client_menu = std::shared_ptr<client_dropdown_menu_t>{new client_dropdown_menu_t(cnx, theme, v, x, y, 300)};

					}
				}
			}
		}


		if (mw != nullptr) {

			if (mw->is(MANAGED_FLOATING)
					and e->detail == XCB_BUTTON_INDEX_1
					and (e->state & (XCB_MOD_MASK_1 | XCB_MOD_MASK_CONTROL))) {

				mode_data_floating.x_offset = e->root_x - mw->base_position().x;
				mode_data_floating.y_offset = e->root_y - mw->base_position().y;
				mode_data_floating.x_root = e->root_x;
				mode_data_floating.y_root = e->root_y;
				mode_data_floating.f = mw;
				mode_data_floating.original_position = mw->get_floating_wished_position();
				mode_data_floating.final_position = mw->get_floating_wished_position();
				mode_data_floating.popup_original_position = mw->get_base_position();
				mode_data_floating.button = XCB_BUTTON_INDEX_1;

				//pfm->move_resize(mw->get_base_position());
				//pfm->update_window(mw, mw->title());
				//pfm->show();

				if ((e->state & ControlMask)) {
					process_mode = PROCESS_FLOATING_RESIZE;
					mode_data_floating.mode = RESIZE_BOTTOM_RIGHT;
					set_window_cursor(mw->base(), cnx->xc_bottom_righ_corner);
					set_window_cursor(mw->orig(), cnx->xc_bottom_righ_corner);
				} else {
					safe_raise_window(mw);
					process_mode = PROCESS_FLOATING_MOVE;
					set_window_cursor(mw->base(), cnx->xc_fleur);
					set_window_cursor(mw->orig(), cnx->xc_fleur);
				}


			} else if (mw->is(MANAGED_FLOATING)
					and e->detail == XCB_BUTTON_INDEX_1
					and e->child != mw->orig()
					and e->event == mw->deco()) {

				auto const * l = mw->floating_areas();
				floating_event_t const * b = nullptr;
				for (auto &i : (*l)) {
					if(i.position.is_inside(e->event_x, e->event_y)) {
						b = &i;
						break;
					}
				}


				if (b != nullptr) {

					mode_data_floating.x_offset = e->root_x - mw->base_position().x;
					mode_data_floating.y_offset = e->root_y - mw->base_position().y;
					mode_data_floating.x_root = e->root_x;
					mode_data_floating.y_root = e->root_y;
					mode_data_floating.f = mw;
					mode_data_floating.original_position = mw->get_floating_wished_position();
					mode_data_floating.final_position = mw->get_floating_wished_position();
					mode_data_floating.popup_original_position = mw->get_base_position();
					mode_data_floating.button = XCB_BUTTON_INDEX_1;

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
						set_window_cursor(mw->base(), cnx->xc_fleur);
					} else {

						//pfm->move_resize(mw->get_base_position());
						//pfm->update_window(mw, mw->title());
						//pfm->show();

						if (b->type == FLOATING_EVENT_GRIP_TOP) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_TOP;
							set_window_cursor(mw->base(), cnx->xc_top_side);
						} else if (b->type == FLOATING_EVENT_GRIP_BOTTOM) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_BOTTOM;
							set_window_cursor(mw->base(), cnx->xc_bottom_side);
						} else if (b->type == FLOATING_EVENT_GRIP_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_LEFT;
							set_window_cursor(mw->base(), cnx->xc_left_side);
						} else if (b->type == FLOATING_EVENT_GRIP_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_RIGHT;
							set_window_cursor(mw->base(), cnx->xc_right_side);
						} else if (b->type == FLOATING_EVENT_GRIP_TOP_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_TOP_LEFT;
							set_window_cursor(mw->base(), cnx->xc_top_left_corner);
						} else if (b->type == FLOATING_EVENT_GRIP_TOP_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_TOP_RIGHT;
							set_window_cursor(mw->base(), cnx->xc_top_right_corner);
						} else if (b->type == FLOATING_EVENT_GRIP_BOTTOM_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_BOTTOM_LEFT;
							set_window_cursor(mw->base(), cnx->xc_bottom_left_corner);
						} else if (b->type == FLOATING_EVENT_GRIP_BOTTOM_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE;
							mode_data_floating.mode = RESIZE_BOTTOM_RIGHT;
							set_window_cursor(mw->base(), cnx->xc_bottom_righ_corner);
						} else {
							safe_raise_window(mw);
							process_mode = PROCESS_FLOATING_MOVE;
							set_window_cursor(mw->base(), cnx->xc_fleur);
						}
					}

				}

			} else if (mw->is(MANAGED_FULLSCREEN) and e->detail == (XCB_BUTTON_INDEX_1)
					and (e->state & (XCB_MOD_MASK_1))) {
				fprintf(stderr, "start FULLSCREEN MOVE\n");
				/** start moving fullscreen window **/
				viewport_t * v = find_mouse_viewport(e->root_x, e->root_y);

				if (v != 0) {
					fprintf(stderr, "start FULLSCREEN MOVE\n");
					process_mode = PROCESS_FULLSCREEN_MOVE;
					mode_data_fullscreen.mw = mw;
					mode_data_fullscreen.v = v;
					pn0->update_window(mw);
					pn0->show();
					pn0->move_resize(v->raw_area());
				}
			} else if (mw->is(MANAGED_NOTEBOOK) and e->detail == (XCB_BUTTON_INDEX_1)
					and (e->state & (XCB_MOD_MASK_1))) {

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
		xcb_allow_events(cnx->xcb(), XCB_ALLOW_REPLAY_POINTER, e->time);
		client_managed_t * mw = find_managed_window_with(e->event);
		if (mw != nullptr) {
			set_focus(mw, e->time);
		}
	} else {
		/* Do not replay events, grab them and process them until Release Button */
		xcb_allow_events(cnx->xcb(), XCB_ALLOW_ASYNC_POINTER, e->time);
	}

}

void page_t::process_event_release(xcb_button_press_event_t const * e) {

	switch (process_mode) {
	case PROCESS_NORMAL:
		fprintf(stderr, "Warning: release and normal mode are incompatible\n");
		break;
	case PROCESS_SPLIT_GRAB:
		if (e->detail == XCB_BUTTON_INDEX_1) {
			process_mode = PROCESS_NORMAL;
			ps->hide();
			rnd->add_damaged(mode_data_split.split->allocation());
			mode_data_split.split->set_split(mode_data_split.split_ratio);
			rpage->add_damaged(mode_data_split.split->allocation());
			mode_data_split.reset();

		}
		break;
	case PROCESS_NOTEBOOK_GRAB:
		if (e->detail == XCB_BUTTON_INDEX_1) {
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
					set_focus(mode_data_notebook.c, e->time);
					mode_data_notebook.from->set_selected(mode_data_notebook.c);
				} else {
					if(mode_data_notebook.from->get_selected() == mode_data_notebook.c
							and _client_focused.front() == mode_data_notebook.c) {
						/** focus root **/
						set_focus(nullptr, e->time);
						/** iconify **/
						mode_data_notebook.from->iconify_client(mode_data_notebook.c);
					} else {
						set_focus(mode_data_notebook.c, e->time);
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
			set_focus(mode_data_notebook.c, e->time);
			if (mode_data_notebook.from != nullptr)
				rpage->add_damaged(mode_data_notebook.from->allocation());
			if (mode_data_notebook.ns != nullptr)
				rpage->add_damaged(mode_data_notebook.ns->allocation());

			mode_data_notebook.reset();
		}
		break;
	case PROCESS_NOTEBOOK_BUTTON_PRESS:
		if (e->detail == XCB_BUTTON_INDEX_1) {
			process_mode = PROCESS_NORMAL;

			{
				page_event_t * b = 0;
				for (auto &i : *page_areas) {
					if (i.position.is_inside(e->event_x, e->event_y)) {
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
						rpage->add_damaged(mode_data_notebook.from->allocation());
						rpage->add_damaged(_desktop_list[_current_desktop]->get_default_pop()->allocation());
						_desktop_list[_current_desktop]->set_default_pop(mode_data_notebook.from);
					} else if (b->type == PAGE_EVENT_NOTEBOOK_CLIENT_CLOSE) {
						mode_data_notebook.c->delete_window(e->time);
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
		if (e->detail == XCB_BUTTON_INDEX_1) {
			pfm->hide();

			cnx->set_window_cursor(mode_data_floating.f->base(), XCB_NONE);
			cnx->set_window_cursor(mode_data_floating.f->orig(), XCB_NONE);

			mode_data_floating.f->set_floating_wished_position(
					mode_data_floating.final_position);
			mode_data_floating.f->reconfigure();

			set_focus(mode_data_floating.f, e->time);

			process_mode = PROCESS_NORMAL;
			mode_data_floating.reset();
		}
		break;
	case PROCESS_FLOATING_MOVE_BY_CLIENT:
		if (e->detail == mode_data_floating.button) {
			pfm->hide();

			xcb_ungrab_pointer(cnx->xcb(), e->time);

			mode_data_floating.f->set_floating_wished_position(
					mode_data_floating.final_position);
			mode_data_floating.f->reconfigure();

			set_focus(mode_data_floating.f, e->time);

			process_mode = PROCESS_NORMAL;
			mode_data_floating.reset();
		}
		break;
	case PROCESS_FLOATING_RESIZE:
		if (e->detail == XCB_BUTTON_INDEX_1) {
			pfm->hide();

			cnx->set_window_cursor(mode_data_floating.f->base(), XCB_NONE);
			cnx->set_window_cursor(mode_data_floating.f->orig(), XCB_NONE);

			mode_data_floating.f->set_floating_wished_position(
					mode_data_floating.final_position);
			mode_data_floating.f->reconfigure();

			set_focus(mode_data_floating.f, e->time);
			process_mode = PROCESS_NORMAL;

			mode_data_floating.reset();
		}
		break;
	case PROCESS_FLOATING_RESIZE_BY_CLIENT:
		if (e->detail == mode_data_floating.button) {

			xcb_ungrab_pointer(cnx->xcb(), e->time);

			mode_data_floating.f->set_floating_wished_position(
					mode_data_floating.final_position);
			mode_data_floating.f->reconfigure();

			set_focus(mode_data_floating.f, e->time);

			process_mode = PROCESS_NORMAL;
			mode_data_floating.reset();
		}
		break;
	case PROCESS_FLOATING_CLOSE:

		if (e->detail == XCB_BUTTON_INDEX_1) {
			client_managed_t * mw = mode_data_floating.f;
			mw->delete_window(e->time);
			/* cleanup */
			process_mode = PROCESS_NORMAL;
			mode_data_floating.reset();
		}

		break;

	case PROCESS_FLOATING_BIND:
		if (e->detail == XCB_BUTTON_INDEX_1) {
			process_mode = PROCESS_NORMAL;

			pn0->hide();

			set_focus(mode_data_bind.c, e->time);

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

		if (e->detail == XCB_BUTTON_INDEX_1) {
			/** drop the fullscreen window to the new viewport **/

			process_mode = PROCESS_NORMAL;
			viewport_t * v = find_mouse_viewport(e->root_x, e->root_y);
			client_managed_t * c = mode_data_fullscreen.mw;
			if (v != nullptr and has_key(_fullscreen_client_to_viewport, c)) {
				fullscreen_data_t & data = _fullscreen_client_to_viewport[c];
				if (v != data.viewport) {
					data.viewport = v;
					data.desktop = find_desktop_of(v);
					update_desktop_visibility();
				}
			}

			pn0->hide();
			mode_data_fullscreen.reset();

		}

		break;
	case PROCESS_NOTEBOOK_MENU:
		if (e->detail == XCB_BUTTON_INDEX_1) {
			if(mode_data_notebook_menu.b.is_inside(e->event_x, e->event_y) and not mode_data_notebook_menu.active_grab) {
				mode_data_notebook_menu.active_grab = true;
				xcb_grab_pointer(cnx->xcb(), true, cnx->root(), XCB_EVENT_MASK_BUTTON_PRESS|XCB_EVENT_MASK_BUTTON_RELEASE|XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, e->time);
			} else {
				if (mode_data_notebook_menu.active_grab) {
					xcb_ungrab_pointer(cnx->xcb(), e->time);
					mode_data_notebook_menu.active_grab = false;
				}

				process_mode = PROCESS_NORMAL;
				if (menu->position().is_inside(e->root_x, e->root_y)) {
					int selx = (int) floor(
							(e->root_y - menu->position().y) / 24.0);
					menu->set_selected(selx);
					mode_data_notebook_menu.from->set_selected(
							const_cast<client_managed_t*>(menu->get_selected()));

					rpage->add_damaged(
							mode_data_notebook_menu.from->allocation());
				}
				mode_data_notebook_menu.from = nullptr;
				rnd->add_damaged(menu->get_visible_region());
				menu.reset();
			}
		}
		break;
	case PROCESS_NOTEBOOK_CLIENT_MENU:
		if (e->detail == XCB_BUTTON_INDEX_3 or e->detail == XCB_BUTTON_INDEX_1) {
			if(mode_data_notebook_client_menu.b.is_inside(e->event_x, e->event_y) and not mode_data_notebook_client_menu.active_grab) {
				mode_data_notebook_client_menu.active_grab = true;
				xcb_grab_pointer(cnx->xcb(), true, cnx->root(), XCB_EVENT_MASK_BUTTON_PRESS|XCB_EVENT_MASK_BUTTON_RELEASE|XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, e->time);
			} else {
				if (mode_data_notebook_client_menu.active_grab) {
					xcb_ungrab_pointer(cnx->xcb(), e->time);
					mode_data_notebook_client_menu.active_grab = false;
				}

				process_mode = PROCESS_NORMAL;
				if (client_menu->position().is_inside(e->root_x, e->root_y)) {
					int selx = (int) floor(
							(e->root_y - client_menu->position().y) / 24.0);
					client_menu->set_selected(selx);

					int selected = client_menu->get_selected();
					printf("Change desktop %d for %u\n", selected, mode_data_notebook_client_menu.client->orig());
					if(selected != _current_desktop) {
						detach(mode_data_notebook_client_menu.client);
						mode_data_notebook_client_menu.client->set_parent(nullptr);
						_desktop_list[selected]->get_default_pop()->add_client(mode_data_notebook_client_menu.client, false);
						mode_data_notebook_client_menu.client->set_current_desktop(selected);
						rpage->add_damaged(_root_position);
					}
				}
				mode_data_notebook_client_menu.reset();
				rnd->add_damaged(client_menu->get_visible_region());
				client_menu.reset();
			}
		}
		break;
	default:
		if (e->detail == XCB_BUTTON_INDEX_1) {
			process_mode = PROCESS_NORMAL;
		}
		break;
	}
}

void page_t::process_event(xcb_motion_notify_event_t const * e) {

	if(_last_focus_time > e->time) {
		_last_focus_time = e->time;
	}

	i_rect old_area;
	i_rect new_position;
	static int count = 0;
	count++;
	switch (process_mode) {
	case PROCESS_NORMAL:
		fprintf(stderr, "Warning: This case should not happen %s:%d\n", __FILE__, __LINE__);
		break;
	case PROCESS_SPLIT_GRAB:

		if (mode_data_split.split->get_split_type() == VERTICAL_SPLIT) {
			mode_data_split.split_ratio = (e->event_x
					- mode_data_split.split->allocation().x)
					/ (double) (mode_data_split.split->allocation().w);
		} else {
			mode_data_split.split_ratio = (e->event_y
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

		/* do not start drag&drop for small move */
		if (!mode_data_notebook.ev.position.is_inside(e->root_x, e->root_y)) {
			if(!pn0->is_visible())
				pn0->show();
		}

		++count;

		auto ln = filter_class<notebook_t>(_desktop_list[_current_desktop]->tree_t::get_all_children());
		for (auto i : ln) {
			if (i->tab_area.is_inside(e->root_x, e->root_y)) {
				if (mode_data_notebook.zone != SELECT_TAB
						|| mode_data_notebook.ns != i) {
					mode_data_notebook.zone = SELECT_TAB;
					mode_data_notebook.ns = i;
					update_popup_position(pn0, i->tab_area);
				}
				break;
			} else if (i->right_area.is_inside(e->root_x,
					e->root_y)) {
				if (mode_data_notebook.zone != SELECT_RIGHT
						|| mode_data_notebook.ns != i) {
					mode_data_notebook.zone = SELECT_RIGHT;
					mode_data_notebook.ns = i;
					update_popup_position(pn0, i->popup_right_area);
				}
				break;
			} else if (i->top_area.is_inside(e->root_x,
					e->root_y)) {
				if (mode_data_notebook.zone != SELECT_TOP
						|| mode_data_notebook.ns != i) {
					mode_data_notebook.zone = SELECT_TOP;
					mode_data_notebook.ns = i;
					update_popup_position(pn0, i->popup_top_area);
				}
				break;
			} else if (i->bottom_area.is_inside(e->root_x,
					e->root_y)) {
				if (mode_data_notebook.zone != SELECT_BOTTOM
						|| mode_data_notebook.ns != i) {
					mode_data_notebook.zone = SELECT_BOTTOM;
					mode_data_notebook.ns = i;
					update_popup_position(pn0, i->popup_bottom_area);
				}
				break;
			} else if (i->left_area.is_inside(e->root_x,
					e->root_y)) {
				if (mode_data_notebook.zone != SELECT_LEFT
						|| mode_data_notebook.ns != i) {
					mode_data_notebook.zone = SELECT_LEFT;
					mode_data_notebook.ns = i;
					update_popup_position(pn0, i->popup_left_area);
				}
				break;
			} else if (i->popup_center_area.is_inside(e->root_x,
					e->root_y)) {
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

		rnd->add_damaged(mode_data_floating.f->visible_area());

		/* compute new window position */
		i_rect new_position = mode_data_floating.original_position;
		new_position.x += e->root_x - mode_data_floating.x_root;
		new_position.y += e->root_y - mode_data_floating.y_root;
		mode_data_floating.final_position = new_position;
		mode_data_floating.f->set_floating_wished_position(new_position);
		mode_data_floating.f->reconfigure();

		_need_render = true;
		break;
	}
	case PROCESS_FLOATING_MOVE_BY_CLIENT: {

		rnd->add_damaged(mode_data_floating.f->visible_area());

		/* compute new window position */
		i_rect new_position = mode_data_floating.original_position;
		new_position.x += e->root_x - mode_data_floating.x_root;
		new_position.y += e->root_y - mode_data_floating.y_root;
		mode_data_floating.final_position = new_position;
		mode_data_floating.f->set_floating_wished_position(new_position);
		mode_data_floating.f->reconfigure();

		_need_render = true;
		break;
	}
	case PROCESS_FLOATING_RESIZE: {

		rnd->add_damaged(mode_data_floating.f->visible_area());

		i_rect size = mode_data_floating.original_position;

		if(mode_data_floating.mode == RESIZE_TOP_LEFT) {
			size.w -= e->root_x - mode_data_floating.x_root;
			size.h -= e->root_y - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_TOP) {
			size.h -= e->root_y - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_TOP_RIGHT) {
			size.w += e->root_x - mode_data_floating.x_root;
			size.h -= e->root_y - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_LEFT) {
			size.w -= e->root_x - mode_data_floating.x_root;
		} else if (mode_data_floating.mode == RESIZE_RIGHT) {
			size.w += e->root_x - mode_data_floating.x_root;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM_LEFT) {
			size.w -= e->root_x - mode_data_floating.x_root;
			size.h += e->root_y - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM) {
			size.h += e->root_y - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM_RIGHT) {
			size.w += e->root_x - mode_data_floating.x_root;
			size.h += e->root_y - mode_data_floating.y_root;
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

		rnd->add_damaged(mode_data_floating.f->visible_area());

		i_rect size = mode_data_floating.original_position;

		if(mode_data_floating.mode == RESIZE_TOP_LEFT) {
			size.w -= e->root_x - mode_data_floating.x_root;
			size.h -= e->root_y - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_TOP) {
			size.h -= e->root_y - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_TOP_RIGHT) {
			size.w += e->root_x - mode_data_floating.x_root;
			size.h -= e->root_y - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_LEFT) {
			size.w -= e->root_x - mode_data_floating.x_root;
		} else if (mode_data_floating.mode == RESIZE_RIGHT) {
			size.w += e->root_x - mode_data_floating.x_root;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM_LEFT) {
			size.w -= e->root_x - mode_data_floating.x_root;
			size.h += e->root_y - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM) {
			size.h += e->root_y - mode_data_floating.y_root;
		} else if (mode_data_floating.mode == RESIZE_BOTTOM_RIGHT) {
			size.w += e->root_x - mode_data_floating.x_root;
			size.h += e->root_y - mode_data_floating.y_root;
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

		/* do not start drag&drop for small move */
		if (e->root_x < mode_data_bind.start_x - 5
				|| e->root_x > mode_data_bind.start_x + 5
				|| e->root_y < mode_data_bind.start_y - 5
				|| e->root_y > mode_data_bind.start_y + 5) {

			if(!pn0->is_visible())
				pn0->show();
		}

		++count;

		auto ln = filter_class<notebook_t>(_desktop_list[_current_desktop]->tree_t::get_all_children());

		for(auto i : ln) {
			if (i->tab_area.is_inside(e->root_x,
					e->root_y)) {
				if (mode_data_bind.zone != SELECT_TAB
						|| mode_data_bind.ns != i) {
					mode_data_bind.zone = SELECT_TAB;
					mode_data_bind.ns = i;
					update_popup_position(pn0,
							i->tab_area);
				}
				break;
			} else if (i->right_area.is_inside(e->root_x,
					e->root_y)) {
				if (mode_data_bind.zone != SELECT_RIGHT
						|| mode_data_bind.ns != i) {
					mode_data_bind.zone = SELECT_RIGHT;
					mode_data_bind.ns = i;
					update_popup_position(pn0,
							i->popup_right_area);
				}
				break;
			} else if (i->top_area.is_inside(e->root_x,
					e->root_y)) {
				if (mode_data_bind.zone != SELECT_TOP
						|| mode_data_bind.ns != i) {
					mode_data_bind.zone = SELECT_TOP;
					mode_data_bind.ns = i;
					update_popup_position(pn0,
							i->popup_top_area);
				}
				break;
			} else if (i->bottom_area.is_inside(e->root_x,
					e->root_y)) {
				if (mode_data_bind.zone != SELECT_BOTTOM
						|| mode_data_bind.ns != i) {
					mode_data_bind.zone = SELECT_BOTTOM;
					mode_data_bind.ns = i;
					update_popup_position(pn0,
							i->popup_bottom_area);
				}
				break;
			} else if (i->left_area.is_inside(e->root_x,
					e->root_y)) {
				if (mode_data_bind.zone != SELECT_LEFT
						|| mode_data_bind.ns != i) {
					mode_data_bind.zone = SELECT_LEFT;
					mode_data_bind.ns = i;
					update_popup_position(pn0,
							i->popup_left_area);
				}
				break;
			} else if (i->popup_center_area.is_inside(e->root_x,
					e->root_y)) {
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

		viewport_t * v = find_mouse_viewport(e->root_x,
				e->root_y);

		if (v != nullptr) {
			if (v != mode_data_fullscreen.v) {
				pn0->move_resize(v->raw_area());
				mode_data_fullscreen.v = v;
			}
		}

		break;
	}
	case PROCESS_NOTEBOOK_MENU:
	{
		int selx = (int) floor((e->root_y - menu->position().y) / 24.0);
		menu->set_selected(selx);
		rnd->add_damaged(menu->get_visible_region());
	}
		break;
	case PROCESS_NOTEBOOK_CLIENT_MENU:
	{
		int selx = (int) floor((e->root_y - client_menu->position().y) / 24.0);
		client_menu->set_selected(selx);
		rnd->add_damaged(client_menu->get_visible_region());
	}
		break;
	default:
		fprintf(stderr, "Warning: Unknown process_mode\n");
		break;
	}
}

void page_t::process_event(xcb_configure_notify_event_t const * e) {
	client_base_t * c = find_client(e->window);

	if(c != nullptr) {
		/** damage corresponding area **/
		if(e->event == cnx->root()) {
			rnd->add_damaged(c->visible_area());
		}

		printf("configure %dx%d+%d+%d\n", e->width, e->height, e->x, e->y);
		c->process_event(e);

		/** damage corresponding area **/
		if(e->event == cnx->root()) {
			rnd->add_damaged(c->visible_area());
		}
	}

	//render();

}

/* track all created window */
void page_t::process_event(xcb_create_notify_event_t const * e) {
	std::cout << format("08", e->sequence) << "create_notify " << e->width << "x" << e->height << "+" << e->x << "+" << e->y
			<< " overide=" << (e->override_redirect?"true":"false")
			<< " boder_width=" << e->border_width << std::endl;
}

void page_t::process_event(xcb_destroy_notify_event_t const * e) {
	client_base_t * c = find_client(e->window);
	if (c != nullptr) {
		if(typeid(*c) == typeid(client_managed_t)) {
			cout << "WARNING: client destroyed a window without sending synthetic unmap" << endl;
			cout << "Sent Event: " << "false" << endl;
			client_managed_t * mw = dynamic_cast<client_managed_t *>(c);
			unmanage(mw);
		} else if(typeid(*c) == typeid(client_not_managed_t)) {
			cleanup_not_managed_client(dynamic_cast<client_not_managed_t *>(c));
		}
	}
}

void page_t::process_event(xcb_gravity_notify_event_t const * e) {
	/* Ignore it, never happen ? */
}

void page_t::process_event(xcb_map_notify_event_t const * e) {

	/* if map event does not occur within root, ignore it */
	if (e->event != cnx->root())
		return;

	onmap(e->window);

	//render();
}

void page_t::process_event(xcb_reparent_notify_event_t const * e) {
	//printf("Reparent window: %lu, parent: %lu, overide: %d, send_event: %d\n",
	//		e.window, e.parent, e.override_redirect, e.send_event);
	/* Reparent the root window ? hu :/ */
	if(e->window == cnx->root())
		return;

	/* If reparent occure on managed windows and new parent is an unknown window then unmanage */
	client_managed_t * mw = find_managed_window_with(e->window);
	if (mw != nullptr) {
		if (e->window == mw->orig() and e->parent != mw->base()) {
			/* unmanage the window */
			unmanage(mw);
		}
	}

	/* if a unmanaged window leave the root window for any reason, this client is forgoten */
	client_not_managed_t * uw = dynamic_cast<client_not_managed_t *>(find_client_with(e->window));
	if(uw != nullptr and e->parent != cnx->root()) {
		cleanup_not_managed_client(uw);
	}

}

void page_t::process_event(xcb_unmap_notify_event_t const * e) {
	client_base_t * c = find_client(e->window);
	if (c != nullptr) {
		rnd->add_damaged(c->visible_area());
		if(typeid(*c) == typeid(client_not_managed_t)) {
			cleanup_not_managed_client(dynamic_cast<client_not_managed_t *>(c));
			render();
		}
	}
}

void page_t::process_fake_event(xcb_unmap_notify_event_t const * e) {
	/**
	 * Client must send a fake unmap event if he want get back the window.
	 * (i.e. he want that we unmanage it.
	 **/

	/* if client is managed */
	client_base_t * c = find_client(e->window);

	if (c != nullptr) {
		rnd->add_damaged(c->visible_area());
		if(typeid(*c) == typeid(client_managed_t)) {
			client_managed_t * mw = dynamic_cast<client_managed_t *>(c);
			cnx->reparentwindow(mw->orig(), cnx->root(), 0.0, 0.0);
			unmanage(mw);
		}
		//render();
	}
}

void page_t::process_event(xcb_circulate_request_event_t const * e) {
	/* will happpen ? */
	client_base_t * c = find_client_with(e->window);
	if (c != nullptr) {
		if (e->place == XCB_PLACE_ON_TOP) {
			safe_raise_window(c);
		} else if (e->place == XCB_PLACE_ON_BOTTOM) {
			cnx->lower_window(e->window);
		}
	}
}

void page_t::process_event(xcb_configure_request_event_t const * e) {

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


	client_base_t * c = find_client(e->window);

	if (c != nullptr) {

		rnd->add_damaged(c->visible_area());

		if(typeid(*c) == typeid(client_managed_t)) {

			client_managed_t * mw = dynamic_cast<client_managed_t *>(c);

			if (mw->lock()) {

				if ((e->value_mask & (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT)) != 0) {

					i_rect old_size = mw->get_floating_wished_position();
					/** compute floating size **/
					i_rect new_size = mw->get_floating_wished_position();

					if (e->value_mask & XCB_CONFIG_WINDOW_X) {
						new_size.x = e->x;
					}

					if (e->value_mask & XCB_CONFIG_WINDOW_Y) {
						new_size.y = e->y;
					}

					if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
						new_size.w = e->width;
					}

					if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
						new_size.h = e->height;
					}

					//printf("new_size = %s\n", new_size.to_std::string().c_str());

//					if ((e.value_mask & (CWX)) and (e.value_mask & (CWY))
//							and e.x == 0 and e.y == 0
//							and !viewport_outputs.empty()) {
//						viewport_t * v = viewport_outputs.begin()->second;
//						i_rect b = v->raw_area();
//						/* place on center */
//						new_size.x = (b.w - new_size.w) / 2 + b.x;
//						new_size.y = (b.h - new_size.h) / 2 + b.y;
//					}

					unsigned int final_width = new_size.w;
					unsigned int final_height = new_size.h;

					compute_client_size_with_constraint(mw->orig(),
							(unsigned) new_size.w, (unsigned) new_size.h,
							final_width, final_height);

					new_size.w = final_width;
					new_size.h = final_height;

					printf("new_size = %s\n", new_size.to_string().c_str());

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

void page_t::ackwoledge_configure_request(xcb_configure_request_event_t const * e) {
	printf("ackwoledge_configure_request ");

	int i = 0;
	uint32_t value[7] = {0};
	uint32_t mask = 0;
	if(e->value_mask & XCB_CONFIG_WINDOW_X) {
		mask |= XCB_CONFIG_WINDOW_X;
		value[i++] = e->x;
		printf("x = %d ", e->x);
	}

	if(e->value_mask & XCB_CONFIG_WINDOW_Y) {
		mask |= XCB_CONFIG_WINDOW_Y;
		value[i++] = e->y;
		printf("y = %d ", e->y);
	}

	if(e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
		mask |= XCB_CONFIG_WINDOW_WIDTH;
		value[i++] = e->width;
		printf("w = %d ", e->width);
	}

	if(e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
		mask |= XCB_CONFIG_WINDOW_HEIGHT;
		value[i++] = e->height;
		printf("h = %d ", e->height);
	}

	if(e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
		mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
		value[i++] = e->border_width;
		printf("border = %d ", e->border_width);
	}

	if(e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
		mask |= XCB_CONFIG_WINDOW_SIBLING;
		value[i++] = e->sibling;
		printf("sibling = %d ", e->sibling);
	}

	if(e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
		mask |= XCB_CONFIG_WINDOW_STACK_MODE;
		value[i++] = e->stack_mode;
		printf("stack_mode = %d ", e->stack_mode);
	}

	printf("\n");

	xcb_void_cookie_t ck = xcb_configure_window(cnx->xcb(), e->window, mask, value);

}

void page_t::process_event(xcb_map_request_event_t const * e) {

	if (e->parent != cnx->root()) {
		xcb_map_window(cnx->xcb(), e->window);
		return;
	}

	onmap(e->window);

}

void page_t::process_event(xcb_property_notify_event_t const * e) {

	if(e->window == cnx->root())
		return;

	client_base_t * c = find_client(e->window);

	if(c != nullptr) {
		if(e->atom == A(WM_NAME)) {
			c->update_wm_name();
		} else if (e->atom == A(WM_ICON_NAME)) {
			c->update_wm_icon_name();
		} else if (e->atom == A(WM_NORMAL_HINTS)) {
			c->update_wm_normal_hints();
		} else if (e->atom == A(WM_HINTS)) {
			c->update_wm_hints();
		} else if (e->atom == A(WM_CLASS)) {
			c->update_wm_class();
		} else if (e->atom == A(WM_TRANSIENT_FOR)) {
			c->update_wm_transient_for();
		} else if (e->atom == A(WM_PROTOCOLS)) {
			c->update_wm_protocols();
		} else if (e->atom == A(WM_COLORMAP_WINDOWS)) {
			c->update_wm_colormap_windows();
		} else if (e->atom == A(WM_CLIENT_MACHINE)) {
			c->update_wm_client_machine();
		} else if (e->atom == A(WM_STATE)) {
			c->update_wm_state();
		} else if (e->atom == A(_NET_WM_NAME)) {
			c->update_net_wm_name();
		} else if (e->atom == A(_NET_WM_VISIBLE_NAME)) {
			c->update_net_wm_visible_name();
		} else if (e->atom == A(_NET_WM_ICON_NAME)) {
			c->update_net_wm_icon_name();
		} else if (e->atom == A(_NET_WM_VISIBLE_ICON_NAME)) {
			c->update_net_wm_visible_icon_name();
		} else if (e->atom == A(_NET_WM_DESKTOP)) {
			c->update_net_wm_desktop();
		} else if (e->atom == A(_NET_WM_WINDOW_TYPE)) {
			c->update_net_wm_window_type();
		} else if (e->atom == A(_NET_WM_STATE)) {
			c->update_net_wm_state();
		} else if (e->atom == A(_NET_WM_ALLOWED_ACTIONS)) {
			c->update_net_wm_allowed_actions();
		} else if (e->atom == A(_NET_WM_STRUT)) {
			c->update_net_wm_struct();
		} else if (e->atom == A(_NET_WM_STRUT_PARTIAL)) {
			c->update_net_wm_struct_partial();
		} else if (e->atom == A(_NET_WM_ICON_GEOMETRY)) {
			c->update_net_wm_icon_geometry();
		} else if (e->atom == A(_NET_WM_ICON)) {
			c->update_net_wm_icon();
		} else if (e->atom == A(_NET_WM_PID)) {
			c->update_net_wm_pid();
		} else if (e->atom == A(_NET_WM_USER_TIME)) {
			c->update_net_wm_user_time();
		} else if (e->atom == A(_NET_WM_USER_TIME_WINDOW)) {
			c->update_net_wm_user_time_window();
		} else if (e->atom == A(_NET_FRAME_EXTENTS)) {
			c->update_net_frame_extents();
		} else if (e->atom == A(_NET_WM_OPAQUE_REGION)) {
			c->update_net_wm_opaque_region();
		} else if (e->atom == A(_NET_WM_BYPASS_COMPOSITOR)) {
			c->update_net_wm_bypass_compositor();
		}  else if (e->atom == A(_MOTIF_WM_HINTS)) {
			c->update_motif_hints();
		}

	}

	xcb_window_t x = e->window;
	client_managed_t * mw = find_managed_window_with(e->window);

	if (e->atom == A(_NET_WM_USER_TIME)) {

	} else if (e->atom == A(_NET_WM_NAME) || e->atom == A(WM_NAME)) {


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

	} else if (e->atom == A(_NET_WM_STRUT_PARTIAL)) {
		update_allocation();
		rpage->mark_durty();
	} else if (e->atom == A(_NET_WM_STRUT)) {
		//x->mark_durty_net_wm_partial_struct();
		//rpage->mark_durty();
	} else if (e->atom == A(_NET_WM_ICON)) {
		if (mw != 0)
			mw->update_icon();
	} else if (e->atom == A(_NET_WM_WINDOW_TYPE)) {
		/* window type must be set on map, I guess it should never change ? */
		/* update cache */

		//window_t::page_window_type_e old = x->get_window_type();
		//x->read_transient_for();
		//x->find_window_type();
		/* I do not see something in ICCCM */
		//if(x->get_window_type() == window_t::PAGE_NORMAL_WINDOW_TYPE && old != window_t::PAGE_NORMAL_WINDOW_TYPE) {
		//	manage_notebook(x);
		//}
	} else if (e->atom == A(WM_NORMAL_HINTS)) {
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
	} else if (e->atom == A(WM_PROTOCOLS)) {
	} else if (e->atom == A(WM_TRANSIENT_FOR)) {
		if (c != 0) {
			safe_update_transient_for(c);
			update_windows_stack();
		}
	} else if (e->atom == A(WM_HINTS)) {
	} else if (e->atom == A(_NET_WM_STATE)) {
		/* this event are generated by page */
		/* change of net_wm_state must be requested by client message */
	} else if (e->atom == A(WM_STATE)) {
		/** this is set by page ... don't read it **/
	} else if (e->atom == A(_NET_WM_DESKTOP)) {
		/* this set by page in most case */
		//x->read_net_wm_desktop();
	}
}

void page_t::process_event(xcb_client_message_event_t const * e) {

	//std::shared_ptr<char> name = cnx->get_atom_name(e->type);
	std::cout << "ClientMessage type = " << cnx->get_atom_name(e->type) << std::endl;

	xcb_window_t w = e->window;
	if (w == XCB_NONE)
		return;

	client_managed_t * mw = find_managed_window_with(e->window);

	if (e->type == A(_NET_ACTIVE_WINDOW)) {
		if (mw != nullptr) {
			if (mw->current_desktop() == _current_desktop) {
				if (mw->lock()) {
					if (e->data.data32[1] == XCB_CURRENT_TIME) {
						printf(
								"Invalid focus request ... but stealing focus\n");
						set_focus(mw, XCB_CURRENT_TIME);
					} else {
						set_focus(mw, e->data.data32[1]);
					}
					mw->unlock();
				}
			}
		}
	} else if (e->type == A(_NET_WM_STATE)) {

		/* process first request */
		process_net_vm_state_client_message(w, e->data.data32[0], e->data.data32[1]);
		/* process second request */
		process_net_vm_state_client_message(w, e->data.data32[0], e->data.data32[2]);

		for (int i = 1; i < 3; ++i) {
			if (std::find(supported_list.begin(), supported_list.end(),
					e->data.data32[i]) != supported_list.end()) {
				switch (e->data.data32[0]) {
				case _NET_WM_STATE_REMOVE:
					//w->unset_net_wm_state(e->data.l[i]);
					break;
				case _NET_WM_STATE_ADD:
					//w->set_net_wm_state(e->data.l[i]);
					break;
				case _NET_WM_STATE_TOGGLE:
					//w->toggle_net_wm_state(e->data.l[i]);
					break;
				}
			}
		}
	} else if (e->type == A(WM_CHANGE_STATE)) {

		/** When window want to become iconic, just bind them **/
		if (mw != nullptr) {
			if (mw->lock()) {
				if (mw->is(MANAGED_FLOATING) and e->data.data32[0] == IconicState) {
					bind_window(mw, false);
				} else if (mw->is(
						MANAGED_NOTEBOOK) and e->data.data32[0] == IconicState) {
					notebook_t * n = dynamic_cast<notebook_t *>(mw->parent());
					n->iconify_client(mw);
					rpage->add_damaged(n->allocation());
					rnd->need_render();
				}
				mw->unlock();
			}
		}

	} else if (e->type == A(PAGE_QUIT)) {
		running = false;
	} else if (e->type == A(WM_PROTOCOLS)) {

	} else if (e->type == A(_NET_CLOSE_WINDOW)) {
		if(mw != nullptr) {
			mw->delete_window(e->data.data32[0]);
		}
	} else if (e->type == A(_NET_REQUEST_FRAME_EXTENTS)) {

	} else if (e->type == A(_NET_WM_MOVERESIZE)) {
		if (mw != nullptr) {
			if (mw->is(MANAGED_FLOATING) and process_mode == PROCESS_NORMAL) {

				if (mw->lock()) {
					xcb_cursor_t xc = XCB_NONE;

					long x_root = e->data.data32[0];
					long y_root = e->data.data32[1];
					long direction = e->data.data32[2];
					long button = e->data.data32[3];
					long source = e->data.data32[4];

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
						xc = cnx->xc_fleur;
					} else {

						if (direction == _NET_WM_MOVERESIZE_SIZE_TOP) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_TOP;
							xc = cnx->xc_top_side;
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_BOTTOM) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_BOTTOM;
							xc = cnx->xc_bottom_side;
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_LEFT) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_LEFT;
							xc = cnx->xc_left_side;
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_RIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_RIGHT;
							xc = cnx->xc_right_side;
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_TOPLEFT) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_TOP_LEFT;
							xc = cnx->xc_top_left_corner;
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_TOPRIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_TOP_RIGHT;
							xc = cnx->xc_top_right_corner;
						} else if (direction
								== _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_BOTTOM_LEFT;
							xc = cnx->xc_bottom_left_corner;
						} else if (direction
								== _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT) {
							process_mode = PROCESS_FLOATING_RESIZE_BY_CLIENT;
							mode_data_floating.mode = RESIZE_BOTTOM_RIGHT;
							xc = cnx->xc_bottom_righ_corner;
						} else {
							safe_raise_window(mw);
							process_mode = PROCESS_FLOATING_MOVE_BY_CLIENT;
							xc = cnx->xc_fleur;
						}
					}

					if (process_mode != PROCESS_NORMAL) {
						xcb_grab_pointer(cnx->xcb(), false, cnx->root(),
								XCB_EVENT_MASK_BUTTON_PRESS
										| XCB_EVENT_MASK_BUTTON_RELEASE
										| XCB_EVENT_MASK_BUTTON_MOTION,
								XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
								XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
					}

					mw->unlock();

				}
			}
		}
	} else if (e->type == A(_NET_CURRENT_DESKTOP)) {
		if(e->data.data32[0] >= 0 and e->data.data32[0] < _desktop_list.size() and e->data.data32[0] != _current_desktop) {
			switch_to_desktop(e->data.data32[0], XCB_CURRENT_TIME);
		}
	}
}

void page_t::process_event(xcb_damage_notify_event_t const * e) {
	render();
}

void page_t::render() {
	/**
	 * Try to render if any damage event is encountered. But limit general
	 * rendering to 60 fps.
	 **/
	time_t cur_tic;
	cur_tic.get_time();
	rnd->clear_renderable();
	std::vector<std::shared_ptr<renderable_t>> ret;
	prepare_render(ret, cur_tic);
	rnd->push_back_renderable(ret);
	rnd->render();

	/** sync with X server to ensure all render are done **/
	cnx->sync();

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
		assert(data.revert_notebook != nullptr);
		data.revert_type = MANAGED_NOTEBOOK;
		if (v == nullptr) {
			data.viewport = find_viewport_of(data.revert_notebook);
		} else {
			data.viewport = v;
		}
		assert(data.viewport != nullptr);
		detach(mw);
	} else if (mw->is(MANAGED_FLOATING)) {
		detach(mw);
		data.revert_type = MANAGED_FLOATING;
		data.revert_notebook = nullptr;
		if (v == nullptr) {
			// TODO: find a free viewport
			data.viewport = _desktop_list[0]->get_any_viewport();
		} else {
			data.viewport = v;
		}
	}

	// unfullscreen client that already use this screen
	for (auto &x : _fullscreen_client_to_viewport) {
		if (x.second.viewport == mode_data_fullscreen.v) {
			unfullscreen(x.first);
			break;
		}
	}

	data.desktop = find_desktop_of(data.viewport);
	data.viewport->hide();
	mw->set_managed_type(MANAGED_FULLSCREEN);
	data.desktop->add_fullscreen_client(mw);
	_fullscreen_client_to_viewport[mw] = data;
	mw->set_notebook_wished_position(data.viewport->raw_area());
	mw->reconfigure();
	mw->normalize();
	update_windows_stack();
}

void page_t::unfullscreen(client_managed_t * mw) {
	/* WARNING: Call order is important, change it with caution */
	mw->net_wm_state_remove(_NET_WM_STATE_FULLSCREEN);
	if(!has_key(_fullscreen_client_to_viewport, mw))
		return;

	detach(mw);

	fullscreen_data_t data = _fullscreen_client_to_viewport[mw];
	_fullscreen_client_to_viewport.erase(mw);

	desktop_t * d = data.desktop;
	viewport_t * v = data.viewport;

	if (data.revert_type == MANAGED_NOTEBOOK) {
		notebook_t * old = data.revert_notebook;
		mw->set_managed_type(MANAGED_NOTEBOOK);
		old->add_client(mw, true);
		mw->reconfigure();
		rpage->add_damaged(old->allocation());
	} else {
		mw->set_managed_type(MANAGED_FLOATING);
		insert_in_tree_using_transient_for(mw);
		mw->reconfigure();
	}

	if(not d->is_hidden()) {
		v->show();
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


void page_t::process_event(xcb_generic_event_t const * e) {

	/**
	 * This print produce damage notify, thus avoid to print this line in that case
	 **/
	if(e->response_type != cnx->damage_event + XCB_DAMAGE_NOTIFY) {
		std::cout << format("08", e->sequence) << " process event: " << cnx->event_type_name[(e->response_type&(~0x80))] << (e->response_type&(0x80)?" (fake)":"") << std::endl;
	}

	if (e->response_type == XCB_BUTTON_PRESS) {
		process_event_press(reinterpret_cast<xcb_button_press_event_t const *>(e));
	} else if (e->response_type == XCB_BUTTON_RELEASE) {
		process_event_release(reinterpret_cast<xcb_button_release_event_t const *>(e));
	} else if (e->response_type == XCB_MOTION_NOTIFY) {
		process_event(reinterpret_cast<xcb_motion_notify_event_t const *>(e));
	} else if (e->response_type == XCB_KEY_PRESS) {
		process_event(reinterpret_cast<xcb_key_press_event_t const *>(e));
	} else if (e->response_type == XCB_KEY_RELEASE) {
		process_event(reinterpret_cast<xcb_key_release_event_t const *>(e));
	} else if (e->response_type == XCB_CONFIGURE_NOTIFY) {
		process_event(reinterpret_cast<xcb_configure_notify_event_t const *>(e));
	} else if (e->response_type == XCB_CREATE_NOTIFY) {
		process_event(reinterpret_cast<xcb_create_notify_event_t const *>(e));
	} else if (e->response_type == XCB_DESTROY_NOTIFY) {
		process_event(reinterpret_cast<xcb_destroy_notify_event_t const *>(e));
	} else if (e->response_type == XCB_GRAVITY_NOTIFY) {
		process_event(reinterpret_cast<xcb_gravity_notify_event_t const *>(e));
	} else if (e->response_type == XCB_MAP_NOTIFY) {
		process_event(reinterpret_cast<xcb_map_notify_event_t const *>(e));
	} else if (e->response_type == XCB_REPARENT_NOTIFY) {
		process_event(reinterpret_cast<xcb_reparent_notify_event_t const *>(e));
	} else if (e->response_type == XCB_UNMAP_NOTIFY) {
		process_event(reinterpret_cast<xcb_unmap_notify_event_t const *>(e));
	} else if (e->response_type == (XCB_UNMAP_NOTIFY|0x80)) {
		process_fake_event(reinterpret_cast<xcb_unmap_notify_event_t const *>(e));
	} else if (e->response_type == XCB_CIRCULATE_NOTIFY) {

	} else if (e->response_type == XCB_CONFIGURE_REQUEST) {
		process_event(reinterpret_cast<xcb_configure_request_event_t const *>(e));
	} else if (e->response_type == XCB_MAP_REQUEST) {
		process_event(reinterpret_cast<xcb_map_request_event_t const *>(e));
	} else if (e->response_type == XCB_PROPERTY_NOTIFY) {
		process_event(reinterpret_cast<xcb_property_notify_event_t const *>(e));
	} else if (e->response_type == (XCB_CLIENT_MESSAGE|0x80)) {
		process_event(reinterpret_cast<xcb_client_message_event_t const *>(e));
	} else if (e->response_type == cnx->damage_event + XCB_DAMAGE_NOTIFY) {
		process_event(reinterpret_cast<xcb_damage_notify_event_t const *>(e));
	} else if (e->response_type == Expose) {
		xcb_expose_event_t const * ev = reinterpret_cast<xcb_expose_event_t const *>(e);
		client_managed_t * mw = find_managed_window_with(ev->window);
		if (mw != nullptr) {
			if (mw->is(MANAGED_FLOATING)) {
				mw->expose();
				//rnd->add_damaged(mw->base_position());
			}
		}

	} else if (e->response_type == cnx->randr_event + RRNotify) {
		xcb_randr_notify_event_t const * ev = reinterpret_cast<xcb_randr_notify_event_t const *>(e);

		char const * s_subtype = "Unknown";

		switch(ev->subCode) {
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

		if (ev->subCode == RRNotify_CrtcChange) {
			update_viewport_layout();
			update_allocation();

			theme->update();

			delete rpage;
			rpage = new renderable_page_t(cnx, theme, _root_position.w,
					_root_position.h);
			rpage->show();

		}

		xcb_grab_button(cnx->xcb(), false, cnx->root(),
				XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
						| XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_SYNC,
				XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY,
				XCB_MOD_MASK_ANY);

		/** put rpage behind all managed windows **/
		update_windows_stack();


	} else if (e->response_type == cnx->randr_event + RRScreenChangeNotify) {
		/** do nothing **/
	} else if (e->response_type == XCB_SELECTION_CLEAR) {
		/** some one want to replace our self **/
		running = false;
	} else if (e->response_type == cnx->shape_event + ShapeNotify) {
		xcb_shape_notify_event_t const * se = reinterpret_cast<xcb_shape_notify_event_t const *>(e);
		if(se->shape_kind == XCB_SHAPE_SK_BOUNDING) {
			xcb_window_t w = se->affected_window;
			client_base_t * c = find_client(w);
			if(c != nullptr) {
				c->update_shape();
			}
		}
	} else if (e->response_type == XCB_FOCUS_OUT) {
		//printf("FocusOut #%lu\n", e.xfocus.window);
	} else if (e->response_type == XCB_FOCUS_IN) {
		//printf("FocusIn #%lu\n", e.xfocus.window);
	} else if (e->response_type == XCB_MAPPING_NOTIFY) {
		update_keymap();
		update_grabkey();
	} else if (e->response_type == 0) {
		xcb_generic_error_t const * ev = reinterpret_cast<xcb_generic_error_t const *>(e);
		cnx->print_error(ev);
	} else {
		std::cout << "not handled event: " << cnx->event_type_name[(e->response_type&(~0x80))] << (e->response_type&(0x80)?" (fake)":"") << std::endl;
	}

}

void page_t::insert_window_in_notebook(client_managed_t * x, notebook_t * n,
		bool prefer_activate) {
	assert(x != nullptr);
	if (n == nullptr)
		n = _desktop_list[_current_desktop]->get_default_pop();
	assert(n != nullptr);
	x->set_managed_type(MANAGED_NOTEBOOK);
	n->add_client(x, prefer_activate);
	rpage->add_damaged(n->allocation());

}

/* update viewport and childs allocation */
void page_t::update_allocation() {
	for(auto v: filter_class<viewport_t>(tree_t::get_all_children())) {
		compute_viewport_allocation(*v);
	}
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
		cnx->set_input_focus(cnx->root(), XCB_INPUT_FOCUS_PARENT, _last_focus_time);
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
	cnx->set_net_active_window(new_focus->orig());

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

void page_t::notebook_close(notebook_t * nbk) {
	/**
	 * Closing notebook mean destroying the split base of this
	 * notebook, plus this notebook.
	 **/

	split_t * splt = dynamic_cast<split_t *>(nbk->parent());

	/* if parent is viewport then we cannot close current notebook */
	if(splt == nullptr)
		return;

	assert(nbk == splt->get_pack0() or nbk == splt->get_pack1());

	/* find the sibling branch of note that we want close */
	page_component_t * dst = (nbk == splt->get_pack0()) ? splt->get_pack1() : splt->get_pack0();

	/* add slit area a damaged */
	rpage->add_damaged(splt->allocation());
	/* remove this split from tree  and replace it by sibling branch */
	splt->parent()->replace(splt, dst);

	/**
	 * if notebook that we want destroy was the default_pop, select
	 * a new one.
	 **/
	if (_desktop_list[_current_desktop]->get_default_pop() == nbk) {
		_desktop_list[_current_desktop]->update_default_pop();
		/* damage the new default pop to show the notebook mark properly */
		rpage->add_damaged(_desktop_list[_current_desktop]->default_pop()->allocation());
	}

	/* move all client from destroyed notebook to new default pop */
	auto clients = nbk->get_clients();
	for(auto i : clients) {
		insert_window_in_notebook(i, nullptr, false);
	}

	/**
	 * if a fullscreen client want revert to this notebook,
	 * change it to default_window_pop
	 **/
	for (auto & i : _fullscreen_client_to_viewport) {
		if (i.second.revert_notebook == nbk) {
			i.second.revert_notebook = _desktop_list[_current_desktop]->default_pop();
		}
	}

	/* cleanup */
	delete nbk;
	delete splt;

}

void page_t::update_popup_position(std::shared_ptr<popup_notebook0_t> p,
		i_rect & position) {
	rnd->add_damaged(p->position());
	p->move_resize(position);
	rnd->add_damaged(p->position());
}

void page_t::update_popup_position(std::shared_ptr<popup_frame_move_t> p,
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

	i_rect const raw_area = v.raw_area();
	int xtop = raw_area.y;
	int xleft = raw_area.x;
	int xright = _root_position.w - raw_area.x - raw_area.w;
	int xbottom = _root_position.h - raw_area.y - raw_area.h;


	for(auto j : filter_class<client_base_t>(tree_t::get_all_children())) {
		if (j->net_wm_struct_partial() != nullptr) {
			auto const & ps = *(j->net_wm_struct_partial());

			if (ps[PS_LEFT] > 0) {
				/* check if raw area intersect current viewport */
				i_rect b(0, ps[PS_LEFT_START_Y], ps[PS_LEFT],
						ps[PS_LEFT_END_Y] - ps[PS_LEFT_START_Y] + 1);
				i_rect x = raw_area & b;
				if (!x.is_null()) {
					xleft = std::max(xleft, ps[PS_LEFT]);
				}
			}

			if (ps[PS_RIGHT] > 0) {
				/* check if raw area intersect current viewport */
				i_rect b(_root_position.w - ps[PS_RIGHT],
						ps[PS_RIGHT_START_Y], ps[PS_RIGHT],
						ps[PS_RIGHT_END_Y] - ps[PS_RIGHT_START_Y] + 1);
				i_rect x = raw_area & b;
				if (!x.is_null()) {
					xright = std::max(xright, ps[PS_RIGHT]);
				}
			}

			if (ps[PS_TOP] > 0) {
				/* check if raw area intersect current viewport */
				i_rect b(ps[PS_TOP_START_X], 0,
						ps[PS_TOP_END_X] - ps[PS_TOP_START_X] + 1, ps[PS_TOP]);
				i_rect x = raw_area & b;
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
				i_rect x = raw_area & b;
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

void page_t::process_net_vm_state_client_message(xcb_window_t c, long type, xcb_atom_t state_properties) {
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
				update_desktop_visibility();
				break;
			case _NET_WM_STATE_TOGGLE:
				toggle_fullscreen(mw);
				update_desktop_visibility();
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
				update_desktop_visibility();
				break;
			case _NET_WM_STATE_TOGGLE:
				toggle_fullscreen(mw);
				update_desktop_visibility();
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
				update_desktop_visibility();
				break;
			case _NET_WM_STATE_ADD:
				break;
			case _NET_WM_STATE_TOGGLE:
				toggle_fullscreen(mw);
				update_desktop_visibility();
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

void page_t::insert_in_tree_using_transient_for(client_managed_t * c) {
	detach(c);
	client_base_t * transient_for = get_transient_for(c);
	if (transient_for != nullptr) {
		transient_for->add_subclient(c);
	} else {
		_desktop_list[c->current_desktop()]->add_floating_client(c);
		if(_desktop_list[c->current_desktop()]->is_hidden()) {
			c->hide();
		} else {
			c->show();
		}
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
	for(auto i : tree_t::get_all_children()) {
		i->remove(t);
	}
	t->set_parent(nullptr);
}

void page_t::safe_raise_window(client_base_t * c) {
	//if(process_mode != PROCESS_NORMAL)
	//	return;
	c->raise_child();
	/** apply change **/
	update_windows_stack();
}

void page_t::compute_client_size_with_constraint(xcb_window_t c,
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

	page::compute_client_size_with_constraint(*(_c->wm_normal_hints()),
			wished_width, wished_height, width, height);

}

void page_t::bind_window(client_managed_t * mw, bool activate) {
	/* update database */
	cout << "bind: " << mw->title() << endl;
	detach(mw);
	insert_window_in_notebook(mw, nullptr, activate);
	safe_raise_window(mw);
	update_client_list();
}

void page_t::unbind_window(client_managed_t * mw) {
	detach(mw);
	/* update database */
	mw->set_managed_type(MANAGED_FLOATING);
	insert_in_tree_using_transient_for(mw);
	mw->expose();
	mw->normalize();
	safe_raise_window(mw);
	update_client_list();
	update_windows_stack();
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
		rnd->add_damaged(menu->get_visible_region());
		menu.reset();
		process_mode = PROCESS_NORMAL;
		mode_data_notebook_menu.from = nullptr;
		break;
	case PROCESS_NOTEBOOK_CLIENT_MENU:
		if (mode_data_notebook_client_menu.client == mw) {
			if (mode_data_notebook_client_menu.active_grab) {
				xcb_ungrab_pointer(cnx->xcb(), XCB_CURRENT_TIME);
			}
			rnd->add_damaged(client_menu->get_visible_region());
			client_menu.reset();
			process_mode = PROCESS_NORMAL;
			mode_data_notebook_client_menu.reset();
		}
		break;
	}
}

/* look for a notebook in tree base, that is deferent from nbk */
notebook_t * page_t::get_another_notebook(tree_t * base, tree_t * nbk) {
	std::vector<notebook_t *> l;

	if (base == nullptr) {
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

std::vector<notebook_t *> page_t::get_notebooks(tree_t * base) {
	if(base == nullptr)
		base = this;
	return filter_class<notebook_t>(base->get_all_children());
}

notebook_t * page_t::find_parent_notebook_for(client_managed_t * mw) {
	return dynamic_cast<notebook_t*>(mw->parent());
}

viewport_t * page_t::find_viewport_of(tree_t * n) {
	tree_t * x = n;
	while (x != nullptr) {
		if (typeid(*x) == typeid(viewport_t))
			return dynamic_cast<viewport_t *>(x);
		x = (x->parent());
	}
	return nullptr;
}

desktop_t * page_t::find_desktop_of(tree_t * n) {
	tree_t * x = n;
	while (x != nullptr) {
		if (typeid(*x) == typeid(desktop_t))
			return dynamic_cast<desktop_t *>(x);
		x = (x->parent());
	}
	return nullptr;
}

void page_t::set_window_cursor(xcb_window_t w, xcb_cursor_t c) {
	uint32_t attrs = c;
	xcb_change_window_attributes(cnx->xcb(), w, XCB_CW_CURSOR, &attrs);
}

void page_t::update_windows_stack() {
	std::vector<client_base_t *> clients =
			filter_class<client_base_t>(tree_t::get_visible_children());
	for(auto i: clients) {
		cnx->raise_window(i->base());
	}
	cnx->raise_window(rnd->get_composite_overlay());
}

void page_t::update_viewport_layout() {

	/** update root size infos **/

	xcb_get_geometry_cookie_t ck0 = xcb_get_geometry(cnx->xcb(), cnx->root());
	xcb_randr_get_screen_resources_cookie_t ck1 = xcb_randr_get_screen_resources(cnx->xcb(), cnx->root());

	xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(cnx->xcb(), ck0, nullptr);
	xcb_randr_get_screen_resources_reply_t * randr_resources = xcb_randr_get_screen_resources_reply(cnx->xcb(), ck1, 0);

	if(geometry == nullptr or randr_resources == nullptr) {
		throw exception_t("FATAL: cannot read root window attributes");
	}


	_root_position = i_rect{geometry->x, geometry->y, geometry->width, geometry->height};
	set_desktop_geometry(_root_position.w, _root_position.h);
	_allocation = _root_position;

	std::map<xcb_randr_crtc_t, xcb_randr_get_crtc_info_reply_t *> crtc_info;

	std::vector<xcb_randr_get_crtc_info_cookie_t> ckx(xcb_randr_get_screen_resources_crtcs_length(randr_resources));
	xcb_randr_crtc_t * crtc_list = xcb_randr_get_screen_resources_crtcs(randr_resources);
	for (unsigned k = 0; k < xcb_randr_get_screen_resources_crtcs_length(randr_resources); ++k) {
		ckx[k] = xcb_randr_get_crtc_info(cnx->xcb(), crtc_list[k], XCB_CURRENT_TIME);
	}

	for (unsigned k = 0; k < xcb_randr_get_screen_resources_crtcs_length(randr_resources); ++k) {
		xcb_randr_get_crtc_info_reply_t * r = xcb_randr_get_crtc_info_reply(cnx->xcb(), ckx[k], 0);
		if(r != nullptr) {
			crtc_info[crtc_list[k]] = r;
		}
	}

	for(auto d: _desktop_list) {
		std::map<xcb_randr_crtc_t, viewport_t *> old_layout{d->get_viewport_map()};

		/** store the newer layout, to be able to cleanup obsolete viewports **/
		std::map<xcb_randr_crtc_t, viewport_t *> new_layout;
		for(auto crtc: crtc_info) {
			xcb_randr_get_crtc_info_reply_t * info = crtc.second;
			if(info->num_outputs <= 0)
				continue;

			i_rect area{info->x, info->y, info->width, info->height};
			/** if this crtc do not has a viewport **/
			if (not has_key(old_layout, crtc.first)) {
				/** then create a new one, and store it in new_layout **/
				viewport_t * v = new viewport_t(theme, area);
				v->set_parent(d);
				new_layout[crtc.first] = v;
			} else {
				/** update allocation, and store it to new_layout **/
				old_layout[crtc.first]->set_raw_area(area);
				new_layout[crtc.first] = old_layout[crtc.first];
			}
			compute_viewport_allocation(*(new_layout[crtc.first]));
		}

		if(new_layout.size() < 1) {
			/** fallback to one screen **/
			i_rect area{_root_position};
			/** if this crtc do not has a viewport **/
			if (not has_key<xcb_randr_crtc_t>(old_layout, XCB_NONE)) {
				/** then create a new one, and store it in new_layout **/
				viewport_t * v = new viewport_t(theme, area);
				v->set_parent(d);
				new_layout[XCB_NONE] = v;
			} else {
				/** update allocation, and store it to new_layout **/
				old_layout[XCB_NONE]->set_raw_area(area);
				new_layout[XCB_NONE] = old_layout[XCB_NONE];
			}
			compute_viewport_allocation(*(new_layout[XCB_NONE]));
		}

		d->set_layout(new_layout);
		d->update_default_pop();

		/** clean up obsolete layout **/
		for (auto i: old_layout) {
			if (not has_key(new_layout, i.first)) {
				/** destroy this viewport **/
				remove_viewport(d, i.second);
				destroy_viewport(i.second);
			}
		}

	}

	for(auto i: crtc_info) {
		if(i.second != nullptr)
			free(i.second);
	}

	if(geometry != nullptr) {
		free(geometry);
	}

	if(randr_resources != nullptr) {
		free(randr_resources);
	}

	update_desktop_visibility();
}

void page_t::remove_viewport(desktop_t * d, viewport_t * v) {

	/* remove fullscreened clients if needed */
	for (auto &x : _fullscreen_client_to_viewport) {
		if (x.second.viewport == v) {
			unfullscreen(x.first);
			break;
		}
	}

	/* Transfer clients to a valid notebook */
	for (auto nbk : filter_class<notebook_t>(v->tree_t::get_all_children())) {
		for (auto c : nbk->get_clients()) {
			d->get_default_pop()->add_client(c, false);
		}
	}

	rpage->add_damaged(d->get_default_pop()->allocation());

}

void page_t::destroy_viewport(viewport_t * v) {
	std::vector<tree_t *> lst;
	v->get_all_children(lst);
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
void page_t::onmap(xcb_window_t w) {
	if (w == rnd->get_composite_overlay())
		return;
	if (w == cnx->root())
		return;

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
	xcb_flush(cnx->xcb());
	cnx->grab();
	cnx->fetch_pending_events();

	if(cnx->check_for_destroyed_window(w)) {
		printf("do not manage %u because it wil be destoyed\n", w);
		cnx->ungrab();
		return;
	}

	if(cnx->check_for_unmap_window(w)) {
		printf("do not manage %u because it wil be unmapped\n", w);
		cnx->ungrab();
		return;
	}

	if(cnx->check_for_reparent_window(w)) {
		printf("do not manage %u because it will be reparented\n", w);
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
			std::shared_ptr<client_properties_t> props{new client_properties_t{cnx, static_cast<xcb_window_t>(w)}};
			if (props->read_window_attributes()) {
				if(props->wa()->_class != XCB_WINDOW_CLASS_INPUT_ONLY) {
					props->read_all_properties();

				props->print_window_attributes();
				props->print_properties();

				xcb_atom_t type = props->wm_type();

				if (not props->wa()->override_redirect) {
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

		} catch (exception_t &e) {
			std::cout << e.what() << std::endl;
			c = nullptr;
			throw;
		}

	}

	cnx->ungrab();

}


void page_t::create_managed_window(std::shared_ptr<client_properties_t> c, xcb_atom_t type) {

	try {
		client_managed_t * mw = new client_managed_t(type, c, theme, cmgr);
		manage_client(mw, type);
	} catch (...) {
		printf("Error while creating managed window\n");
		throw;
	}
}

void page_t::manage_client(client_managed_t * mw, xcb_atom_t type) {

	try {

		safe_update_transient_for(mw);
		mw->raise_child(nullptr);
		update_client_list();
		update_windows_stack();

		mw->set_current_desktop(_current_desktop);
		if(not _desktop_list[_current_desktop]->is_hidden()) {
			mw->show();
		} else {
			mw->hide();
		}

		/* HACK OLD FASHION FULLSCREEN */
		if (mw->geometry()->x == 0 and mw->geometry()->y == 0
				and mw->geometry()->width == _allocation.w
				and mw->geometry()->height == _allocation.h
				and mw->wm_type() == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
			mw->net_wm_state_add(_NET_WM_STATE_FULLSCREEN);
		}

		if (mw->is_fullscreen()) {
			mw->normalize();
			fullscreen(mw);
			update_desktop_visibility();
			mw->reconfigure();
			xcb_timestamp_t time = 0;
			if (get_safe_net_wm_user_time(mw, time)) {
				set_focus(mw, time);
			} else {
				mw->set_focus_state(false);
			}

		} else if ((type == A(_NET_WM_WINDOW_TYPE_NORMAL)
				or type == A(_NET_WM_WINDOW_TYPE_DESKTOP))
				and get_transient_for(mw) == nullptr
				and mw->has_motif_border()) {

			/** select if the client want to appear mapped or iconic **/
			bool activate = true;

			/**
			 * first try if previous vm has put this window in IconicState, then
			 * Check if the client have a preferred initial state.
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
			xcb_timestamp_t time = 0;
			if (get_safe_net_wm_user_time(mw, time)) {
				set_focus(mw, time);
			} else {
				mw->set_focus_state(false);
			}
		}
	} catch (exception_t & e) {
		cout << e.what() << endl;
	} catch (...) {
		printf("Error while creating managed window\n");
		throw;
	}
}

void page_t::create_unmanaged_window(std::shared_ptr<client_properties_t> c, xcb_atom_t type) {
	try {
		client_not_managed_t * uw = new client_not_managed_t(type, c, cmgr->get_managed_composite_surface(c->id()));
		uw->map();
		safe_update_transient_for(uw);
		safe_raise_window(uw);
		rnd->add_damaged(uw->visible_area());
		update_windows_stack();
		_need_render = true;
	} catch (exception_t & e) {
		cout << e.what() << endl;
	} catch (...) {
		printf("Error while creating managed window\n");
		throw;
	}
}

void page_t::create_dock_window(std::shared_ptr<client_properties_t> c, xcb_atom_t type) {
	client_not_managed_t * uw = new client_not_managed_t(type, c, cmgr->get_managed_composite_surface(c->id()));
	uw->map();
	attach_dock(uw);
	safe_raise_window(uw);
	update_allocation();
}

viewport_t * page_t::find_mouse_viewport(int x, int y) {
	std::vector<viewport_t*> viewports = _desktop_list[_current_desktop]->get_viewports();
	for (auto v: viewports) {
		if (v->raw_area().is_inside(x, y))
			return v;
	}
	return nullptr;
}

/**
 * Read user time with proper fallback
 * @return: true if successfully find usertime, otherwise false.
 * @output time: if time is found time is set to the found value.
 * @input c: a window client handler.
 **/
bool page_t::get_safe_net_wm_user_time(client_base_t * c, xcb_timestamp_t & time) {
	if (c->net_wm_user_time() != nullptr) {
		time = *(c->net_wm_user_time());
		return true;
	} else {
		if (c->net_wm_user_time_window() == nullptr)
			return false;
		if (*(c->net_wm_user_time_window()) == XCB_WINDOW_NONE)
			return false;

		net_wm_user_time_t xtime;
		auto x = display_t::make_property_fetcher_t(xtime, cnx,
				*(c->net_wm_user_time_window()));
		x.update(cnx);

		if(xtime.data == nullptr)
			return false;

		if (*xtime == 0)
			return false;

		time = *(xtime);
		return true;

	}
}

std::vector<client_managed_t *> page_t::get_managed_windows() {
	return filter_class<client_managed_t>(tree_t::get_all_children());
}

client_managed_t * page_t::find_managed_window_with(xcb_window_t w) {
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
void page_t::cleanup_not_managed_client(client_not_managed_t * c) {
	remove_client(c);
	update_client_list();
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

		if (uw->wm_type() == A(_NET_WM_WINDOW_TYPE_TOOLTIP)) {
			detach(uw);
			tooltips.push_back(uw);
			uw->set_parent(this);
		} else if (uw->wm_type() == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
			detach(uw);
			notifications.push_back(uw);
			uw->set_parent(this);
		} else if (uw->net_wm_state() != nullptr
				and has_key<xcb_atom_t>(*(uw->net_wm_state()), A(_NET_WM_STATE_ABOVE))) {
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

	if (page_areas != nullptr) {
		delete page_areas;
	}

	std::vector<tree_t *> tmp = _desktop_list[_current_desktop]->tree_t::get_all_children();
	std::list<tree_t const *> lc(tmp.begin(), tmp.end());
	page_areas = compute_page_areas(lc);
}

void page_t::set_desktop_geometry(long width, long height) {
	/* define desktop geometry */
	uint32_t desktop_geometry[2];
	desktop_geometry[0] = width;
	desktop_geometry[1] = height;
	cnx->change_property(cnx->root(), _NET_DESKTOP_GEOMETRY,
			CARDINAL, 32, desktop_geometry, 2);
}

client_base_t * page_t::find_client_with(xcb_window_t w) {
	for(auto i: filter_class<client_base_t>(tree_t::get_all_children())) {
		if(i->has_window(w)) {
			return i;
		}
	}
	return nullptr;
}

client_base_t * page_t::find_client(xcb_window_t w) {
	for(auto i: filter_class<client_base_t>(tree_t::get_all_children())) {
		if(i->orig() == w) {
			return i;
		}
	}
	return nullptr;
}

void page_t::remove_client(client_base_t * c) {
	tree_t * parent = c->parent();
	if (parent != nullptr) {
		if (typeid(*parent) == typeid(notebook_t)) {
			notebook_t * nb = dynamic_cast<notebook_t*>(parent);
			rpage->add_damaged(nb->allocation());
		}
	}
	detach(c);
	for(auto i: c->childs()) {
		client_base_t * c = dynamic_cast<client_base_t *>(i);
		if(c != nullptr) {
			insert_in_tree_using_transient_for(c);
		}
	}
	delete c;
}

void page_t::get_all_children(std::vector<tree_t *> & out) const {

	for (auto v: _desktop_list) {
		out.push_back(v);
		v->get_all_children(out);
	}

	for(auto x: below) {
		out.push_back(x);
		x->get_all_children(out);
	}

	for(auto x: docks) {
		out.push_back(x);
		x->get_all_children(out);
	}

//	for(auto x: _fullscreen_client_to_viewport) {
//		out.push_back(x.first);
//		x.first->get_all_children(out);
//	}

	for(auto x: root_subclients) {
		out.push_back(x);
		x->get_all_children(out);
	}

	for(auto x: tooltips) {
		out.push_back(x);
		x->get_all_children(out);
	}

	for(auto x: notifications) {
		out.push_back(x);
		x->get_all_children(out);
	}

	for(auto x: above) {
		out.push_back(x);
		x->get_all_children(out);
	}

}

std::string page_t::get_node_name() const {
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

	if(has_key(std::vector<tree_t*>{_desktop_list.begin(), _desktop_list.end()}, t)) {
		throw exception_t("ERROR: using page::remove to remove desktop is not recommended, use page::remove_viewport instead");
	}

	root_subclients.remove(dynamic_cast<client_base_t *>(t));
	tooltips.remove(dynamic_cast<client_not_managed_t *>(t));
	notifications.remove(dynamic_cast<client_not_managed_t *>(t));
	//_fullscreen_client_to_viewport.erase(dynamic_cast<client_managed_t *>(t));
	docks.remove(dynamic_cast<client_not_managed_t*>(t));
	above.remove(dynamic_cast<client_not_managed_t*>(t));
	below.remove(dynamic_cast<client_not_managed_t*>(t));

}

void page_t::create_identity_window() {
	/* create an invisible window to identify page */

	uint32_t attrs = XCB_EVENT_MASK_STRUCTURE_NOTIFY|XCB_EVENT_MASK_PROPERTY_CHANGE;
	uint32_t attrs_mask = XCB_CW_EVENT_MASK;
	identity_window = cnx->create_input_only_window(cnx->root(), i_rect { -100,
			-100, 1, 1 }, attrs_mask, &attrs);
	std::string name("page");
	cnx->change_property(identity_window, _NET_WM_NAME, UTF8_STRING, 8, name.c_str(),
			name.length() + 1);
	cnx->change_property(identity_window, _NET_SUPPORTING_WM_CHECK, WINDOW, 32,
			&identity_window, 1);
	cnx->change_property(cnx->root(), _NET_SUPPORTING_WM_CHECK,
			WINDOW, 32, &identity_window, 1);
	uint32_t pid = getpid();
	cnx->change_property(identity_window, _NET_WM_PID, CARDINAL, 32, &pid, 1);

}

void page_t::register_wm() {
	if (!cnx->register_wm(identity_window, replace_wm)) {
		throw exception_t("Cannot register window manager");
	}
}

void page_t::register_cm() {
	if (!cnx->register_cm(identity_window, replace_wm)) {
		throw exception_t("Cannot register composite manager");
	}
}

void grab_key(xcb_connection_t * xcb, xcb_window_t w, key_desc_t & key, keymap_t * keymap) {
	int kc = 0;
	if ((kc = keymap->find_keysim(key.ks))) {
		xcb_grab_key(xcb, true, w, key.mod, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		if(keymap->numlock_mod_mask() != 0) {
			xcb_grab_key(xcb, true, w, key.mod| keymap->numlock_mod_mask(), kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		}
	}
}

/**
 * Update grab keys aware of current keymap
 */
void page_t::update_grabkey() {

	assert(keymap != nullptr);

	/** ungrab all previews key **/
	xcb_ungrab_key(cnx->xcb(), XCB_GRAB_ANY, cnx->root(), XCB_MOD_MASK_ANY);

	int kc = 0;

	grab_key(cnx->xcb(), cnx->root(), bind_debug_1, keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_debug_2, keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_debug_3, keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_debug_4, keymap);

	grab_key(cnx->xcb(), cnx->root(), bind_page_quit, keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_toggle_fullscreen, keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_right_desktop, keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_left_desktop, keymap);

	/* Alt-Tab */
	if ((kc = keymap->find_keysim(XK_Tab))) {
		xcb_grab_key(cnx->xcb(), true, cnx->root(), XCB_MOD_MASK_1, kc,
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
		if (keymap->numlock_mod_mask() != 0) {
			xcb_grab_key(cnx->xcb(), true, cnx->root(),
					XCB_MOD_MASK_1 | keymap->numlock_mod_mask(), kc,
					XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
		}
	}


}

void page_t::update_keymap() {
	if(keymap != nullptr) {
		delete keymap;
	}
	keymap = new keymap_t(cnx->xcb());
}

void page_t::prepare_render(std::vector<std::shared_ptr<renderable_t>> & out, page::time_t const & time) {

	rnd->add_damaged(rpage->get_damaged());
	out += dynamic_pointer_cast<renderable_t>(rpage->prepare_render());
 	rpage->repair_damaged(tree_t::get_visible_children());

	for(auto i: tree_t::children()) {
		i->prepare_render(out, time);
	}

	/** Add all possible popups **/
	if(pat != nullptr) {
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

	if(menu != nullptr) {
		out.push_back(menu);
	}

	if(client_menu != nullptr) {
		out.push_back(client_menu);
	}

	_need_render = false;

}


std::vector<page_event_t> * page_t::compute_page_areas(
		std::list<tree_t const *> const & page) const {

	std::vector<page_event_t> * ret = new std::vector<page_event_t>();

	for (auto i : page) {
		if(dynamic_cast<split_t const *>(i)) {
			split_t const * s = dynamic_cast<split_t const *>(i);
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
		} else if (dynamic_cast<notebook_t const *>(i)) {
			notebook_t const * n = dynamic_cast<notebook_t const *>(i);
			n->compute_areas_for_notebook(ret);
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

void page_t::children(std::vector<tree_t *> & out) const {
	for (auto i: _desktop_list) {
		out.push_back(i);
	}

	for(auto x: below) {
		out.push_back(x);
	}

	for(auto x: docks) {
		out.push_back(x);
	}

	for(auto x: root_subclients) {
		out.push_back(x);
	}

	for(auto x: tooltips) {
		out.push_back(x);
	}

	for(auto x: notifications) {
		out.push_back(x);
	}

	for(auto x: above) {
		out.push_back(x);
	}
}

/** debug function that try to print the state of page in stdout **/
void page_t::print_state() const {
	print_tree(0);
	cout << "_current_desktop = " << _current_desktop << endl;

	cout << "clients list:" << endl;
	for(auto c: filter_class<client_base_t>(tree_t::get_all_children())) {
		cout << "client " << c->get_node_name() << " id = " << c->orig() << " ptr = " << c << " parent = " << c->parent() << endl;
	}
	cout << "end" << endl;;

}

void page_t::update_current_desktop() const {
	/* set current desktop */
	uint32_t current_desktop = _current_desktop;
	cnx->change_property(cnx->root(), _NET_CURRENT_DESKTOP, CARDINAL,
			32, &current_desktop, 1);
}

void page_t::hide() {
	for(auto i: tree_t::children()) {
		i->hide();
	}
}

void page_t::show() {
	for(auto i: tree_t::children()) {
		i->show();
	}
}

void page_t::switch_to_desktop(int desktop, Time time) {
	if (desktop != _current_desktop) {
		_current_desktop = desktop;
		update_current_desktop();
		update_desktop_visibility();
		rpage->add_damaged(_root_position);
		set_focus(nullptr, time);
	}
}

void page_t::update_fullscreen_clients_position() {
	for(auto i: _fullscreen_client_to_viewport) {
		i.first->set_notebook_wished_position(i.second.viewport->raw_area());
	}
}

void page_t::update_desktop_visibility() {
	/** hide only desktop that must be hidden first **/
	for(unsigned k = 0; k < _desktop_list.size(); ++k) {
		if(k != _current_desktop) {
			_desktop_list[k]->hide();
		}
	}

	/** and show the desktop that have to be show **/
	_desktop_list[_current_desktop]->show();

	for(auto i: _fullscreen_client_to_viewport) {
		if(not i.second.desktop->is_hidden()) {
			i.second.viewport->hide();
		}
	}
}

void page_t::get_visible_children(std::vector<tree_t *> & out) {
	out.push_back(this);
	/** all children are visible within page **/
	for(auto i: tree_t::children()) {
		i->get_visible_children(out);
	}
}


}

