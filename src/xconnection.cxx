/*
 * xconnection.cxx
 *
 *  Created on: Oct 30, 2011
 *      Author: gschwind
 */

#include "xconnection.hxx"
#include "X11/Xproto.h"

namespace page {

std::map<Display *, xconnection_t *> xconnection_t::open_connections;

xconnection_t::serial_filter::serial_filter(unsigned long int serial) : last_serial(serial) {

}

bool xconnection_t::serial_filter::operator() (event_t e) {
	return (e.serial < last_serial);
}

xconnection_t::xconnection_t() : dpy(_dpy) {
	old_error_handler = XSetErrorHandler(error_handler);

	_dpy = XOpenDisplay(0);
	if (_dpy == NULL) {
		throw std::runtime_error("Could not open display");
	} else {
		cnx_printf("Open display : Success\n");
	}

	//XSynchronize(dpy, True);

	connection_fd = ConnectionNumber(_dpy);

	grab_count = 0;
	screen = DefaultScreen(_dpy);
	xroot = DefaultRootWindow(_dpy) ;
	if (!get_window_attributes(xroot, &root_wa)) {
		throw std::runtime_error("Cannot get root window attributes");
	} else {
		cnx_printf("Get root windows attribute Success\n");
	}

	root_size.x = 0;
	root_size.y = 0;
	root_size.w = root_wa.width;
	root_size.h = root_wa.height;

	// Check if composite is supported.
	if (XQueryExtension(_dpy, COMPOSITE_NAME, &composite_opcode,
			&composite_event, &composite_error)) {
		int major = 0, minor = 0; // The highest version we support
		XCompositeQueryVersion(_dpy, &major, &minor);
		if (major != 0 || minor < 4) {
			throw std::runtime_error("X Server doesn't support Composite 0.4");
		} else {
			cnx_printf("using composite %d.%d\n", major, minor);
			extension_request_name_map[composite_opcode] =
					xcomposite_request_name;
		}
	} else {
		throw std::runtime_error("X Server doesn't support Composite");
	}

	// check/init Damage.
	if (!XQueryExtension(_dpy, DAMAGE_NAME, &damage_opcode, &damage_event,
			&damage_error)) {
		throw std::runtime_error("Damage extension is not supported");
	} else {
		int major = 0, minor = 0;
		XDamageQueryVersion(_dpy, &major, &minor);
		cnx_printf("Damage Extension version %d.%d found\n", major, minor);
		cnx_printf("Damage error %d, Damage event %d\n", damage_error,
				damage_event);
		extension_request_name_map[damage_opcode] = xdamage_func;
	}

	/* No macro for XINERAMA_NAME, use one we know */
	if (!XQueryExtension(_dpy, "XINERAMA", &xinerama_opcode, &xinerama_event,
			&xinerama_error)) {
		throw std::runtime_error("Fixes extension is not supported");
	} else {
		int major = 0, minor = 0;
		XineramaQueryVersion(_dpy, &major, &minor);
		cnx_printf("Xinerama Extension version %d.%d found\n", major, minor);
	}

	if (!XQueryExtension(_dpy, SHAPENAME, &xshape_opcode, &xshape_event,
			&xshape_error)) {
		throw std::runtime_error(SHAPENAME " extension is not supported");
	} else {
		int major = 0, minor = 0;
		XShapeQueryVersion(_dpy, &major, &minor);
		cnx_printf("Shape Extension version %d.%d found\n", major, minor);
	}

	if (!XQueryExtension(_dpy, RANDR_NAME, &xrandr_opcode, &xrandr_event,
			&xrandr_error)) {
		throw std::runtime_error(RANDR_NAME " extension is not supported");
	} else {
		int major = 0, minor = 0;
		XRRQueryVersion(_dpy, &major, &minor);
		cnx_printf(RANDR_NAME " Extension version %d.%d found\n", major, minor);
	}

	XRRSelectInput(_dpy, xroot,
			RRScreenChangeNotifyMask | RRCrtcChangeNotifyMask
					| RROutputChangeNotifyMask | RROutputPropertyNotifyMask
					| RRProviderChangeNotifyMask | RRProviderChangeNotifyMask
					| RRProviderPropertyNotifyMask | RRResourceChangeNotifyMask);


	/* map & passtrough the overlay */
	composite_overlay = XCompositeGetOverlayWindow(_dpy, xroot);
	allow_input_passthrough(composite_overlay);
	XCompositeRedirectSubwindows(_dpy, xroot, CompositeRedirectManual);

	/* initialize all atoms for this connection */
#define ATOM_INIT(name) atoms.name = XInternAtom(dpy, #name, False)

	ATOM_INIT(ATOM);
	ATOM_INIT(CARDINAL);
	ATOM_INIT(WINDOW);
	ATOM_INIT(UTF8_STRING);

	ATOM_INIT(WM_STATE);
	ATOM_INIT(WM_NAME);
	ATOM_INIT(WM_DELETE_WINDOW);
	ATOM_INIT(WM_PROTOCOLS);
	ATOM_INIT(WM_TAKE_FOCUS);

	ATOM_INIT(WM_NORMAL_HINTS);
	ATOM_INIT(WM_CHANGE_STATE);

	ATOM_INIT(WM_HINTS);

	ATOM_INIT(_NET_SUPPORTED);
	ATOM_INIT(_NET_WM_NAME);
	ATOM_INIT(_NET_WM_STATE);
	ATOM_INIT(_NET_WM_STRUT_PARTIAL);

	ATOM_INIT(_NET_WM_WINDOW_TYPE);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_DOCK);

	ATOM_INIT(_NET_WM_WINDOW_TYPE);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_DESKTOP);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_DOCK);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_TOOLBAR);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_MENU);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_UTILITY);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_SPLASH);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_DIALOG);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_POPUP_MENU);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_TOOLTIP);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_NOTIFICATION);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_COMBO);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_DND);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_NORMAL);

	ATOM_INIT(_NET_WM_USER_TIME);

	ATOM_INIT(_NET_CLIENT_LIST);
	ATOM_INIT(_NET_CLIENT_LIST_STACKING);

	ATOM_INIT(_NET_NUMBER_OF_DESKTOPS);
	ATOM_INIT(_NET_DESKTOP_GEOMETRY);
	ATOM_INIT(_NET_DESKTOP_VIEWPORT);
	ATOM_INIT(_NET_CURRENT_DESKTOP);
	ATOM_INIT(_NET_WM_DESKTOP);

	ATOM_INIT(_NET_SHOWING_DESKTOP);
	ATOM_INIT(_NET_WORKAREA);

	ATOM_INIT(_NET_ACTIVE_WINDOW);

	ATOM_INIT(_NET_WM_STATE_MODAL);
	ATOM_INIT(_NET_WM_STATE_STICKY);
	ATOM_INIT(_NET_WM_STATE_MAXIMIZED_VERT);
	ATOM_INIT(_NET_WM_STATE_MAXIMIZED_HORZ);
	ATOM_INIT(_NET_WM_STATE_SHADED);
	ATOM_INIT(_NET_WM_STATE_SKIP_TASKBAR);
	ATOM_INIT(_NET_WM_STATE_SKIP_PAGER);
	ATOM_INIT(_NET_WM_STATE_HIDDEN);
	ATOM_INIT(_NET_WM_STATE_FULLSCREEN);
	ATOM_INIT(_NET_WM_STATE_ABOVE);
	ATOM_INIT(_NET_WM_STATE_BELOW);
	ATOM_INIT(_NET_WM_STATE_DEMANDS_ATTENTION);
	ATOM_INIT(_NET_WM_STATE_FOCUSED);

	ATOM_INIT(_NET_WM_ALLOWED_ACTIONS);

	/* _NET_WM_ALLOWED_ACTIONS */
	ATOM_INIT(_NET_WM_ACTION_MOVE);
	/*never allowed */
	ATOM_INIT(_NET_WM_ACTION_RESIZE);
	/* never allowed */
	ATOM_INIT(_NET_WM_ACTION_MINIMIZE);
	/* never allowed */
	ATOM_INIT(_NET_WM_ACTION_SHADE);
	/* never allowed */
	ATOM_INIT(_NET_WM_ACTION_STICK);
	/* never allowed */
	ATOM_INIT(_NET_WM_ACTION_MAXIMIZE_HORZ);
	/* never allowed */
	ATOM_INIT(_NET_WM_ACTION_MAXIMIZE_VERT);
	/* never allowed */
	ATOM_INIT(_NET_WM_ACTION_FULLSCREEN);
	/* allowed */
	ATOM_INIT(_NET_WM_ACTION_CHANGE_DESKTOP);
	/* never allowed */
	ATOM_INIT(_NET_WM_ACTION_CLOSE);
	/* always allowed */
	ATOM_INIT(_NET_WM_ACTION_ABOVE);
	/* never allowed */
	ATOM_INIT(_NET_WM_ACTION_BELOW);
	/* never allowed */

	ATOM_INIT(_NET_CLOSE_WINDOW);

	ATOM_INIT(_NET_REQUEST_FRAME_EXTENTS);
	ATOM_INIT(_NET_FRAME_EXTENTS);

	ATOM_INIT(_NET_WM_ICON);

	ATOM_INIT(WM_TRANSIENT_FOR);

	ATOM_INIT(PAGE_QUIT);

	ATOM_INIT(_NET_SUPPORTING_WM_CHECK);

#undef ATOM_INIT

	/* try to register composite manager */
	if (!register_cm())
		throw std::runtime_error("Another compositor running");

	open_connections[_dpy] = this;

	last_know_time = 0;

}

void xconnection_t::grab() {
	if (grab_count == 0) {
		unsigned long serial = XNextRequest(_dpy);
		cnx_printf(">%08lu XGrabServer\n", serial);
		XGrabServer(_dpy);
		serial = XNextRequest(_dpy);
		cnx_printf(">%08lu XSync\n", serial);
		XSync(_dpy, False);
	}
	++grab_count;
}

void xconnection_t::ungrab() {
	if (grab_count == 0) {
		fprintf(stderr, "TRY TO UNGRAB NOT GRABBED CONNECTION!\n");
		return;
	}
	--grab_count;
	if (grab_count == 0) {
		unsigned long serial = XNextRequest(_dpy);
		cnx_printf(">%08lu XUngrabServer\n", serial);
		XUngrabServer(_dpy);
		serial = XNextRequest(_dpy);
		cnx_printf(">%08lu XFlush\n", serial);
		XFlush(_dpy);
	}
}

bool xconnection_t::is_not_grab() {
	return grab_count == 0;
}

int xconnection_t::error_handler(Display * dpy, XErrorEvent * ev) {
	cnx_printf("#%08lu ERROR, major_code: %u, minor_code: %u, error_code: %u\n",
			ev->serial, ev->request_code, ev->minor_code, ev->error_code);

	if (open_connections.find(dpy) == open_connections.end()) {
		cnx_printf("Error on unknow connection\n");
		return 0;
	}

	xconnection_t * ths = open_connections[dpy];

	/* TODO: dump some use full information */
	char buffer[1024];
	XGetErrorText(dpy, ev->error_code, buffer, 1024);

	char const * func = 0;
	if (ev->request_code < 127 && ev->request_code > 0) {
		func = x_function_codes[ev->request_code];
	} else if (ths->extension_request_name_map.find(ev->request_code)
			!= ths->extension_request_name_map.end()) {
		func =
				ths->extension_request_name_map[ev->request_code][ev->minor_code];
	}

	if (func != 0) {
		cnx_printf("\e[1;31m%s: %s %lu\e[m\n", func, buffer, ev->serial);
	} else {
		cnx_printf("XXXXX %u: %s %lu\n", (unsigned) ev->request_code, buffer,
				ev->serial);
	}

	return 0;
}

void xconnection_t::allow_input_passthrough(Window w) {
	// undocumented : http://lists.freedesktop.org/pipermail/xorg/2005-January/005954.html
	XserverRegion region = XFixesCreateRegion(_dpy, NULL, 0);
	// Shape for the entire of window.
	XFixesSetWindowShapeRegion(_dpy, w, ShapeBounding, 0, 0, 0);
	// input shape was introduced by Keith Packard to define an input area of window
	// by default is the ShapeBounding which is used.
	// here we set input area an empty region.
	XFixesSetWindowShapeRegion(_dpy, w, ShapeInput, 0, 0, region);
	XFixesDestroyRegion(_dpy, region);
}

void xconnection_t::unmap(Window w) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu X_UnmapWindow: win = %lu\n", serial, w);
	XUnmapWindow(_dpy, w);
	event_t e;
	e.serial = serial;
	e.type = UnmapNotify;
	pending.push_back(e);
}

void xconnection_t::reparentwindow(Window w, Window parent, int x, int y) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf("Reparent serial: #%lu win: #%lu, parent: #%lu\n", serial, w, parent);
	XReparentWindow(_dpy, w, parent, x, y);
	event_t e;
	e.serial = serial;
	e.type = UnmapNotify;
	pending.push_back(e);
}

void xconnection_t::map_window(Window w) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu X_MapWindow: win = %lu\n", serial, w);
	XMapWindow(_dpy, w);
	event_t e;
	e.serial = serial;
	e.type = MapNotify;
	pending.push_back(e);
}

bool xconnection_t::find_pending_event(event_t & e) {
	std::list<event_t>::iterator i = pending.begin();
	while (i != pending.end()) {
		if (e.type == (*i).type && e.serial == (*i).serial)
			return true;
		++i;
	}
	return false;
}

void xconnection_t::xnextevent(XEvent * ev) {
	XNextEvent(_dpy, ev);
	xconnection_t::last_serial = ev->xany.serial;
	serial_filter filter(ev->xany.serial);
	std::remove_if(pending.begin(), pending.end(), filter);
}

/* this fonction come from xcompmgr
 * it is intend to make page as composite manager */
bool xconnection_t::register_cm() {
	Window w;
	Atom a;
	static char net_wm_cm[] = "_NET_WM_CM_Sxx";

	snprintf(net_wm_cm, sizeof(net_wm_cm), "_NET_WM_CM_S%d", screen);
	a = XInternAtom(_dpy, net_wm_cm, False);

	w = XGetSelectionOwner(_dpy, a);
	if (w != None) {
		XTextProperty tp;
		char **strs;
		int count;
		Atom winNameAtom = XInternAtom(_dpy, "_NET_WM_NAME", False);

		if (!get_text_property(w, &tp, winNameAtom)
				&& !get_text_property(w, &tp, XA_WM_NAME )) {
			fprintf(stderr,
					"Another composite manager is already running (0x%lx)\n",
					(unsigned long) w);
			return false;
		}
		if (XmbTextPropertyToTextList(_dpy, &tp, &strs, &count) == Success) {
			fprintf(stderr,
					"Another composite manager is already running (%s)\n",
					strs[0]);

			XFreeStringList(strs);
		}

		XFree(tp.value);

		return false;
	}

	w = XCreateSimpleWindow(_dpy, RootWindow (_dpy, screen) , 0, 0, 1, 1, 0,
	None, None);

	Xutf8SetWMProperties(_dpy, w, "page", "page", NULL, 0, NULL, NULL, NULL);

	XSetSelectionOwner(_dpy, a, w, CurrentTime);

	return true;
}

void xconnection_t::add_to_save_set(Window w) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu XAddToSaveSet: win = %lu\n", serial, w);
	XAddToSaveSet(_dpy, w);
}

void xconnection_t::remove_from_save_set(Window w) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu XRemoveFromSaveSet: win = %lu\n", serial, w);
	XRemoveFromSaveSet(_dpy, w);
}

void xconnection_t::select_input(Window w, long int mask) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu XSelectInput: win = %lu, mask = %08lx\n", serial, w,
	//		(unsigned long) mask);
	XSelectInput(_dpy, w, mask);
}

void xconnection_t::move_resize(Window w, box_int_t const & size) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf(">%08lu XMoveResizeWindow: win = %lu, %dx%d+%d+%d\n", serial, w,
			size.w, size.h, size.x, size.y);

	XWindowChanges ev;
	ev.x = size.x;
	ev.y = size.y;
	ev.width = size.w;
	ev.height = size.h;
	ev.border_width = 0;

	XConfigureWindow(_dpy, w, (CWX | CWY | CWHeight | CWWidth | CWBorderWidth), &ev);

	event_t e;
	e.serial = serial;
	e.type = ConfigureNotify;
	pending.push_back(e);
}

void xconnection_t::set_window_border_width(Window w, unsigned int width) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu XSetWindowBorderWidth: win = %lu, width = %u\n", serial, w,
	//		width);
	XSetWindowBorderWidth(_dpy, w, width);
	event_t e;
	e.serial = serial;
	e.type = ConfigureNotify;
	pending.push_back(e);
}

void xconnection_t::raise_window(Window w) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu XRaiseWindow: win = %lu\n", serial, w);
	XRaiseWindow(_dpy, w);
	event_t e;
	e.serial = serial;
	e.type = ConfigureNotify;
	pending.push_back(e);
}

void xconnection_t::add_event_handler(xevent_handler_t * func) {
	event_handler_list.push_back(func);
}

void xconnection_t::remove_event_handler(xevent_handler_t * func) {
	assert(
			std::find(event_handler_list.begin(), event_handler_list.end(), func) != event_handler_list.end());
	event_handler_list.remove(func);
}

void xconnection_t::process_next_event() {
	XEvent e;
	xnextevent(&e);

	/* since event handler can be removed on event, we copy it
	 * and check for event removed each time.
	 */
	std::vector<xevent_handler_t *> v(event_handler_list.begin(),
			event_handler_list.end());
	for (unsigned int i = 0; i < v.size(); ++i) {
		if (std::find(event_handler_list.begin(), event_handler_list.end(),
				v[i]) != event_handler_list.end()) {
			v[i]->process_event(e);
		}
	}
}

bool xconnection_t::process_check_event() {
	XEvent e;
	if (XCheckMaskEvent(_dpy, 0xffffffff, &e)) {

		/* update last known time */
		switch(e.type) {
		case KeyPress:
		case KeyRelease:
			last_know_time = e.xkey.time;
			break;
		case ButtonPress:
		case ButtonRelease:
			last_know_time = e.xbutton.time;
			break;
		case MotionNotify:
			last_know_time = e.xmotion.time;
			break;
		case EnterNotify:
		case LeaveNotify:
			last_know_time = e.xcrossing.time;
			break;
		case PropertyNotify:
			last_know_time = e.xproperty.time;
			break;
		case SelectionClear:
			last_know_time = e.xselectionclear.time;
			break;
		case SelectionRequest:
			last_know_time = e.xselectionrequest.time;
			break;
		case SelectionNotify:
			last_know_time = e.xselection.time;
			break;
		default:
			break;
		}

		/* since event handler can be removed on event, we copy it
		 * and check for event removed each time.
		 */
		std::vector<xevent_handler_t *> v(event_handler_list.begin(),
				event_handler_list.end());
		for (unsigned int i = 0; i < v.size(); ++i) {
			if (std::find(event_handler_list.begin(), event_handler_list.end(),
					v[i]) != event_handler_list.end()) {
				v[i]->process_event(e);
			}
		}

		return true;
	} else {
		return false;
	}
}

int xconnection_t::change_property(Window w, Atom property, Atom type,
		int format, int mode, unsigned char const * data, int nelements) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu XChangeProperty: win = %lu\n", serial, w);
	return XChangeProperty(_dpy, w, property, type, format, mode, data,
			nelements);
}

Status xconnection_t::get_window_attributes(Window w,
		XWindowAttributes * window_attributes_return) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu XGetWindowAttributes: win = %lu\n", serial, w);
	return XGetWindowAttributes(_dpy, w, window_attributes_return);
}

Status xconnection_t::get_text_property(Window w,
		XTextProperty * text_prop_return, Atom property) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu XGetTextProperty: win = %lu\n", serial, w);
	return XGetTextProperty(_dpy, w, text_prop_return, property);
}

int xconnection_t::lower_window(Window w) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu XLowerWindow: win = %lu\n", serial, w);
	return XLowerWindow(_dpy, w);
}

int xconnection_t::configure_window(Window w, unsigned int value_mask,
		XWindowChanges * values) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf(">%08lu XConfigureWindow: win = %lu\n", serial, w);
	if(value_mask & CWX)
		cnx_printf("has x: %d\n", values->x);
	if(value_mask & CWY)
		cnx_printf("has y: %d\n", values->y);
	if(value_mask & CWWidth)
		cnx_printf("has width: %d\n", values->width);
	if(value_mask & CWHeight)
		cnx_printf("has height: %d\n", values->height);

	if(value_mask & CWSibling)
		cnx_printf("has sibling: %lu\n", values->sibling);
	if(value_mask & CWStackMode)
		cnx_printf("has stack mode: %d\n", values->stack_mode);

	if(value_mask & CWBorderWidth)
		cnx_printf("has border: %d\n", values->border_width);
	return XConfigureWindow(_dpy, w, value_mask, values);
}

char * xconnection_t::_get_atom_name(Atom atom) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu XGetAtomName: atom = %lu\n", serial, atom);
	return XGetAtomName(_dpy, atom);
}

Status xconnection_t::send_event(Window w, Bool propagate, long event_mask,
		XEvent* event_send) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu XSendEvent: win = %lu\n", serial, w);
	return XSendEvent(_dpy, w, propagate, event_mask, event_send);
}

int xconnection_t::set_input_focus(Window focus, int revert_to, Time time) {
	unsigned long serial = XNextRequest(_dpy);
	printf(">%08lu XSetInputFocus: win = %lu\n", serial, focus);
	return XSetInputFocus(_dpy, focus, revert_to, time);
}

int xconnection_t::get_window_property(Window w, Atom property,
		long long_offset, long long_length, Bool c_delete, Atom req_type,
		Atom* actual_type_return, int* actual_format_return,
		unsigned long* nitems_return, unsigned long* bytes_after_return,
		unsigned char** prop_return) {
	unsigned long serial = XNextRequest(_dpy);

	//char * prop = get_atom_name(property);
	//char * type = get_atom_name(req_type);
	//cnx_printf(">%08lu XGetWindowProperty: win = %lu, prop = %s, type = %s\n", serial, w, prop, type);

	return XGetWindowProperty(_dpy, w, property, long_offset, long_length,
			c_delete, req_type, actual_type_return, actual_format_return,
			nitems_return, bytes_after_return, prop_return);
}

XWMHints * xconnection_t::get_wm_hints(Window w) {
	unsigned long serial = XNextRequest(_dpy);
	//cnx_printf(">%08lu XGetWMHints: win = %lu\n", serial, w);
	return XGetWMHints(_dpy, w);
}

Window xconnection_t::create_window(Visual * visual, int x, int y, unsigned w, unsigned h) {
	XSetWindowAttributes wa;
	unsigned long value_mask = CWOverrideRedirect;
	wa.override_redirect = True;

	if(visual != 0) {
		wa.colormap = XCreateColormap(_dpy, xroot, visual, AllocNone);
		wa.background_pixel = BlackPixel(_dpy, screen);
		wa.border_pixel = BlackPixel(_dpy, screen);
		value_mask |= CWColormap | CWBackPixel | CWBorderPixel;
	}

	return XCreateWindow(_dpy, xroot, x, y, w, h, 0, (visual == 0)? root_wa.depth : 32 , InputOutput, (visual == 0)? root_wa.visual : visual, value_mask, &wa);

}

void xconnection_t::fake_configure(Window w, box_int_t location, int border_width) {
	XEvent ev;
	ev.xconfigure.type = ConfigureNotify;
	ev.xconfigure.display = _dpy;
	ev.xconfigure.event = w;
	ev.xconfigure.window = w;
	ev.xconfigure.send_event = True;

	/* if ConfigureRequest happen, override redirect is False */
	ev.xconfigure.override_redirect = False;
	ev.xconfigure.border_width = border_width;
	ev.xconfigure.above = None;

	/* send mandatory fake event */
	ev.xconfigure.x = location.x;
	ev.xconfigure.y = location.y;
	ev.xconfigure.width = location.w;
	ev.xconfigure.height = location.h;

	send_event(w, False, StructureNotifyMask, &ev);

	ev.xconfigure.event = xroot;
	send_event(xroot, False, SubstructureNotifyMask, &ev);


}

string const & xconnection_t::get_atom_name(Atom a) {
	std::map<Atom, string>::iterator i = atom_name_cache.find(a);
	if(i != atom_name_cache.end()) {
		return i->second;
	} else {
		char * name = _get_atom_name(a);
		if(name == 0) {
			stringstream os;
			os << "UNKNOWN_" << hex << a;
			atom_name_cache[a] = os.str();
		} else {
			atom_name_cache[a] = string(name);
			XFree(name);
		}
		return atom_name_cache[a];
	}
}


}

