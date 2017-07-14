/*
 * page.cxx
 *
 * copyright (2010-2015) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

/* According to POSIX.1-2001 */
#include <sys/select.h>
#include <poll.h>

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

#include "page-types.hxx"
#include "renderable.hxx"
#include "renderable_fadeout_pixmap.hxx"
#include "key_desc.hxx"
#include "time.hxx"
#include "atoms.hxx"
#include "client_managed.hxx"
#include "grab_handlers.hxx"

#include "simple2_theme.hxx"
#include "tiny_theme.hxx"

#include "notebook.hxx"
#include "workspace.hxx"
#include "split.hxx"
#include "page.hxx"
#include "view.hxx"
#include "view_fullscreen.hxx"
#include "view_notebook.hxx"
#include "view_floating.hxx"
#include "view_dock.hxx"
#include "view_popup.hxx"

#include "popup_alt_tab.hxx"

/* ICCCM definition */
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

namespace page {

time64_t const page_t::default_wait{1000000000L / 120L};
bool mainloop_t::got_sigterm = false;


page_t::page_t(int argc, char ** argv)
{
	frame_alarm = 0;
	_current_workspace = 0;
	_grab_handler = nullptr;
	_schedule_repaint = false;

	identity_window = XCB_NONE;

	char const * conf_file_name = 0;

	configuration._replace_wm = false;

	_need_restack = false;
	_need_update_client_list = false;

	configuration._menu_drop_down_shadow = false;

	/** parse command line **/

	int k = 1;
	while(k < argc) {
		string x = argv[k];
		if(x == "--replace") {
			configuration._replace_wm = true;
		} else {
			conf_file_name = argv[k];
		}
		++k;
	}

	_keymap = nullptr;
	_dpy = nullptr;
	_compositor = nullptr;

	/* load configurations, from lower priority to high one */

	/* load default configuration */
	_conf.merge_from_file_if_exist(string{DATA_DIR "/page/page.conf"});

	/* load homedir configuration */
	{
		char const * chome = getenv("HOME");
		if(chome != nullptr) {
			string xhome = chome;
			string file = xhome + "/.page.conf";
			_conf.merge_from_file_if_exist(file);
		}
	}

	/* load file in arguments if provided */
	if (conf_file_name != nullptr) {
		string s(conf_file_name);
		_conf.merge_from_file_if_exist(s);
	}

	page_base_dir = _conf.get_string("default", "theme_dir");
	_theme_engine = _conf.get_string("default", "theme_engine");

	_last_focus_time = XCB_TIME_CURRENT_TIME;
	_last_button_press = XCB_TIME_CURRENT_TIME;
	_left_most_border = std::numeric_limits<int>::max();
	_top_most_border = std::numeric_limits<int>::max();

	_theme = nullptr;

	bind_page_quit           = _conf.get_string("default", "bind_page_quit");
	bind_close               = _conf.get_string("default", "bind_close");
	bind_exposay_all         = _conf.get_string("default", "bind_exposay_all");
	bind_toggle_fullscreen   = _conf.get_string("default", "bind_toggle_fullscreen");
	bind_toggle_compositor   = _conf.get_string("default", "bind_toggle_compositor");
	bind_right_workspace     = _conf.get_string("default", "bind_right_desktop");
	bind_left_workspace      = _conf.get_string("default", "bind_left_desktop");

	bind_bind_window         = _conf.get_string("default", "bind_bind_window");
	bind_fullscreen_window   = _conf.get_string("default", "bind_fullscreen_window");
	bind_float_window        = _conf.get_string("default", "bind_float_window");

	bind_debug_1 = _conf.get_string("default", "bind_debug_1");
	bind_debug_2 = _conf.get_string("default", "bind_debug_2");
	bind_debug_3 = _conf.get_string("default", "bind_debug_3");
	bind_debug_4 = _conf.get_string("default", "bind_debug_4");

	bind_cmd[0].key = _conf.get_string("default", "bind_cmd_0");
	bind_cmd[1].key = _conf.get_string("default", "bind_cmd_1");
	bind_cmd[2].key = _conf.get_string("default", "bind_cmd_2");
	bind_cmd[3].key = _conf.get_string("default", "bind_cmd_3");
	bind_cmd[4].key = _conf.get_string("default", "bind_cmd_4");
	bind_cmd[5].key = _conf.get_string("default", "bind_cmd_5");
	bind_cmd[6].key = _conf.get_string("default", "bind_cmd_6");
	bind_cmd[7].key = _conf.get_string("default", "bind_cmd_7");
	bind_cmd[8].key = _conf.get_string("default", "bind_cmd_8");
	bind_cmd[9].key = _conf.get_string("default", "bind_cmd_9");

	bind_cmd[0].cmd = _conf.get_string("default", "exec_cmd_0");
	bind_cmd[1].cmd = _conf.get_string("default", "exec_cmd_1");
	bind_cmd[2].cmd = _conf.get_string("default", "exec_cmd_2");
	bind_cmd[3].cmd = _conf.get_string("default", "exec_cmd_3");
	bind_cmd[4].cmd = _conf.get_string("default", "exec_cmd_4");
	bind_cmd[5].cmd = _conf.get_string("default", "exec_cmd_5");
	bind_cmd[6].cmd = _conf.get_string("default", "exec_cmd_6");
	bind_cmd[7].cmd = _conf.get_string("default", "exec_cmd_7");
	bind_cmd[8].cmd = _conf.get_string("default", "exec_cmd_8");
	bind_cmd[9].cmd = _conf.get_string("default", "exec_cmd_9");

	if(_conf.get_string("default", "auto_refocus") == "true") {
		configuration._auto_refocus = true;
	} else {
		configuration._auto_refocus = false;
	}

	if(_conf.get_string("default", "enable_shade_windows") == "true") {
		configuration._enable_shade_windows = true;
	} else {
		configuration._enable_shade_windows = false;
	}

	if(_conf.get_string("default", "mouse_focus") == "true") {
		configuration._mouse_focus = true;
	} else {
		configuration._mouse_focus = false;
	}

	if(_conf.get_string("default", "menu_drop_down_shadow") == "true") {
		configuration._menu_drop_down_shadow = true;
	} else {
		configuration._menu_drop_down_shadow = false;
	}

	configuration._fade_in_time = _conf.get_long("compositor", "fade_in_time");

}

page_t::~page_t() {
	// cleanup cairo, for valgrind happiness.
	//cairo_debug_reset_static_data();
}

void page_t::run() {

	_dpy = new display_t;

	/* check for required page extension */
	_dpy->check_x11_extension();

	{ // check for sync system counters
		xcb_generic_error_t * e;
		auto ck = xcb_sync_list_system_counters(_dpy->xcb());
		auto r = xcb_sync_list_system_counters_reply(_dpy->xcb(), ck, &e);
		//printf("counter length %u\n", r->counters_len);
		if (r != nullptr) {
			// the first item is correctly computed by libxcb but I can extract it
			// without xcb_sync_list_system_counters_counters_iterator, thus
			// extract it manually.
			fixed_xcb_sync_systemcounter_t * item =
					(fixed_xcb_sync_systemcounter_t*) (r + 1);
			for (int i = 0; i < r->counters_len; ++i) {
				char * name = xcb_sync_system_counter_dup_name(item);
				//printf("COUNTER_NAME %d = %s, id = %u, resolution = %lu\n", i, name,
				//		item->counter, xcb_sync_system_counter_int64_swap(&item->resolution));

				if(strcmp("SERVERTIME", name) == 0) {
					printf("found SERVERTIME\n");
					/* about 30 fps */
					frame_alarm = _dpy->create_alarm_delay(item->counter, 32);
				}

				free(name);
				/* next item */
				item = (struct fixed_xcb_sync_systemcounter_t*) (((char*) item)
						+ xcb_sync_system_counter_sizeof_item(item));
			}
			free(r);
		}

	}

	{
		xcb_generic_error_t * e;
		auto ck = xcb_sync_get_priority(_dpy->xcb(), frame_alarm);
		auto r = xcb_sync_get_priority_reply(_dpy->xcb(), ck, &e);
		if (r != nullptr) {
			//printf("priority is %d\n", r->priority);
		}
		free(r);

		xcb_sync_set_priority(_dpy->xcb(), frame_alarm, 16);

	}

	/** Initialize theme **/

	if(_theme_engine == "tiny") {
		cout << "using tiny theme engine" << endl;
		_theme = new tiny_theme_t{_dpy, _conf};
	} else {
		/* The default theme engine */
		cout << "using simple theme engine" << endl;
		_theme = new simple2_theme_t{_dpy, _conf};
	}

	/* Before doing anything, trying to register wm and cm */
	create_identity_window();
	register_wm();

	/** initialize the empty workspace **/

	/* start the compositor once the window manager is fully started */
	start_compositor();

	_dpy->grab();
	_dpy->change_property(_dpy->root(), _NET_SUPPORTING_WM_CHECK,
			WINDOW, 32, &identity_window, 1);

	_bind_all_default_event();

	{
		/** check for number of workspace and create them **/
		net_number_of_desktops_t number_of_workspace;
		auto nd = number_of_workspace.read(_dpy->xcb(), _dpy->_A, _dpy->root());
		int n = 1;
		if(nd != nullptr and *nd != 0)
			n = *nd;
		update_number_of_workspace(n);
		number_of_workspace.release(_dpy->xcb());
	}

	/* start listen root event before anything, each event will be stored to be processed later */
	_dpy->select_input(_dpy->root(), ROOT_EVENT_MASK);

	/**
	 * listen RRCrtcChangeNotifyMask for possible change in screen layout.
	 **/
	xcb_randr_select_input(_dpy->xcb(), _dpy->root(), XCB_RANDR_NOTIFY_MASK_CRTC_CHANGE);

	update_viewport_layout();

	_dpy->load_cursors();
	_dpy->set_window_cursor(_dpy->root(), _dpy->xc_left_ptr);

	update_net_supported();

	{
		/* page does not have showing workspace mode */
		uint32_t showing_workspace = 0;
		_dpy->change_property(_dpy->root(), _NET_SHOWING_DESKTOP, CARDINAL, 32,
				&showing_workspace, 1);
	}

	{
		wm_icon_size_t icon_size{16,16,16,16,16,16};
		_dpy->change_property(_dpy->root(), WM_ICON_SIZE, CARDINAL, 32,
				&icon_size, 6);

	}

	scan();

	/* setup _NET_ACTIVE_WINDOW */
	_dpy->set_input_focus(identity_window, XCB_INPUT_FOCUS_PARENT, XCB_CURRENT_TIME);
	_dpy->set_net_active_window(XCB_WINDOW_NONE);

	update_keymap();
	update_grabkey();

	update_windows_stack();
	xcb_flush(_dpy->xcb());

	_dpy->ungrab();

	/** switch back to the first workspace by default */
	switch_to_workspace(0, XCB_CURRENT_TIME);
	update_windows_stack();
	xcb_flush(_dpy->xcb());

	/* process messages as soon as we get messages, or every 1/60 of seconds */
	_mainloop.add_poll(_dpy->fd(), POLLIN|POLLPRI|POLLERR,
		[this](struct pollfd const & x) -> void {
			this->process_pending_events();
		});

	_mainloop.run();

	cout << "Page END" << endl;

	_mainloop.remove_poll(_dpy->fd());

	_dpy->unload_cursors();

	for (auto & i : net_client_list()) {
		_dpy->reparentwindow(i->_client_proxy->id(), _dpy->root(), 0, 0);
		//detach(i);
	}

	_workspace_list.clear();

	delete _keymap; _keymap = nullptr;
	delete _theme; _theme = nullptr;
	delete _compositor; _compositor = nullptr;

	if(_dpy != nullptr) {
		/** clean up properties defined by Window Manager **/
		_dpy->delete_property(_dpy->root(), _NET_SUPPORTED);
		_dpy->delete_property(_dpy->root(), _NET_CLIENT_LIST);
		_dpy->delete_property(_dpy->root(), _NET_CLIENT_LIST_STACKING);
		_dpy->delete_property(_dpy->root(), _NET_ACTIVE_WINDOW);
		_dpy->delete_property(_dpy->root(), _NET_NUMBER_OF_DESKTOPS);
		_dpy->delete_property(_dpy->root(), _NET_DESKTOP_GEOMETRY);
		_dpy->delete_property(_dpy->root(), _NET_DESKTOP_VIEWPORT);
		_dpy->delete_property(_dpy->root(), _NET_CURRENT_DESKTOP);
		_dpy->delete_property(_dpy->root(), _NET_DESKTOP_NAMES);
		_dpy->delete_property(_dpy->root(), _NET_WORKAREA);
		_dpy->delete_property(_dpy->root(), _NET_SUPPORTING_WM_CHECK);
		_dpy->delete_property(_dpy->root(), _NET_SHOWING_DESKTOP);

		delete _dpy; _dpy = nullptr;
	}

}

void page_t::unmanage(client_managed_p mw)
{
	assert(mw != nullptr);
	_net_client_list.remove(mw);

	/* if window is in move/resize/notebook move, do cleanup */
	cleanup_grab();

	for (auto & d : _workspace_list) {
		d->unmanage(mw);
	}

	mw->_client_proxy->remove_from_save_set();
	mw->_client_proxy->set_wm_state(WithdrawnState);
	mw->_client_proxy->unmap();
	_need_update_client_list = true;
	update_workarea();
}

void page_t::scan() {

	_dpy->grab();
	_dpy->fetch_pending_events();

	xcb_query_tree_cookie_t ck = xcb_query_tree(_dpy->xcb(), _dpy->root());
	xcb_query_tree_reply_t * r = xcb_query_tree_reply(_dpy->xcb(), ck, 0);

	if(r == nullptr)
		throw exception_t("Cannot query tree");

	xcb_window_t * children = xcb_query_tree_children(r);
	unsigned n_children = xcb_query_tree_children_length(r);

	for (unsigned i = 0; i < n_children; ++i) {
		xcb_window_t w = children[i];

		auto c = _dpy->ensure_client_proxy(w);
		if (not c)
			continue;

		if (c->wa()._class == XCB_WINDOW_CLASS_INPUT_ONLY)
			continue;

		c->read_all_properties();

		if (c->wa().map_state != XCB_MAP_STATE_UNMAPPED) {
			onmap(w);
		} else {
			/**
			 * if the window is not mapped, check if previous windows manager has set WM_STATE to iconic
			 * if this is the case, that mean that is a managed window, otherwise it is a WithDrwn window
			 **/
			auto wm_state = c->get<p_wm_state>();
			if (wm_state != nullptr) {
				if (wm_state->state == IconicState) {
					onmap(w);
				}
			}
		}
	}

	free(r);


	update_workarea();

	_need_update_client_list = true;
	_need_restack = true;

	_dpy->ungrab();

	//print_state();

}

void page_t::update_net_supported() {
	vector<xcb_atom_t> supported_list;

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
	supported_list.push_back(A(_NET_WM_DESKTOP));
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

	_dpy->change_property(_dpy->root(), _NET_SUPPORTED, ATOM, 32, &supported_list[0],
			supported_list.size());

}

void page_t::update_client_list() {
	_net_client_list.remove_if([](client_managed_w const & w) { return w.expired(); });

	/** set _NET_CLIENT_LIST : client list from oldest to newer client **/
	vector<xcb_window_t> xid_client_list;
	for(auto i : _net_client_list) {
		xid_client_list.push_back(i->_client_proxy->id());
	}

	_dpy->change_property(_dpy->root(), _NET_CLIENT_LIST, WINDOW, 32,
			&xid_client_list[0], xid_client_list.size());

}

void page_t::update_client_list_stacking() {

	/** set _NET_CLIENT_LIST_STACKING : bottom to top staking **/
	auto managed = net_client_list();
	vector<xcb_window_t> client_list_stack;
	for(auto c: managed) {
		client_list_stack.push_back(c->_client_proxy->id());
	}

	_dpy->change_property(_dpy->root(), _NET_CLIENT_LIST_STACKING,
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

	key_desc_t key;

	key.ks = _keymap->get(e->detail);
	key.mod = e->state;

	if (key.ks == 0)
		return;


	/** XCB_MOD_MASK_2 is num_lock, thus ignore his state **/
	if(_keymap->numlock_mod_mask() != 0) {
		key.mod &= ~_keymap->numlock_mod_mask();
	}

	if (key == bind_page_quit) {
		_mainloop.stop();
	}

	if(_grab_handler != nullptr) {
		auto grab = _grab_handler;// hold grab handdler in case of the handler stop the grab.
		grab->key_press(e);
		return;
	}

	if (key == bind_close) {
		view_p mw;
		if (get_current_workspace()->client_focus_history_front(mw)) {
			mw->_client->delete_window(e->time);
		}
		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
		return;
	}

//	if (key == bind_exposay_all) {
//		auto child = filter_class<notebook_t>(get_current_workspace()->get_all_children());
//		for (auto c : child) {
//			c->start_exposay();
//		}
//		return;
//	}

	if (key == bind_toggle_fullscreen) {
		view_p mw;
		if (get_current_workspace()->client_focus_history_front(mw)) {
			toggle_fullscreen(mw, e->time);
		}
		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
		return;
	}

	if (key == bind_toggle_compositor) {
		if (_compositor == nullptr) {
			start_compositor();
		} else {
			stop_compositor();
		}
		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
		return;
	}

	if (key == bind_right_workspace) {
		unsigned new_workspace = ((_current_workspace + _workspace_list.size()) + 1) % _workspace_list.size();
		switch_to_workspace(new_workspace, e->time);
		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
		return;
	}

	if (key == bind_left_workspace) {
		unsigned new_workspace = ((_current_workspace + _workspace_list.size()) - 1) % _workspace_list.size();
		switch_to_workspace(new_workspace, e->time);
		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
		return;
	}

	if (key == bind_bind_window) {
		view_p mw;
		if (get_current_workspace()->client_focus_history_front(mw)) {
			get_current_workspace()->switch_view_to_notebook(mw, e->time);
		}
		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
		return;
	}

	if (key == bind_fullscreen_window) {
		view_p mw;
		if (get_current_workspace()->client_focus_history_front(mw)) {
			get_current_workspace()->switch_view_to_fullscreen(mw, e->time);
		}
		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
		return;
	}

	if (key == bind_float_window) {
		view_p mw;
		if (get_current_workspace()->client_focus_history_front(mw)) {
			get_current_workspace()->switch_view_to_floating(mw, e->time);
		}
		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
		return;
	}

	if (_compositor != nullptr) {
		if (key == bind_debug_1) {
			if (_fps_overlay == nullptr) {

				auto v = get_current_workspace()->get_any_viewport();
				int y_pos = v->allocation().y + v->allocation().h - 100;
				int x_pos = v->allocation().x + (v->allocation().w - 400)/2;

				_fps_overlay = make_shared<compositor_overlay_t>(get_current_workspace().get(), rect{x_pos, y_pos, 400, 100});
				get_current_workspace()->add_overlay(_fps_overlay);
				_fps_overlay->show();
			} else {
				_fps_overlay->detach_myself();
				_fps_overlay = nullptr;
			}
			xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
			return;
		}

		if (key == bind_debug_2) {
			if (_compositor->show_damaged()) {
				_compositor->set_show_damaged(false);
			} else {
				_compositor->set_show_damaged(true);
			}
			xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
			return;
		}

		if (key == bind_debug_3) {
			if (_compositor->show_opac()) {
				_compositor->set_show_opac(false);
			} else {
				_compositor->set_show_opac(true);
			}
			xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
			return;
		}
	}

	if (key == bind_debug_4) {
		get_current_workspace()->print_tree(0);
		for (auto i : net_client_list()) {
			switch (i->get_type()) {
			case MANAGED_NOTEBOOK:
				cout << "[" << i->_client_proxy->id() << "] notebook : " << i->title()
						<< endl;
				break;
			case MANAGED_FLOATING:
				cout << "[" << i->_client_proxy->id() << "] floating : " << i->title()
						<< endl;
				break;
			case MANAGED_FULLSCREEN:
				cout << "[" << i->_client_proxy->id() << "] fullscreen : " << i->title()
						<< endl;
				break;
			case MANAGED_DOCK:
				cout << "[" << i->_client_proxy->id() << "] dock : " << i->title() << endl;
				break;
			}
		}

		if(not global_focus_history_is_empty()) {
			cout << "active window is : ";
			for(auto & focus: global_client_focus_history()) {
				cout << focus.lock()->_client->_client_proxy->id() << ",";
			}
			cout << endl;
		} else {
			cout << "active window is : " << "NONE" << endl;
		}

		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
		return;
	}

	for(int i; i < bind_cmd.size(); ++i) {
		if (key == bind_cmd[i].key) {
			run_cmd(bind_cmd[i].cmd);
			xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_KEYBOARD, e->time);
			return;
		}
	}

	if (key.ks == XK_Tab and (key.mod == XCB_MOD_MASK_1)) {
		if (_grab_handler == nullptr) {
			start_alt_tab(e->time);
		}
		return;
	}

	if(not _grab_handler)
		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_REPLAY_KEYBOARD, e->time);

}

void page_t::process_key_release_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_key_release_event_t const *>(_e);

	if(_grab_handler != nullptr) {
		auto grab = _grab_handler; // hold grab handdler in case of the handler stop the grab.
		grab->key_release(e);
		return;
	}

}

/* Button event make page to grab pointer */
void page_t::process_button_press_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_button_press_event_t const *>(_e);
	log(LOG_BUTTONS, "Button Event Press ");
	log(LOG_BUTTONS, " event=0x%x", e->event);
	log(LOG_BUTTONS, " child=0x%x", e->child);
	log(LOG_BUTTONS, " root=0x%x", e->root);
	log(LOG_BUTTONS, " button=", static_cast<int>(e->detail));
	log(LOG_BUTTONS, " mod1=", (e->state & XCB_MOD_MASK_1 ? "true" : "false"));
	log(LOG_BUTTONS, " mod2=", (e->state & XCB_MOD_MASK_2 ? "true" : "false"));
	log(LOG_BUTTONS, " mod3=", (e->state & XCB_MOD_MASK_3 ? "true" : "false"));
	log(LOG_BUTTONS, " mod4=", (e->state & XCB_MOD_MASK_4 ? "true" : "false"));
	log(LOG_BUTTONS, " mod5=", (e->state & XCB_MOD_MASK_5 ? "true" : "false"));
	log(LOG_BUTTONS, " time=", e->time);
	log(LOG_BUTTONS, "\n");

	if(_grab_handler != nullptr) {
		auto grab = _grab_handler; // hold grab handdler in case of the handler stop the grab.
		grab->button_press(e);
		return;
	}

    if(e->root_x == 0 and e->root_y == 0) {
        start_alt_tab(e->time);
        xcb_flush(_dpy->xcb());
    } else {
        auto status = get_current_workspace()->broadcast_button_press(e);
        switch(status) {
        case BUTTON_ACTION_GRAB_ASYNC:
    		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_ASYNC_POINTER, e->time);
    		break;
        case BUTTON_ACTION_GRAB_SYNC:
    		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_SYNC_POINTER, e->time);
    		break;
        case BUTTON_ACTION_HAS_ACTIVE_GRAB:
        	/* do nothing */
        	break;
        case BUTTON_ACTION_CONTINUE:
        default:
        	xcb_allow_events(_dpy->xcb(), XCB_ALLOW_REPLAY_POINTER, e->time);
        	break;
        }
        xcb_flush(_dpy->xcb());
    }


//	/**
//	 * if no change happened to process mode
//	 * We allow events (remove the grab), and focus those window.
//	 **/
//	if (_grab_handler == nullptr) {
//		xcb_allow_events(_dpy->xcb(), XCB_ALLOW_REPLAY_POINTER, e->time);
//		xcb_flush(_dpy->xcb());
//		/* TODO */
////		auto mw = find_managed_window_with(e->event);
////		if (mw != nullptr) {
////			activate(mw, e->time);
////		}
//	}

}

void page_t::process_configure_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_configure_notify_event_t const *>(_e);

	//printf("configure (%d) %dx%d+%d+%d\n", e->window, e->width, e->height, e->x, e->y);

	auto c = lookup_client_managed_with_orig_window(e->window);
	if(c != nullptr) {
		c->process_event(e);
	}

	/** damage corresponding area **/
	if(e->event == _dpy->root()) {
		add_global_damage(_root_position);
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
	//printf("destroy window = %d\n", e->window);

	if(has_key(_page_windows, e->window))
		return;

	auto c = lookup_client_managed_with_orig_window(e->window);
	if (c != nullptr) {
		cout << "WARNING: client destroyed a window without sending synthetic unmap" << endl;
		unmanage(c);
	}
}

void page_t::process_gravity_notify_event(xcb_generic_event_t const * e) {
	/* Ignore it, never happen ? */
}

void page_t::process_map_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_map_notify_event_t const *>(_e);
	/* if map event does not occur within root, ignore it */
	if (e->event != _dpy->root())
		return;

	//printf("map notify window #%u\n", e->window);
	if(e->override_redirect)
		onmap(e->window);

	add_global_damage(_root_position);

}

void page_t::process_reparent_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_reparent_notify_event_t const *>(_e);
	//printf("Reparent window: %lu, parent: %lu, overide: %d, send_event: %d\n",
	//		e.window, e.parent, e.override_redirect, e.send_event);
	/* Reparent the root window ? hu :/ */
	if(e->window == _dpy->root())
		return;

	/* If reparent occur on managed windows and new parent is an unknown window then unmanage */
	auto mw = lookup_client_managed_with_orig_window(e->window);
	if (mw != nullptr) {
		if (e->window == mw->_client_proxy->id() and not has_key(_page_windows, e->parent) and e->parent != _dpy->root()) {
			/* unmanage the window */
			unmanage(mw);
		}
	}

}

void page_t::process_unmap_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_unmap_notify_event_t const *>(_e);
	auto c = lookup_client_managed_with_orig_window(e->window);
	if (c != nullptr) {
		//printf("unmap serial = %u, window = %d, event = %d\n", e->sequence, e->window, e->event);

		// client do not use fake unmap for popup, and popup never be reparented.
		if(c->is(MANAGED_POPUP))
			unmanage(c);

	}
}

void page_t::process_fake_unmap_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_unmap_notify_event_t const *>(_e);
	/**
	 * Client must send a fake unmap event if he want get back the window.
	 * (i.e. he want that we unmanage it.
	 **/

	//printf("fake unmap window = %d\n", e->window);

	/* if client is managed */
	auto c = lookup_client_managed_with_orig_window(e->window);
	if (c != nullptr) {
		unmanage(c);
	}
}

void page_t::process_circulate_request_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_circulate_request_event_t const *>(_e);
	/* will happpen ? */
	auto c = lookup_client_managed_with_orig_window(e->window);
	if (c != nullptr) {
		if (e->place == XCB_PLACE_ON_TOP) {
			// TODO
			//c->raise();
		} else if (e->place == XCB_PLACE_ON_BOTTOM) {
			_dpy->lower_window(e->window);
		}
	}
}

void page_t::process_configure_request_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_configure_request_event_t const *>(_e);

	log(LOG_CONFIGURE_REQUEST, "Configure request on 0x%x\n", e->window);
	if (e->value_mask & CWX)
		log(LOG_CONFIGURE_REQUEST, "has x: %d\n", e->x);
	if (e->value_mask & CWY)
		log(LOG_CONFIGURE_REQUEST, "has y: %d\n", e->y);
	if (e->value_mask & CWWidth)
		log(LOG_CONFIGURE_REQUEST, "has width: %d\n", e->width);
	if (e->value_mask & CWHeight)
		log(LOG_CONFIGURE_REQUEST, "has height: %d\n", e->height);
	if (e->value_mask & CWSibling)
		log(LOG_CONFIGURE_REQUEST, "has sibling: 0x%x\n", e->sibling);
	if (e->value_mask & CWStackMode)
		log(LOG_CONFIGURE_REQUEST, "has stack mode: %d\n", e->stack_mode);
	if (e->value_mask & CWBorderWidth)
		log(LOG_CONFIGURE_REQUEST, "has border: %d\n", e->border_width);

	auto c = lookup_client_managed_with_orig_window(e->window);

	if (c == nullptr) {
		/** validate configure when window is not managed **/
		ackwoledge_configure_request(e);
		_dpy->flush();
		return;
	} else {
		c->singal_configure_request(e);
	}



	rect new_size = c->get_floating_wished_position();

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

	dimention_t<unsigned> final_size = c->compute_size_with_constrain(new_size.w, new_size.h);

	new_size.w = final_size.width;
	new_size.h = final_size.height;

	auto view = get_current_workspace()->lookup_view_for(c);
	auto v = dynamic_pointer_cast<view_floating_t>(view);
	if(v) {
		/** only affect floating windows **/
		c->set_floating_wished_position(new_size);
		v->reconfigure();
	} else {
		view->reconfigure();
	}

	_dpy->flush();

}

void page_t::process_fake_configure_request_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_configure_request_event_t const *>(_e);

	printf("Fake ConfigureRequest\n");
	if (e->value_mask & CWX)
		printf("has x: %d\n", e->x);
	if (e->value_mask & CWY)
		printf("has y: %d\n", e->y);
	if (e->value_mask & CWWidth)
		printf("has width: %d\n", e->width);
	if (e->value_mask & CWHeight)
		printf("has height: %d\n", e->height);
	if (e->value_mask & CWSibling)
		printf("has sibling: %u\n", e->sibling);
	if (e->value_mask & CWStackMode)
		printf("has stack mode: %d\n", e->stack_mode);
	if (e->value_mask & CWBorderWidth)
		printf("has border: %d\n", e->border_width);

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

	xcb_void_cookie_t ck = xcb_configure_window(_dpy->xcb(), e->window, mask, value);

}

void page_t::process_map_request_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_map_request_event_t const *>(_e);
	//printf("map request window #%u\n", e->window);

	if (e->parent != _dpy->root()) {
		xcb_map_window(_dpy->xcb(), e->window);
		return;
	}

	onmap(e->window);

}

void page_t::process_property_notify_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_property_notify_event_t const *>(_e);
	if(e->window == _dpy->root())
		return;

	/** update the property **/
	auto c = lookup_client_managed_with_orig_window(e->window);
	if(c == nullptr)
		return;

	c->on_property_notify(e);

	if (e->atom == A(_NET_WM_USER_TIME)) {
		/* ignore */
	} else if (e->atom == A(_NET_WM_STRUT_PARTIAL)) {
		update_workarea();
	} else if (e->atom == A(_NET_WM_STRUT)) {
		update_workarea();
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
		/* ignore WM_NORMAL_HINTS updates */
//		if (mw->is(MANAGED_NOTEBOOK)) {
//			find_parent_notebook_for(mw)->update_client_position(mw);
//		}
//
//		/* apply normal hint to floating window */
//		rect new_size = mw->get_wished_position();
//
//		dimention_t<unsigned> final_size = mw->compute_size_with_constrain(
//				new_size.w, new_size.h);
//		new_size.w = final_size.width;
//		new_size.h = final_size.height;
//		mw->set_floating_wished_position(new_size);
//		mw->reconfigure();
	} else if (e->atom == A(WM_PROTOCOLS)) {
		/* do nothing */
	} else if (e->atom == A(WM_TRANSIENT_FOR)) {
		/* only use transient for on map, ignore other cases */
		//safe_update_transient_for(mw);
		//_need_restack = true;
	} else if (e->atom == A(WM_HINTS)) {
		/* do nothing */
	} else if (e->atom == A(_NET_WM_STATE)) {
		/* this event are generated by page */
		/* change of net_wm_state must be requested by client message */
	} else if (e->atom == A(WM_STATE)) {
		/** this is set by page ... don't read it **/
	} else if (e->atom == A(_NET_WM_DESKTOP)) {
		/* this must be set by the WM, moving a client to a workspace is requested by client message */
	} else if (e->atom == A(_MOTIF_WM_HINTS)) {
		/* ignore _MOTIF_WM_HINTS updates */
		//mw->reconfigure();
	} else if (e->atom == A(_NET_DESKTOP_NAMES)) {
		update_workspace_names();
	}

}

void page_t::process_fake_client_message_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_client_message_event_t const *>(_e);
	//std::shared_ptr<char> name = cnx->get_atom_name(e->type);
	//std::cout << "ClientMessage type = " << cnx->get_atom_name(e->type) << std::endl;

	xcb_window_t w = e->window;
	if (w == XCB_NONE)
		return;

	auto mw = lookup_client_managed_with_orig_window(e->window);

	if (e->type == A(_NET_ACTIVE_WINDOW)) {
		auto view = get_current_workspace()->lookup_view_for(mw);
		if (mw != nullptr) {
			auto view = get_current_workspace()->lookup_view_for(mw);
			if(view)
				activate(view, e->data.data32[1]);
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

			if (mw->is(MANAGED_FLOATING) and e->data.data32[0] == IconicState) {
				/* ignore */
				//bind_window(mw, false);
			} else if (mw->is(
					MANAGED_NOTEBOOK) and e->data.data32[0] == IconicState) {
//				auto n = dynamic_pointer_cast<notebook_t>(mw->parent()->shared_from_this());
//				auto x = dynamic_pointer_cast<view_notebook_t>(n->lookup_view_for(mw));
//				n->iconify_client(x);
			}
		}

	} else if (e->type == A(PAGE_QUIT)) {
		_mainloop.stop();
	} else if (e->type == A(WM_PROTOCOLS)) {

	} else if (e->type == A(_NET_CLOSE_WINDOW)) {
		if(mw != nullptr) {
			mw->delete_window(e->data.data32[0]);
		}
	} else if (e->type == A(_NET_REQUEST_FRAME_EXTENTS)) {

	} else if (e->type == A(_NET_WM_MOVERESIZE)) {
		//printf("moveresize XxX\n");
		if (mw != nullptr and _grab_handler == nullptr) {
			int root_x = e->data.data32[0];
			int root_y = e->data.data32[1];
			int direction = e->data.data32[2];
			xcb_button_t button = static_cast<xcb_button_t>(e->data.data32[3]);
			int source = e->data.data32[4];

			auto view = dynamic_pointer_cast<view_floating_t>(get_current_workspace()->lookup_view_for(mw));
			if(not view)
				return;

			if (direction == _NET_WM_MOVERESIZE_MOVE) {
				grab_start(make_shared<grab_floating_move_t>(this, view, button, root_x, root_y), XCB_TIME_CURRENT_TIME);
			} else {

				if (direction == _NET_WM_MOVERESIZE_SIZE_TOP) {
					grab_start(make_shared<grab_floating_resize_t>(this, view, button, root_x, root_y, RESIZE_TOP), XCB_TIME_CURRENT_TIME);
				} else if (direction == _NET_WM_MOVERESIZE_SIZE_BOTTOM) {
					grab_start(make_shared<grab_floating_resize_t>(this, view, button, root_x, root_y, RESIZE_BOTTOM), XCB_TIME_CURRENT_TIME);
				} else if (direction == _NET_WM_MOVERESIZE_SIZE_LEFT) {
					grab_start(make_shared<grab_floating_resize_t>(this, view, button, root_x, root_y, RESIZE_LEFT), XCB_TIME_CURRENT_TIME);
				} else if (direction == _NET_WM_MOVERESIZE_SIZE_RIGHT) {
					grab_start(make_shared<grab_floating_resize_t>(this, view, button, root_x, root_y, RESIZE_RIGHT), XCB_TIME_CURRENT_TIME);
				} else if (direction == _NET_WM_MOVERESIZE_SIZE_TOPLEFT) {
					grab_start(make_shared<grab_floating_resize_t>(this, view, button, root_x, root_y, RESIZE_TOP_LEFT), XCB_TIME_CURRENT_TIME);
				} else if (direction == _NET_WM_MOVERESIZE_SIZE_TOPRIGHT) {
					grab_start(make_shared<grab_floating_resize_t>(this, view, button, root_x, root_y, RESIZE_TOP_RIGHT), XCB_TIME_CURRENT_TIME);
				} else if (direction
						== _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT) {
					grab_start(make_shared<grab_floating_resize_t>(this, view, button, root_x, root_y, RESIZE_BOTTOM_LEFT), XCB_TIME_CURRENT_TIME);
				} else if (direction
						== _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT) {
					grab_start(make_shared<grab_floating_resize_t>(this, view, button, root_x, root_y, RESIZE_BOTTOM_RIGHT), XCB_TIME_CURRENT_TIME);
				} else {
					grab_start(make_shared<grab_floating_move_t>(this, view, button, root_x, root_y), XCB_TIME_CURRENT_TIME);
				}
			}
		}
	} else if (e->type == A(_NET_CURRENT_DESKTOP)) {
		if(e->data.data32[0] >= 0 and e->data.data32[0] < _workspace_list.size() and e->data.data32[0] != _current_workspace) {
			switch_to_workspace(e->data.data32[0], e->data.data32[1]);
		}
	} else if (e->type == A(_NET_NUMBER_OF_DESKTOPS)) {
		update_number_of_workspace(e->data.data32[0]);
	} else if (e->type == A(_NET_WM_DESKTOP)) {
		if (mw != nullptr) {
			move_client_to_workspace(mw, e->data.data32[0]);
		}
	} else if (e->type == A(_NET_STARTUP_INFO_BEGIN)) {
		char buf[21];
		buf[20] = 0;
		std::copy(&e->data.data8[0], &e->data.data8[20], buf);
		//printf("received _NET_STARTUP_INFO_BEGIN (window=%d, format=%d) data=`%s'\n", e->window, e->format, buf);

	} else if (e->type == A(_NET_STARTUP_INFO)) {
		char buf[21];
		buf[20] = 0;
		std::copy(&e->data.data8[0], &e->data.data8[20], buf);
		//printf("received _NET_STARTUP_INFO (window=%d, format=%d) data=`%s'\n", e->window, e->format, buf);
	}
}

void page_t::process_damage_notify_event(xcb_generic_event_t const * e) {
	schedule_repaint(0L);
}

void page_t::render() {
	_scheduled_repaint_timeout = nullptr;
	//printf("call %s\n", __PRETTY_FUNCTION__);

	// ask to update everything to draw the time64_t::now() frame
	get_current_workspace()->broadcast_update_layout(time64_t::now());
	// ask to flush all pending drawing
	get_current_workspace()->broadcast_trigger_redraw();
	// render on screen if we need too.
	xcb_flush(_dpy->xcb());

	if (_compositor != nullptr) {
		_compositor->render(get_current_workspace().get());
	}
	xcb_flush(_dpy->xcb());

	get_current_workspace()->broadcast_render_finished();
}

void page_t::insert_as_fullscreen(client_managed_p c, xcb_timestamp_t time) {
	//printf("call %s\n", __PRETTY_FUNCTION__);

	auto wid = c->ensure_workspace();
	workspace_p workspace;
	if(wid == ALL_DESKTOP) {
		workspace = get_current_workspace();
	} else {
		workspace = get_workspace(wid);
	}

	workspace->insert_as_fullscreen(c, time);
}

void page_t::insert_as_notebook(client_managed_p c, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	c->set_managed_type(MANAGED_NOTEBOOK);

	auto wid = c->ensure_workspace();
	workspace_p workspace;
	if(wid == ALL_DESKTOP) {
		workspace = get_current_workspace();
	} else {
		workspace = get_workspace(wid);
	}

	workspace->insert_as_notebook(c, time);
	_need_restack = true;
}

void page_t::move_view_to_notebook(view_p v, notebook_p n, xcb_timestamp_t time)
{
	auto vn = dynamic_pointer_cast<view_notebook_t>(v);
	if(vn) {
		move_notebook_to_notebook(vn, n, time);
		return;
	}

	auto vf = dynamic_pointer_cast<view_floating_t>(v);
	if(vf) {
		move_floating_to_notebook(vf, n, time);
		return;
	}
}

void page_t::move_notebook_to_notebook(view_notebook_p vn, notebook_p n, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	vn->remove_this_view();
	n->add_client_from_view(vn, time);
}

void page_t::move_floating_to_notebook(view_floating_p vf, notebook_p n, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	vf->detach_myself();
	n->add_client_from_view(vf, time);
}

void page_t::toggle_fullscreen(view_p c, xcb_timestamp_t time) {
	auto vf = dynamic_pointer_cast<view_fullscreen_t>(c);
	if(vf) {
		vf->workspace()->switch_fullscreen_to_prefered_view_mode(vf, time);
	} else {
		c->workspace()->switch_view_to_fullscreen(c, time);
	}
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

void page_t::insert_window_in_notebook(
		client_managed_p x,
		notebook_p n,
		bool prefer_activate) {
	assert(x != nullptr);
	assert(n != nullptr);
	n->add_client(x, prefer_activate);
	_need_restack = true;
	_need_update_client_list = true;
}

/* update _viewport and childs allocation */
void page_t::update_workarea() {
	for (auto d : _workspace_list) {
		for (auto v : d->get_viewports()) {
			compute_viewport_allocation(d, v);
		}
		//d->set_workarea(d->primary_viewport()->allocation());
		d->set_workarea(_root_position);
	}

	std::vector<uint32_t> workarea_data(_workspace_list.size()*4);
	for(unsigned k = 0; k < _workspace_list.size(); ++k) {
		workarea_data[k*4+0] = _workspace_list[k]->workarea().x;
		workarea_data[k*4+1] = _workspace_list[k]->workarea().y;
		workarea_data[k*4+2] = _workspace_list[k]->workarea().w;
		workarea_data[k*4+3] = _workspace_list[k]->workarea().h;
	}

	_dpy->change_property(_dpy->root(), _NET_WORKAREA, CARDINAL, 32,
			&workarea_data[0], workarea_data.size());

}

void page_t::apply_focus(xcb_timestamp_t time) {
	if(time == XCB_CURRENT_TIME) {
		//printf("invalid focus time\n");
		return;
	}

	auto w = get_current_workspace();
	if(w->_net_active_window.expired()) {
		//printf("apply focus NONE at %d\n", time);
		_dpy->set_input_focus(identity_window, XCB_INPUT_FOCUS_PARENT, time);
		_dpy->set_net_active_window(XCB_WINDOW_NONE);
	} else {
		auto focus = w->_net_active_window.lock();
		//printf("apply focus %d at %d\n", focus->_client->_client_proxy->id(), time);
		_dpy->set_net_active_window(focus->_client->_client_proxy->id());
		focus->focus(time);
		xcb_flush(_dpy->xcb());
	}
}

void page_t::split_left(notebook_p nbk, view_p c, xcb_timestamp_t time) {
	auto parent = dynamic_pointer_cast<page_component_t>(nbk->parent()->shared_from_this());
	auto n = make_shared<notebook_t>(nbk.get());
	auto split = make_shared<split_t>(nbk.get(), VERTICAL_SPLIT);
	parent->replace(nbk, split);
	split->set_pack0(n);
	split->set_pack1(nbk);
	split->show();
	if (c != nullptr)
		move_view_to_notebook(c, n, time);
}

void page_t::split_right(notebook_p nbk, view_p c, xcb_timestamp_t time) {
	auto parent = dynamic_pointer_cast<page_component_t>(nbk->parent()->shared_from_this());
	auto n = make_shared<notebook_t>(nbk.get());
	auto split = make_shared<split_t>(nbk.get(), VERTICAL_SPLIT);
	parent->replace(nbk, split);
	split->set_pack0(nbk);
	split->set_pack1(n);
	split->show();
	if (c != nullptr)
		move_view_to_notebook(c, n, time);
}

void page_t::split_top(notebook_p nbk, view_p c, xcb_timestamp_t time) {
	auto parent = dynamic_pointer_cast<page_component_t>(nbk->parent()->shared_from_this());
	auto n = make_shared<notebook_t>(nbk.get());
	auto split = make_shared<split_t>(nbk.get(), HORIZONTAL_SPLIT);
	parent->replace(nbk, split);
	split->set_pack0(n);
	split->set_pack1(nbk);
	split->show();
	if (c != nullptr)
		move_view_to_notebook(c, n, time);
}

void page_t::split_bottom(notebook_p nbk, view_p c, xcb_timestamp_t time) {
	auto parent = dynamic_pointer_cast<page_component_t>(nbk->parent()->shared_from_this());
	auto n = make_shared<notebook_t>(nbk.get());
	auto split = make_shared<split_t>(nbk.get(), HORIZONTAL_SPLIT);
	parent->replace(nbk, split);
	split->set_pack0(nbk);
	split->set_pack1(n);
	split->show();
	if (c != nullptr)
		move_view_to_notebook(c, n, time);
}

void page_t::notebook_close(notebook_p nbk, xcb_timestamp_t time) {
	/**
	 * Closing notebook mean destroying the split base of this
	 * notebook, plus this notebook.
	 **/

	assert(nbk->parent() != nullptr);

	auto workspace = nbk->workspace();

	auto splt = dynamic_pointer_cast<split_t>(nbk->parent()->shared_from_this());

	/* if parent is _viewport then we cannot close current notebook */
	if(splt == nullptr)
		return;

	assert(nbk == splt->get_pack0() or nbk == splt->get_pack1());

	/* find the sibling branch of note that we want close */
	auto dst = dynamic_pointer_cast<page_component_t>((nbk == splt->get_pack0()) ? splt->get_pack1() : splt->get_pack0());

	assert(dst != nullptr);

	/* remove this split from tree  and replace it by sibling branch */
	dst->detach_myself();
	dynamic_pointer_cast<page_component_t>(splt->parent()->shared_from_this())->replace(splt, dst);

	/* move all client from destroyed notebook to new default pop */
	auto clients = nbk->gather_children_root_first<view_notebook_t>();
	auto default_notebook = workspace->ensure_default_notebook();
	for(auto i : clients) {
		default_notebook->add_client_from_view(i, XCB_CURRENT_TIME);
	}

	workspace->set_focus(nullptr, time);

}

/*
 * Compute the usable workspace area and dock allocation.
 */
void page_t::compute_viewport_allocation(workspace_p d, viewport_p v) {

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

	auto children = d->gather_children_root_first<view_t>();
	for(auto y: children) {
		auto j = y->_client;
		int32_t ps[PS_LAST];
		bool has_strut{false};

		auto net_wm_strut_partial = j->_net_wm_strut_partial;
		if(net_wm_strut_partial != nullptr) {
			if(net_wm_strut_partial->size() == 12) {
				std::copy(net_wm_strut_partial->begin(), net_wm_strut_partial->end(), &ps[0]);
				has_strut = true;
			}
		}

		auto net_wm_strut = j->_net_wm_strut;
		if (net_wm_strut != nullptr and not has_strut) {
			if(net_wm_strut->size() == 4) {

				/** if strut is found, fake strut_partial **/

				std::copy(net_wm_strut->begin(), net_wm_strut->end(), &ps[0]);

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
				/* check if raw area intersect current _viewport */
				rect b(0, ps[PS_LEFT_START_Y], ps[PS_LEFT],
						ps[PS_LEFT_END_Y] - ps[PS_LEFT_START_Y] + 1);
				rect x = raw_area & b;
				if (!x.is_null()) {
					margin_left = std::max(margin_left, ps[PS_LEFT]);
				}
			}

			if (ps[PS_RIGHT] > 0) {
				/* check if raw area intersect current _viewport */
				rect b(_root_position.w - ps[PS_RIGHT],
						ps[PS_RIGHT_START_Y], ps[PS_RIGHT],
						ps[PS_RIGHT_END_Y] - ps[PS_RIGHT_START_Y] + 1);
				rect x = raw_area & b;
				if (!x.is_null()) {
					margin_right = std::max(margin_right, ps[PS_RIGHT]);
				}
			}

			if (ps[PS_TOP] > 0) {
				/* check if raw area intersect current _viewport */
				rect b(ps[PS_TOP_START_X], 0,
						ps[PS_TOP_END_X] - ps[PS_TOP_START_X] + 1, ps[PS_TOP]);
				rect x = raw_area & b;
				if (!x.is_null()) {
					margin_top = std::max(margin_top, ps[PS_TOP]);
				}
			}

			if (ps[PS_BOTTOM] > 0) {
				/* check if raw area intersect current _viewport */
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
//				<< _dpy->get_atom_name(state_properties) << std::endl;
//	}

	auto mw = lookup_client_managed_with_orig_window(c);
	if(mw == nullptr)
		return;

	xcb_timestamp_t time;
	if(not get_safe_net_wm_user_time(mw, time)) {
		time = XCB_CURRENT_TIME;
	}

	for(auto workspace: _workspace_list) {
		auto view = workspace->lookup_view_for(mw);
		if(view == nullptr)
			continue;

		if (state_properties == A(_NET_WM_STATE_FULLSCREEN)) {
			switch (type) {
			case _NET_WM_STATE_REMOVE:
				view->workspace()->switch_fullscreen_to_prefered_view_mode(view, time);
				break;
			case _NET_WM_STATE_ADD:
				view->workspace()->switch_view_to_fullscreen(view, time);
				break;
			case _NET_WM_STATE_TOGGLE:
				toggle_fullscreen(view, time);
				break;
			}
		} else if (state_properties == A(_NET_WM_STATE_HIDDEN)) {
			/* Ignore as specified by ICCCM/EWMH: client cannot change this state */
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

client_managed_p page_t::get_transient_for(client_managed_p c) {
	assert(c != nullptr);
	client_managed_p transient_for = nullptr;
	auto wm_transient_for = c->_client_proxy->wm_transiant_for();
	if (wm_transient_for != nullptr) {
		transient_for = lookup_client_managed_with_orig_window(*(wm_transient_for));
		if (transient_for == nullptr)
			printf("Warning transient for an unknown client\n");
	}
	return transient_for;
}

vector<shared_ptr<tree_t>> page_t::get_all_children() const
{
	vector<shared_ptr<tree_t>> ret;
	for(auto const & x: _workspace_list) {
		auto tmp = x->get_all_children();
		ret.insert(ret.end(), tmp.begin(), tmp.end());
	}
	return ret;
}

void page_t::move_fullscreen_to_viewport(view_fullscreen_p fv, viewport_p v) {

	if (not fv->_viewport.expired()) {
		fv->_viewport.lock()->show();
	}

	fv->_client->_absolute_position = v->raw_area();
	fv->_viewport = v;
	fv->show();
	v->hide();

}

void page_t::cleanup_grab() {
	_grab_handler = nullptr;
}

/* look for a notebook in tree base, that is deferent from nbk */
shared_ptr<notebook_t> page_t::get_another_notebook(shared_ptr<tree_t> base, shared_ptr<tree_t> nbk) {
	vector<shared_ptr<notebook_t>> l;

	if (base == nullptr) {
		l = get_current_workspace()->gather_children_root_first<notebook_t>();
	} else {
		l = base->gather_children_root_first<notebook_t>();
	}

	if (!l.empty()) {
		if (l.front() != nbk)
			return l.front();
		if (l.back() != nbk)
			return l.back();
	}

	return nullptr;

}

shared_ptr<workspace_t> page_t::find_workspace_of(shared_ptr<tree_t> n) {
	return n->workspace();
}

void page_t::update_windows_stack() {

	auto tree = get_current_workspace()->get_all_children();

	{
		int k = 0;
		for (int i = 0; i < tree.size(); ++i) {
			if(tree[i]->get_toplevel_xid() != XCB_WINDOW_NONE) {
				tree[k++] = tree[i];
			}
		}
		tree.resize(k);
	}

	reverse(tree.begin(), tree.end());
	vector<xcb_window_t> stack;

	if (_compositor != nullptr)
		stack.push_back(_compositor->get_composite_overlay());

	for (auto i : tree) {
		stack.push_back(i->get_toplevel_xid());
	}

	unsigned k = 0;

	/* place the first one on top */
	if (stack.size() > k) {
		uint32_t value[1];
		uint32_t mask{0};
		value[0] = XCB_STACK_MODE_ABOVE;
		mask |= XCB_CONFIG_WINDOW_STACK_MODE;
		xcb_configure_window(_dpy->xcb(),
				stack[k], mask, value);
		++k;
	}

	/* place following windows bellow */
	while (stack.size() > k) {
		uint32_t value[2];
		uint32_t mask { 0 };

		value[0] = stack[k-1];
		mask |= XCB_CONFIG_WINDOW_SIBLING;

		value[1] = XCB_STACK_MODE_BELOW;
		mask |= XCB_CONFIG_WINDOW_STACK_MODE;

		xcb_configure_window(_dpy->xcb(), stack[k], mask,
				value);
		++k;
	}

}

/**
 * This function will update _viewport layout on xrandr events.
 *
 * It cut the visible outputs area in rectangle, where _viewport will cover. The
 * rule is that the first output get the area first, the last one is cut in
 * sub-rectangle that do not overlap previous allocated area.
 **/
void page_t::update_viewport_layout() {
	_left_most_border = std::numeric_limits<int>::max();
	_top_most_border = std::numeric_limits<int>::max();

	/** update root size infos **/
	xcb_get_geometry_cookie_t ck0 = xcb_get_geometry(_dpy->xcb(), _dpy->root());
	xcb_randr_get_screen_resources_cookie_t ck1 = xcb_randr_get_screen_resources(_dpy->xcb(), _dpy->root());

	xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(_dpy->xcb(), ck0, nullptr);
	xcb_randr_get_screen_resources_reply_t * randr_resources = xcb_randr_get_screen_resources_reply(_dpy->xcb(), ck1, 0);

	if(geometry == nullptr or randr_resources == nullptr) {
		throw exception_t("FATAL: cannot read root window attributes");
	}

	_root_position = rect{geometry->x, geometry->y, geometry->width, geometry->height};
	set_workspace_geometry(_root_position.w, _root_position.h);

	map<xcb_randr_crtc_t, xcb_randr_get_crtc_info_reply_t *> crtc_info;

	vector<xcb_randr_get_crtc_info_cookie_t> ckx(xcb_randr_get_screen_resources_crtcs_length(randr_resources));
	xcb_randr_crtc_t * crtc_list = xcb_randr_get_screen_resources_crtcs(randr_resources);
	for (unsigned k = 0; k < xcb_randr_get_screen_resources_crtcs_length(randr_resources); ++k) {
		ckx[k] = xcb_randr_get_crtc_info(_dpy->xcb(), crtc_list[k], XCB_CURRENT_TIME);
	}

	for (unsigned k = 0; k < xcb_randr_get_screen_resources_crtcs_length(randr_resources); ++k) {
		xcb_randr_get_crtc_info_reply_t * r = xcb_randr_get_crtc_info_reply(_dpy->xcb(), ckx[k], 0);
		if(r != nullptr) {
			crtc_info[crtc_list[k]] = r;
		}

		// keep left more screen to move iconnified window there
		if(r->x < _left_most_border) {
			_left_most_border = r->x;
		}

		if(r->y < _top_most_border) {
			_top_most_border = r->y;
		}

	}

	// compute all viewport that does not overlap and cover the full area of
	// crts
	vector<rect> viewport_allocation; // the list of _viewport locations.
	region already_allocated;
	for (auto crtc: crtc_info) {
		if (crtc.second->num_outputs <= 0)
			continue;

		/* the location of crts */
		region location{crtc.second->x, crtc.second->y, crtc.second->width,
			crtc.second->height};
		location -= already_allocated;
		for (auto & b: location.rects()) {
			viewport_allocation.push_back(b);
		}
		already_allocated += location;
	}

	if (viewport_allocation.size() < 1) {
		viewport_allocation.push_back(_root_position);
	}

	for(auto d: _workspace_list) {
		d->update_viewports_layout(viewport_allocation);
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

	update_workspace_visibility(XCB_CURRENT_TIME);

	/* set _viewport */
	std::vector<uint32_t> viewport(_workspace_list.size()*2);
	std::fill_n(viewport.begin(), _workspace_list.size()*2, 0);
	_dpy->change_property(_dpy->root(), _NET_DESKTOP_VIEWPORT,
			CARDINAL, 32, &viewport[0], _workspace_list.size()*2);

	/* define workspace geometry */
	set_workspace_geometry(_root_position.w, _root_position.h);

	update_workarea();

}

void page_t::remove_viewport(shared_ptr<workspace_t> d, shared_ptr<viewport_t> v) {

	/* remove fullscreened clients if needed */
	for (auto &x : v->_root->gather_children_root_first<view_fullscreen_t>()) {
		if (x->_viewport.lock() == v) {
			x->workspace()->switch_fullscreen_to_prefered_view_mode(x, XCB_CURRENT_TIME);
			break;
		}
	}

	/* Transfer clients to a valid notebook */
	for (auto x : v->gather_children_root_first<view_notebook_t>()) {
		d->ensure_default_notebook()->add_client_from_view(x, XCB_CURRENT_TIME);
	}

	for (auto x : v->gather_children_root_first<view_floating_t>()) {
		d->insert_as_floating(x->_client, XCB_CURRENT_TIME);
	}

}

/**
 * this function will check if a window must be managed or not.
 * If a window have to be managed, this function manage this window, if not
 * The function create unmanaged window.
 **/
void page_t::onmap(xcb_window_t w) {
	if(_compositor != nullptr)
		if (w == _compositor->get_composite_overlay())
			return;
	if(w == _dpy->root())
		return;

	/* check if this window is a page window */
	if(has_key(_page_windows, w))
		return;

	/* check if the window is already managed */
	if(find_client_managed_with(w) != nullptr)
		return;

	auto props = _dpy->ensure_client_proxy(w);
	if(not props) { // the window is already destroyed.
		return;
	}

	if(props->wa()._class == XCB_WINDOW_CLASS_INPUT_ONLY) {
		return;
	}

	//props->print_window_attributes();
	//props->print_properties();

	xcb_atom_t type = props->wm_type();

	if (not props->wa().override_redirect) {
		if (type == A(_NET_WM_WINDOW_TYPE_DESKTOP)) {
			create_managed_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DOCK)) {
			create_managed_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLBAR)) {
			create_managed_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_MENU)) {
			create_managed_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_UTILITY)) {
			create_managed_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_SPLASH)) {
			create_managed_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DIALOG)) {
			create_managed_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_POPUP_MENU)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLTIP)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_COMBO)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DND)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
			create_managed_window(props);
		}
	} else {
		if (type == A(_NET_WM_WINDOW_TYPE_DESKTOP)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DOCK)) {
			create_managed_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLBAR)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_MENU)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_UTILITY)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_SPLASH)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DIALOG)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_POPUP_MENU)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_TOOLTIP)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_COMBO)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_DND)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
			create_unmanaged_window(props);
		} else if (type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
			create_unmanaged_window(props);
		}
	}

	schedule_repaint();
	_dpy->flush();

}


void page_t::create_managed_window(client_proxy_p proxy) {
	auto mw = make_shared<client_managed_t>(this, proxy);
	mw->read_all_properties();
	_net_client_list.push_back(mw);
	manage_client(mw, proxy->wm_type());

	if (mw->_net_wm_strut != nullptr
			or mw->_net_wm_strut_partial != nullptr) {
		update_workarea();
	}
}

void page_t::manage_client(shared_ptr<client_managed_t> mw, xcb_atom_t type) {

	if(not mw->skip_task_bar()) {
		_need_update_client_list = true;
		_need_restack = true;
	}

	/* find the workspace for this window */
	{
		auto final_workspace = mw->ensure_workspace();
		if(final_workspace == ALL_DESKTOP) {
			//printf("final workspace = %u\n", final_workspace);
			mw->net_wm_state_add(_NET_WM_STATE_STICKY);
			//mw->show();
		} else {
			//printf("final workspace = %u\n", final_workspace);
			if(_current_workspace == final_workspace) {
				//mw->show();
			} else {
				//mw->hide();
			}
		}
	}

	/* HACK OLD FASHION FULLSCREEN */
	auto wm_normal_hints = mw->get<p_wm_normal_hints>();
	if (wm_normal_hints != nullptr and mw->wm_type() == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
		auto size_hints = wm_normal_hints;
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

	bool _has_net_wm_state_above = false;
	if(mw->_net_wm_state) {
		_has_net_wm_state_above = has_key(*mw->_net_wm_state, A(_NET_WM_STATE_ABOVE));
	}

	if (mw->has_wm_state_fullscreen()) {
		insert_as_fullscreen(mw);
	} else if (((type == A(_NET_WM_WINDOW_TYPE_NORMAL))
			and get_transient_for(mw) == nullptr
			and not mw->has_wm_state_modal()
			and not _has_net_wm_state_above)
			or type == A(_NET_WM_WINDOW_TYPE_DESKTOP)) {
		insert_as_notebook(mw);
	} else if (type == A(_NET_WM_WINDOW_TYPE_DOCK)) {
		insert_as_dock(mw);
	} else {
		insert_as_floating(mw);
	}
}

void page_t::create_unmanaged_window(client_proxy_p proxy) {
	auto mw = make_shared<client_managed_t>(this, proxy);
	mw->read_all_properties();
	_net_client_list.push_back(mw);
	insert_as_popup(mw);
}

shared_ptr<viewport_t> page_t::find_mouse_viewport(int x, int y) const {
	auto viewports = get_current_workspace()->get_viewports();
	for (auto v: viewports) {
		if (v->raw_area().is_inside(x, y))
			return v;
	}
	return shared_ptr<viewport_t>{};
}

/**
 * Read user time with proper fallback
 * @return: true if successfully find usertime, otherwise false.
 * @output time: if time is found time is set to the found value.
 * @input c: a window client handler.
 **/
bool page_t::get_safe_net_wm_user_time(client_managed_p c, xcb_timestamp_t & time)
{
	//printf("call %s for %x\n", __PRETTY_FUNCTION__, c->_client_proxy->id());
	auto net_wm_user_time = c->get<p_net_wm_user_time>();
	if (net_wm_user_time != nullptr) {
		time = *(net_wm_user_time);
		//printf("found net_wm_user_time = %d\n", time);
		return true;
	} else {
		//printf("net_wm_user_time not found, looking for net_wm_user_time_window\n");
		auto net_wm_user_time_window = c->get<p_net_wm_user_time_window>();
		if (net_wm_user_time_window == nullptr) {
			//printf("net_wm_user_time_window not found\n");
			return false;
		}
		if (*(net_wm_user_time_window) == XCB_WINDOW_NONE) {
			//printf("net_wm_user_time_window is NONE\n");
			return false;
		}
		//printf("found net_wm_user_time_window = %x\n", *(net_wm_user_time_window));
		try {
			auto xc = _dpy->ensure_client_proxy(*(net_wm_user_time_window));
			if (not xc) {
				//printf("net_wm_user_time_window does not exists\n");
				return false;
			}
			net_wm_user_time = xc->get<p_net_wm_user_time>();
			if(net_wm_user_time == nullptr) {
				//printf("net_wm_user_time_window does not have net_wm_user_time\n");
				return false;
			}
			if (*(net_wm_user_time) == XCB_CURRENT_TIME) {
				//printf("net_wm_user_time_window's net_wm_user_time is invalid\n");
				return false;
			}
			time = *(net_wm_user_time);
			//printf("found net_wm_user_time = %d\n", time);
			return true;
		} catch (invalid_client_t & e) {
			//printf("invalid net_wm_user_time_window\n");
			return false;
		}
	}
}

void page_t::insert_as_popup(client_managed_p c, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	c->set_managed_type(MANAGED_POPUP);

	auto wid = c->ensure_workspace();
	if(wid == ALL_DESKTOP) {
		for (auto &w: _workspace_list) {
			w->insert_as_popup(c, time);
		}
	} else {
		// Ignore net_wm_desktop for popup, use current workspace
		workspace_p workspace = get_current_workspace();
		c->set_net_wm_desktop(workspace->id());
		workspace->insert_as_popup(c, time);
	}

	schedule_repaint(0L);

}

void page_t::insert_as_dock(client_managed_p c, xcb_timestamp_t time) {
	//printf("call %s\n", __PRETTY_FUNCTION__);
	c->set_managed_type(MANAGED_DOCK);

	auto wid = c->ensure_workspace();
	vector<workspace_p> workspace_list;
	if(wid == ALL_DESKTOP) {
		workspace_list = _workspace_list;
	} else {
		workspace_list.push_back(get_workspace(wid));
	}

	for (auto & workspace: workspace_list) {
		workspace->insert_as_dock(c, time);
	}

	update_workarea();
	_need_restack = true;
}

void page_t::insert_as_floating(client_managed_p c, xcb_timestamp_t time) {
	//printf("call %s\n", __PRETTY_FUNCTION__);
	c->set_managed_type(MANAGED_FLOATING);

	auto wid = c->ensure_workspace();
	workspace_p workspace;
	if(wid == ALL_DESKTOP) {
		workspace = get_current_workspace();
	} else {
		workspace = get_workspace(wid);
	}

	workspace->insert_as_floating(c, time);
	_need_restack = true;
}

void page_t::set_workspace_geometry(long width, long height) {
	/* define workspace geometry */
	uint32_t workspace_geometry[2];
	workspace_geometry[0] = width;
	workspace_geometry[1] = height;
	_dpy->change_property(_dpy->root(), _NET_DESKTOP_GEOMETRY,
			CARDINAL, 32, workspace_geometry, 2);
}

auto page_t::find_client_managed_with(xcb_window_t w) -> client_managed_p
{
	auto i = std::find_if(_net_client_list.begin(), _net_client_list.end(),
			[w](client_managed_p const & p) -> bool {
				return p->is_window(w);
			});
	if (i != _net_client_list.end())
		return *i;
	return nullptr;
}

auto page_t::lookup_client_managed_with_orig_window(xcb_window_t w) const -> client_managed_p {
	for(auto & i: _net_client_list) {
		if(i->is_window(w)) {
			return i;
		}
	}
	return nullptr;
}

auto page_t::lookup_client_managed_with_base_window(xcb_window_t w) const -> client_managed_p
{
	auto views = get_current_workspace()->gather_children_root_first<view_rebased_t>();
	for(auto v: views) {
		if (v->_base != nullptr) {
			if (v->_base->_window->id() == w) {
				return v->_client;
			}
		}
	}
	return nullptr;
}


void replace(shared_ptr<page_component_t> const & src, shared_ptr<page_component_t> by) {
	throw exception_t{"Unexpectected use of page::replace function\n"};
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
	identity_window = xcb_generate_id(_dpy->xcb());
	_page_windows.insert(identity_window);
	xcb_void_cookie_t ck = xcb_create_window(_dpy->xcb(), XCB_COPY_FROM_PARENT, identity_window,
			_dpy->root(), -100, -100, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			XCB_COPY_FROM_PARENT, attrs_mask, attrs);

	std::string name{"page"};
	_dpy->change_property(identity_window, _NET_WM_NAME, UTF8_STRING, 8, name.c_str(),
			name.length() + 1);
	_dpy->change_property(identity_window, _NET_SUPPORTING_WM_CHECK, WINDOW, 32,
			&identity_window, 1);
	_dpy->change_property(identity_window, _NET_WM_PID, CARDINAL, 32, &pid, 1);
	_dpy->map(identity_window);
}

void page_t::register_wm() {
	if (!_dpy->register_wm(identity_window, configuration._replace_wm)) {
		throw exception_t("Cannot register window manager");
	}
}

void page_t::register_cm() {
	if (!_dpy->register_cm(identity_window)) {
		throw exception_t("Cannot register composite manager");
	}
}

inline void grab_key(xcb_connection_t * xcb, xcb_window_t w, key_desc_t & key, keymap_t * _keymap) {
	int kc = 0;
	if ((kc = _keymap->find_keysim(key.ks))) {
		xcb_grab_key(xcb, true, w, key.mod, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
		if(_keymap->numlock_mod_mask() != 0) {
			xcb_grab_key(xcb, true, w, key.mod|_keymap->numlock_mod_mask(), kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
		}
	}
}

/**
 * Update grab keys aware of current _keymap
 */
void page_t::update_grabkey() {

	assert(_keymap != nullptr);

	/** ungrab all previews key **/
	xcb_ungrab_key(_dpy->xcb(), XCB_GRAB_ANY, _dpy->root(), XCB_MOD_MASK_ANY);

	int kc = 0;

	grab_key(_dpy->xcb(), _dpy->root(), bind_debug_1, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_debug_2, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_debug_3, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_debug_4, _keymap);

	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[0].key, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[1].key, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[2].key, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[3].key, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[4].key, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[5].key, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[6].key, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[7].key, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[8].key, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_cmd[9].key, _keymap);

	grab_key(_dpy->xcb(), _dpy->root(), bind_page_quit, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_close, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_exposay_all, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_toggle_fullscreen, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_toggle_compositor, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_right_workspace, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_left_workspace, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_bind_window, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_fullscreen_window, _keymap);
	grab_key(_dpy->xcb(), _dpy->root(), bind_float_window, _keymap);

	/* Alt-Tab */
	if ((kc = _keymap->find_keysim(XK_Tab))) {
		xcb_grab_key(_dpy->xcb(), true, _dpy->root(), XCB_MOD_MASK_1, kc,
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
		if (_keymap->numlock_mod_mask() != 0) {
			xcb_grab_key(_dpy->xcb(), true, _dpy->root(),
					XCB_MOD_MASK_1 | _keymap->numlock_mod_mask(), kc,
					XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
		}
	}

}

void page_t::update_keymap() {
	if(_keymap != nullptr) {
		delete _keymap;
	}
	_keymap = new keymap_t{_dpy->xcb()};
}

/** debug function that try to print the state of page in stdout **/
void page_t::print_state() const {
	get_current_workspace()->print_tree(0);
	cout << "_current_workspace = " << _current_workspace << endl;

//	cout << "clients list:" << endl;
//	for(auto c: filter_class<client_base_t>(get_all_children())) {
//		cout << "client " << c->get_node_name() << " id = " << c->orig() << " ptr = " << c << " parent = " << c->parent() << endl;
//	}
//	cout << "end" << endl;

}

void page_t::update_current_workspace() const {
	/* set current workspace */
	uint32_t current_workspace = _current_workspace;
	_dpy->change_property(_dpy->root(), _NET_CURRENT_DESKTOP, CARDINAL,
			32, &current_workspace, 1);
}

void page_t::switch_to_workspace(unsigned int workspace, xcb_timestamp_t time) {
	assert(workspace < _workspace_list.size());
	if (workspace != _current_workspace and workspace != ALL_DESKTOP) {
		//std::cout << "switch to workspace #" << workspace << std::endl;
		start_switch_to_workspace_animation(workspace);
		_current_workspace = workspace;
		update_current_workspace();
		update_workspace_visibility(time);
	}
}

void page_t::start_switch_to_workspace_animation(unsigned int workspace) {
	auto new_workspace = _workspace_list[workspace];

	for(auto const & v : new_workspace->get_viewports()) {
		auto pix = theme()->workspace_switch_popup(new_workspace->name());
		auto loc = v->allocation();
		auto rnd = make_shared<renderable_fadeout_pixmap_t>(new_workspace.get(), pix, loc.x + (loc.w-pix->witdh())/2.0, loc.y + (loc.h-pix->height())/2.0);
		new_workspace->add_overlay(rnd);
		rnd->show();
	}

	schedule_repaint();

}

void page_t::update_workspace_visibility(xcb_timestamp_t time) {
	/** hide only workspace that must be hidden first **/
	for(unsigned k = 0; k < _workspace_list.size(); ++k) {
		if(k != _current_workspace) {
			_workspace_list[k]->disable();
		}
	}

	/** and show the workspace that have to be show **/
	_workspace_list[_current_workspace]->enable(time);
	_need_restack = true;
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
	_event_handler_bind(XCB_CONFIGURE_REQUEST|0x80, &page_t::process_fake_configure_request_event);

	/** Extension **/
	_event_handler_bind(_dpy->damage_event + XCB_DAMAGE_NOTIFY, &page_t::process_damage_notify_event);
	_event_handler_bind(_dpy->randr_event + XCB_RANDR_NOTIFY, &page_t::process_randr_notify_event);
	_event_handler_bind(_dpy->shape_event + XCB_SHAPE_NOTIFY, &page_t::process_shape_notify_event);
	_event_handler_bind(_dpy->sync_event + XCB_SYNC_COUNTER_NOTIFY, &page_t::process_counter_notify_event);
	_event_handler_bind(_dpy->sync_event + XCB_SYNC_ALARM_NOTIFY, &page_t::process_alarm_notify_event);

}


void page_t::process_mapping_notify_event(xcb_generic_event_t const * e) {
	update_keymap();
	update_grabkey();
}

void page_t::process_selection_clear_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_selection_clear_event_t const *>(_e);
	if(e->selection == _dpy->wm_sn_atom)
		_mainloop.stop();
	if(e->selection == _dpy->cm_sn_atom)
		stop_compositor();
}

void page_t::process_focus_in_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_focus_in_event_t const *>(_e);

	focus_in_to_string(e);

	/**
	 * Since we can detect the client_id the focus rules is based on current
	 * focussed client. i.e. as soon as the a client has the focus, he is
	 * allowed to give the focus to any window he own.
	 **/

	if (e->event == _dpy->root() and e->detail == XCB_NOTIFY_DETAIL_NONE) {

		view_p focused;
		if (get_current_workspace()->client_focus_history_front(focused)) {
			_dpy->set_input_focus(focused->_client->_client_proxy->id(),
					XCB_INPUT_FOCUS_PARENT, XCB_CURRENT_TIME);
		} else {
			_dpy->set_input_focus(identity_window, XCB_INPUT_FOCUS_PARENT,
					XCB_CURRENT_TIME);
		}

		return;
	}

	/**
	 * Some client start a grab without taking the focus. Page will
	 * automatically give them the focus, inconditionnally.
	 **/
	if (e->mode == XCB_NOTIFY_MODE_GRAB) {
		// Allow focus stilling when the client grab the keyboard
		auto mw = lookup_client_managed_with_orig_window(e->event);
		if (mw) {
			auto v = get_current_workspace()->lookup_view_for(mw);
			if (v) {
				get_current_workspace()->set_focus(v, XCB_CURRENT_TIME);
				_dpy->set_input_focus(mw->_client_proxy->id(), XCB_INPUT_FOCUS_PARENT, XCB_CURRENT_TIME);
				xcb_flush(_dpy->xcb());
			}
		}
	}

	{
		/**
		 * This code follow the focus to ensure that GUI show the correct
		 * actual focussed window.
		 **/
		auto c = find_client_managed_with(e->event);
		if(c == nullptr)
			return;
		if (c->_current_owner_view != nullptr) {
			auto v = c->_current_owner_view->shared_from_this();

			if (typeid(*v) == typeid(view_popup_t))
				return;

			// this switch is not needed, keeped for doc.
			switch(e->detail) {
			case XCB_NOTIFY_DETAIL_INFERIOR:
			case XCB_NOTIFY_DETAIL_ANCESTOR:
			case XCB_NOTIFY_DETAIL_VIRTUAL:
			case XCB_NOTIFY_DETAIL_NONLINEAR:
			case XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL:
			default:
				if (not _actual_focussed_client.expired()) {
					if(v == _actual_focussed_client.lock())
						return;
					_actual_focussed_client.lock()->set_focus_state(false);
				}
				_actual_focussed_client = v;
				v->set_focus_state(true);
				break;
			}
		}
	}

}

void page_t::process_focus_out_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_focus_in_event_t const *>(_e);
	focus_in_to_string(e);
}

void page_t::process_enter_window_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_enter_notify_event_t const *>(_e);
	get_current_workspace()->broadcast_enter(e);

	char const * xx = nullptr;
	switch (e->mode) {
	case XCB_NOTIFY_MODE_GRAB:
		xx = "GRAB";
		break;
	case XCB_NOTIFY_MODE_NORMAL:
		xx = "NORMAL";
		break;
	case XCB_NOTIFY_MODE_UNGRAB:
		xx = "UNGRAB";
		break;
	default:
		break;
	}

	log(LOG_LEAVE_ENTER, "Enter window = 0x%x mode = %s time = %u\n", e->event, xx, e->time);

	if(not configuration._mouse_focus)
		return;

	/* TODO */
//	auto mw = find_managed_window_with(e->event);
//	if(mw != nullptr) {
//		set_focus(mw, e->time);
//	}
}

void page_t::process_leave_window_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_leave_notify_event_t const *>(_e);

	char const * xx = nullptr;
	switch (e->mode) {
	case XCB_NOTIFY_MODE_GRAB:
		xx = "GRAB";
		break;
	case XCB_NOTIFY_MODE_NORMAL:
		xx = "NORMAL";
		break;
	case XCB_NOTIFY_MODE_UNGRAB:
		xx = "UNGRAB";
		break;
	default:
		break;
	}

	log(LOG_LEAVE_ENTER, "Leave window = 0x%x mode = %s time = %u\n", e->event, xx, e->time);

	get_current_workspace()->broadcast_leave(e);
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
		if(_compositor != nullptr)
			_compositor->update_layout();
		_theme->update();
	}

	_need_restack = true;

}

void page_t::process_shape_notify_event(xcb_generic_event_t const * e) {
	auto se = reinterpret_cast<xcb_shape_notify_event_t const *>(e);
	if (se->shape_kind == XCB_SHAPE_SK_BOUNDING) {
		xcb_window_t w = se->affected_window;
		auto c = lookup_client_managed_with_orig_window(w);
		if (c != nullptr) {
			c->update_shape();
		}

		/* TODO */
//		auto mw = dynamic_pointer_cast<client_managed_t>(c);
//		if(mw != nullptr) {
//			mw->reconfigure();
//		}

	}
}

void page_t::process_counter_notify_event(xcb_generic_event_t const * _e)
{
	auto e = reinterpret_cast<xcb_sync_counter_notify_event_t const *>(_e);
	printf("counter notify id = %u, value = %lu, timestamp = %u\n", e->counter,
			xcb_sync_system_counter_int64_swap(&e->counter_value), e->timestamp);
}

void page_t::process_alarm_notify_event(xcb_generic_event_t const * _e)
{
	auto e = reinterpret_cast<xcb_sync_alarm_notify_event_t const *>(_e);
	//printf("alarm notify id = %u, value = %lu, timestamp = %u\n", e->alarm,
	//		xcb_sync_system_counter_int64_swap(&e->counter_value), e->timestamp);
	if(_schedule_repaint) {
		_schedule_repaint = false;
		render();
	}

}

void page_t::process_motion_notify(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_motion_notify_event_t const *>(_e);
	//printf("motion #%x %d %d\n", e->event, e->event_x, e->event_y);
	if(_grab_handler != nullptr) {
		auto grab = _grab_handler; // hold grab handdler in case of the handler stop the grab.
		grab->button_motion(e);
		return;
	} else {
		get_current_workspace()->broadcast_button_motion(e);
	}
}

void page_t::process_button_release(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_button_release_event_t const *>(_e);
	log(LOG_BUTTONS, "Button Event Release ");
	log(LOG_BUTTONS, " event=0x%x", e->event);
	log(LOG_BUTTONS, " child=0x%x", e->child);
	log(LOG_BUTTONS, " root=0x%x", e->root);
	log(LOG_BUTTONS, " button=", static_cast<int>(e->detail));
	log(LOG_BUTTONS, " mod1=", (e->state & XCB_MOD_MASK_1 ? "true" : "false"));
	log(LOG_BUTTONS, " mod2=", (e->state & XCB_MOD_MASK_2 ? "true" : "false"));
	log(LOG_BUTTONS, " mod3=", (e->state & XCB_MOD_MASK_3 ? "true" : "false"));
	log(LOG_BUTTONS, " mod4=", (e->state & XCB_MOD_MASK_4 ? "true" : "false"));
	log(LOG_BUTTONS, " mod5=", (e->state & XCB_MOD_MASK_5 ? "true" : "false"));
	log(LOG_BUTTONS, " time=", e->time);
	log(LOG_BUTTONS, "\n");

	if(_grab_handler != nullptr) {
		auto grab = _grab_handler; // hold grab handdler in case of the handler stop the grab.
		grab->button_release(e);
	} else {
		get_current_workspace()->broadcast_button_release(e);
	}
}

void page_t::start_compositor() {
	if (_dpy->has_composite) {
		try {
			register_cm();
		} catch (std::exception & e) {
			std::cout << e.what() << std::endl;
			return;
		}
		_compositor = new compositor_t{_dpy};
		_dpy->enable();
	}
}

void page_t::stop_compositor() {
	if (_dpy->has_composite) {
		_dpy->unregister_cm();
		_dpy->disable();
		delete _compositor;
		_compositor = nullptr;
	}
}

void page_t::process_expose_event(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_expose_event_t const *>(_e);
	get_current_workspace()->broadcast_expose(e);
}

void page_t::process_error(xcb_generic_event_t const * _e) {
	auto e = reinterpret_cast<xcb_generic_error_t const *>(_e);
	_dpy->print_error(e);
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

void page_t::update_workspace_names() {
	net_desktop_names_t workspace_names;
	auto names = workspace_names.read(_dpy->xcb(), _dpy->_A, _dpy->root());
	if(names != nullptr) {
		for(int k = 0; k < _workspace_list.size() and k < names->size(); ++k) {
			_workspace_list[k]->set_name((*names)[k]);
		}
	}
	workspace_names.release(_dpy->xcb());
}

void page_t::update_number_of_workspace(int n) {
	/* only add ne workspace, ignore request for reducing workspace numbers */
	for(int i = _workspace_list.size(); i < n; ++i) {
		auto d = make_shared<workspace_t>(this, i);
		_workspace_list.push_back(d);
		d->disable();
		d->show();
	}
	update_workspace_names();

	{ // update the _NET_DESKTOP_NAMES property.
		vector<char> names_list;
		for (unsigned k = 0; k < _workspace_list.size(); ++k) {
			std::string s { get_workspace(k)->name() };
			names_list.insert(names_list.end(), s.begin(), s.end());
			/* end of string */
			names_list.push_back(0);
		}
		_dpy->change_property(_dpy->root(), _NET_DESKTOP_NAMES, UTF8_STRING, 8,
				&names_list[0], names_list.size());
	}

	/* update number of workspace */
	uint32_t number_of_workspace = _workspace_list.size();
	_dpy->change_property(_dpy->root(), _NET_NUMBER_OF_DESKTOPS,
			CARDINAL, 32, &number_of_workspace, 1);

}

void page_t::move_client_to_workspace(shared_ptr<client_managed_t> mw, unsigned workspace)
{
	/* TODO */

//	printf("move to %u\n", workspace);
//	if(workspace == ALL_DESKTOP) {
//		mw->net_wm_state_add(_NET_WM_STATE_STICKY);
//		mw->set_net_wm_desktop(workspace);
//		return;
//	}
//
//	if(find_current_workspace(mw) == workspace)
//		return;
//
//	detach(mw);
//	mw->set_net_wm_desktop(workspace);
//	manage_client(mw, mw->wm_type());

}

shared_ptr<viewport_t> page_t::find_viewport_of(shared_ptr<tree_t> t) {
	while(t != nullptr) {
		auto ret = dynamic_pointer_cast<viewport_t>(t);
		if(ret != nullptr)
			return ret;
		t = t->parent()->shared_from_this();
	}

	return nullptr;
}

void page_t::process_pending_events() {

	/* on connection error, terminate */
	if(xcb_connection_has_error(_dpy->xcb()) != 0) {
		_mainloop.stop();
		return;
	}

	bool has_schedule_repaint = _schedule_repaint;

	while (_dpy->has_pending_events()) {
		process_event(_dpy->front_event());
		_dpy->pop_event();
	}

	if (_need_restack) {
		_need_restack = false;
		update_windows_stack();
		_need_update_client_list = true;
	}

	if(_need_update_client_list) {
		_need_update_client_list = false;
		update_client_list();
		update_client_list_stacking();
	}

	if (_schedule_repaint) {
		_schedule_repaint = false;
		render();
	}

	xcb_flush(_dpy->xcb());

}

theme_t const * page_t::theme() const {
	return _theme;
}

display_t * page_t::dpy() const {
	return _dpy;
}

compositor_t * page_t::cmp() const {
	return _compositor;
}

void page_t::grab_start(shared_ptr<grab_handler_t> handler, xcb_timestamp_t time) {
	assert(_grab_handler == nullptr);

	_dpy->grab();

	auto ck0 = xcb_grab_pointer(_dpy->xcb(), false, _dpy->root(),
			DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
			XCB_NONE, XCB_NONE, time);

	auto ck1 = xcb_grab_keyboard(_dpy->xcb(), false, _dpy->root(),
			time, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

	xcb_generic_error_t * e;
	bool has_pointer_grab = false;
	bool has_keyboard_grab = false;

	auto r0 = xcb_grab_pointer_reply(_dpy->xcb(), ck0, &e);
	auto r1 = xcb_grab_keyboard_reply(_dpy->xcb(), ck1, &e);

	if (r0 != nullptr) {
		if (r0->status == XCB_GRAB_STATUS_SUCCESS) {
			has_pointer_grab = true;
		} else {
			printf("grab pointer failed with status %d", r0->status);
		}
		free(r0);
	} else {
		printf("grab pointer failed with error");
	}

	if (r1 != nullptr) {
		if (r1->status == XCB_GRAB_STATUS_SUCCESS) {
			has_keyboard_grab = true;
		} else {
			printf("grab keyboard failed with status %d", r0->status);
		}
		free(r1);
	} else {
		printf("grab keyboard failed with error");
	}

	if (has_pointer_grab and has_keyboard_grab) {
		_grab_handler = handler;
	} else {
		xcb_ungrab_pointer(_dpy->xcb(), XCB_CURRENT_TIME);
		xcb_ungrab_keyboard(_dpy->xcb(), XCB_CURRENT_TIME);
	}

	_dpy->ungrab();

}

void page_t::grab_stop(xcb_timestamp_t time) {
	assert(_grab_handler != nullptr);
	assert(time != XCB_CURRENT_TIME);
	_grab_handler = nullptr;
	xcb_ungrab_keyboard(_dpy->xcb(), time);
	xcb_ungrab_pointer(_dpy->xcb(), time);
	xcb_flush(_dpy->xcb());
	/* apply regular keyboard focus */
	view_p focused;
	if (get_current_workspace()->client_focus_history_front(focused)) {
		focused->focus(time);
	}
}

void page_t::overlay_add(shared_ptr<tree_t> x) {
	get_current_workspace()->add_overlay(x);
}

void page_t::add_global_damage(region const & r) {
	if(_compositor != nullptr) {
		_compositor->add_damaged(r);
	}
	schedule_repaint();
}

shared_ptr<workspace_t> const & page_t::get_current_workspace() const {
	return _workspace_list[_current_workspace];
}

shared_ptr<workspace_t> const & page_t::get_workspace(int id) const {
	return _workspace_list[id];
}

int page_t::get_workspace_count() const {
	return _workspace_list.size();
}

int page_t::create_workspace() {
	auto d = make_shared<workspace_t>(this, _workspace_list.size());
	_workspace_list.push_back(d);
	d->disable();
	d->show();

	update_viewport_layout();

	if(d != get_current_workspace()) {
		for (auto &x: get_current_workspace()->gather_children_root_first<view_t>()) {
			auto w = x->_client->ensure_workspace();
			if (w == ALL_DESKTOP) {
				/** TODO: insert desktop **/
				auto const & type = typeid(*(x.get()));
				if (type == typeid(view_notebook_t)) {
					d->insert_as_notebook(x->_client, XCB_CURRENT_TIME);
				} else if (type == typeid(view_floating_t)) {
					d->insert_as_floating(x->_client, XCB_CURRENT_TIME);
				} else if (type == typeid(view_dock_t)) {
					d->insert_as_dock(x->_client, XCB_CURRENT_TIME);
				} else if (type == typeid(view_fullscreen_t)) {
					d->insert_as_fullscreen(x->_client, XCB_CURRENT_TIME);
				}
			}
		}
	}

	update_workarea();
	/* update number of workspace */
	uint32_t number_of_workspace = _workspace_list.size();
	_dpy->change_property(_dpy->root(), _NET_NUMBER_OF_DESKTOPS,
			CARDINAL, 32, &number_of_workspace, 1);
	update_current_workspace();
	return d->id();
}

int page_t::left_most_border() {
	return _left_most_border;
}
int page_t::top_most_border() {
	return _top_most_border;
}

keymap_t const * page_t::keymap() const {
	return _keymap;
}

xcb_atom_t page_t::A(atom_e atom) {
	return _dpy->A(atom);
}

list<view_w> page_t::global_client_focus_history() {
	return _global_focus_history;
}

bool page_t::global_focus_history_front(view_p & out) {
	if(not global_focus_history_is_empty()) {
		out = _global_focus_history.front().lock();
		return true;
	}
	return false;
}

void page_t::global_focus_history_remove(view_p in) {
	_global_focus_history.remove_if([in](view_w const & w) { return w.expired() or w.lock() == in; });
}

void page_t::global_focus_history_move_front(view_p in) {
	move_front(_global_focus_history, in);
}

bool page_t::global_focus_history_is_empty() {
	_global_focus_history.remove_if([](view_w const & w) { return w.expired(); });
	return _global_focus_history.empty();
}

void page_t::start_alt_tab(xcb_timestamp_t time) {
	auto _x = get_current_workspace()->gather_children_root_first<view_rebased_t>();
	list<view_p> managed_window{_x.begin(), _x.end()};

	auto focus_history = get_current_workspace()->client_focus_history();
	/* reorder client to follow focused order */
	for (auto i = focus_history.rbegin(); i != focus_history.rend(); ++i) {
		if(i->expired())
			continue;
		auto x = std::find(managed_window.begin(), managed_window.end(), i->lock());
		if(x != managed_window.end()) {
			managed_window.splice(managed_window.begin(), managed_window, x);
		}
	}

	/* Grab keyboard */
	grab_start(make_shared<grab_alt_tab_t>(this, managed_window, time), time);

}

auto page_t::conf() const -> page_configuration_t const & {
	return configuration;
}

auto page_t::create_view(xcb_window_t w) -> shared_ptr<client_view_t> {
	return _dpy->create_view(w);
}

void page_t::make_surface_stats(int & size, int & count) {
	_dpy->make_surface_stats(size, count);
}

auto page_t::net_client_list() -> list<client_managed_p> const &
{
	return _net_client_list;
}

auto page_t::mainloop() -> mainloop_t * {
	return &_mainloop;
}

void page_t::schedule_repaint(int64_t timeout)
{
	if (not _schedule_repaint) {
		_schedule_repaint = true;
		/* about 30 fps max for schedule repaint */
		_dpy->change_alarm_delay(frame_alarm, 32);
	}
}

void page_t::damage_all() {
	add_global_damage(_root_position);
	schedule_repaint();
}

void page_t::activate(view_p c, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	c->xxactivate(time);
	_need_restack = true;

}

}

