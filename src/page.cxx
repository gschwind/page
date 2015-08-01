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

#include <poll.h>

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
#include "grab_handlers.hxx"

#include "simple2_theme.hxx"
#include "tiny_theme.hxx"

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

time_t const page_t::default_wait{1000000000L / 120L};

page_t::page_t(int argc, char ** argv)
{

	_grab_handler = nullptr;

	identity_window = XCB_NONE;
	cmgr = nullptr;

	/** initialize the empty desktop **/
	_current_desktop = 0;
	for(unsigned k = 0; k < 4; ++k) {
		workspace_t * d = new workspace_t{this, k};
		d->set_parent(this);
		_desktop_list.push_back(d);
		_desktop_stack.push_front(d);
		d->hide();
	}

	_desktop_list[_current_desktop]->show();

	replace_wm = false;
	char const * conf_file_name = 0;

	_need_render = false;
	_need_restack = false;
	_need_update_client_list = false;
	_menu_drop_down_shadow = false;

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

	_keymap = nullptr;

	cnx = new display_t;
	rnd = nullptr;

	/* load configurations, from lower priority to high one */

	/* load default configuration */
	conf.merge_from_file_if_exist(std::string{DATA_DIR "/page/page.conf"});

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
	_theme_engine = conf.get_string("default", "theme_engine");

	_last_focus_time = XCB_TIME_CURRENT_TIME;
	_last_button_press = XCB_TIME_CURRENT_TIME;
	_left_most_border = std::numeric_limits<int>::max();
	_top_most_border = std::numeric_limits<int>::max();

	_theme = nullptr;

	find_key_from_string(conf.get_string("default", "bind_page_quit"), bind_page_quit);
	find_key_from_string(conf.get_string("default", "bind_close"), bind_close);
	find_key_from_string(conf.get_string("default", "bind_exposay_all"), bind_exposay_all);
	find_key_from_string(conf.get_string("default", "bind_toggle_fullscreen"), bind_toggle_fullscreen);
	find_key_from_string(conf.get_string("default", "bind_toggle_compositor"), bind_toggle_compositor);
	find_key_from_string(conf.get_string("default", "bind_right_desktop"), bind_right_desktop);
	find_key_from_string(conf.get_string("default", "bind_left_desktop"), bind_left_desktop);

	find_key_from_string(conf.get_string("default", "bind_bind_window"), bind_bind_window);
	find_key_from_string(conf.get_string("default", "bind_fullscreen_window"), bind_fullscreen_window);
	find_key_from_string(conf.get_string("default", "bind_float_window"), bind_float_window);

	find_key_from_string(conf.get_string("default", "bind_debug_1"), bind_debug_1);
	find_key_from_string(conf.get_string("default", "bind_debug_2"), bind_debug_2);
	find_key_from_string(conf.get_string("default", "bind_debug_3"), bind_debug_3);
	find_key_from_string(conf.get_string("default", "bind_debug_4"), bind_debug_4);


	find_key_from_string(conf.get_string("default", "bind_cmd_0"), bind_cmd[0].key);
	find_key_from_string(conf.get_string("default", "bind_cmd_1"), bind_cmd[1].key);
	find_key_from_string(conf.get_string("default", "bind_cmd_2"), bind_cmd[2].key);
	find_key_from_string(conf.get_string("default", "bind_cmd_3"), bind_cmd[3].key);
	find_key_from_string(conf.get_string("default", "bind_cmd_4"), bind_cmd[4].key);
	find_key_from_string(conf.get_string("default", "bind_cmd_5"), bind_cmd[5].key);
	find_key_from_string(conf.get_string("default", "bind_cmd_6"), bind_cmd[6].key);
	find_key_from_string(conf.get_string("default", "bind_cmd_7"), bind_cmd[7].key);
	find_key_from_string(conf.get_string("default", "bind_cmd_8"), bind_cmd[8].key);
	find_key_from_string(conf.get_string("default", "bind_cmd_9"), bind_cmd[9].key);

	bind_cmd[0].cmd = conf.get_string("default", "exec_cmd_0");
	bind_cmd[1].cmd = conf.get_string("default", "exec_cmd_1");
	bind_cmd[2].cmd = conf.get_string("default", "exec_cmd_2");
	bind_cmd[3].cmd = conf.get_string("default", "exec_cmd_3");
	bind_cmd[4].cmd = conf.get_string("default", "exec_cmd_4");
	bind_cmd[5].cmd = conf.get_string("default", "exec_cmd_5");
	bind_cmd[6].cmd = conf.get_string("default", "exec_cmd_6");
	bind_cmd[7].cmd = conf.get_string("default", "exec_cmd_7");
	bind_cmd[8].cmd = conf.get_string("default", "exec_cmd_8");
	bind_cmd[9].cmd = conf.get_string("default", "exec_cmd_9");

	if(conf.get_string("default", "auto_refocus") == "true") {
		_auto_refocus = true;
	} else {
		_auto_refocus = false;
	}

	if(conf.get_string("default", "enable_shade_windows") == "true") {
		_enable_shade_windows = true;
	} else {
		_enable_shade_windows = false;
	}

	if(conf.get_string("default", "mouse_focus") == "true") {
		_mouse_focus = true;
	} else {
		_mouse_focus = false;
	}

	if(conf.get_string("default", "menu_drop_down_shadow") == "true") {
		_menu_drop_down_shadow = true;
	} else {
		_menu_drop_down_shadow = false;
	}

	_global_client_focus_history.push_back(nullptr);

}

page_t::~page_t() {

	cnx->unload_cursors();

	for (auto i : filter_class<client_managed_t>(tree_t::get_all_children())) {
		if (cnx->lock(i->orig())) {
			cnx->reparentwindow(i->orig(), cnx->root(), 0, 0);
			cnx->unlock();
		}
	}

	for (auto i : tree_t::get_all_children()) {
		delete i;
	}

	delete _theme;
	delete rnd;
	delete _keymap;

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

	cnx->grab();

	cnx->change_property(cnx->root(), _NET_SUPPORTING_WM_CHECK,
			WINDOW, 32, &identity_window, 1);

	_bind_all_default_event();

	/** Initialize theme **/

	if(_theme_engine == "tiny") {
//		/* TODO */
//		std::cout << "using tiny theme engine" << std::endl;
//		_theme = new tiny_theme_t{cnx, conf};
		std::cout << "using simple theme engine" << std::endl;
		_theme = new simple2_theme_t{cnx, conf};
	} else {
		/* The default theme engine */
		std::cout << "using simple theme engine" << std::endl;
		_theme = new simple2_theme_t{cnx, conf};
	}

	cmgr = new composite_surface_manager_t{cnx};

	/* start listen root event before anything, each event will be stored to be processed later */
	cnx->select_input(cnx->root(), ROOT_EVENT_MASK);

	/**
	 * listen RRCrtcChangeNotifyMask for possible change in screen layout.
	 **/
	xcb_randr_select_input(cnx->xcb(), cnx->root(), XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE);

	update_viewport_layout();

	cnx->load_cursors();
	cnx->set_window_cursor(cnx->root(), cnx->xc_left_ptr);

	update_net_supported();

	/* update number of desktop */
	uint32_t number_of_desktop = _desktop_list.size();
	cnx->change_property(cnx->root(), _NET_NUMBER_OF_DESKTOPS,
			CARDINAL, 32, &number_of_desktop, 1);


	update_current_desktop();

	{
		/* page does not have showing desktop mode */
		uint32_t showing_desktop = 0;
		cnx->change_property(cnx->root(), _NET_SHOWING_DESKTOP, CARDINAL, 32,
				&showing_desktop, 1);
	}

	{
		std::vector<char> names_list;
		for (unsigned k = 0; k < _desktop_list.size(); ++k) {
			std::ostringstream os;
			os << "Desktop #" << k;
			std::string s { os.str() };
			names_list.insert(names_list.end(), s.begin(), s.end());
			/* end of string */
			names_list.push_back(0);
		}
		cnx->change_property(cnx->root(), _NET_DESKTOP_NAMES, UTF8_STRING, 8,
				&names_list[0], names_list.size());

	}

	{
		wm_icon_size_t icon_size{16,16,16,16,16,16};
		cnx->change_property(cnx->root(), WM_ICON_SIZE, CARDINAL, 32,
				&icon_size, 6);

	}

	scan();

	/* setup _NET_ACTIVE_WINDOW */
	set_focus(nullptr, XCB_CURRENT_TIME);

	update_keymap();
	update_grabkey();

	update_windows_stack();
	xcb_flush(cnx->xcb());

	cnx->ungrab();

	/* start the compositor once the window manager is fully started */
	start_compositor();

	add_poll(cnx->fd(), POLLIN|POLLPRI|POLLERR, [this](struct pollfd const & x) -> void { this->process_pending_events(); });
	/* call this 120 times per second */
	add_timeout(1000000000L/120L, [this]() -> bool { return this->render_timeout(); });

	mainloop_t::run();

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
	for(auto i: mw->tree_t::children()) {
		client_base_t * c = dynamic_cast<client_base_t *>(i);
		if(c != nullptr) {
			insert_in_tree_using_transient_for(c);
		}
	}

	/** if the window is destroyed, this not work, see fix on destroy **/
	for(auto x: _desktop_list) {
		x->client_focus.remove(mw);
	}

	_global_client_focus_history.remove(mw);

	/* as recommended by EWMH delete _NET_WM_STATE when window become unmanaged */
	mw->net_wm_state_delete();
	mw->wm_state_delete();

	_need_update_client_list = true;;
	update_workarea();

	_clients_list.remove(mw);

	cmgr->unregister_window(mw->base());
	delete mw;
	mw = nullptr;

	auto new_focus = _desktop_list[_current_desktop]->client_focus.front();

	if (new_focus == nullptr or not _auto_refocus) {
		set_focus(nullptr, XCB_CURRENT_TIME);
	} else {
		if (new_focus->is(MANAGED_NOTEBOOK)) {
			notebook_t * nk = find_parent_notebook_for(new_focus);
			if (nk != nullptr) {
				new_focus->activate();
			}
		}
		new_focus->activate();
		set_focus(new_focus, XCB_CURRENT_TIME);
	}
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
			 * if the window is not mapped, check if previous windows manager has set WM_STATE to iconic
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

	_need_update_client_list = true;
	update_workarea();
	for(auto x: _desktop_list) {
		reconfigure_docks(x);
	}
	_need_restack = true;

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

	/** set _NET_CLIENT_LIST : client list from oldest to newer client **/
	std::vector<xcb_window_t> client_list;
	for(auto c: _clients_list) {
		client_list.push_back(c->orig());
	}

	cnx->change_property(cnx->root(), _NET_CLIENT_LIST, WINDOW, 32,
			&client_list[0], client_list.size());

	/** set _NET_CLIENT_LIST_STACKING : bottom to top staking **/
	auto client_managed_list_stack = filter_class<client_managed_t>(tree_t::get_all_children());
	std::vector<xcb_window_t> client_list_stack;
	for(auto c: client_managed_list_stack) {
		client_list_stack.push_back(c->orig());
	}
	cnx->change_property(cnx->root(), _NET_CLIENT_LIST_STACKING,
			WINDOW, 32, &client_list_stack[0], client_list_stack.size());

}

void page_t::process_key_press_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_key_press_event_t const *>(_e);

//	printf("%s key = %d, mod4 = %s, mod1 = %s\n",
//			e->response_type == XCB_KEY_PRESS ? "KeyPress" : "KeyRelease",
//			e->detail,
//			e->state & XCB_MOD_MASK_4 ? "true" : "false",
//			e->state & XCB_MOD_MASK_1 ? "true" : "false");

	/* get KeyCode for Unmodified Key */
	xcb_keysym_t k = _keymap->get(e->detail);

	if (k == 0)
		return;


	/** XCB_MOD_MASK_2 is num_lock, thus ignore his state **/
	unsigned int state = e->state;
	if(_keymap->numlock_mod_mask() != 0) {
		state &= ~_keymap->numlock_mod_mask();
	}

	if (k == bind_page_quit.ks and (state == bind_page_quit.mod)) {
		stop();
	}

	if(_grab_handler != nullptr) {
		_grab_handler->key_press(e);
		return;
	}

	if (k == bind_close.ks and (state == bind_close.mod)) {
		if (not _desktop_list[_current_desktop]->client_focus.empty()) {
			if (_desktop_list[_current_desktop]->client_focus.front()
					!= nullptr) {
				client_managed_t * mw =
						_desktop_list[_current_desktop]->client_focus.front();
				mw->delete_window(e->time);
			}
		}
	}

	if (k == bind_exposay_all.ks and (state == bind_exposay_all.mod)) {
		auto child = filter_class<notebook_t>(
				_desktop_list[_current_desktop]->tree_t::get_all_children());
		for (auto c : child) {
			c->start_exposay();
		}
	}

	if (k == bind_toggle_fullscreen.ks
			and (state == bind_toggle_fullscreen.mod)) {
		if (not _desktop_list[_current_desktop]->client_focus.empty()) {
			if (_desktop_list[_current_desktop]->client_focus.front()
					!= nullptr) {
				toggle_fullscreen(
						_desktop_list[_current_desktop]->client_focus.front());
			}
		}
	}

	if (k == bind_toggle_compositor.ks
			and (state == bind_toggle_compositor.mod)) {
		if (rnd == nullptr) {
			start_compositor();
		} else {
			stop_compositor();
		}
	}

	if (k == bind_right_desktop.ks and (state == bind_right_desktop.mod)) {
		switch_to_desktop((_current_desktop + 1) % _desktop_list.size());
		set_focus(_desktop_list[_current_desktop]->client_focus.front(),
				e->time);
	}

	if (k == bind_left_desktop.ks and (state == bind_left_desktop.mod)) {
		switch_to_desktop((_current_desktop - 1) % _desktop_list.size());
		set_focus(_desktop_list[_current_desktop]->client_focus.front(),
				e->time);
	}

	if (k == bind_bind_window.ks and (state == bind_bind_window.mod)) {
		if (not _desktop_list[_current_desktop]->client_focus.empty()) {
			if (_desktop_list[_current_desktop]->client_focus.front()
					!= nullptr) {
				if (_desktop_list[_current_desktop]->client_focus.front()->is(
						MANAGED_FULLSCREEN)) {
					unfullscreen(
							_desktop_list[_current_desktop]->client_focus.front());
				}

				if (_desktop_list[_current_desktop]->client_focus.front()->is(
						MANAGED_FLOATING)) {
					bind_window(
							_desktop_list[_current_desktop]->client_focus.front(),
							true);
				}
			}
		}
	}

	if (k == bind_fullscreen_window.ks
			and (state == bind_fullscreen_window.mod)) {
		if (not _desktop_list[_current_desktop]->client_focus.empty()) {
			if (_desktop_list[_current_desktop]->client_focus.front()
					!= nullptr) {
				if (not _desktop_list[_current_desktop]->client_focus.front()->is(
						MANAGED_FULLSCREEN)) {
					fullscreen(
							_desktop_list[_current_desktop]->client_focus.front(),
							nullptr);
				}
			}
		}
	}

	if (k == bind_float_window.ks and (state == bind_float_window.mod)) {
		if (not _desktop_list[_current_desktop]->client_focus.empty()) {
			if (_desktop_list[_current_desktop]->client_focus.front()
					!= nullptr) {
				if (_desktop_list[_current_desktop]->client_focus.front()->is(
						MANAGED_FULLSCREEN)) {
					unfullscreen(
							_desktop_list[_current_desktop]->client_focus.front());
				}

				if (_desktop_list[_current_desktop]->client_focus.front()->is(
						MANAGED_NOTEBOOK)) {
					unbind_window(
							_desktop_list[_current_desktop]->client_focus.front());
				}
			}
		}
	}

	if (rnd != nullptr) {
		if (k == bind_debug_1.ks and (state == bind_debug_1.mod)) {
			if (rnd->show_fps()) {
				rnd->set_show_fps(false);
			} else {
				rnd->set_show_fps(true);
			}
		}

		if (k == bind_debug_2.ks and (state == bind_debug_2.mod)) {
			if (rnd->show_damaged()) {
				rnd->set_show_damaged(false);
			} else {
				rnd->set_show_damaged(true);
			}
		}

		if (k == bind_debug_3.ks and (state == bind_debug_3.mod)) {
			if (rnd->show_opac()) {
				rnd->set_show_opac(false);
			} else {
				rnd->set_show_opac(true);
			}
		}
	}

	if (k == bind_debug_4.ks and (state == bind_debug_4.mod)) {
		_need_restack = true;
		print_tree(0);
		for (auto i : _clients_list) {
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
			case MANAGED_DOCK:
				cout << "[" << i->orig() << "] dock : " << i->title() << endl;
				break;
			}
		}
	}

	for(int i; i < bind_cmd.size(); ++i) {
		if (k == bind_cmd[i].key.ks and (state == bind_cmd[i].key.mod)) {
			run_cmd(bind_cmd[i].cmd);
		}
	}

	if (k == XK_Tab and (state == XCB_MOD_MASK_1)) {

		if (_grab_handler == nullptr and not _clients_list.empty()) {

			/* Grab keyboard */
			/** TODO: check for success or failure **/
			xcb_grab_keyboard_unchecked(cnx->xcb(), false, cnx->root(), e->time,
					XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

			/** Continue to play event as usual (Alt+Tab is in Sync mode) **/
			xcb_allow_events(cnx->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
			grab_start(new grab_alt_tab_t{this});

		} else {
			xcb_allow_events(cnx->xcb(), XCB_ALLOW_REPLAY_KEYBOARD, e->time);
		}

		_need_render = true;

	}

}

void page_t::process_key_release_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_key_release_event_t const *>(_e);

	if(_grab_handler != nullptr) {
		_grab_handler->key_release(e);
		return;
	}

}

/* Button event make page to grab pointer */
void page_t::process_button_press_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_button_press_event_t const *>(_e);
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


	if(_grab_handler != nullptr) {
		_grab_handler->button_press(e);
		xcb_allow_events(cnx->xcb(), XCB_ALLOW_ASYNC_POINTER, e->time);
		return;
	}

	broadcast_button_press(e);

	/**
	 * if no change happened to process mode
	 * We allow events (remove the grab), and focus those window.
	 **/
	if (_grab_handler == nullptr) {
		xcb_allow_events(cnx->xcb(), XCB_ALLOW_REPLAY_POINTER, e->time);
		client_managed_t * mw = find_managed_window_with(e->event);
		if (mw != nullptr) {
			mw->activate();
			set_focus(mw, e->time);
		}
	} else {
		/* Do not replay events, grab them and process them until Release Button */
		xcb_allow_events(cnx->xcb(), XCB_ALLOW_ASYNC_POINTER, e->time);
	}

}

void page_t::process_configure_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_configure_notify_event_t const *>(_e);

	//printf("configure (%d) %dx%d+%d+%d\n", e->window, e->width, e->height, e->x, e->y);

	client_base_t * c = find_client(e->window);
	if(c != nullptr) {
		c->process_event(e);
	}

	/** damage corresponding area **/
	if(e->event == cnx->root()) {
		add_compositor_damaged(_root_position);
	}

}

/* track all created window */
void page_t::process_create_notify_event(xcb_generic_event_t const * e) {
//	std::cout << format("08", e->sequence) << " create_notify " << e->width << "x" << e->height << "+" << e->x << "+" << e->y
//			<< " overide=" << (e->override_redirect?"true":"false")
//			<< " boder_width=" << e->border_width << std::endl;
}

void page_t::process_destroy_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_destroy_notify_event_t const *>(_e);
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

	_need_restack = true;
}

void page_t::process_gravity_notify_event(xcb_generic_event_t const * e) {
	/* Ignore it, never happen ? */
}

void page_t::process_map_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_map_notify_event_t const *>(_e);
	/* if map event does not occur within root, ignore it */
	if (e->event != cnx->root())
		return;
	onmap(e->window);
}

void page_t::process_reparent_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_reparent_notify_event_t const *>(_e);
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

void page_t::process_unmap_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_unmap_notify_event_t const *>(_e);
	client_base_t * c = find_client(e->window);
	if (c != nullptr) {
		add_compositor_damaged(c->visible_area());
		if(typeid(*c) == typeid(client_not_managed_t)) {
			cleanup_not_managed_client(dynamic_cast<client_not_managed_t *>(c));
			_need_render = true;
		}
	}
}

void page_t::process_fake_unmap_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_unmap_notify_event_t const *>(_e);
	/**
	 * Client must send a fake unmap event if he want get back the window.
	 * (i.e. he want that we unmanage it.
	 **/

	/* if client is managed */
	client_base_t * c = find_client(e->window);

	if (c != nullptr) {
		cmgr->freeze(c->base(), true);
		add_compositor_damaged(c->visible_area());
		if(typeid(*c) == typeid(client_managed_t)) {
			client_managed_t * mw = dynamic_cast<client_managed_t *>(c);

			if (cnx->lock(mw->orig())) {
				cnx->reparentwindow(mw->orig(), cnx->root(), 0.0, 0.0);
				cnx->unlock();
			}

			unmanage(mw);
		}
		//render();
	}
}

void page_t::process_circulate_request_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_circulate_request_event_t const *>(_e);
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

void page_t::process_configure_request_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_configure_request_event_t const *>(_e);
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

		add_compositor_damaged(c->visible_area());

		if(typeid(*c) == typeid(client_managed_t)) {

			client_managed_t * mw = dynamic_cast<client_managed_t *>(c);

			if (mw->lock()) {

				if ((e->value_mask & (XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT)) != 0) {

					rect old_size = mw->get_floating_wished_position();
					/** compute floating size **/
					rect new_size = mw->get_floating_wished_position();

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

					dimention_t<unsigned> final_size = mw->compute_size_with_constrain(new_size.w, new_size.h);

					new_size.w = final_size.width;
					new_size.h = final_size.height;

					//printf("new_size = %s\n", new_size.to_string().c_str());

					if (new_size != old_size) {
						/** only affect floating windows **/
						mw->set_floating_wished_position(new_size);
						mw->reconfigure();
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
	//printf("ackwoledge_configure_request ");

	int i = 0;
	uint32_t value[7] = {0};
	uint32_t mask = 0;
	if(e->value_mask & XCB_CONFIG_WINDOW_X) {
		mask |= XCB_CONFIG_WINDOW_X;
		value[i++] = e->x;
		//printf("x = %d ", e->x);
	}

	if(e->value_mask & XCB_CONFIG_WINDOW_Y) {
		mask |= XCB_CONFIG_WINDOW_Y;
		value[i++] = e->y;
		//printf("y = %d ", e->y);
	}

	if(e->value_mask & XCB_CONFIG_WINDOW_WIDTH) {
		mask |= XCB_CONFIG_WINDOW_WIDTH;
		value[i++] = e->width;
		//printf("w = %d ", e->width);
	}

	if(e->value_mask & XCB_CONFIG_WINDOW_HEIGHT) {
		mask |= XCB_CONFIG_WINDOW_HEIGHT;
		value[i++] = e->height;
		//printf("h = %d ", e->height);
	}

	if(e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) {
		mask |= XCB_CONFIG_WINDOW_BORDER_WIDTH;
		value[i++] = e->border_width;
		//printf("border = %d ", e->border_width);
	}

	if(e->value_mask & XCB_CONFIG_WINDOW_SIBLING) {
		mask |= XCB_CONFIG_WINDOW_SIBLING;
		value[i++] = e->sibling;
		//printf("sibling = %d ", e->sibling);
	}

	if(e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE) {
		mask |= XCB_CONFIG_WINDOW_STACK_MODE;
		value[i++] = e->stack_mode;
		//printf("stack_mode = %d ", e->stack_mode);
	}

	//printf("\n");

	xcb_void_cookie_t ck = xcb_configure_window(cnx->xcb(), e->window, mask, value);

}

void page_t::process_map_request_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_map_request_event_t const *>(_e);
	if (e->parent != cnx->root()) {
		xcb_map_window(cnx->xcb(), e->window);
		return;
	}

	onmap(e->window);

}

void page_t::process_property_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_property_notify_event_t const *>(_e);
	if(e->window == cnx->root())
		return;

	/** update the property **/
	client_base_t * c = find_client(e->window);
	if(c != nullptr) {
		c->on_property_notify(e);
	}

	client_managed_t * mw = dynamic_cast<client_managed_t*>(c);
	if(mw == nullptr)
		return;

	if (e->atom == A(_NET_WM_USER_TIME)) {
		/* ignore */
	} else if (e->atom == A(_NET_WM_NAME) or e->atom == A(WM_NAME)) {
		mw->update_title();
	} else if (e->atom == A(_NET_WM_STRUT_PARTIAL)) {
		update_workarea();
	} else if (e->atom == A(_NET_WM_STRUT)) {
		update_workarea();
	} else if (e->atom == A(_NET_WM_ICON)) {
		mw->update_icon();
		mw->queue_redraw();
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
		if (mw->is(MANAGED_NOTEBOOK)) {
			find_parent_notebook_for(mw)->update_client_position(mw);
		}

		/* apply normal hint to floating window */
		rect new_size = mw->get_wished_position();

		dimention_t<unsigned> final_size = mw->compute_size_with_constrain(
				new_size.w, new_size.h);
		new_size.w = final_size.width;
		new_size.h = final_size.height;
		mw->set_floating_wished_position(new_size);
		mw->reconfigure();
	} else if (e->atom == A(WM_PROTOCOLS)) {
		/* do nothing */
	} else if (e->atom == A(WM_TRANSIENT_FOR)) {
		safe_update_transient_for(mw);
		_need_restack = true;
	} else if (e->atom == A(WM_HINTS)) {
		/* do nothing */
	} else if (e->atom == A(_NET_WM_STATE)) {
		/* this event are generated by page */
		/* change of net_wm_state must be requested by client message */
	} else if (e->atom == A(WM_STATE)) {
		/** this is set by page ... don't read it **/
	} else if (e->atom == A(_NET_WM_DESKTOP)) {
		/* this set by page in most case */
	} else if (e->atom == A(_MOTIF_WM_HINTS)) {
		mw->reconfigure();
	}
}

void page_t::process_fake_client_message_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_client_message_event_t const *>(_e);
	//std::shared_ptr<char> name = cnx->get_atom_name(e->type);
	//std::cout << "ClientMessage type = " << cnx->get_atom_name(e->type) << std::endl;

	xcb_window_t w = e->window;
	if (w == XCB_NONE)
		return;

	client_managed_t * mw = find_managed_window_with(e->window);

	if (e->type == A(_NET_ACTIVE_WINDOW)) {
		if (mw != nullptr) {
			if (mw->lock()) {
				mw->activate();
				if (e->data.data32[1] == XCB_CURRENT_TIME) {
					printf("Invalid focus request ... but stealing focus\n");
					set_focus(mw, XCB_CURRENT_TIME);
				} else {
					set_focus(mw, e->data.data32[1]);
				}
				mw->unlock();
			}
		}
	} else if (e->type == A(_NET_WM_STATE)) {

		/* process first request */
		process_net_vm_state_client_message(w, e->data.data32[0], e->data.data32[1]);
		/* process second request */
		process_net_vm_state_client_message(w, e->data.data32[0], e->data.data32[2]);

//		for (int i = 1; i < 3; ++i) {
//			if (std::find(supported_list.begin(), supported_list.end(),
//					e->data.data32[i]) != supported_list.end()) {
//				switch (e->data.data32[0]) {
//				case _NET_WM_STATE_REMOVE:
//					//w->unset_net_wm_state(e->data.l[i]);
//					break;
//				case _NET_WM_STATE_ADD:
//					//w->set_net_wm_state(e->data.l[i]);
//					break;
//				case _NET_WM_STATE_TOGGLE:
//					//w->toggle_net_wm_state(e->data.l[i]);
//					break;
//				}
//			}
//		}
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
				}
				mw->unlock();
			}
		}

	} else if (e->type == A(PAGE_QUIT)) {
		stop();
	} else if (e->type == A(WM_PROTOCOLS)) {

	} else if (e->type == A(_NET_CLOSE_WINDOW)) {
		if(mw != nullptr) {
			mw->delete_window(e->data.data32[0]);
		}
	} else if (e->type == A(_NET_REQUEST_FRAME_EXTENTS)) {

	} else if (e->type == A(_NET_WM_MOVERESIZE)) {
		if (mw != nullptr) {
			if (mw->is(MANAGED_FLOATING) and _grab_handler == nullptr) {

				if (mw->lock()) {
					int root_x = e->data.data32[0];
					int root_y = e->data.data32[1];
					int direction = e->data.data32[2];
					xcb_button_t button = static_cast<xcb_button_t>(e->data.data32[3]);
					int source = e->data.data32[4];

					if (direction == _NET_WM_MOVERESIZE_MOVE) {
						grab_start(new grab_floating_move_t{this, mw, button, root_x, root_y});
					} else {

						if (direction == _NET_WM_MOVERESIZE_SIZE_TOP) {
							grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_TOP});
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_BOTTOM) {
							grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_BOTTOM});
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_LEFT) {
							grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_LEFT});
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_RIGHT) {
							grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_RIGHT});
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_TOPLEFT) {
							grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_TOP_LEFT});
						} else if (direction == _NET_WM_MOVERESIZE_SIZE_TOPRIGHT) {
							grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_TOP_RIGHT});
						} else if (direction
								== _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT) {
							grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_BOTTOM_LEFT});
						} else if (direction
								== _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT) {
							grab_start(new grab_floating_resize_t{this, mw, button, root_x, root_y, RESIZE_BOTTOM_RIGHT});
						} else {
							grab_start(new grab_floating_move_t{this, mw, button, root_x, root_y});
						}
					}

					if (_grab_handler != nullptr) {
						xcb_grab_pointer(cnx->xcb(), FALSE, cnx->root(),
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
			switch_to_desktop(e->data.data32[0]);
			set_focus(_desktop_list[_current_desktop]->client_focus.front(), XCB_CURRENT_TIME);
		}
	}
}

void page_t::process_damage_notify_event(xcb_generic_event_t const * e) {
	_need_render = true;
}

void page_t::render() {
	_need_render = false;
	if (rnd != nullptr) {
		/**
		 * Try to render if any damage event is encountered. But limit general
		 * rendering to 60 fps.
		 **/
		time_t cur_tic;
		cur_tic.update_to_current_time();
		rnd->clear_renderable();
		std::vector<std::shared_ptr<renderable_t>> ret;
		prepare_render(ret, cur_tic);
		rnd->push_back_renderable(ret);
		rnd->render();
		xcb_flush(cnx->xcb());
		cur_tic.update_to_current_time();
	}
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
		if (x.second.viewport == v) {
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
	_need_restack = true;
}

void page_t::unfullscreen(client_managed_t * mw) {
	/* WARNING: Call order is important, change it with caution */
	mw->net_wm_state_remove(_NET_WM_STATE_FULLSCREEN);
	if(!has_key(_fullscreen_client_to_viewport, mw))
		return;

	detach(mw);

	fullscreen_data_t data = _fullscreen_client_to_viewport[mw];
	_fullscreen_client_to_viewport.erase(mw);

	workspace_t * d = data.desktop;
	viewport_t * v = data.viewport;

	if (data.revert_type == MANAGED_NOTEBOOK) {
		notebook_t * old = data.revert_notebook;
		mw->set_managed_type(MANAGED_NOTEBOOK);
		old->add_client(mw, true);
		mw->reconfigure();
	} else {
		mw->set_managed_type(MANAGED_FLOATING);
		insert_in_tree_using_transient_for(mw);
		mw->reconfigure();
	}

	if(not d->is_hidden()) {
		v->show();
	}

	update_workarea();

}

void page_t::toggle_fullscreen(client_managed_t * c) {
	if(c->is(MANAGED_FULLSCREEN))
		unfullscreen(c);
	else
		fullscreen(c);
}


void page_t::process_event(xcb_generic_event_t const * e) {
	auto x = _event_handlers.find(e->response_type);
	if(x != _event_handlers.end()) {
		if(x->second != nullptr) {
			(this->*(x->second))(e);
		}
	} else {
		//std::cout << "not handled event: " << cnx->event_type_name[(e->response_type&(~0x80))] << (e->response_type&(0x80)?" (fake)":"") << std::endl;
	}
}

void page_t::insert_window_in_notebook(client_managed_t * x, notebook_t * n,
		bool prefer_activate) {
	assert(x != nullptr);
	if (n == nullptr)
		n = get_current_workspace()->default_pop();
	assert(n != nullptr);
	x->set_managed_type(MANAGED_NOTEBOOK);
	n->add_client(x, prefer_activate);

}

/* update viewport and childs allocation */
void page_t::update_workarea() {
	for (auto d : _desktop_list) {
		for (auto v : d->get_viewports()) {
			compute_viewport_allocation(d, v);
		}
		d->set_workarea(d->primary_viewport()->allocation());
	}

	std::vector<uint32_t> workarea_data(_desktop_list.size()*4);
	for(unsigned k = 0; k < _desktop_list.size(); ++k) {
		workarea_data[k*4+0] = _desktop_list[k]->workarea().x;
		workarea_data[k*4+1] = _desktop_list[k]->workarea().y;
		workarea_data[k*4+2] = _desktop_list[k]->workarea().w;
		workarea_data[k*4+3] = _desktop_list[k]->workarea().h;
	}

	cnx->change_property(cnx->root(), _NET_WORKAREA, CARDINAL, 32,
			&workarea_data[0], workarea_data.size());

}

void page_t::set_focus(client_managed_t * new_focus, xcb_timestamp_t tfocus) {
	if(tfocus == XCB_CURRENT_TIME and new_focus != nullptr)
		std::cout << "Warning: Invalid focus time (0)" << std::endl;
	if(tfocus <= _last_focus_time and tfocus != XCB_CURRENT_TIME)
		return;
	if(tfocus != XCB_CURRENT_TIME)
		_last_focus_time = tfocus;

	auto old_focus = _global_client_focus_history.front();

	if (old_focus != nullptr) {
		old_focus->set_focus_state(false);
	}

	_desktop_list[_current_desktop]->client_focus.remove(new_focus);
	_desktop_list[_current_desktop]->client_focus.push_front(new_focus);
	_global_client_focus_history.remove(new_focus);
	_global_client_focus_history.push_front(new_focus);

	if(new_focus == nullptr) {
		cnx->set_input_focus(identity_window, XCB_INPUT_FOCUS_NONE, XCB_CURRENT_TIME);
		cnx->set_net_active_window(XCB_NONE);
	} else {
		cnx->set_net_active_window(new_focus->orig());
		new_focus->focus(tfocus);
	}

	_need_restack = true;

}

void page_t::split_left(notebook_t * nbk, client_managed_t * c) {
	page_component_t * parent = nbk->parent();
	notebook_t * n = new notebook_t{this, _auto_refocus};
	split_t * split = new split_t{this, VERTICAL_SPLIT};
	parent->replace(nbk, split);
	split->set_pack0(n);
	split->set_pack1(nbk);
	if (c != nullptr) {
		detach(c);
		insert_window_in_notebook(c, n, true);
	}
}

void page_t::split_right(notebook_t * nbk, client_managed_t * c) {
	page_component_t * parent = nbk->parent();
	notebook_t * n = new notebook_t{this, _auto_refocus};
	split_t * split = new split_t{this, VERTICAL_SPLIT};
	parent->replace(nbk, split);
	split->set_pack0(nbk);
	split->set_pack1(n);
	if (c != nullptr) {
		detach(c);
		insert_window_in_notebook(c, n, true);
	}
}

void page_t::split_top(notebook_t * nbk, client_managed_t * c) {
	page_component_t * parent = nbk->parent();
	notebook_t * n = new notebook_t{this, _auto_refocus};
	split_t * split = new split_t{this, HORIZONTAL_SPLIT};
	parent->replace(nbk, split);
	split->set_pack0(n);
	split->set_pack1(nbk);
	if (c != nullptr) {
		detach(c);
		insert_window_in_notebook(c, n, true);
	}
}

void page_t::split_bottom(notebook_t * nbk, client_managed_t * c) {
	page_component_t * parent = nbk->parent();
	notebook_t * n = new notebook_t{this, _auto_refocus};
	split_t * split = new split_t{this, HORIZONTAL_SPLIT};
	parent->replace(nbk, split);
	split->set_pack0(nbk);
	split->set_pack1(n);
	if (c != nullptr) {
		detach(c);
		insert_window_in_notebook(c, n, true);
	}
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

	/* remove this split from tree  and replace it by sibling branch */
	splt->parent()->replace(splt, dst);

	/**
	 * if notebook that we want destroy was the default_pop, select
	 * a new one.
	 **/
	if (_desktop_list[_current_desktop]->default_pop() == nbk) {
		_desktop_list[_current_desktop]->update_default_pop();
		/* damage the new default pop to show the notebook mark properly */
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

/*
 * Compute the usable desktop area and dock allocation.
 */
void page_t::compute_viewport_allocation(workspace_t * d, viewport_t * v) {

	/* Partial struct content definition */
	enum : uint32_t {
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
		PS_LAST = 12
	};

	rect const raw_area = v->raw_area();

	int margin_left = _root_position.x + raw_area.x;
	int margin_top = _root_position.y + raw_area.y;
	int margin_right = _root_position.w - raw_area.x - raw_area.w;
	int margin_bottom = _root_position.h - raw_area.y - raw_area.h;

	auto children = filter_class<client_base_t>(d->tree_t::get_all_children());
	for(auto j: children) {
		int32_t ps[PS_LAST];
		bool has_strut{false};

		if(j->net_wm_strut_partial() != nullptr) {
			if(j->net_wm_strut_partial()->size() == 12) {
				std::copy(j->net_wm_strut_partial()->begin(), j->net_wm_strut_partial()->end(), &ps[0]);
				has_strut = true;
			}
		}

		if (j->net_wm_strut() != nullptr and not has_strut) {
			if(j->net_wm_strut()->size() == 4) {

				/** if strut is found, fake strut_partial **/

				std::copy(j->net_wm_strut()->begin(), j->net_wm_strut()->end(), &ps[0]);

				if(ps[PS_TOP] > 0) {
					ps[PS_TOP_START_X] = _root_position.x;
					ps[PS_TOP_END_X] = _root_position.x + _root_position.w;
				}

				if(ps[PS_BOTTOM] > 0) {
					ps[PS_BOTTOM_START_X] = _root_position.x;
					ps[PS_BOTTOM_END_X] = _root_position.x + _root_position.w;
				}

				if(ps[PS_LEFT] > 0) {
					ps[PS_LEFT_START_Y] = _root_position.y;
					ps[PS_LEFT_END_Y] = _root_position.y + _root_position.h;
				}

				if(ps[PS_RIGHT] > 0) {
					ps[PS_RIGHT_START_Y] = _root_position.y;
					ps[PS_RIGHT_END_Y] = _root_position.y + _root_position.h;
				}

				has_strut = true;
			}
		}

		if (has_strut) {

			if (ps[PS_LEFT] > 0) {
				/* check if raw area intersect current viewport */
				rect b(0, ps[PS_LEFT_START_Y], ps[PS_LEFT],
						ps[PS_LEFT_END_Y] - ps[PS_LEFT_START_Y] + 1);
				rect x = raw_area & b;
				if (!x.is_null()) {
					margin_left = std::max(margin_left, ps[PS_LEFT]);
				}
			}

			if (ps[PS_RIGHT] > 0) {
				/* check if raw area intersect current viewport */
				rect b(_root_position.w - ps[PS_RIGHT],
						ps[PS_RIGHT_START_Y], ps[PS_RIGHT],
						ps[PS_RIGHT_END_Y] - ps[PS_RIGHT_START_Y] + 1);
				rect x = raw_area & b;
				if (!x.is_null()) {
					margin_right = std::max(margin_right, ps[PS_RIGHT]);
				}
			}

			if (ps[PS_TOP] > 0) {
				/* check if raw area intersect current viewport */
				rect b(ps[PS_TOP_START_X], 0,
						ps[PS_TOP_END_X] - ps[PS_TOP_START_X] + 1, ps[PS_TOP]);
				rect x = raw_area & b;
				if (!x.is_null()) {
					margin_top = std::max(margin_top, ps[PS_TOP]);
				}
			}

			if (ps[PS_BOTTOM] > 0) {
				/* check if raw area intersect current viewport */
				rect b(ps[PS_BOTTOM_START_X],
						_root_position.h - ps[PS_BOTTOM],
						ps[PS_BOTTOM_END_X] - ps[PS_BOTTOM_START_X] + 1,
						ps[PS_BOTTOM]);
				rect x = raw_area & b;
				if (!x.is_null()) {
					margin_bottom = std::max(margin_bottom, ps[PS_BOTTOM]);
				}
			}
		}
	}

	rect final_size;

	final_size.x = margin_left;
	final_size.w = _root_position.w - margin_right - margin_left;
	final_size.y = margin_top;
	final_size.h = _root_position.h - margin_bottom - margin_top;

	v->set_allocation(final_size);

}

/*
 * Reconfigure docks.
 */
void page_t::reconfigure_docks(workspace_t * d) {

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

	auto children = filter_class<client_managed_t>(d->tree_t::get_all_children());
	for(auto j: children) {

		if(not j->is(MANAGED_DOCK))
			continue;

		int32_t ps[12] = { 0 };
		bool has_strut{false};

		if(j->net_wm_strut_partial() != nullptr) {
			if(j->net_wm_strut_partial()->size() == 12) {
				std::copy(j->net_wm_strut_partial()->begin(), j->net_wm_strut_partial()->end(), &ps[0]);
				has_strut = true;
			}
		}

		if (j->net_wm_strut() != nullptr and not has_strut) {
			if(j->net_wm_strut()->size() == 4) {

				/** if strut is found, fake strut_partial **/

				std::copy(j->net_wm_strut()->begin(), j->net_wm_strut()->end(), &ps[0]);

				if(ps[PS_TOP] > 0) {
					ps[PS_TOP_START_X] = _root_position.x;
					ps[PS_TOP_END_X] = _root_position.x + _root_position.w;
				}

				if(ps[PS_BOTTOM] > 0) {
					ps[PS_BOTTOM_START_X] = _root_position.x;
					ps[PS_BOTTOM_END_X] = _root_position.x + _root_position.w;
				}

				if(ps[PS_LEFT] > 0) {
					ps[PS_LEFT_START_Y] = _root_position.y;
					ps[PS_LEFT_END_Y] = _root_position.y + _root_position.h;
				}

				if(ps[PS_RIGHT] > 0) {
					ps[PS_RIGHT_START_Y] = _root_position.y;
					ps[PS_RIGHT_END_Y] = _root_position.y + _root_position.h;
				}

				has_strut = true;
			}
		}

		if (has_strut) {

			if (ps[PS_LEFT] > 0) {
				rect pos;
				pos.x = 0;
				pos.y = ps[PS_LEFT_START_Y];
				pos.w = ps[PS_LEFT];
				pos.h = ps[PS_LEFT_END_Y] - ps[PS_LEFT_START_Y] + 1;
				j->set_floating_wished_position(pos);
				j->reconfigure();
				j->normalize();
				continue;
			}

			if (ps[PS_RIGHT] > 0) {
				rect pos;
				pos.x = _root_position.w - ps[PS_RIGHT];
				pos.y = ps[PS_RIGHT_START_Y];
				pos.w = ps[PS_RIGHT];
				pos.h = ps[PS_RIGHT_END_Y] - ps[PS_RIGHT_START_Y] + 1;
				j->set_floating_wished_position(pos);
				j->reconfigure();
				j->normalize();
				continue;
			}

			if (ps[PS_TOP] > 0) {
				rect pos;
				pos.x = ps[PS_TOP_START_X];
				pos.y = 0;
				pos.w = ps[PS_TOP_END_X] - ps[PS_TOP_START_X] + 1;
				pos.h = ps[PS_TOP];
				j->set_floating_wished_position(pos);
				j->reconfigure();
				j->normalize();
				continue;
			}

			if (ps[PS_BOTTOM] > 0) {
				rect pos;
				pos.x = ps[PS_BOTTOM_START_X];
				pos.y = _root_position.h - ps[PS_BOTTOM];
				pos.w = ps[PS_BOTTOM_END_X] - ps[PS_BOTTOM_START_X] + 1;
				pos.h = ps[PS_BOTTOM];
				j->set_floating_wished_position(pos);
				j->reconfigure();
				j->normalize();
				continue;
			}
		}
	}
}

void page_t::process_net_vm_state_client_message(xcb_window_t c, long type, xcb_atom_t state_properties) {
	if(state_properties == XCB_ATOM_NONE)
		return;

	/* debug print */
//	if(true) {
//		char const * action;
//		switch (type) {
//		case _NET_WM_STATE_REMOVE:
//			action = "remove";
//			break;
//		case _NET_WM_STATE_ADD:
//			action = "add";
//			break;
//		case _NET_WM_STATE_TOGGLE:
//			action = "toggle";
//			break;
//		default:
//			action = "invalid";
//			break;
//		}
//		std::cout << "_NET_WM_STATE: " << action << " "
//				<< cnx->get_atom_name(state_properties) << std::endl;
//	}

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
			update_workarea();
		} else if (state_properties == A(_NET_WM_STATE_HIDDEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE: {
				notebook_t * n = dynamic_cast<notebook_t*>(mw->parent());
				if (n != nullptr)
					mw->activate();
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
		} else if (state_properties == A(_NET_WM_STATE_DEMANDS_ATTENTION)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				mw->set_demands_attention(false);
				mw->queue_redraw();
				break;
			case _NET_WM_STATE_ADD:
				mw->set_demands_attention(true);
				mw->queue_redraw();
				break;
			case _NET_WM_STATE_TOGGLE:
				mw->set_demands_attention(not mw->demands_attention());
				mw->queue_redraw();
				break;
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
			update_workarea();
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
		} else if (state_properties == A(_NET_WM_STATE_DEMANDS_ATTENTION)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				mw->set_demands_attention(false);
				mw->queue_redraw();
				break;
			case _NET_WM_STATE_ADD:
				mw->set_demands_attention(true);
				mw->queue_redraw();
				break;
			case _NET_WM_STATE_TOGGLE:
				mw->set_demands_attention(not mw->demands_attention());
				mw->queue_redraw();
				break;
			default:
				break;
			}
		}
	} else if (mw->is(MANAGED_DOCK)) {

		if (state_properties == A(_NET_WM_STATE_FULLSCREEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				break;
			case _NET_WM_STATE_ADD:
				//fullscreen(mw);
				//update_desktop_visibility();
				break;
			case _NET_WM_STATE_TOGGLE:
				//toggle_fullscreen(mw);
				//update_desktop_visibility();
				break;
			}
			update_workarea();
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
		} else if (state_properties == A(_NET_WM_STATE_DEMANDS_ATTENTION)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				mw->set_demands_attention(false);
				mw->queue_redraw();
				break;
			case _NET_WM_STATE_ADD:
				mw->set_demands_attention(true);
				mw->queue_redraw();
				break;
			case _NET_WM_STATE_TOGGLE:
				mw->set_demands_attention(not mw->demands_attention());
				mw->queue_redraw();
				break;
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
			update_workarea();
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
		} else if (state_properties == A(_NET_WM_STATE_DEMANDS_ATTENTION)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				mw->set_demands_attention(false);
				break;
			case _NET_WM_STATE_ADD:
				mw->set_demands_attention(true);
				break;
			case _NET_WM_STATE_TOGGLE:
				mw->set_demands_attention(not mw->demands_attention());
				break;
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
	client_base_t * transient_for = get_transient_for(c);
	if (transient_for != nullptr) {
		transient_for->add_subclient(c);
	} else {
		if(find_current_desktop(c) == ALL_DESKTOP) {
			get_current_workspace()->add_floating_client(c);
			c->show();
		} else {
			_desktop_list[find_current_desktop(c)]->add_floating_client(c);
			if(_desktop_list[find_current_desktop(c)]->is_hidden()) {
				c->hide();
			} else {
				c->show();
			}
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
	c->activate();
	/** apply change **/
	_need_restack = true;
}


void page_t::fullscreen_client_to_viewport(client_managed_t * c, viewport_t * v) {
	if (has_key(_fullscreen_client_to_viewport, c)) {
		fullscreen_data_t & data = _fullscreen_client_to_viewport[c];
		if (v != data.viewport) {
			data.viewport = v;
			data.desktop = find_desktop_of(v);
			update_desktop_visibility();
		}
	}
}

void page_t::bind_window(client_managed_t * mw, bool activate) {
	/* update database */
	cout << "bind: " << mw->title() << endl;
	detach(mw);
	insert_window_in_notebook(mw, nullptr, activate);
	if(activate) {
		set_focus(mw, XCB_CURRENT_TIME);
	} else {
		safe_raise_window(mw);
	}
	_need_update_client_list = true;
}

void page_t::unbind_window(client_managed_t * mw) {
	detach(mw);
	/* update database */
	if(mw->is(MANAGED_DOCK))
		return;
	mw->set_managed_type(MANAGED_FLOATING);
	insert_in_tree_using_transient_for(mw);
	mw->queue_redraw();
	mw->normalize();
	safe_raise_window(mw);
	_need_update_client_list = true;
	_need_restack = true;
}

void page_t::cleanup_grab(client_managed_t * mw) {
	if(_grab_handler != nullptr) {
		delete _grab_handler;
		_grab_handler = nullptr;
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

std::vector<viewport_t *> page_t::get_viewports(tree_t * base) {
	if(base == nullptr)
		base = this;
	return filter_class<viewport_t>(base->get_all_children());
}

notebook_t * page_t::find_parent_notebook_for(client_managed_t * mw) {
	return dynamic_cast<notebook_t*>(mw->parent());
}

workspace_t * page_t::find_desktop_of(tree_t * n) {
	tree_t * x = n;
	while (x != nullptr) {
		if (typeid(*x) == typeid(workspace_t))
			return dynamic_cast<workspace_t *>(x);
		x = (x->parent());
	}
	return nullptr;
}

void page_t::update_windows_stack() {

	std::vector<tree_t *> tree = tree_t::get_visible_children();

	{
		/**
		 * only re-stack managed clients or unmanaged clients with transient_for
		 * or unmanaged window with net_wm_window_type.
		 * Other client are expected to re-stack them self (it's a guess).
		 **/
		int k = 0;
		for (int i = 0; i < tree.size(); ++i) {
			if (typeid(*tree[i]) == typeid(client_managed_t)) {
				tree[k++] = tree[i];
			} else if (typeid(*tree[i]) == typeid(client_not_managed_t)) {
				client_not_managed_t * nc =
						dynamic_cast<client_not_managed_t*>(tree[i]);
				if (nc->wm_transient_for() != nullptr) {
					tree[k++] = tree[i];
				} else if(nc->net_wm_window_type() != nullptr) {
					tree[k++] = tree[i];
				}
			}
		}
		tree.resize(k);
	}

	std::vector<client_base_t *> clients = filter_class<client_base_t>(tree);
	std::reverse(clients.begin(), clients.end());
	std::vector<xcb_window_t> stack;


	for(auto x: _overlays) {
		if(x->id() != XCB_WINDOW_NONE) {
			stack.push_back(x->id());
		}
	}

	/** place overlay on top **/
	if (rnd != nullptr)
		stack.push_back(rnd->get_composite_overlay());

	for (auto i : clients) {
		stack.push_back(i->base());
	}

	/** place page window at bottom **/
	//stack.push_back(rpage->wid());
	auto viewports = get_viewports();
	for(auto x: viewports) {
		if(x->is_visible()) {
			stack.push_back(x->wid());
		}
	}

	xcb_window_t win = XCB_WINDOW_NONE;

	unsigned k = 0;
	if (stack.size() > k) {
		uint32_t value[1];
		uint32_t mask{0};
		value[0] = XCB_STACK_MODE_ABOVE;
		mask |= XCB_CONFIG_WINDOW_STACK_MODE;
		xcb_configure_window(cnx->xcb(),
				stack[k], mask, value);
		++k;
	}

	while (stack.size() > k) {
		uint32_t value[2];
		uint32_t mask { 0 };

		value[0] = stack[k-1];
		mask |= XCB_CONFIG_WINDOW_SIBLING;

		value[1] = XCB_STACK_MODE_BELOW;
		mask |= XCB_CONFIG_WINDOW_STACK_MODE;

		xcb_configure_window(cnx->xcb(), stack[k], mask,
				value);
		++k;
	}

}

void page_t::update_viewport_layout() {
	_left_most_border = std::numeric_limits<int>::max();
	_top_most_border = std::numeric_limits<int>::max();

	/** update root size infos **/
	xcb_get_geometry_cookie_t ck0 = xcb_get_geometry(cnx->xcb(), cnx->root());
	xcb_randr_get_screen_resources_cookie_t ck1 = xcb_randr_get_screen_resources(cnx->xcb(), cnx->root());

	xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(cnx->xcb(), ck0, nullptr);
	xcb_randr_get_screen_resources_reply_t * randr_resources = xcb_randr_get_screen_resources_reply(cnx->xcb(), ck1, 0);

	if(geometry == nullptr or randr_resources == nullptr) {
		throw exception_t("FATAL: cannot read root window attributes");
	}

	_root_position = rect{geometry->x, geometry->y, geometry->width, geometry->height};
	set_desktop_geometry(_root_position.w, _root_position.h);

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

		/** keep left more screen to move iconnified window there **/
		if(r->x < _left_most_border) {
			_left_most_border = r->x;
		}

		if(r->y < _top_most_border) {
			_top_most_border = r->y;
		}

	}

	/* compute all viewport  that does not overlap and cover the full area of crts */
	region already_allocated;
	std::vector<rect> viewport_allocation;
	for(auto crtc: crtc_info) {
		if(crtc.second->num_outputs <= 0)
			continue;

		/* the location of crts */
		region location{crtc.second->x, crtc.second->y, crtc.second->width, crtc.second->height};
		location -= already_allocated;
		for(auto & b: location) {
			viewport_allocation.push_back(b);
		}
		already_allocated += location;
	}

	for(auto d: _desktop_list) {
		/** get old layout to recycle old viewport, and keep unchanged outputs **/
		std::vector<viewport_t *> old_layout{d->get_viewport_map()};
		/** store the newer layout, to be able to cleanup obsolete viewports **/
		std::vector<viewport_t *> new_layout;
		/** for each not overlaped rectangle **/
		for(unsigned i = 0; i < viewport_allocation.size(); ++i) {
			printf("%d: found viewport (%d,%d,%d,%d)\n", d->id(), viewport_allocation[i].x, viewport_allocation[i].y, viewport_allocation[i].w, viewport_allocation[i].h);
			viewport_t * vp;
			if(i < old_layout.size()) {
				vp = old_layout[i];
				vp->set_raw_area(viewport_allocation[i]);
			} else {
				vp = new viewport_t{this, viewport_allocation[i], _auto_refocus};
				vp->set_parent(d);
			}
			compute_viewport_allocation(d, vp);
			new_layout.push_back(vp);
		}

		/** if no layout is found fallback to on screen **/
		if(new_layout.size() < 1) {
			rect area{_root_position};
			viewport_t * vp;
			if(0 < old_layout.size()) {
				vp = old_layout[0];
				vp->set_raw_area(area);
			} else {
				vp = new viewport_t{this, area, _auto_refocus};
				vp->set_parent(d);
			}
			compute_viewport_allocation(d, vp);
			new_layout.push_back(vp);
		}

		if(new_layout.size() > 0) {
			d->set_primary_viewport(new_layout[0]);
		}

		d->set_layout(new_layout);
		d->update_default_pop();

		/** clean up obsolete layout **/
		for (unsigned i = new_layout.size(); i < old_layout.size(); ++i) {
			/** destroy this viewport **/
			remove_viewport(d, old_layout[i]);
			destroy_viewport(old_layout[i]);
		}

		if(new_layout.size() > 0) {
			/** update position of floating managed clients to avoid offscreen floating window**/
			for(auto x: filter_class<client_managed_t>(tree_t::get_all_children())) {
				if(x->is(MANAGED_FLOATING)) {
					auto r = x->position();
					r.x = new_layout[0]->allocation().x + _theme->floating.margin.left;
					r.y = new_layout[0]->allocation().y + _theme->floating.margin.top;
					x->set_floating_wished_position(r);
					x->reconfigure();
				}
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

	/* set viewport */
	std::vector<uint32_t> viewport(_desktop_list.size()*2);
	std::fill_n(viewport.begin(), _desktop_list.size()*2, 0);
	cnx->change_property(cnx->root(), _NET_DESKTOP_VIEWPORT,
			CARDINAL, 32, &viewport[0], _desktop_list.size()*2);

	/* define desktop geometry */
	set_desktop_geometry(_root_position.w, _root_position.h);

	update_workarea();
	reconfigure_docks(_desktop_list[_current_desktop]);

}

void page_t::remove_viewport(workspace_t * d, viewport_t * v) {

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
			d->default_pop()->add_client(c, false);
		}
	}

}

void page_t::destroy_viewport(viewport_t * v) {
	for (auto i : v->get_all_children()) {
		client_base_t * cb = dynamic_cast<client_base_t*>(i);
		if (cb == nullptr) {
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
	if(rnd != nullptr)
		if (w == rnd->get_composite_overlay())
			return;
	if(w == cnx->root())
		return;
	auto viewports = get_viewports();
	for(auto x: viewports) {
		if(x->wid() == w)
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
		printf("do not manage %u because it will be destoyed\n", w);
		cnx->ungrab();
		return;
	}

	if(cnx->check_for_unmap_window(w)) {
		printf("do not manage %u because it will be unmapped\n", w);
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
		_need_restack = true;

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

				//props->print_window_attributes();
				//props->print_properties();

				xcb_atom_t type = props->wm_type();

				if (not props->wa()->override_redirect) {
					if (type == A(_NET_WM_WINDOW_TYPE_DESKTOP)) {
						create_managed_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_DOCK)) {
						create_managed_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLBAR)) {
						create_managed_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_MENU)) {
						create_managed_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_UTILITY)) {
						create_managed_window(props, type);
					} else if (type == A(_NET_WM_WINDOW_TYPE_SPLASH)) {
						create_managed_window(props, type);
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
						create_managed_window(props, type);
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
		auto mw = new client_managed_t{this, type, c};
		cmgr->register_window(mw->base());
		_clients_list.push_back(mw);
		manage_client(mw, type);

		if(mw->net_wm_strut() != nullptr or mw->net_wm_strut_partial() != nullptr) {
			update_workarea();
		}

	} catch (...) {
		printf("Error while creating managed window\n");
		throw;
	}
}

void page_t::manage_client(client_managed_t * mw, xcb_atom_t type) {

	try {

		safe_update_transient_for(mw);
		mw->activate();
		_need_update_client_list = true;
		_need_restack = true;

		/* find the desktop for this window */
		{
			unsigned int const * desktop = mw->net_wm_desktop();
			if(desktop != nullptr) {
				if(*desktop >= _desktop_list.size()) {
					mw->set_current_desktop(ALL_DESKTOP);
					mw->net_wm_state_add(_NET_WM_STATE_STICKY);
				}
			} else {
				if(mw->wm_transient_for() != nullptr) {
					auto parent = dynamic_cast<client_managed_t*>(find_client_with(*(mw->wm_transient_for())));
					if(parent != nullptr) {
						mw->set_current_desktop(find_current_desktop(parent));
					} else {
						if(mw->is_stiky()) {
							mw->set_current_desktop(ALL_DESKTOP);
						} else {
							mw->set_current_desktop(_current_desktop);
						}
					}
				}
			}
		}

		if(find_current_desktop(mw) == ALL_DESKTOP) {
			mw->show();
		} else if(not _desktop_list[find_current_desktop(mw)]->is_hidden()) {
			mw->show();
		} else {
			mw->hide();
		}

		/* HACK OLD FASHION FULLSCREEN */
		if (mw->wm_normal_hints() != nullptr and mw->wm_type() == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
			XSizeHints const * size_hints = mw->wm_normal_hints();
			if ((size_hints->flags & PMaxSize)
					and (size_hints->flags & PMinSize)) {
				if (size_hints->min_width == _root_position.w
						and size_hints->min_height == _root_position.h
						and size_hints->max_width == _root_position.w
						and size_hints->max_height == _root_position.h) {
					mw->net_wm_state_add(_NET_WM_STATE_FULLSCREEN);
				}
			}
		}

		/**
		 * Here client that default to fullscreen
		 **/
		if (mw->is_fullscreen()) {
			mw->normalize();
			fullscreen(mw);
			update_desktop_visibility();
			mw->reconfigure();
			mw->activate();
			xcb_timestamp_t time = 0;
			if (get_safe_net_wm_user_time(mw, time)) {
				set_focus(mw, time);
			} else {
				set_focus(mw, XCB_CURRENT_TIME);
			}

			/**
			 * Here client that default to notebook
			 **/
		} else if (((type == A(_NET_WM_WINDOW_TYPE_NORMAL))
				//or type == A(_NET_WM_WINDOW_TYPE_DOCK))
				and get_transient_for(mw) == nullptr
				//and mw->has_motif_border()
				and not mw->is_modal())
				or type == A(_NET_WM_WINDOW_TYPE_DESKTOP)) {

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
				if(has_key(*mw->net_wm_state(), A(_NET_WM_STATE_HIDDEN))) {
					activate = false;
				}
			}

			if(not mw->is(MANAGED_DOCK)) {
				bind_window(mw, activate);
			} else {
				mw->net_wm_state_add(_NET_WM_STATE_STICKY);
			}
			mw->reconfigure();

			/**
			 * Here client that default to floating
			 **/
		} else {
			mw->normalize();
			mw->reconfigure();
			mw->activate();
			xcb_timestamp_t time = 0;
			if (get_safe_net_wm_user_time(mw, time)) {
				set_focus(mw, time);
			} else {
				set_focus(mw, XCB_CURRENT_TIME);
			}
		}
	} catch (exception_t & e) {
		cout << e.what() << endl;
	} catch (...) {
		printf("Unknown exception while creating managed window\n");
		throw;
	}
}

void page_t::create_unmanaged_window(std::shared_ptr<client_properties_t> c, xcb_atom_t type) {
	try {
		cmgr->register_window(c->id());
		auto uw = new client_not_managed_t{this, type, c};
		if(not uw->wa()->override_redirect)
			uw->map();
		safe_update_transient_for(uw);
		add_compositor_damaged(uw->visible_area());
	} catch (exception_t & e) {
		cout << e.what() << endl;
	} catch (...) {
		printf("Error while creating managed window\n");
		throw;
	}
}

void page_t::create_dock_window(std::shared_ptr<client_properties_t> c, xcb_atom_t type) {
	cmgr->register_window(c->id());
	auto uw = new client_not_managed_t{this, type, c};
	uw->map();
	attach_dock(uw);
	safe_raise_window(uw);
	update_workarea();
}

viewport_t * page_t::find_mouse_viewport(int x, int y) const {
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
}

void page_t::safe_update_transient_for(client_base_t * c) {
	if (typeid(*c) == typeid(client_managed_t)) {
		client_managed_t * mw = dynamic_cast<client_managed_t*>(c);
		if (mw->is(MANAGED_FLOATING)) {
			detach(mw);
			insert_in_tree_using_transient_for(mw);
		} else if (mw->is(MANAGED_NOTEBOOK)) {
			/* DO NOTHING */
		} else if (mw->is(MANAGED_FULLSCREEN)) {
			/* DO NOTHING */
		} else if (mw->is(MANAGED_DOCK)) {
			detach(mw);
			insert_in_tree_using_transient_for(mw);
		}
	} else if (typeid(*c) == typeid(client_not_managed_t)) {
		client_not_managed_t * uw = dynamic_cast<client_not_managed_t*>(c);
		detach(uw);
		if (uw->wm_type() == A(_NET_WM_WINDOW_TYPE_TOOLTIP)) {
			tooltips.push_back(uw);
			uw->set_parent(this);
		} else if (uw->wm_type() == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
			notifications.push_back(uw);
			uw->set_parent(this);
		} else if (uw->wm_type() == A(_NET_WM_WINDOW_TYPE_DOCK)) {
			attach_dock(uw);
		} else if (uw->net_wm_state() != nullptr
				and has_key<xcb_atom_t>(*(uw->net_wm_state()), A(_NET_WM_STATE_ABOVE))) {
			above.push_back(uw);
			uw->set_parent(this);
		} else if (uw->net_wm_state() != nullptr
				and has_key(*(uw->net_wm_state()), static_cast<xcb_atom_t>(A(_NET_WM_STATE_BELOW)))) {
			below.push_back(uw);
			uw->set_parent(this);
		} else {
			insert_in_tree_using_transient_for(uw);
		}
	}
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
		}
	}
	detach(c);
	for(auto i: c->tree_t::children()) {
		client_base_t * c = dynamic_cast<client_base_t *>(i);
		if(c != nullptr) {
			insert_in_tree_using_transient_for(c);
		}
	}
	cmgr->unregister_window(c->base());
	delete c;
}

std::string page_t::get_node_name() const {
	return _get_node_name<'P'>();
}

void page_t::replace(page_component_t * src, page_component_t * by) {
	printf("Unexpectected use of page::replace function\n");
}

void page_t::activate(tree_t * t) {

	if(t == nullptr)
		return;

	workspace_t * w = dynamic_cast<workspace_t*>(t);
	if(w != nullptr) {
		switch_to_desktop(w->id());
	}

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
	above.remove(dynamic_cast<client_not_managed_t*>(t));
	below.remove(dynamic_cast<client_not_managed_t*>(t));

}

void page_t::create_identity_window() {
	/* create an invisible window to identify page */
	uint32_t pid = getpid();
	uint32_t attrs[2];

	/* OVERRIDE_REDIRECT */
	attrs[0] = 1;
	/* EVENT_MASK */
	attrs[1] = XCB_EVENT_MASK_STRUCTURE_NOTIFY|XCB_EVENT_MASK_PROPERTY_CHANGE;

	uint32_t attrs_mask = XCB_CW_OVERRIDE_REDIRECT|XCB_CW_EVENT_MASK;

	/* Warning: This window must be focusable, thus it MUST be an INPUT_OUTPUT window */
	identity_window = xcb_generate_id(cnx->xcb());
	xcb_void_cookie_t ck = xcb_create_window(cnx->xcb(), XCB_COPY_FROM_PARENT, identity_window,
			cnx->root(), -100, -100, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			XCB_COPY_FROM_PARENT, attrs_mask, attrs);

	std::string name{"page"};
	cnx->change_property(identity_window, _NET_WM_NAME, UTF8_STRING, 8, name.c_str(),
			name.length() + 1);
	cnx->change_property(identity_window, _NET_SUPPORTING_WM_CHECK, WINDOW, 32,
			&identity_window, 1);
	cnx->change_property(identity_window, _NET_WM_PID, CARDINAL, 32, &pid, 1);
	cnx->map(identity_window);
}

void page_t::register_wm() {
	if (!cnx->register_wm(identity_window, replace_wm)) {
		throw exception_t("Cannot register window manager");
	}
}

void page_t::register_cm() {
	if (!cnx->register_cm(identity_window)) {
		throw exception_t("Cannot register composite manager");
	}
}

void grab_key(xcb_connection_t * xcb, xcb_window_t w, key_desc_t & key, keymap_t * _keymap) {
	int kc = 0;
	if ((kc = _keymap->find_keysim(key.ks))) {
		xcb_grab_key(xcb, true, w, key.mod, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		if(_keymap->numlock_mod_mask() != 0) {
			xcb_grab_key(xcb, true, w, key.mod| _keymap->numlock_mod_mask(), kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
		}
	}
}

/**
 * Update grab keys aware of current _keymap
 */
void page_t::update_grabkey() {

	assert(_keymap != nullptr);

	/** ungrab all previews key **/
	xcb_ungrab_key(cnx->xcb(), XCB_GRAB_ANY, cnx->root(), XCB_MOD_MASK_ANY);

	int kc = 0;

	grab_key(cnx->xcb(), cnx->root(), bind_debug_1, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_debug_2, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_debug_3, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_debug_4, _keymap);

	grab_key(cnx->xcb(), cnx->root(), bind_cmd[0].key, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_cmd[1].key, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_cmd[2].key, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_cmd[3].key, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_cmd[4].key, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_cmd[5].key, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_cmd[6].key, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_cmd[7].key, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_cmd[8].key, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_cmd[9].key, _keymap);

	grab_key(cnx->xcb(), cnx->root(), bind_page_quit, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_close, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_exposay_all, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_toggle_fullscreen, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_toggle_compositor, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_right_desktop, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_left_desktop, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_bind_window, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_fullscreen_window, _keymap);
	grab_key(cnx->xcb(), cnx->root(), bind_float_window, _keymap);

	/* Alt-Tab */
	if ((kc = _keymap->find_keysim(XK_Tab))) {
		xcb_grab_key(cnx->xcb(), true, cnx->root(), XCB_MOD_MASK_1, kc,
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
		if (_keymap->numlock_mod_mask() != 0) {
			xcb_grab_key(cnx->xcb(), true, cnx->root(),
					XCB_MOD_MASK_1 | _keymap->numlock_mod_mask(), kc,
					XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
		}
	}


}

void page_t::update_keymap() {
	if(_keymap != nullptr) {
		delete _keymap;
	}
	_keymap = new keymap_t(cnx->xcb());
}

void page_t::prepare_render(std::vector<std::shared_ptr<renderable_t>> & out, page::time_t const & time) {
	out.push_back(_theme->get_background(0,0));
	for(auto i: tree_t::children()) {
		i->prepare_render(out, time);
	}
}

page_component_t * page_t::parent() const {
	return nullptr;
}

void page_t::set_parent(tree_t * parent) {
	throw exception_t("page::page_t can't have parent nor tree_t parent");
}

void page_t::set_allocation(rect const & r) {
	throw exception_t("page_t::set_allocation should be called");
}

void page_t::children(std::vector<tree_t *> & out) const {
	for (auto i: _desktop_stack) {
		out.push_back(i);
	}

	for(auto x: below) {
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

void page_t::switch_to_desktop(unsigned int desktop) {
	if (desktop != _current_desktop and desktop != ALL_DESKTOP) {
		std::cout << "switch to desktop #" << desktop << std::endl;

		auto stiky_list = get_sticky_client_managed(_desktop_list[_current_desktop]);

		_current_desktop = desktop;
		_desktop_stack.remove(_desktop_list[_current_desktop]);
		_desktop_stack.push_back(_desktop_list[_current_desktop]);

		/** move stiky to current desktop **/
		for(auto s : stiky_list) {
			detach(s);
			insert_in_tree_using_transient_for(s);
		}

		update_viewport_layout();
		update_current_desktop();
		update_desktop_visibility();
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

void page_t::_event_handler_bind(int type, callback_event_t f) {
	_event_handlers[type] = f;
}

void page_t::_bind_all_default_event() {
	_event_handlers.clear();

	_event_handler_bind(XCB_BUTTON_PRESS, &page_t::process_button_press_event);
	_event_handler_bind(XCB_BUTTON_RELEASE, &page_t::process_button_release);
	_event_handler_bind(XCB_MOTION_NOTIFY, &page_t::process_motion_notify);
	_event_handler_bind(XCB_KEY_PRESS, &page_t::process_key_press_event);
	_event_handler_bind(XCB_KEY_RELEASE, &page_t::process_key_release_event);
	_event_handler_bind(XCB_CONFIGURE_NOTIFY, &page_t::process_configure_notify_event);
	_event_handler_bind(XCB_CREATE_NOTIFY, &page_t::process_create_notify_event);
	_event_handler_bind(XCB_DESTROY_NOTIFY, &page_t::process_destroy_notify_event);
	_event_handler_bind(XCB_GRAVITY_NOTIFY, &page_t::process_gravity_notify_event);
	_event_handler_bind(XCB_MAP_NOTIFY, &page_t::process_map_notify_event);
	_event_handler_bind(XCB_REPARENT_NOTIFY, &page_t::process_reparent_notify_event);
	_event_handler_bind(XCB_UNMAP_NOTIFY, &page_t::process_unmap_notify_event);
	//_event_handler_bind(XCB_CIRCULATE_NOTIFY, &page_t::process_circulate_notify_event);
	_event_handler_bind(XCB_CONFIGURE_REQUEST, &page_t::process_configure_request_event);
	_event_handler_bind(XCB_MAP_REQUEST, &page_t::process_map_request_event);
	_event_handler_bind(XCB_MAPPING_NOTIFY, &page_t::process_mapping_notify_event);
	_event_handler_bind(XCB_SELECTION_CLEAR, &page_t::process_selection_clear_event);
	_event_handler_bind(XCB_PROPERTY_NOTIFY, &page_t::process_property_notify_event);
	_event_handler_bind(XCB_EXPOSE, &page_t::process_expose_event);
	_event_handler_bind(XCB_FOCUS_IN, &page_t::process_focus_in_event);
	_event_handler_bind(XCB_FOCUS_OUT, &page_t::process_focus_out_event);
	_event_handler_bind(XCB_ENTER_NOTIFY, &page_t::process_enter_window_event);
	_event_handler_bind(XCB_LEAVE_NOTIFY, &page_t::process_leave_window_event);


	_event_handler_bind(0, &page_t::process_error);

	_event_handler_bind(XCB_UNMAP_NOTIFY|0x80, &page_t::process_fake_unmap_notify_event);
	_event_handler_bind(XCB_CLIENT_MESSAGE|0x80, &page_t::process_fake_client_message_event);

	/** Extension **/
	_event_handler_bind(cnx->damage_event + XCB_DAMAGE_NOTIFY, &page_t::process_damage_notify_event);
	_event_handler_bind(cnx->randr_event + XCB_RANDR_NOTIFY, &page_t::process_randr_notify_event);
	_event_handler_bind(cnx->shape_event + XCB_SHAPE_NOTIFY, &page_t::process_shape_notify_event);

}


void page_t::process_mapping_notify_event(xcb_generic_event_t const * e) {
	update_keymap();
	update_grabkey();
}

void page_t::process_selection_clear_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_selection_clear_event_t const *>(_e);
	if(e->selection == cnx->wm_sn_atom)
		stop();
	if(e->selection == cnx->cm_sn_atom)
		stop_compositor();
}

void page_t::process_focus_in_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_focus_in_event_t const *>(_e);

//	std::cout << "Focus IN 0x" << format("08x", e->event) << " ";
//
//	switch(e->mode) {
//	case XCB_NOTIFY_MODE_GRAB:
//		std::cout << "MODE_GRAB";
//		break;
//	case XCB_NOTIFY_MODE_NORMAL:
//		std::cout << "MODE_NORMAL";
//		break;
//	case XCB_NOTIFY_MODE_UNGRAB:
//		std::cout << "MODE_UNGRAB";
//		break;
//	case XCB_NOTIFY_MODE_WHILE_GRABBED:
//		std::cout << "MODE_WHILE_GRABBED";
//		break;
//	default:
//		std::cout << "MODE_UNKNOWN";
//	}
//
//	std::cout << " ";
//
//	switch(e->detail) {
//	case XCB_NOTIFY_DETAIL_ANCESTOR:
//		std::cout << "DETAIL_ANCESTOR";
//		break;
//	case XCB_NOTIFY_DETAIL_INFERIOR:
//		std::cout << "DETAIL_INFERIOR";
//		break;
//	case XCB_NOTIFY_DETAIL_NONE:
//		std::cout << "DETAIL_NONE";
//		break;
//	case XCB_NOTIFY_DETAIL_NONLINEAR:
//		std::cout << "DETAIL_NONLINEAR";
//		break;
//	case XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL:
//		std::cout << "DETAIL_NONLINEAR_VIRTUAL";
//		break;
//	case XCB_NOTIFY_DETAIL_POINTER:
//		std::cout << "DETAIL_POINTER";
//		break;
//	case XCB_NOTIFY_DETAIL_POINTER_ROOT:
//		std::cout << "DETAIL_POINTER_ROOT";
//		break;
//	case XCB_NOTIFY_DETAIL_VIRTUAL:
//		std::cout << "DETAIL_ANCESTOR";
//		break;
//	}
//
//	std::cout << std::endl;


	/* ignore all focus event related to the pointer */
	if(e->detail == XCB_NOTIFY_DETAIL_POINTER)
		return;

	if (e->event == cnx->root() and e->detail == XCB_NOTIFY_DETAIL_NONE) {
		cnx->set_input_focus(identity_window, XCB_INPUT_FOCUS_NONE, XCB_CURRENT_TIME);
	}

}

void page_t::process_focus_out_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_focus_in_event_t const *>(_e);


//	std::cout << "Focus OUT 0x" << format("08x", e->event) << " ";
//
//	switch(e->mode) {
//	case XCB_NOTIFY_MODE_GRAB:
//		std::cout << "MODE_GRAB";
//		break;
//	case XCB_NOTIFY_MODE_NORMAL:
//		std::cout << "MODE_NORMAL";
//		break;
//	case XCB_NOTIFY_MODE_UNGRAB:
//		std::cout << "MODE_UNGRAB";
//		break;
//	case XCB_NOTIFY_MODE_WHILE_GRABBED:
//		std::cout << "MODE_WHILE_GRABBED";
//		break;
//	default:
//		std::cout << "MODE_UNKNOWN";
//	}
//
//	std::cout << " ";
//
//	switch(e->detail) {
//	case XCB_NOTIFY_DETAIL_ANCESTOR:
//		std::cout << "DETAIL_ANCESTOR";
//		break;
//	case XCB_NOTIFY_DETAIL_INFERIOR:
//		std::cout << "DETAIL_INFERIOR";
//		break;
//	case XCB_NOTIFY_DETAIL_NONE:
//		std::cout << "DETAIL_NONE";
//		break;
//	case XCB_NOTIFY_DETAIL_NONLINEAR:
//		std::cout << "DETAIL_NONLINEAR";
//		break;
//	case XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL:
//		std::cout << "DETAIL_NONLINEAR_VIRTUAL";
//		break;
//	case XCB_NOTIFY_DETAIL_POINTER:
//		std::cout << "DETAIL_POINTER";
//		break;
//	case XCB_NOTIFY_DETAIL_POINTER_ROOT:
//		std::cout << "DETAIL_POINTER_ROOT";
//		break;
//	case XCB_NOTIFY_DETAIL_VIRTUAL:
//		std::cout << "DETAIL_ANCESTOR";
//		break;
//	}
//
//	std::cout << std::endl;

	/* ignore all focus event related to the pointer */
	if(e->detail == XCB_NOTIFY_DETAIL_POINTER)
		return;
	/* ignore all focus due to grabs */
	if(e->mode != XCB_NOTIFY_MODE_NORMAL)
		return;

	if (_desktop_list[_current_desktop]->client_focus.front() == nullptr) {
		if (e->event == identity_window or e->event == cnx->root()) {
			cnx->set_input_focus(identity_window, XCB_INPUT_FOCUS_NONE,
					XCB_CURRENT_TIME);
		}
	} else {
//		auto focus = _desktop_list[_current_desktop]->client_focus.front();
//		/* only inferior focus is allowed */
//		if(e->detail != XCB_NOTIFY_DETAIL_INFERIOR and e->event == focus->orig()) {
//			focus->focus(XCB_CURRENT_TIME);
//		}
	}

}

void page_t::process_enter_window_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_enter_notify_event_t const *>(_e);
	broadcast_enter(e);

	if(not _mouse_focus)
		return;

	client_managed_t * mw = find_managed_window_with(e->event);
	if(mw != nullptr) {
		set_focus(mw, e->time);
	}
}

void page_t::process_leave_window_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_leave_notify_event_t const *>(_e);
	broadcast_leave(e);
}

void page_t::process_randr_notify_event(xcb_generic_event_t const * e) {
	auto ev = reinterpret_cast<xcb_randr_notify_event_t const *>(e);

	//		char const * s_subtype = "Unknown";
	//
	//		switch(ev->subCode) {
	//		case XCB_RANDR_NOTIFY_CRTC_CHANGE:
	//			s_subtype = "RRNotify_CrtcChange";
	//			break;
	//		case XCB_RANDR_NOTIFY_OUTPUT_CHANGE:
	//			s_subtype = "RRNotify_OutputChange";
	//			break;
	//		case XCB_RANDR_NOTIFY_OUTPUT_PROPERTY:
	//			s_subtype = "RRNotify_OutputProperty";
	//			break;
	//		case XCB_RANDR_NOTIFY_PROVIDER_CHANGE:
	//			s_subtype = "RRNotify_ProviderChange";
	//			break;
	//		case XCB_RANDR_NOTIFY_PROVIDER_PROPERTY:
	//			s_subtype = "RRNotify_ProviderProperty";
	//			break;
	//		case XCB_RANDR_NOTIFY_RESOURCE_CHANGE:
	//			s_subtype = "RRNotify_ResourceChange";
	//			break;
	//		default:
	//			break;
	//		}

	if (ev->subCode == XCB_RANDR_NOTIFY_CRTC_CHANGE) {
		update_viewport_layout();
		if(rnd != nullptr)
			rnd->update_layout();
		_theme->update();
	}

	xcb_grab_button(cnx->xcb(), false, cnx->root(), DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE,
			XCB_BUTTON_INDEX_ANY, XCB_MOD_MASK_ANY);

	/** put rpage behind all managed windows **/
	_need_restack = true;

}

void page_t::process_shape_notify_event(xcb_generic_event_t const * e) {
	auto se = reinterpret_cast<xcb_shape_notify_event_t const *>(e);
	if (se->shape_kind == XCB_SHAPE_SK_BOUNDING) {
		xcb_window_t w = se->affected_window;
		client_base_t * c = find_client(w);
		if (c != nullptr) {
			c->update_shape();
		}
	}
}

void page_t::process_motion_notify(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_motion_notify_event_t const *>(_e);
	if(_grab_handler != nullptr) {
		_grab_handler->button_motion(e);
		return;
	} else {
		broadcast_button_motion(e);
	}
}

void page_t::process_button_release(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_button_release_event_t const *>(_e);
	if(_grab_handler != nullptr) {
		_grab_handler->button_release(e);
		return;
	} else {
		broadcast_button_release(e);
	}
}

void page_t::add_compositor_damaged(region const & r) {
	if(rnd != nullptr) {
		rnd->add_damaged(r);
	}
}

void page_t::start_compositor() {
#ifdef WITH_COMPOSITOR
	if (cnx->has_composite) {
		try {
			register_cm();
		} catch (std::exception & e) {
			std::cout << e.what() << std::endl;
			return;
		}
		rnd = new compositor_t { cnx, cmgr };
		cmgr->enable();
	}
#endif
}

void page_t::stop_compositor() {
#ifdef WITH_COMPOSITOR
	if (cnx->has_composite) {
		cnx->unregister_cm();
		cmgr->disable();
		delete rnd;
		rnd = nullptr;
	}
#endif
}

void page_t::process_expose_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_expose_event_t const *>(_e);

	broadcast_expose(e);

	for(auto x: _overlays) {
		if(x->id() == e->window) {
			x->expose();
		}
	}
}

void page_t::process_error(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_generic_error_t const *>(_e);
	cnx->print_error(e);
}


/* Inspired from openbox */
void page_t::run_cmd(std::string const & cmd_with_args)
{
	printf("executing %s\n", cmd_with_args.c_str());

    GError *e;
    gchar **argv = NULL;
    gchar *cmd;

    if (cmd_with_args == "null")
    	return;

    cmd = g_filename_from_utf8(cmd_with_args.c_str(), -1, NULL, NULL, NULL);
    if (!cmd) {
        printf("Failed to convert the path \"%s\" from utf8\n", cmd_with_args.c_str());
        return;
    }

    e = NULL;
    if (!g_shell_parse_argv(cmd, NULL, &argv, &e)) {
        printf("%s\n", e->message);
        g_error_free(e);
    } else {
        gchar *program = NULL;
        gboolean ok;

        e = NULL;
        ok = g_spawn_async(NULL, argv, NULL,
                           (GSpawnFlags)(G_SPAWN_SEARCH_PATH |
                           G_SPAWN_DO_NOT_REAP_CHILD),
                           NULL, NULL, NULL, &e);
        if (!ok) {
            printf("%s\n", e->message);
            g_error_free(e);
        }

        g_free(program);
        g_strfreev(argv);
    }

    g_free(cmd);

    return;
}

std::vector<client_managed_t *> page_t::get_sticky_client_managed(tree_t * base) {
	std::vector<client_managed_t *> ret;
	for(auto c: base->get_all_children()) {
		client_managed_t * mw = dynamic_cast<client_managed_t *>(c);
		if(mw != nullptr) {
			if(mw->is_stiky() and (not mw->is(MANAGED_NOTEBOOK)) and (not mw->is(MANAGED_FULLSCREEN)))
				ret.push_back(mw);
		}
	}

	return ret;
}

viewport_t * page_t::find_viewport_of(tree_t * t) {
	while(t != nullptr) {
		viewport_t * ret = dynamic_cast<viewport_t*>(t);
		if(ret != nullptr)
			return ret;
		t = t->parent();
	}

	return nullptr;
}

unsigned int page_t::find_current_desktop(client_base_t * c) {
	if(c->net_wm_desktop() != nullptr)
		return *(c->net_wm_desktop());
	auto d = find_desktop_of(c);
	if(d != nullptr)
		return d->id();
	return _current_desktop;
}

void page_t::process_pending_events() {
	/** Here we try to process all pending events then render if needed **/
	cnx->grab();

	while (cnx->has_pending_events()) {
		while (cnx->has_pending_events()) {
			cmgr->pre_process_event(cnx->front_event());
			process_event(cnx->front_event());
			cnx->pop_event();
			xcb_flush(cnx->xcb());
		}

		if (_need_restack) {
			_need_restack = false;
			update_windows_stack();
		}

		if(_need_update_client_list) {
			_need_update_client_list = false;
			update_client_list();
		}

		cnx->sync();

	}

	cmgr->apply_updates();

	/* redraw page component that need a redraw */
	broadcast_trigger_redraw();

	cnx->ungrab();
	xcb_flush(cnx->xcb());

	if (_need_render) {
		render();
	}

}

bool page_t::render_timeout() {
	_need_render = true;
	process_pending_events();
	return true;
}

theme_t const * page_t::theme() const {
	return _theme;
}

composite_surface_manager_t * page_t::csm() const {
	return cmgr;
}

display_t * page_t::dpy() const {
	return cnx;
}

compositor_t * page_t::cmp() const {
	return rnd;
}

void page_t::grab_start(grab_handler_t * handler) {
	assert(_grab_handler == nullptr);
	_grab_handler = handler;
}

void page_t::grab_stop() {
	assert(_grab_handler != nullptr);
	delete _grab_handler;
	_grab_handler = nullptr;
}

void page_t::overlay_add(std::shared_ptr<overlay_t> x) {
	_overlays.push_back(x);
}

void page_t::overlay_remove(std::shared_ptr<overlay_t> x) {
	_overlays.remove(x);
}

void page_t::add_global_damage(region const & r) {
	_need_render = true;
	add_compositor_damaged(r);
}



workspace_t * page_t::get_current_workspace() const {
	return _desktop_list[_current_desktop];
}

workspace_t * page_t::get_workspace(int id) const {
	return _desktop_list[id];
}

int page_t::get_workspace_count() const {
	return _desktop_list.size();
}

int page_t::left_most_border() {
	return _left_most_border;
}
int page_t::top_most_border() {
	return _top_most_border;
}

std::list<client_managed_t *> page_t::global_client_focus_history() {
	return _global_client_focus_history;
}

std::list<client_managed_t *> page_t::clients_list() {
	return _clients_list;
}

keymap_t const * page_t::keymap() const {
	return _keymap;
}

bool page_t::menu_drop_down_shadow() const {
	return _menu_drop_down_shadow;
}

}

