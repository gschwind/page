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

Window display_t::root() {
	return DefaultRootWindow(_dpy);
}

/* conveniant macro to get atom XID */
Atom display_t::A(atom_e atom) {
	return (*_A)(atom);
}

int display_t::screen() {
	return DefaultScreen(_dpy);
}

//std::vector<std::string> * display_t::read_wm_class(Window w) {
//	std::vector<char> * tmp = ::page::get_window_property<char>(_dpy, w, A(WM_CLASS), A(STRING));
//
//	if(tmp == nullptr)
//		return nullptr;
//
//	std::vector<char>::iterator x = find(tmp->begin(), tmp->end(), 0);
//	if(x != tmp->end()) {
//		std::vector<std::string> * ret = new std::vector<std::string>;
//		ret->push_back(std::string(tmp->begin(), x));
//		auto x1 = find(++x, tmp->end(), 0);
//		ret->push_back(std::string(x, x1));
//		delete tmp;
//		return ret;
//	}
//
//	delete tmp;
//	return 0;
//}
//
//XWMHints * display_t::read_wm_hints(Window w) {
//	std::vector<long> * tmp = ::page::get_window_property<long>(_dpy, w, A(WM_HINTS),
//			A(WM_HINTS));
//	if (tmp != 0) {
//		if (tmp->size() == 9) {
//			XWMHints * hints = new XWMHints;
//			if (hints != 0) {
//				hints->flags = (*tmp)[0];
//				hints->input = (*tmp)[1];
//				hints->initial_state = (*tmp)[2];
//				hints->icon_pixmap = (*tmp)[3];
//				hints->icon_window = (*tmp)[4];
//				hints->icon_x = (*tmp)[5];
//				hints->icon_y = (*tmp)[6];
//				hints->icon_mask = (*tmp)[7];
//				hints->window_group = (*tmp)[8];
//			}
//			delete tmp;
//			return hints;
//		}
//		delete tmp;
//	}
//	return 0;
//}
//
//motif_wm_hints_t * display_t::read_motif_wm_hints(Window w) {
//	std::vector<long> * tmp = ::page::get_window_property<long>(_dpy, w,
//			A(_MOTIF_WM_HINTS), A(_MOTIF_WM_HINTS));
//	if (tmp != 0) {
//		motif_wm_hints_t * hints = new motif_wm_hints_t;
//		if (tmp->size() == 5) {
//			if (hints != 0) {
//				hints->flags = (*tmp)[0];
//				hints->functions = (*tmp)[1];
//				hints->decorations = (*tmp)[2];
//				hints->input_mode = (*tmp)[3];
//				hints->status = (*tmp)[4];
//			}
//			delete tmp;
//			return hints;
//		}
//		delete tmp;
//	}
//	return 0;
//}
//
//XSizeHints * display_t::read_wm_normal_hints(Window w) {
//	std::vector<long> * tmp = ::page::get_window_property<long>(_dpy, w,
//			A(WM_NORMAL_HINTS), A(WM_SIZE_HINTS));
//	if (tmp != 0) {
//		if (tmp->size() == 18) {
//			XSizeHints * size_hints = new XSizeHints;
//			if (size_hints) {
//				size_hints->flags = (*tmp)[0];
//				size_hints->x = (*tmp)[1];
//				size_hints->y = (*tmp)[2];
//				size_hints->width = (*tmp)[3];
//				size_hints->height = (*tmp)[4];
//				size_hints->min_width = (*tmp)[5];
//				size_hints->min_height = (*tmp)[6];
//				size_hints->max_width = (*tmp)[7];
//				size_hints->max_height = (*tmp)[8];
//				size_hints->width_inc = (*tmp)[9];
//				size_hints->height_inc = (*tmp)[10];
//				size_hints->min_aspect.x = (*tmp)[11];
//				size_hints->min_aspect.y = (*tmp)[12];
//				size_hints->max_aspect.x = (*tmp)[13];
//				size_hints->max_aspect.y = (*tmp)[14];
//				size_hints->base_width = (*tmp)[15];
//				size_hints->base_height = (*tmp)[16];
//				size_hints->win_gravity = (*tmp)[17];
//			}
//			delete tmp;
//			return size_hints;
//		}
//		delete tmp;
//	}
//	return 0;
//}
//
//void display_t::write_net_wm_allowed_actions(Window w, std::list<Atom> & list) {
//	std::vector<long> v(list.begin(), list.end());
//	write_window_property(_dpy, w, A(_NET_WM_ALLOWED_ACTIONS), A(ATOM), v);
//}
//
//void display_t::write_net_wm_state(Window w, std::list<Atom> & list) {
//	std::vector<long> v(list.begin(), list.end());
//	write_window_property(_dpy, w, A(_NET_WM_STATE), A(ATOM), v);
//}
//
//void display_t::write_wm_state(Window w, long state, Window icon) {
//	std::vector<long> v(2);
//	v[0] = state;
//	v[1] = icon;
//	write_window_property(_dpy, w, A(WM_STATE), A(WM_STATE), v);
//}
//
//void display_t::write_net_active_window(Window w) {
//	std::vector<long> v(1);
//	v[0] = w;
//	write_window_property(_dpy, DefaultRootWindow(_dpy), A(_NET_ACTIVE_WINDOW),
//			A(WINDOW), v);
//}

int display_t::move_window(Window w, int x, int y) {
	//printf("XMoveWindow #%lu %d %d\n", w, x, y);
	return XMoveWindow(_dpy, w, x, y);
}

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

bool display_t::register_wm(bool replace, Window w) {
	Atom wm_sn_atom;
	Window current_wm_sn_owner;

	static char wm_sn[] = "WM_Sxx";
	snprintf(wm_sn, sizeof(wm_sn), "WM_S%d", screen());
	wm_sn_atom = XInternAtom(_dpy, wm_sn, false);

	current_wm_sn_owner = XGetSelectionOwner(_dpy, wm_sn_atom);
	if (current_wm_sn_owner == w)
		current_wm_sn_owner = None;
	if (current_wm_sn_owner != None) {
		if (!replace) {
			printf("A window manager is already running on screen %d",
					DefaultScreen(_dpy));
			return false;
		} else {
			/* We want to find out when the current selection owner dies */
			XSelectInput(_dpy, current_wm_sn_owner, StructureNotifyMask);
			XSync(_dpy, false);

			XSetSelectionOwner(_dpy, wm_sn_atom, w, CurrentTime);

			if (XGetSelectionOwner(_dpy, wm_sn_atom) != w) {
				printf(
						"Could not acquire window manager selection on screen %d",
						screen());
				return false;
			}

			/* Wait for old window manager to go away */
			if (current_wm_sn_owner != None) {
				page::time_t end;
				page::time_t cur;

				struct pollfd fds[1];
				fds[0].fd = fd();
				fds[0].events = POLLIN | POLLOUT | POLLERR | POLLHUP;

				cur.get_time();
				end = cur + page::time_t(15L, 1000000000L);

				XEvent ev;

				while (cur < end) {
					int timeout = static_cast<int64_t>(end - cur) / 1000000000L;
					poll(fds, 1, timeout);
					/* Checks the local queue and incoming events for this event */
					if (XCheckTypedWindowEvent(_dpy, current_wm_sn_owner,
					DestroyNotify, &ev) == True)
						break;
					cur.get_time();
				}

				if (cur >= end) {
					printf("The WM on screen %d is not exiting", screen());
					return false;
				}
			}

		}
	} else {
		XSetSelectionOwner(_dpy, wm_sn_atom, w, CurrentTime);

		if (XGetSelectionOwner(_dpy, wm_sn_atom) != w) {
			printf("Could not acquire window manager selection on screen %d",
					screen());
			return false;
		}
	}

	return true;
}


/**
 * Register composite manager. if another one is in place just fail to take
 * the ownership.
 **/
bool display_t::register_cm(Window w) {
	Window current_cm;
	Atom a_cm;
	static char net_wm_cm[] = "_NET_WM_CM_Sxx";
	snprintf(net_wm_cm, sizeof(net_wm_cm), "_NET_WM_CM_S%d", screen());
	a_cm = XInternAtom(_dpy, net_wm_cm, False);

	/** read if there is a compositor **/
	current_cm = XGetSelectionOwner(_dpy, a_cm);
	if (current_cm != None) {
		printf("Another composite manager is running\n");
		return false;
	} else {

		/** become the compositor **/
		XSetSelectionOwner(_dpy, a_cm, w, CurrentTime);

		/** check is we realy are the current compositor **/
		if (XGetSelectionOwner(_dpy, a_cm) != w) {
			printf("Could not acquire window manager selection on screen %d\n",
					screen());
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

void display_t::set_window_border_width(Window w, unsigned int width) {
	cnx_printf("XSetWindowBorderWidth: win = %lu, width = %u\n", w, width);
	XSetWindowBorderWidth(_dpy, w, width);
}

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

int display_t::configure_window(Window w, unsigned int value_mask,
		XWindowChanges * values) {

	cnx_printf("XConfigureWindow: win = %lu\n", w);
	if (value_mask & CWX)
		cnx_printf("has x: %d\n", values->x);
	if (value_mask & CWY)
		cnx_printf("has y: %d\n", values->y);
	if (value_mask & CWWidth)
		cnx_printf("has width: %d\n", values->width);
	if (value_mask & CWHeight)
		cnx_printf("has height: %d\n", values->height);

	if (value_mask & CWSibling)
		cnx_printf("has sibling: %lu\n", values->sibling);
	if (value_mask & CWStackMode)
		cnx_printf("has stack mode: %d\n", values->stack_mode);

	if (value_mask & CWBorderWidth)
		cnx_printf("has border: %d\n", values->border_width);
	return XConfigureWindow(_dpy, w, value_mask, values);
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

//bool display_t::motif_has_border(Window w) {
//	motif_wm_hints_t * motif_hints = read_motif_wm_hints(w);
//	if (motif_hints != 0) {
//		if (motif_hints->flags & MWM_HINTS_DECORATIONS) {
//			if (not (motif_hints->decorations & MWM_DECOR_BORDER)
//					and not ((motif_hints->decorations & MWM_DECOR_ALL))) {
//				delete motif_hints;
//				return false;
//			}
//		}
//		delete motif_hints;
//	}
//	return true;
//}

Display * display_t::dpy() {
	return _dpy;
}

xcb_connection_t * display_t::xcb() {
	return _xcb;
}

bool display_t::check_composite_extension(int * opcode, int * event,
		int * error) {
	// Check if composite is supported.
	if (XQueryExtension(_dpy, COMPOSITE_NAME, opcode, event, error)) {
		int major = 0, minor = 0; // The highest version we support
		XCompositeQueryVersion(_dpy, &major, &minor);
		if (major != 0 || minor < 4) {
			return false;
		} else {
			printf("using composite %d.%d\n", major, minor);
			return true;
		}
	} else {
		return false;
	}
}

bool display_t::check_damage_extension(int * opcode, int * event, int * error) {
	if (!XQueryExtension(_dpy, DAMAGE_NAME, opcode, event, error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XDamageQueryVersion(_dpy, &major, &minor);
		printf("Damage Extension version %d.%d found\n", major, minor);
		printf("Damage error %d, Damage event %d\n", *error, *event);
		return true;
	}
}

bool display_t::check_shape_extension(int * opcode, int * event, int * error) {
	if (!XQueryExtension(_dpy, SHAPENAME, opcode, event, error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XShapeQueryVersion(_dpy, &major, &minor);
		printf("Shape Extension version %d.%d found\n", major, minor);
		return true;
	}
}

bool display_t::check_randr_extension(int * opcode, int * event, int * error) {
	if (!XQueryExtension(_dpy, RANDR_NAME, opcode, event, error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XRRQueryVersion(_dpy, &major, &minor);
		printf(RANDR_NAME " Extension version %d.%d found\n", major, minor);
		return true;
	}
}


bool display_t::check_dbe_extension(int * opcode, int * event, int * error) {
	if (!XQueryExtension(_dpy, DBE_PROTOCOL_NAME, opcode, event, error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XRRQueryVersion(_dpy, &major, &minor);
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
bool display_t::check_for_fake_unmap_window(Window w) {
	for (auto &ev : pending_event) {
		if (ev.type == UnmapNotify) {
			if (ev.xunmap.window == w and ev.xunmap.send_event) {
				return true;
			}
		}
	}
	return false;
}

bool display_t::check_for_unmap_window(Window w) {
	for (auto &ev : pending_event) {
		if (ev.type == UnmapNotify) {
			if (ev.xunmap.window == w) {
				return true;
			}
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
bool display_t::check_for_reparent_window(Window w) {
	for (auto &ev : pending_event) {
		if (ev.type == ReparentNotify) {
			if (root() != ev.xreparent.parent and ev.xreparent.window == w) {
				return true;
			}
		}
	}
	return false;
}

/**
 * Look for coming event if the window is destroyed. Used to
 * Skip events related to destroyed windows.
 **/
bool display_t::check_for_destroyed_window(Window w) {
	for (auto &ev : pending_event) {
		if (ev.type == DestroyNotify) {
			if (ev.xdestroywindow.window == w
					and not ev.xdestroywindow.send_event) {
				return true;
			}
		}
	}
	return false;
}

void display_t::fetch_pending_events() {
	/** get all event and store them in pending event **/
	while (XPending(_dpy)) {
		XEvent ev;
		XNextEvent(_dpy, &ev);
		pending_event.push_back(ev);
	}
}

XEvent * display_t::front_event() {
	if(not pending_event.empty()) {
		return &pending_event.front();
	} else {
		if(XPending(_dpy)) {
			XEvent ev;
			XNextEvent(_dpy, &ev);
			pending_event.push_back(ev);
		}
		return &pending_event.front();
	}
	return nullptr;
}

void display_t::pop_event() {
	pending_event.pop_front();
}

bool display_t::has_pending_events() {
	if (not pending_event.empty()) {
		return true;
	} else {
		if (XPending(_dpy)) {
			XEvent ev;
			XNextEvent(_dpy, &ev);
			pending_event.push_back(ev);
		}
		return not pending_event.empty();
	}
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
void display_t::allow_input_passthrough(Display * dpy, Window w) {
	XserverRegion region = XFixesCreateRegion(dpy, NULL, 0);
	/**
	 * Shape for the entire of window.
	 **/
	XFixesSetWindowShapeRegion(dpy, w, ShapeBounding, 0, 0, None);
	/**
	 * input shape was introduced by Keith Packard to define an input area of
	 * window by default is the ShapeBounding which is used. here we set input
	 * area an empty region.
	 **/
	XFixesSetWindowShapeRegion(dpy, w, ShapeInput, 0, 0, region);
	XFixesDestroyRegion(dpy, region);
}

void display_t::disable_input_passthrough(Display * dpy, Window w) {
	XFixesSetWindowShapeRegion(dpy, w, ShapeBounding, 0, 0, None);
	XFixesSetWindowShapeRegion(dpy, w, ShapeInput, 0, 0, None);
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

}

