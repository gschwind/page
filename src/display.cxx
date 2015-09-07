/*
 * display.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <poll.h>
#include <memory>

#include "display.hxx"

#include "utils.hxx"
#include "time.hxx"
#include "exception.hxx"
#include "client_proxy.hxx"

namespace page {

static int surf_count = 0;

int display_t::get_surf_count() {
	return surf_count;
}

static int context_count = 0;

int display_t::get_context_count() {
	return context_count;
}


int display_t::fd() {
	return _fd;
}

xcb_window_t display_t::root() {
	return _screen->root;
}

/* convenient macro to get atom XID */
xcb_atom_t display_t::A(atom_e atom) {
	return (*_A)(atom);
}

int display_t::screen() {
	return _default_screen;
}

display_t::display_t() {
	_xcb = xcb_connect(nullptr, &_default_screen);
	_fd = xcb_get_file_descriptor(_xcb);
	_grab_count = 0;
	_A = std::shared_ptr<atom_handler_t>(new atom_handler_t(_xcb));

	_enable_composite = false;

	update_default_visual();

	/** get default WM_Sxx atom **/
	char wm_sn[] = "WM_Sxx";
	snprintf(wm_sn, sizeof(wm_sn), "WM_S%d", screen());
	wm_sn_atom = get_atom(wm_sn);

	/** get default _NET_WM_CM_Sxx atom **/
	char net_wm_cm[] = "_NET_WM_CM_Sxx";
	snprintf(net_wm_cm, sizeof(net_wm_cm), "_NET_WM_CM_S%d", screen());
	cm_sn_atom = get_atom(net_wm_cm);

}

display_t::~display_t() {

	for(auto x: pending_event) {
		free(x);
	}

	xcb_disconnect(_xcb);
}

void display_t::grab() {
	if (_grab_count == 0) {
		xcb_void_cookie_t ck = xcb_grab_server_checked(_xcb);
		xcb_generic_error_t * err = xcb_request_check(_xcb, ck);
		if(err != nullptr) {
			throw exception_t{"%s:%d unable to grab X11 server", __FILE__, __LINE__};
		}
		xcb_discard_reply(_xcb, ck.sequence);
	}
	++_grab_count;
}

void display_t::ungrab() {
	if (_grab_count == 0) {
		throw exception_t{"Error: Trying to ungrab not grabbed connection\n"};
	}
	--_grab_count;
	if (_grab_count == 0) {
		xcb_ungrab_server(_xcb);
		/**
		 * Don't wait to ungrab the server to allow other client to continue
		 * their business
		 **/
		if (xcb_flush(_xcb) <= 0)
			throw exception_t { "%s:%d unable to to flush X11 server", __FILE__,
					__LINE__ };
	}
}

void display_t::unmap(xcb_window_t w) {
	xcb_unmap_window(_xcb, w);
}

void display_t::reparentwindow(xcb_window_t w, xcb_window_t parent, int x, int y) {
	xcb_reparent_window(_xcb, w, parent, x, y);
}

void display_t::map(xcb_window_t w) {
	xcb_map_window(_xcb, w);
}

/**
 * This function try to become the owner of WM_Sxx to be the offical
 * window manager.
 *
 * @input w: the window that will be the owner
 * @input replace: try to replace ?
 * @return: true if success or false if fail.
 *
 **/
bool display_t::register_wm(xcb_window_t w, bool replace) {
	/**
	 * The current window manager must own WM_Sxx with xx the screen number
	 * to be the official window manager.
	 **/

	xcb_window_t current_wm_sn_owner;

	/** who is the current owner ? **/
	xcb_get_selection_owner_cookie_t ck = xcb_get_selection_owner(_xcb, wm_sn_atom);
	xcb_get_selection_owner_reply_t * r = xcb_get_selection_owner_reply(_xcb, ck, nullptr);

	if(r == nullptr) {
		std::cout << "Error while getting selection owner of " << get_atom_name(wm_sn_atom) << std::endl;
		return false;
	}

	current_wm_sn_owner = r->owner;
	free(r);

	std::cout << "Found " << current_wm_sn_owner << " as owner of " << get_atom_name(wm_sn_atom) << std::endl;

	/* if there is a current owner */
	if (current_wm_sn_owner != XCB_WINDOW_NONE) {
		if (!replace) {
			std::cout << "A window manager is already running on screen "
					<< screen() << std::endl;
			return false;
		} else {
			std::cout << "trying to replace the current owner" << std::endl;
			select_input(current_wm_sn_owner, XCB_EVENT_MASK_STRUCTURE_NOTIFY);

			/**
			 * we request to become the owner then we check if we successfully
			 * become the owner.
			 **/
			xcb_set_selection_owner(_xcb, w, wm_sn_atom, XCB_CURRENT_TIME);

			xcb_get_selection_owner_cookie_t ck = xcb_get_selection_owner(_xcb, wm_sn_atom);
			xcb_get_selection_owner_reply_t * r = xcb_get_selection_owner_reply(_xcb, ck, nullptr);

			/** If we are not the owner -> exit **/
			if(r == nullptr) {
				std::cout << "Error while getting selection owner of " << get_atom_name(wm_sn_atom) << std::endl;
				return false;
			}

			xcb_window_t new_wm_sn_owner = r->owner;
			free(r);

			if (new_wm_sn_owner != w) {
				std::cout << "Could not acquire the ownership of " << get_atom_name(wm_sn_atom) << std::endl;
				return false;
			}

			/** Now we are the owner, wait for max. 5 second that the previous owner to exit */
			{
				page::time64_t end;
				page::time64_t cur;

				struct pollfd fds[1];
				fds[0].fd = fd();
				fds[0].events = POLLIN | POLLOUT | POLLERR | POLLHUP;

				cur.update_to_current_time();
				end = cur + page::time64_t{5L, 0L};

				bool destroyed = false;
				xcb_flush(_xcb);
				while (cur < end and not destroyed) {
					int timeout = (end - cur).milliseconds();
					poll(fds, 1, timeout);
					fetch_pending_events();
					destroyed = check_for_destroyed_window(current_wm_sn_owner);
					cur.update_to_current_time();
				}

				if (not destroyed) {
					printf("The WM on screen %d is not exiting\n", screen());
					return false;
				}
			}

		}
	} else {
		xcb_set_selection_owner(_xcb, w, wm_sn_atom, XCB_CURRENT_TIME);

		xcb_get_selection_owner_cookie_t ck = xcb_get_selection_owner(_xcb, wm_sn_atom);
		xcb_get_selection_owner_reply_t * r = xcb_get_selection_owner_reply(_xcb, ck, nullptr);

		if(r == nullptr) {
			std::cout << "Error while getting selection owner of " << get_atom_name(wm_sn_atom) << std::endl;
			return false;
		}

		current_wm_sn_owner = r->owner;
		free(r);

		if (current_wm_sn_owner != w) {
			std::cout << "Could not acquire the ownership of " << get_atom_name(wm_sn_atom) << std::endl;
			return false;
		}

	}

	printf("Window manager is registered on screen %d.\n", screen());
	return true;
}


/**
 * Register composite manager. if another one is in place just fail to take
 * the ownership.
 **/
bool display_t::register_cm(xcb_window_t w) {
	xcb_generic_error_t * err;
	xcb_window_t current_cm;

	/** read if there is a compositor **/
	xcb_get_selection_owner_cookie_t ck = xcb_get_selection_owner(_xcb, cm_sn_atom);
	xcb_get_selection_owner_reply_t * r = xcb_get_selection_owner_reply(_xcb, ck, &err);

	if(r == nullptr or err != nullptr) {
		std::cout << "Error while getting selection owner of " << get_atom_name(cm_sn_atom) << std::endl;
		return false;
	}

	current_cm = r->owner;
	free(r);

	if (current_cm != XCB_WINDOW_NONE) {
		std::cout << "Could not acquire the ownership of " << get_atom_name(cm_sn_atom) << std::endl;
		return false;
	} else {
		/** become the compositor **/
		xcb_set_selection_owner(_xcb, w, cm_sn_atom, XCB_CURRENT_TIME);

		xcb_get_selection_owner_cookie_t ck = xcb_get_selection_owner(_xcb, cm_sn_atom);
		xcb_get_selection_owner_reply_t * r = xcb_get_selection_owner_reply(_xcb, ck, &err);

		if(r == nullptr or err != nullptr) {
			std::cout << "Error while getting selection owner of " << get_atom_name(cm_sn_atom) << std::endl;
			return false;
		}

		current_cm = r->owner;
		free(r);

		if (current_cm != w) {
			std::cout << "Could not acquire the ownership of " << get_atom_name(cm_sn_atom) << std::endl;
			return false;
		}
		printf("Composite manager is registered on screen %d.\n", screen());
		return true;
	}
}

void display_t::unregister_cm() {
	xcb_atom_t a_cm;
	static char net_wm_cm[] = "_NET_WM_CM_Sxx";
	snprintf(net_wm_cm, sizeof(net_wm_cm), "_NET_WM_CM_S%d", screen());
	a_cm = get_atom(net_wm_cm);
	xcb_set_selection_owner(_xcb, XCB_WINDOW_NONE, a_cm, XCB_CURRENT_TIME);
}

void display_t::add_to_save_set(xcb_window_t w) {
	xcb_change_save_set(_xcb, XCB_SET_MODE_INSERT, w);
}

void display_t::remove_from_save_set(xcb_window_t w) {
	xcb_change_save_set(_xcb, XCB_SET_MODE_DELETE, w);
}

void display_t::move_resize(xcb_window_t w, rect const & size) {

	uint16_t mask = 0;
	uint32_t value[4];

	mask |= XCB_CONFIG_WINDOW_X;
	value[0] = size.x;

	mask |= XCB_CONFIG_WINDOW_Y;
	value[1] = size.y;

	mask |= XCB_CONFIG_WINDOW_WIDTH;
	value[2] = size.w;

	mask |= XCB_CONFIG_WINDOW_HEIGHT;
	value[3] = size.h;
	xcb_void_cookie_t ck = xcb_configure_window(_xcb, w, mask, value);
	//printf("%08u move_resize(%u, %d, %d, %d, %d)\n", ck.sequence, w, size.x, size.y, size.w, size.h);

}

void display_t::raise_window(xcb_window_t w) {
	uint32_t mode = XCB_STACK_MODE_ABOVE;
	xcb_configure_window(_xcb, w, XCB_CONFIG_WINDOW_STACK_MODE, &mode);
}

void display_t::delete_property(xcb_window_t w, atom_e property) {
	xcb_delete_property(_xcb, w, A(property));
}

void display_t::lower_window(xcb_window_t w) {
	uint32_t mode = XCB_STACK_MODE_BELOW;
	xcb_configure_window(_xcb, w, XCB_CONFIG_WINDOW_STACK_MODE, &mode);
}

void display_t::set_input_focus(xcb_window_t focus, int revert_to, xcb_timestamp_t time) {
	xcb_void_cookie_t ck = xcb_set_input_focus(_xcb, revert_to, focus, time);
	//std::cout << "set_input_focus #" << focus << " #" << ck.sequence << std::endl;
}

void display_t::fake_configure(xcb_window_t w, rect location, int border_width) {
	xcb_configure_notify_event_t xev;
	xev.response_type = XCB_CONFIGURE_NOTIFY;
	xev.event = w;
	xev.window = w;

	/* if ConfigureRequest happen, override redirect is False */
	xev.override_redirect = False;
	xev.border_width = border_width;
	xev.above_sibling = None;

	/* send mandatory fake event */
	xev.x = location.x;
	xev.y = location.y;
	xev.width = location.w;
	xev.height = location.h;

	xcb_send_event(xcb(), false, w, XCB_EVENT_MASK_STRUCTURE_NOTIFY, reinterpret_cast<char*>(&xev));

}

//Display * display_t::dpy() {
//	return _dpy;
//}

xcb_connection_t * display_t::xcb() {
	return _xcb;
}

bool display_t::query_extension(char const * name, int * opcode, int * event, int * error) {
	xcb_generic_error_t * err;
	xcb_query_extension_cookie_t ck = xcb_query_extension(_xcb, strlen(name), name);
	xcb_query_extension_reply_t * r = xcb_query_extension_reply(_xcb, ck, &err);
	if (err != nullptr or r == nullptr) {
		return false;
	} else {
		*opcode = r->major_opcode;
		*event = r->first_event;
		*error = r->first_error;
		std::cout << "Extension " << name << " found opcode=" << static_cast<int>(r->major_opcode) << " event=" << static_cast<int>(r->first_event) << " error=" << static_cast<int>(r->first_error) << std::endl;
		free(r);
		return true;
	}
}


bool display_t::check_composite_extension() {
	if (not query_extension("Composite", &composite_opcode, &composite_event, &composite_error)) {
		return false;
	} else {
		xcb_generic_error_t * err;
		xcb_composite_query_version_cookie_t ck = xcb_composite_query_version(_xcb, XCB_COMPOSITE_MAJOR_VERSION, XCB_COMPOSITE_MINOR_VERSION);
		xcb_composite_query_version_reply_t * r = xcb_composite_query_version_reply(_xcb, ck, &err);

		if(r == nullptr or err != nullptr)
			throw exception_t("ERROR: fail to get Composite version");

		printf("Composite Extension version %d.%d found\n", r->major_version, r->minor_version);
		free(r);
		return true;
	}
}

bool display_t::check_damage_extension() {
	if (not query_extension("DAMAGE", &damage_opcode, &damage_event, &damage_error)) {
		return false;
	} else {
		xcb_generic_error_t * err;
		xcb_damage_query_version_cookie_t ck = xcb_damage_query_version(_xcb, XCB_DAMAGE_MAJOR_VERSION, XCB_DAMAGE_MINOR_VERSION);
		xcb_damage_query_version_reply_t * r = xcb_damage_query_version_reply(_xcb, ck, &err);

		if(r == nullptr or err != nullptr)
			throw exception_t("ERROR: fail to get DAMAGE version");

		printf("DAMAGE Extension version %d.%d found\n", r->major_version, r->minor_version);
		free(r);
		return true;
	}
}

bool display_t::check_xfixes_extension() {
	if (not query_extension("XFIXES", &fixes_opcode, &fixes_event, &fixes_error)) {
		return false;
	} else {
		xcb_generic_error_t * err;
		xcb_xfixes_query_version_cookie_t ck = xcb_xfixes_query_version(_xcb, XCB_XFIXES_MAJOR_VERSION, XCB_XFIXES_MINOR_VERSION);
		xcb_xfixes_query_version_reply_t * r = xcb_xfixes_query_version_reply(_xcb, ck, &err);

		if(r == nullptr or err != nullptr)
			throw exception_t("ERROR: fail to get XFIXES version");

		printf("XFIXES Extension version %d.%d found\n", r->major_version, r->minor_version);
		free(r);
		return true;
	}
}


bool display_t::check_shape_extension() {
	if (not query_extension(SHAPENAME, &shape_opcode, &shape_event, &shape_error)) {
		return false;
	} else {
		xcb_generic_error_t * err;
		xcb_shape_query_version_cookie_t ck = xcb_shape_query_version(_xcb);
		xcb_shape_query_version_reply_t * r = xcb_shape_query_version_reply(_xcb, ck, &err);

		if(r == nullptr or err != nullptr)
			throw exception_t("ERROR: fail to get " SHAPENAME " version");

		printf(SHAPENAME " Extension version %d.%d found\n", r->major_version, r->minor_version);
		free(r);
		return true;
	}
}

bool display_t::check_randr_extension() {
	if (not query_extension("RANDR", &randr_opcode, &randr_event, &randr_error)) {
		return false;
	} else {
		xcb_generic_error_t * err;
		xcb_randr_query_version_cookie_t ck = xcb_randr_query_version(_xcb, XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);
		xcb_randr_query_version_reply_t * r = xcb_randr_query_version_reply(_xcb, ck, &err);

		if(r == nullptr or err != nullptr)
			throw exception_t("ERROR: fail to get RANDR version");

		printf("RANDR Extension version %d.%d found\n", r->major_version, r->minor_version);
		free(r);
		return true;
	}
}

xcb_screen_t * display_t::screen_of_display (xcb_connection_t *c, int screen)
{
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(c));
  for(; iter.rem; --screen, xcb_screen_next(&iter))
    if (screen == 0)
      return iter.data;

  return nullptr;
}

void display_t::update_default_visual() {

	/* you init the connection and screen_nbr */
	_screen = screen_of_display(xcb(), _default_screen);

	printf("found screen %p\n", _screen);
	if (_screen != nullptr) {
		xcb_depth_iterator_t depth_iter;
		depth_iter = xcb_screen_allowed_depths_iterator(_screen);
		for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
			xcb_visualtype_iterator_t visual_iter;

			visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
			for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {

				_xcb_visual_data[visual_iter.data->visual_id] = visual_iter.data;
				_xcb_visual_depth[visual_iter.data->visual_id] = depth_iter.data->depth;

				if(visual_iter.data->_class == XCB_VISUAL_CLASS_TRUE_COLOR
						and depth_iter.data->depth == 32) {
					_xcb_default_visual_type = visual_iter.data;
				}

				if(visual_iter.data->visual_id == _screen->root_visual
						and depth_iter.data->depth == _screen->root_depth) {
					_xcb_root_visual_type = visual_iter.data;
				}
			}
		}
	}
}

xcb_visualtype_t * display_t::default_visual_rgba() {
	return _xcb_default_visual_type;
}

xcb_visualtype_t * display_t::find_visual(xcb_visualid_t id) {
	return _xcb_visual_data[id];
}

uint32_t display_t::find_visual_depth(xcb_visualid_t id) {
	return _xcb_visual_depth[id];
}

xcb_visualtype_t * display_t::root_visual() {
	return _xcb_root_visual_type;
}

void display_t::print_visual_type(xcb_visualtype_t * vis) {
	printf("visual id: 0x%x\n", vis->visual_id);
	printf("visual class: %u\n", vis->_class);
	printf("visual bits_per_rgb_value: %u\n", vis->bits_per_rgb_value);
	printf("visual blue_mask: %x\n", vis->blue_mask);
	printf("visual red_mask: %x\n", vis->red_mask);
	printf("visual green_mask: %x\n", vis->green_mask);
	printf("visual colormap_entries: %d\n", vis->colormap_entries);
}

xcb_screen_t * display_t::xcb_screen() {
	return _screen;
}


/**
 * Look at coming events if a client is manageable.
 *
 * i.e. a client is manageable if :
 *  1. is child of root window.
 *  2. is mapped.
 *  3. is not destroyed.
 **/
bool display_t::check_for_fake_unmap_window(xcb_window_t w) {
	for (auto i : pending_event) {
		if (i->response_type == (XCB_UNMAP_NOTIFY|0x80)) {
			if (reinterpret_cast<xcb_unmap_notify_event_t const *>(i)->window == w)
				return true;
		}
	}
	return false;
}

bool display_t::check_for_unmap_window(xcb_window_t w) {
	for (auto i : pending_event) {
		if (i->response_type == XCB_UNMAP_NOTIFY) {
			if (reinterpret_cast<xcb_unmap_notify_event_t const *>(i)->window == w)
				return true;
		}
	}
	return false;
}


/**
 * Look at coming events if a client is manageable.
 *
 * i.e. a client is manageable if :
 *  1. is child of root window.
 *  2. is mapped.
 *  3. is not destroyed.
 **/
bool display_t::check_for_reparent_window(xcb_window_t w) {
	for (auto i : pending_event) {
		if (i->response_type == XCB_REPARENT_NOTIFY) {
			if (reinterpret_cast<xcb_reparent_notify_event_t const *>(i)->window == w)
				return true;
		}
	}
	return false;
}

/**
 * Look for coming event if the window is destroyed. Used to
 * Skip events related to destroyed windows.
 **/
bool display_t::check_for_destroyed_window(xcb_window_t w) {
	for (auto i : pending_event) {
		if (i->response_type == XCB_DESTROY_NOTIFY) {
			if (reinterpret_cast<xcb_destroy_notify_event_t const *>(i)->window == w)
				return true;
		}
	}
	return false;
}

void display_t::fetch_pending_events() {
	/** get all event and store them in pending event **/
	xcb_generic_event_t * e = xcb_poll_for_event(_xcb);
	while (e != nullptr) {
		pending_event.push_back(e);
		filter_events(e);
		e = xcb_poll_for_event(_xcb);
	}
}

xcb_generic_event_t * display_t::front_event() {
	if(not pending_event.empty()) {
		return pending_event.front();
	} else {
		xcb_generic_event_t * e = xcb_poll_for_event(_xcb);
		if(e != nullptr) {
			pending_event.push_back(e);
			filter_events(e);
			return pending_event.front();
		}
	}
	return nullptr;
}

void display_t::pop_event() {
	if(not pending_event.empty()) {
		free(pending_event.front());
		pending_event.pop_front();
	}
}

bool display_t::has_pending_events() {
	return front_event() != nullptr;
}

void display_t::clear_events() {
	pending_event.clear();
}

std::list<xcb_generic_event_t *> const & display_t::get_pending_events_list() {
	return pending_event;
}

xcb_cursor_t display_t::_load_cursor(uint16_t cursor_id) {
	xcb_cursor_t cursor{xcb_generate_id(_xcb)};
	xcb_create_glyph_cursor(_xcb, cursor, cursor_font, cursor_font, cursor_id, cursor_id+1, 0, 0, 0, 0xffff, 0xffff, 0xffff);
	return cursor;

}

void display_t::load_cursors() {

	cursor_font = xcb_generate_id(_xcb);
	xcb_open_font(_xcb, cursor_font, strlen("cursor"), "cursor");

	xc_left_ptr = _load_cursor(XC_left_ptr);
	xc_fleur = _load_cursor(XC_fleur);
	xc_bottom_left_corner = _load_cursor(XC_bottom_left_corner);
	xc_bottom_righ_corner = _load_cursor(XC_bottom_right_corner);
	xc_bottom_side = _load_cursor(XC_bottom_side);
	xc_left_side = _load_cursor(XC_left_side);
	xc_right_side = _load_cursor(XC_right_side);
	xc_top_right_corner = _load_cursor(XC_top_right_corner);
	xc_top_left_corner = _load_cursor(XC_top_left_corner);
	xc_top_side = _load_cursor(XC_top_side);
	xc_sb_h_double_arrow = _load_cursor(XC_sb_h_double_arrow);
	xc_sb_v_double_arrow = _load_cursor(XC_sb_v_double_arrow);

}

void display_t::unload_cursors() {
	xcb_free_cursor(_xcb, xc_left_ptr);
	xcb_free_cursor(_xcb, xc_fleur);
	xcb_free_cursor(_xcb, xc_bottom_left_corner);
	xcb_free_cursor(_xcb, xc_bottom_righ_corner);
	xcb_free_cursor(_xcb, xc_bottom_side);
	xcb_free_cursor(_xcb, xc_left_side);
	xcb_free_cursor(_xcb, xc_right_side);
	xcb_free_cursor(_xcb, xc_top_right_corner);
	xcb_free_cursor(_xcb, xc_top_left_corner);
	xcb_free_cursor(_xcb, xc_top_side);
	xcb_free_cursor(_xcb, xc_sb_h_double_arrow);
	xcb_free_cursor(_xcb, xc_sb_v_double_arrow);

	xcb_close_font(_xcb, cursor_font);
}

void display_t::set_window_cursor(xcb_window_t w, xcb_cursor_t c) {
	xcb_change_window_attributes(_xcb, w, XCB_CW_CURSOR, &c);
}

xcb_window_t display_t::create_input_only_window(xcb_window_t parent,
		rect const & pos, uint32_t attrs_mask, uint32_t * attrs) {
	xcb_window_t id = xcb_generate_id(_xcb);
	xcb_void_cookie_t ck = xcb_create_window(_xcb, XCB_COPY_FROM_PARENT, id,
			parent, pos.x, pos.y, pos.w, pos.h, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
			XCB_COPY_FROM_PARENT, attrs_mask, attrs);
	return id;
}


/** undocumented : http://lists.freedesktop.org/pipermail/xorg/2005-January/005954.html **/
void display_t::allow_input_passthrough(xcb_window_t w) {
	xcb_xfixes_region_t region = xcb_generate_id(_xcb);
	xcb_void_cookie_t ck = xcb_xfixes_create_region_checked(_xcb, region, 0, 0);
	xcb_generic_error_t * err = xcb_request_check(_xcb, ck);
	if(err != nullptr) {
		throw exception_t("Fail to create region %d %d", err->major_code, err->minor_code);
	}

	xcb_discard_reply(_xcb, ck.sequence);

	/**
	 * Shape for the entire of window.
	 **/
	xcb_xfixes_set_window_shape_region(_xcb, w, XCB_SHAPE_SK_BOUNDING, 0, 0, XCB_XFIXES_REGION_NONE);
	/**
	 * input shape was introduced by Keith Packard to define an input area of
	 * window by default is the ShapeBounding which is used. here we set input
	 * area an empty region.
	 **/
	xcb_xfixes_set_window_shape_region(_xcb, w, XCB_SHAPE_SK_INPUT, 0, 0, region);
	xcb_xfixes_destroy_region(_xcb, region);
}

void display_t::disable_input_passthrough(xcb_window_t w) {
	xcb_xfixes_set_window_shape_region(_xcb, w, XCB_SHAPE_SK_BOUNDING, 0, 0, XCB_XFIXES_REGION_NONE);
	xcb_xfixes_set_window_shape_region(_xcb, w, XCB_SHAPE_SK_INPUT, 0, 0, XCB_XFIXES_REGION_NONE);
}

void display_t::set_net_active_window(xcb_window_t w) {
	net_active_window_t active;
	active.push(_xcb, _A, root(), new xcb_window_t{w});
	active.release(_xcb);
}

void display_t::select_input(xcb_window_t w, uint32_t mask) {
	xcb_change_window_attributes(_xcb, w, XCB_CW_EVENT_MASK, &mask);
}

void display_t::set_border_width(xcb_window_t w, uint32_t width) {
	xcb_configure_window(_xcb, w, XCB_CONFIG_WINDOW_BORDER_WIDTH, &width);
}

region display_t::read_damaged_region(xcb_damage_damage_t d) {

	region result;

	/* create an empty region */
	xcb_xfixes_region_t region{xcb_generate_id(_xcb)};
	xcb_xfixes_create_region(_xcb, region, 0, 0);

	xcb_xfixes_set_region(_xcb, region, 0, nullptr);

	/* get damaged region and remove them from damaged status */
	xcb_damage_subtract(_xcb, d, XCB_XFIXES_REGION_NONE, region);

	/* get all i_rects for the damaged region */
	xcb_xfixes_fetch_region_cookie_t ck = xcb_xfixes_fetch_region(_xcb, region);

	xcb_generic_error_t * err;
	xcb_xfixes_fetch_region_reply_t * r = xcb_xfixes_fetch_region_reply(_xcb, ck, &err);

	if (err == nullptr and r != nullptr) {
		xcb_rectangle_iterator_t i = xcb_xfixes_fetch_region_rectangles_iterator(r);
		while(i.rem > 0) {
			//printf("rect %dx%d+%d+%d\n", i.data->width, i.data->height, i.data->x, i.data->y);
			result += rect{i.data->x, i.data->y, i.data->width, i.data->height};
			xcb_rectangle_next(&i);
		}
		free(r);
	} else {
		throw exception_t{"Could not fetch region"};
	}

	xcb_xfixes_destroy_region(_xcb, region);

	return result;
}

void display_t::check_x11_extension() {
	if (not check_xfixes_extension()) {
		throw std::runtime_error("XFIXES extension is not supported");
	}

	if (not check_shape_extension()) {
		throw std::runtime_error(SHAPENAME " extension is not supported");
	}

	if (not check_randr_extension()) {
		throw std::runtime_error("RANDR extension is not supported");
	}

	has_composite = check_composite_extension();

	if (not check_damage_extension()) {
		throw std::runtime_error("DAMAGE extension is not supported");
	}

	for(auto &i: event_type_name) {
		i = "UnknownEvent";
	}

	for(unsigned k = 0; k < (sizeof(xcore_event_name)/sizeof(char *)); ++k) {
		event_type_name[k] = xcore_event_name[k];
	}

	for(unsigned k = 0; k < (sizeof(xdamage_event_name)/sizeof(char *)); ++k) {
		event_type_name[damage_event+k] = xdamage_event_name[k];
	}

	for(unsigned k = 0; k < (sizeof(xcomposite_event_name)/sizeof(char *)); ++k) {
		event_type_name[composite_event+k] = xcomposite_event_name[k];
	}

	for(unsigned k = 0; k < (sizeof(xfixes_event_name)/sizeof(char *)); ++k) {
		event_type_name[fixes_event+k] = xfixes_event_name[k];
	}

	for(unsigned k = 0; k < (sizeof(xshape_event_name)/sizeof(char *)); ++k) {
		event_type_name[shape_event+k] = xshape_event_name[k];
	}

	for(unsigned k = 0; k < (sizeof(xrandr_event_name)/sizeof(char *)); ++k) {
		event_type_name[randr_event+k] = xrandr_event_name[k];
	}

	for(auto &i: error_type_name) {
		i = "UnknownError";
	}

	for(unsigned k = 0; k < (sizeof(xcore_minor_error_name)/sizeof(char *)); ++k) {
		error_type_name[k] = xcore_minor_error_name[k];
	}

	if(damage_error != 0)
	for(unsigned k = 0; k < (sizeof(xdamage_error_name)/sizeof(char *)); ++k) {
		error_type_name[damage_error+k] = xdamage_error_name[k];
	}

	if(composite_error != 0)
	for(unsigned k = 0; k < (sizeof(xcomposite_error_name)/sizeof(char *)); ++k) {
		error_type_name[composite_error+k] = xcomposite_error_name[k];
	}

	if(fixes_error != 0)
	for(unsigned k = 0; k < (sizeof(xfixes_error_name)/sizeof(char *)); ++k) {
		error_type_name[fixes_error+k] = xfixes_error_name[k];
	}

	if(shape_error != 0)
	for(unsigned k = 0; k < (sizeof(xshape_error_name)/sizeof(char *)); ++k) {
		error_type_name[shape_error+k] = xshape_error_name[k];
	}

	if(randr_error != 0)
	for(unsigned k = 0; k < (sizeof(xrandr_error_name)/sizeof(char *)); ++k) {
		error_type_name[randr_error+k] = xrandr_error_name[k];
	}

}

xcb_atom_t display_t::get_atom(char const * name) {
	xcb_intern_atom_cookie_t ck = xcb_intern_atom(_xcb, false, strlen(name), name);
	xcb_intern_atom_reply_t * r = xcb_intern_atom_reply(_xcb, ck, 0);
	if(r == nullptr)
		throw exception_t("Error while getting atom '%s'", name);
	xcb_atom_t atom = r->atom;
	free(r);
	return atom;
}

void display_t::print_error(xcb_generic_error_t const * err) {
	char const * fail_request = "UnkwonRequest";
	char const * type_name = error_type_name[err->error_code];

	if (err->major_code >= 0 and err->major_code < (sizeof(xcore_request_name)/sizeof(char *))) {
		fail_request = xcore_request_name[err->major_code];

	} else if (err->major_code == damage_opcode) {
		if(err->minor_code >= 0 and err->minor_code < (sizeof(xdamage_request_name)/sizeof(char *))) {
			fail_request = xdamage_request_name[err->minor_code];
		}
	} else if (err->major_code == composite_opcode) {
		if(err->minor_code >= 0 and err->minor_code < (sizeof(xcomposite_request_name)/sizeof(char *))) {
			fail_request = xcomposite_request_name[err->minor_code];
		}
	} else if (err->major_code == fixes_opcode) {
		if(err->minor_code >= 0 and err->minor_code < (sizeof(xfixes_request_name)/sizeof(char *))) {
			fail_request = xfixes_request_name[err->minor_code];
		}
	} else if (err->major_code == randr_opcode) {
		if(err->minor_code >= 0 and err->minor_code < (sizeof(xrandr_request_name)/sizeof(char *))) {
			fail_request = xrandr_request_name[err->minor_code];
		}
	} else if (err->major_code == shape_opcode) {
		if(err->minor_code >= 0 and err->minor_code < (sizeof(xshape_request_name)/sizeof(char *))) {
			fail_request = xshape_request_name[err->minor_code];
		}
	}

	printf("#%08d ERROR %s: %s (%u,%u,%u)\n", static_cast<int>(err->sequence), fail_request, type_name, static_cast<unsigned>(err->major_code), static_cast<unsigned>(err->minor_code), static_cast<unsigned>(err->error_code));
}

auto display_t::create_client_proxy(xcb_window_t w) -> shared_ptr<client_proxy_t> {
	auto x = _client_proxies.find(w);
	if(x == _client_proxies.end()) {
		auto y = make_shared<client_proxy_t>(this, w);
		_client_proxies[w] = y;
		return y;
	} else {
		return x->second;
	}
}

void display_t::filter_events(xcb_generic_event_t const * e) {
	if(e->response_type == XCB_DESTROY_NOTIFY) {
		auto ev = reinterpret_cast<xcb_destroy_notify_event_t const *>(e);
		auto x = _client_proxies.find(ev->window);
		if(x != _client_proxies.end()) {
			if(x->second->_views.empty()) {
				_client_proxies.erase(x);
			} else {
				x->second->destroyed(true);
			}
		}
	} else if (e->response_type == XCB_CONFIGURE_NOTIFY) {
		auto ev = reinterpret_cast<xcb_configure_notify_event_t const *>(e);
		auto x = _client_proxies.find(ev->window);
		if (x != _client_proxies.end()) {
			x->second->process_event(ev);
		}
	} else if (e->response_type == XCB_MAP_NOTIFY) {
		auto ev = reinterpret_cast<xcb_map_notify_event_t const *>(e);
		auto x = _client_proxies.find(ev->window);
		if (x != _client_proxies.end()) {
			x->second->on_map();
		}
	} else if (e->response_type == damage_event + XCB_DAMAGE_NOTIFY) {
		auto ev = reinterpret_cast<xcb_damage_notify_event_t const *>(e);
		auto x = _client_proxies.find(ev->drawable);
		if (x != _client_proxies.end()) {
			x->second->process_event(ev);
		}
	} else if (e->response_type == XCB_PROPERTY_NOTIFY) {
		auto ev = reinterpret_cast<xcb_property_notify_event_t const *>(e);
		auto x = _client_proxies.find(ev->window);
		if (x != _client_proxies.end()) {
			x->second->process_event(ev);
		}
	}
}

void display_t::disable() {
	if (_enable_composite) {
		_enable_composite = false;
		for (auto const & x : _client_proxies) {
			x.second->disable_redirect();
		}
	}
}


void display_t::enable() {
	if (not _enable_composite) {
		_enable_composite = true;
		for (auto const & x : _client_proxies) {
			x.second->enable_redirect();
		}
	}
}

void display_t::make_surface_stats(int & size, int & count) {
	size = 0;
	count = 0;
	for (auto &i : _client_proxies) {
		if (i.second->_pixmap != nullptr) {
			count += 1;
			size += (i.second->depth() / 8) * i.second->_geometry.width * i.second->_geometry.height;
		}
	}
}

auto display_t::create_view(xcb_window_t w) -> shared_ptr<client_view_t> {
	auto p = create_client_proxy(w);
	if(p->_views.empty()) {
		if(_enable_composite)
			p->enable_redirect();
		on_visibility_change.signal(p->_id, true);
	}
	auto x = p->create_view();
	return shared_ptr<client_view_t>(x, [this](client_view_t * x) -> void { this->destroy_view(x); });
}

void display_t::destroy_view(client_view_t * v) {
	auto p = v->_parent.lock();
	p->remove_view(v);
	delete v;

	if(p->_views.empty()) {
		on_visibility_change.signal(p->_id, false);
		p->_pixmap = nullptr;
		if(_enable_composite)
			p->disable_redirect();
		if(p->destroyed()) {
			auto x = _client_proxies.find(p->_id);
			if(x != _client_proxies.end()) {
				_client_proxies.erase(x);
			}
		}
	}
}


}

