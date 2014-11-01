/*
 * display.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "display.hxx"

#include <poll.h>

#include "utils.hxx"
#include "time.hxx"
#include "exception.hxx"

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

/* conveniant macro to get atom XID */
Atom display_t::A(atom_e atom) {
	return (*_A)(atom);
}

int display_t::screen() {
	return DefaultScreen(_dpy);
}

//int display_t::move_window(Window w, int x, int y) {
//	//printf("XMoveWindow #%lu %d %d\n", w, x, y);
//	return XMoveWindow(_dpy, w, x, y);
//}

display_t::display_t() {
	old_error_handler = XSetErrorHandler(error_handler);

	_dpy = XOpenDisplay(NULL);
	if (_dpy == NULL) {
		throw std::runtime_error("Could not open display");
	} else {
		cnx_printf("Open display : Success\n");
	}

	_fd = ConnectionNumber(_dpy);

	/** get xcb connection handler to enable xcb request **/
	_xcb = XGetXCBConnection(_dpy);
	XSetEventQueueOwner(_dpy, XCBOwnsEventQueue);

	grab_count = 0;

	_A = std::shared_ptr<atom_handler_t>(new atom_handler_t(_dpy));

	update_default_visual();

}

display_t::~display_t() {
	XCloseDisplay(_dpy);
}

void display_t::grab() {
	if (grab_count == 0) {
		xcb_void_cookie_t ck = xcb_grab_server_checked(_xcb);
		xcb_generic_error_t * err = xcb_request_check(_xcb, ck);
		if(err != nullptr) {
			throw exception_t{"%s:%d unable to grab X11 server", __FILE__, __LINE__};
		}
	}
	++grab_count;
}

void display_t::ungrab() {
	if (grab_count == 0) {
		fprintf(stderr, "TRY TO UNGRAB NOT GRABBED CONNECTION!\n");
		return;
	}
	--grab_count;
	if (grab_count == 0) {
		xcb_ungrab_server(_xcb);
		/* Ungrab the server immediately */
		if (xcb_flush(_xcb) <= 0)
			throw exception_t { "%s:%d unable to to flush X11 server", __FILE__,
					__LINE__ };
	}
}

bool display_t::is_not_grab() {
	return grab_count == 0;
}

void display_t::unmap(Window w) {
	cnx_printf("X_UnmapWindow: win = %lu\n", w);
	xcb_unmap_window(_xcb, w);
}

void display_t::reparentwindow(xcb_window_t w, xcb_window_t parent, int x, int y) {
	cnx_printf("Reparent serial: #%lu win: #%lu, parent: #%lu\n", w, parent);
	xcb_reparent_window(_xcb, w, parent, x, y);
}

void display_t::map(xcb_window_t w) {
	cnx_printf("X_MapWindow: win = %lu\n", w);
	xcb_map_window(_xcb, w);
}

void display_t::xnextevent(XEvent * ev) {
	XNextEvent(_dpy, ev);
}

/* this function come from xcompmgr
 * it is intend to make page as composite manager */

bool display_t::register_wm(xcb_window_t w, bool replace) {
	xcb_generic_error_t * err;
	Atom wm_sn_atom;
	xcb_window_t current_wm_sn_owner;

	static char wm_sn[] = "WM_Sxx";
	snprintf(wm_sn, sizeof(wm_sn), "WM_S%d", screen());
	wm_sn_atom = XInternAtom(_dpy, wm_sn, false);

	/** who is the current owner ? **/
	xcb_get_selection_owner_cookie_t ck = xcb_get_selection_owner(_xcb, wm_sn_atom);
	xcb_get_selection_owner_reply_t * r = xcb_get_selection_owner_reply(_xcb, ck, &err);

	if(r == nullptr or err != nullptr) {
		std::cout << "Error while getting selection owner of " << wm_sn << std::endl;
		return false;
	}

	current_wm_sn_owner = r->owner;
	free(r);

	/* if there is a current owner */
	if (current_wm_sn_owner != XCB_WINDOW_NONE) {
		if (!replace) {
			std::cout << "A window manager is already running on screen "
					<< screen() << std::endl;
			return false;
		} else {
			select_input(current_wm_sn_owner, XCB_EVENT_MASK_STRUCTURE_NOTIFY);

			/**
			 * we request to become the owner  then we check if we successfuly
			 * become the owner.
			 **/
			xcb_set_selection_owner(_xcb, w, wm_sn_atom, XCB_CURRENT_TIME);

			xcb_get_selection_owner_cookie_t ck = xcb_get_selection_owner(_xcb, wm_sn_atom);
			xcb_get_selection_owner_reply_t * r = xcb_get_selection_owner_reply(_xcb, ck, &err);

			/** If we are not the owner -> exit **/
			if(r == nullptr or err != nullptr) {
				std::cout << "Error while getting selection owner of " << wm_sn << std::endl;
				return false;
			} else if (r->owner != w) {
				std::cout << "Could not acquire the ownership of " << wm_sn << std::endl;
				free(r);
				return false;
			}

			free(r);

			/** Now we are the owner, wait for max. 5 second that the previous owner to exit */
			if (current_wm_sn_owner != XCB_WINDOW_NONE) {
				page::time_t end;
				page::time_t cur;

				struct pollfd fds[1];
				fds[0].fd = fd();
				fds[0].events = POLLIN | POLLOUT | POLLERR | POLLHUP;

				cur.get_time();
				end = cur + page::time_t{5L, 0L};

				bool destroyed = false;
				xcb_flush(_xcb);
				while (cur < end and not destroyed) {
					int timeout = (end - cur).milliseconds();
					poll(fds, 1, timeout);
					fetch_pending_events();
					destroyed = check_for_destroyed_window(current_wm_sn_owner);
					cur.get_time();
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
		xcb_get_selection_owner_reply_t * r = xcb_get_selection_owner_reply(_xcb, ck, &err);

		if(r == nullptr or err != nullptr) {
			std::cout << "Error while getting selection owner of " << wm_sn << std::endl;
			return false;
		} else if (r->owner != w) {
			std::cout << "Could not acquire the ownership of " << wm_sn << std::endl;
			free(r);
			return false;
		}

		free(r);

	}

	return true;
}


/**
 * Register composite manager. if another one is in place just fail to take
 * the ownership.
 **/
bool display_t::register_cm(xcb_window_t w, bool replace) {
	xcb_generic_error_t * err;
	xcb_window_t current_cm;
	Atom a_cm;
	static char net_wm_cm[] = "_NET_WM_CM_Sxx";
	snprintf(net_wm_cm, sizeof(net_wm_cm), "_NET_WM_CM_S%d", screen());
	a_cm = XInternAtom(_dpy, net_wm_cm, False);

	/** read if there is a compositor **/
	xcb_get_selection_owner_cookie_t ck = xcb_get_selection_owner(_xcb, a_cm);
	xcb_get_selection_owner_reply_t * r = xcb_get_selection_owner_reply(_xcb, ck, &err);

	if(r == nullptr or err != nullptr) {
		std::cout << "Error while getting selection owner of " << net_wm_cm << std::endl;
		return false;
	}

	current_cm = r->owner;
	free(r);

	if (r->owner != XCB_WINDOW_NONE) {
		std::cout << "Could not acquire the ownership of " << net_wm_cm << std::endl;
		return false;
	} else {
		/** become the compositor **/
		xcb_set_selection_owner(_xcb, w, a_cm, XCB_CURRENT_TIME);

		xcb_get_selection_owner_cookie_t ck = xcb_get_selection_owner(_xcb, a_cm);
		xcb_get_selection_owner_reply_t * r = xcb_get_selection_owner_reply(_xcb, ck, &err);

		if(r == nullptr or err != nullptr) {
			std::cout << "Error while getting selection owner of " << net_wm_cm << std::endl;
			return false;
		} else if (r->owner != w) {
			std::cout << "Could not acquire the ownership of " << net_wm_cm << std::endl;
			return false;
		}
		printf("Composite manager is registered on screen %d\n", screen());
		return true;
	}
}

void display_t::add_to_save_set(Window w) {
	cnx_printf("XAddToSaveSet: win = %lu\n", w);
	XAddToSaveSet(_dpy, w);
}

void display_t::remove_from_save_set(Window w) {
	cnx_printf("XRemoveFromSaveSet: win = %lu\n", w);
	XRemoveFromSaveSet(_dpy, w);
}

void display_t::move_resize(Window w, i_rect const & size) {

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

	//printf("move_resize(%lu, %d, %d, %d, %d)\n", w, size.x, size.y, size.w, size.h);
	xcb_configure_window(_xcb, w, mask, value);

}

//void display_t::set_window_border_width(Window w, unsigned int width) {
//	cnx_printf("XSetWindowBorderWidth: win = %lu, width = %u\n", w, width);
//	XSetWindowBorderWidth(_dpy, w, width);
//}

void display_t::raise_window(Window w) {
	cnx_printf("XRaiseWindow: win = %lu\n", w);
	XRaiseWindow(_dpy, w);
}

void display_t::delete_property(Window w, atom_e property) {
	XDeleteProperty(_dpy, w, A(property));
}

Status display_t::get_window_attributes(Window w,
		XWindowAttributes * window_attributes_return) {
	cnx_printf("XGetWindowAttributes: win = %lu\n", w);
	return XGetWindowAttributes(_dpy, w, window_attributes_return);
}

Status display_t::get_text_property(Window w, XTextProperty * text_prop_return,
		atom_e property) {

	cnx_printf("XGetTextProperty: win = %lu\n", w);
	return XGetTextProperty(_dpy, w, text_prop_return, A(property));
}

int display_t::lower_window(Window w) {

	cnx_printf("XLowerWindow: win = %lu\n", w);
	return XLowerWindow(_dpy, w);
}

static void _safe_xfree(void * x) {
	if (x != NULL)
		XFree(x);
}

/* used for debuging, do not optimize with some cache */
std::shared_ptr<char> display_t::get_atom_name(Atom atom) {
	cnx_printf("XGetAtomName: atom = %lu\n", atom);
	return std::shared_ptr<char>(XGetAtomName(_dpy, atom), _safe_xfree);
}

Status display_t::send_event(Window w, Bool propagate, long event_mask,
		XEvent* event_send) {

	cnx_printf("XSendEvent: win = %lu\n", w);
	return XSendEvent(_dpy, w, propagate, event_mask, event_send);
}

int display_t::set_input_focus(Window focus, int revert_to, Time time) {
	cnx_printf("XSetInputFocus: win = %lu, time = %lu\n", focus, time);
	fflush(stdout);
	return XSetInputFocus(_dpy, focus, revert_to, time);
}

void display_t::fake_configure(Window w, i_rect location, int border_width) {
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

	xcb_send_event(xcb(), False, w, XCB_EVENT_MASK_STRUCTURE_NOTIFY, reinterpret_cast<char*>(&xev));

}

Display * display_t::dpy() {
	return _dpy;
}

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
		free(r);
		return true;
	}
}


bool display_t::check_composite_extension() {
	if (not query_extension(COMPOSITE_NAME, &composite_opcode, &composite_event, &composite_error)) {
		return false;
	} else {
		xcb_generic_error_t * err;
		xcb_composite_query_version_cookie_t ck = xcb_composite_query_version(_xcb, XCB_COMPOSITE_MAJOR_VERSION, XCB_COMPOSITE_MINOR_VERSION);
		xcb_composite_query_version_reply_t * r = xcb_composite_query_version_reply(_xcb, ck, &err);

		if(r == nullptr or err != nullptr)
			throw exception_t("ERROR: fail to get " COMPOSITE_NAME " version");

		printf(COMPOSITE_NAME " Extension version %d.%d found\n", r->major_version, r->minor_version);
		return true;
	}
}

bool display_t::check_damage_extension() {
	if (not query_extension(DAMAGE_NAME, &damage_opcode, &damage_event, &damage_error)) {
		return false;
	} else {
		xcb_generic_error_t * err;
		xcb_damage_query_version_cookie_t ck = xcb_damage_query_version(_xcb, XCB_DAMAGE_MAJOR_VERSION, XCB_DAMAGE_MINOR_VERSION);
		xcb_damage_query_version_reply_t * r = xcb_damage_query_version_reply(_xcb, ck, &err);

		if(r == nullptr or err != nullptr)
			throw exception_t("ERROR: fail to get " DAMAGE_NAME " version");

		printf(DAMAGE_NAME " Extension version %d.%d found\n", r->major_version, r->minor_version);
		return true;
	}
}

bool display_t::check_xfixes_extension() {
	if (not query_extension(XFIXES_NAME, &fixes_opcode, &fixes_event, &fixes_error)) {
		return false;
	} else {
		xcb_generic_error_t * err;
		xcb_xfixes_query_version_cookie_t ck = xcb_xfixes_query_version(_xcb, XCB_XFIXES_MAJOR_VERSION, XCB_XFIXES_MINOR_VERSION);
		xcb_xfixes_query_version_reply_t * r = xcb_xfixes_query_version_reply(_xcb, ck, &err);

		if(r == nullptr or err != nullptr)
			throw exception_t("ERROR: fail to get " XFIXES_NAME " version");

		printf(XFIXES_NAME " Extension version %d.%d found\n", r->major_version, r->minor_version);
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
		return true;
	}
}

bool display_t::check_randr_extension() {
	if (not query_extension(RANDR_NAME, &randr_opcode, &randr_event, &randr_error)) {
		return false;
	} else {
		xcb_generic_error_t * err;
		xcb_randr_query_version_cookie_t ck = xcb_randr_query_version(_xcb, XCB_RANDR_MAJOR_VERSION, XCB_RANDR_MINOR_VERSION);
		xcb_randr_query_version_reply_t * r = xcb_randr_query_version_reply(_xcb, ck, &err);

		if(r == nullptr or err != nullptr)
			throw exception_t("ERROR: fail to get " RANDR_NAME " version");

		printf(RANDR_NAME " Extension version %d.%d found\n", r->major_version, r->minor_version);
		return true;
	}
}


bool display_t::check_dbe_extension() {
	if (!XQueryExtension(_dpy, DBE_PROTOCOL_NAME, &dbe_opcode, &dbe_event, &dbe_error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XdbeQueryExtension(_dpy, &major, &minor);
		printf(DBE_PROTOCOL_NAME " Extension version %d.%d found\n", major,
				minor);
		return true;
	}
}

xcb_screen_t * display_t::screen_of_display (xcb_connection_t *c, int screen)
{
  xcb_screen_iterator_t iter;

  iter = xcb_setup_roots_iterator (xcb_get_setup (c));
  for (; iter.rem; --screen, xcb_screen_next (&iter))
    if (screen == 0)
      return iter.data;

  return NULL;
}

void display_t::update_default_visual() {
	int screen_nbr = this->screen();
	/* you init the connection and screen_nbr */

	_screen = screen_of_display(xcb(), screen_nbr);

	printf("found screen %p\n", _screen);
	if (_screen) {
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

xcb_visualtype_t * display_t::default_visual() {
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
	xcb_generic_event_t * e = xcb_poll_for_queued_event(_xcb);
	while (e != nullptr) {
		pending_event.push_back(e);
		e = xcb_poll_for_event(_xcb);
	}
}

xcb_generic_event_t * display_t::front_event() {
	if(not pending_event.empty()) {
		return pending_event.front();
	} else {
		xcb_generic_event_t * e = xcb_poll_for_queued_event(_xcb);
		if(e != nullptr) {
			pending_event.push_back(e);
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

	xcb_close_font(_xcb, cursor_font);
}

void display_t::set_window_cursor(xcb_window_t w, xcb_cursor_t c) {
	uint32_t attrs = c;
	xcb_change_window_attributes(_xcb, w, XCB_CW_CURSOR, &attrs);
}

xcb_window_t display_t::create_input_only_window(xcb_window_t parent,
		i_rect const & pos, uint32_t attrs_mask, uint32_t * attrs) {
	xcb_window_t id = xcb_generate_id(_xcb);
	xcb_void_cookie_t ck = xcb_create_window(_xcb, XCB_COPY_FROM_PARENT, id,
			parent, pos.x, pos.y, pos.w, pos.h, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
			XCB_COPY_FROM_PARENT, attrs_mask, attrs);
	return id;
}


/** undocumented : http://lists.freedesktop.org/pipermail/xorg/2005-January/005954.html **/
void display_t::allow_input_passthrough(xcb_window_t w) {
	xcb_xfixes_region_t region = xcb_generate_id(_xcb);
	xcb_rectangle_t nill{0,0,0,0};
	xcb_void_cookie_t ck = xcb_xfixes_create_region_checked(_xcb, region, 0, 0);
	xcb_generic_error_t * err = xcb_request_check(_xcb, ck);
	if(err != nullptr) {
		throw exception_t("Fail to create region %d %d", err->major_code, err->minor_code);
	}
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

int display_t::error_handler(Display * dpy, XErrorEvent * ev) {
	fprintf(stderr,"#%08lu ERROR, major_code: %u, minor_code: %u, error_code: %u\n",
			ev->serial, ev->request_code, ev->minor_code, ev->error_code);

	static const unsigned int XFUNCSIZE = (sizeof(x_function_codes)/sizeof(char *));

	if (ev->request_code < XFUNCSIZE) {
		char const * func_name = x_function_codes[ev->request_code];
		char error_text[1024];
		error_text[0] = 0;
		XGetErrorText(dpy, ev->error_code, error_text, 1024);
		fprintf(stderr, "#%08lu ERROR, %s : %s\n", ev->serial, func_name, error_text);
	}
	return 0;
}

void display_t::set_net_active_window(xcb_window_t w) {
	net_active_window_t active{new xcb_window_t{w}};
	write_property(root(), active);
}

void display_t::select_input(xcb_window_t w, uint32_t mask) {
	xcb_change_window_attributes(_xcb, w, XCB_CW_EVENT_MASK, &mask);
}


region display_t::read_damaged_region(xcb_damage_damage_t d) {

	region result;

	/* create an empty region */
	xcb_xfixes_region_t region = xcb_generate_id(_xcb);
	xcb_xfixes_create_region(_xcb, region, 0, 0);

	/* get damaged region and remove them from damaged status */
	xcb_damage_subtract(_xcb, d, XCB_XFIXES_REGION_NONE, region);


	/* get all i_rects for the damaged region */

	xcb_xfixes_fetch_region_cookie_t ck = xcb_xfixes_fetch_region(_xcb, region);

	xcb_generic_error_t * err;
	xcb_xfixes_fetch_region_reply_t * r = xcb_xfixes_fetch_region_reply(_xcb, ck, &err);

	if (err == nullptr and r != nullptr) {
		xcb_rectangle_t * rect = xcb_xfixes_fetch_region_rectangles(r);
		for (unsigned k = 0; k < r->length; ++k) {
			//printf("rect %dx%d+%d+%d\n", rects[i].width, rects[i].height, rects[i].x, rects[i].y);
			result += i_rect(rect[k]);
		}
		//free(rect);
		free(r);
	}

	xcb_xfixes_destroy_region(_xcb, region);

	return result;
}

void display_t::check_x11_extension() {
	if (not check_xfixes_extension()) {
		throw std::runtime_error(XFIXES_NAME " extension is not supported");
	}

	if (not check_shape_extension()) {
		throw std::runtime_error(SHAPENAME " extension is not supported");
	}

	if (not check_randr_extension()) {
		throw std::runtime_error(RANDR_NAME " extension is not supported");
	}

	if (not check_composite_extension()) {
		throw std::runtime_error("X Server doesn't support Composite 0.4");
	}

	if (not check_damage_extension()) {
		throw std::runtime_error("Damage extension is not supported");
	}

	if (not check_shape_extension()) {
		throw std::runtime_error(SHAPENAME " extension is not supported");
	}

	if (not check_randr_extension()) {
		throw std::runtime_error(RANDR_NAME " extension is not supported");
	}


	for(unsigned k = 0; k < (sizeof(x_function_codes)/sizeof(char *)); ++k) {
		request_type_name[k] = x_function_codes[k];
	}

	for(unsigned k = 0; k < (sizeof(xdamage_func)/sizeof(char *)); ++k) {
		request_type_name[damage_opcode+k] = xdamage_func[k];
	}

	for(unsigned k = 0; k < (sizeof(xcomposite_request_name)/sizeof(char *)); ++k) {
		request_type_name[composite_opcode+k] = xcomposite_request_name[k];
	}

}

}

