/*
 * xconnection.cxx
 *
 *  Created on: Oct 30, 2011
 *      Author: gschwind
 */

#include "xconnection.hxx"
#include "X11/Xproto.h"
#include "glib.h"

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

	/** for testing **/
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

	open_connections[_dpy] = this;

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
	fprintf(stderr,"#%08lu ERROR, major_code: %u, minor_code: %u, error_code: %u\n",
			ev->serial, ev->request_code, ev->minor_code, ev->error_code);

	if (open_connections.find(dpy) == open_connections.end()) {
		fprintf(stderr, "Error on unknown connection\n");
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
		fprintf(stderr,"\e[1;31m%s: %s %lu\e[m\n", func, buffer, ev->serial);
	} else {
		fprintf(stderr,"Error %u: %s %lu\n", (unsigned) ev->request_code, buffer,
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
	cnx_printf(">%08lu X_UnmapWindow: win = %lu\n", serial, w);
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

/* this function come from xcompmgr
 * it is intend to make page as composite manager */

/**
 * Register composite manager. if another one is in place just fail to take
 * the ownership.
 **/
bool xconnection_t::register_cm(Window w) {
	Window current_cm;
	Atom a_cm;
	static char net_wm_cm[] = "_NET_WM_CM_Sxx";
	snprintf(net_wm_cm, sizeof(net_wm_cm), "_NET_WM_CM_S%d", screen);
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
            printf("Could not acquire window manager selection on screen %d\n", screen);
            return false;
        }

		printf("Composite manager is registered on screen %d\n", screen);
		return true;
	}

}

bool xconnection_t::register_wm(bool replace, Window w) {
    Atom wm_sn_atom;
    Window current_wm_sn_owner;

    static char wm_sn[] = "WM_Sxx";
    snprintf(wm_sn, sizeof(wm_sn), "WM_S%d", screen);
    wm_sn_atom = XInternAtom(_dpy, wm_sn, FALSE);

    current_wm_sn_owner = XGetSelectionOwner(_dpy, wm_sn_atom);
    if (current_wm_sn_owner == w)
        current_wm_sn_owner = None;
    if (current_wm_sn_owner != None) {
        if (!replace) {
            printf("A window manager is already running on screen %d",
                      screen);
            return false;
        } else {
            /* We want to find out when the current selection owner dies */
            XSelectInput(_dpy, current_wm_sn_owner, StructureNotifyMask);
            XSync(_dpy, FALSE);

            XSetSelectionOwner(_dpy, wm_sn_atom, w, CurrentTime);

            if (XGetSelectionOwner(_dpy, wm_sn_atom) != w) {
                printf("Could not acquire window manager selection on screen %d",
                          screen);
                return false;
            }

            /* Wait for old window manager to go away */
            if (current_wm_sn_owner != None) {
              unsigned long wait = 0;
              const unsigned long timeout = G_USEC_PER_SEC * 15; /* wait for 15s max */

              XEvent ev;

              while (wait < timeout) {
                  /* Checks the local queue and incoming events for this event */
                  if (XCheckTypedWindowEvent(_dpy, current_wm_sn_owner, DestroyNotify, &ev) == True)
                      break;
                  g_usleep(G_USEC_PER_SEC / 10);
                  wait += G_USEC_PER_SEC / 10;
              }

              if (wait >= timeout) {
                  printf("The WM on screen %d is not exiting", screen);
                  return false;
              }
            }

        }

    } else {
        XSetSelectionOwner(_dpy, wm_sn_atom, w, CurrentTime);

        if (XGetSelectionOwner(_dpy, wm_sn_atom) != w) {
            printf("Could not acquire window manager selection on screen %d",
                      screen);
            return false;
        }
    }

    return true;
}


void xconnection_t::add_to_save_set(Window w) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf(">%08lu XAddToSaveSet: win = %lu\n", serial, w);
	XAddToSaveSet(_dpy, w);
}

void xconnection_t::remove_from_save_set(Window w) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf(">%08lu XRemoveFromSaveSet: win = %lu\n", serial, w);
	XRemoveFromSaveSet(_dpy, w);
}

void xconnection_t::select_input(Window w, long int mask) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf(">%08lu XSelectInput: win = %lu, mask = %08lx\n", serial, w,
			(unsigned long) mask);
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
	cnx_printf(">%08lu XSetWindowBorderWidth: win = %lu, width = %u\n", serial, w,
			width);
	XSetWindowBorderWidth(_dpy, w, width);
	event_t e;
	e.serial = serial;
	e.type = ConfigureNotify;
	pending.push_back(e);
}

void xconnection_t::raise_window(Window w) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf(">%08lu XRaiseWindow: win = %lu\n", serial, w);
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

int xconnection_t::_return_true(Display * dpy, XEvent * ev, XPointer none) {
	return True;
}
/**
 * Check event without blocking.
 **/
bool xconnection_t::process_check_event() {
	XEvent e;

	int x;
	/** Passive check of events in queue, never flush any thing **/
	if ((x = XEventsQueued(dpy, QueuedAlready)) > 0) {
		/** should not block or flush **/
		xnextevent(&e);

		/**
		 * since event handler can be removed on event, we copy it
		 * and check for event removed each time.
		 **/
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

int xconnection_t::change_property(Window w, char const * property, char const * type,
		int format, int mode, unsigned char const * data, int nelements) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf(">%08lu XChangeProperty: win = %lu\n", serial, w);
	return XChangeProperty(_dpy, w, get_atom(property), get_atom(type), format, mode, data,
			nelements);
}

Status xconnection_t::get_window_attributes(Window w,
		XWindowAttributes * window_attributes_return) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf(">%08lu XGetWindowAttributes: win = %lu\n", serial, w);
	return XGetWindowAttributes(_dpy, w, window_attributes_return);
}

Status xconnection_t::get_text_property(Window w,
		XTextProperty * text_prop_return, char const * property) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf(">%08lu XGetTextProperty: win = %lu\n", serial, w);
	return XGetTextProperty(_dpy, w, text_prop_return, get_atom(property));
}

int xconnection_t::lower_window(Window w) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf(">%08lu XLowerWindow: win = %lu\n", serial, w);
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
	cnx_printf(">%08lu XGetAtomName: atom = %lu\n", serial, atom);
	return XGetAtomName(_dpy, atom);
}

Status xconnection_t::send_event(Window w, Bool propagate, long event_mask,
		XEvent* event_send) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf(">%08lu XSendEvent: win = %lu\n", serial, w);
	return XSendEvent(_dpy, w, propagate, event_mask, event_send);
}

int xconnection_t::set_input_focus(Window focus, int revert_to, Time time) {
	unsigned long serial = XNextRequest(_dpy);
	printf(">%08lu XSetInputFocus: win = %lu, time = %lu\n", serial, focus, time);
	fflush(stdout);
	return XSetInputFocus(_dpy, focus, revert_to, time);
}

int xconnection_t::get_window_property(Window w, char const * property,
		long long_offset, long long_length, Bool c_delete, char const * req_type,
		Atom* actual_type_return, int* actual_format_return,
		unsigned long* nitems_return, unsigned long* bytes_after_return,
		unsigned char** prop_return) {
	unsigned long serial = XNextRequest(_dpy);

	//char * prop = get_atom_name(property);
	//char * type = get_atom_name(req_type);
	//cnx_printf(">%08lu XGetWindowProperty: win = %lu, prop = %s, type = %s\n", serial, w, prop, type);

	Atom aprop = get_atom(property);
	Atom atype = get_atom(req_type);

	return XGetWindowProperty(_dpy, w, aprop, long_offset, long_length,
			c_delete, atype, actual_type_return, actual_format_return,
			nitems_return, bytes_after_return, prop_return);
}

XWMHints * xconnection_t::get_wm_hints(Window w) {
	unsigned long serial = XNextRequest(_dpy);
	cnx_printf(">%08lu XGetWMHints: win = %lu\n", serial, w);
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

	//ev.xconfigure.event = xroot;
	//send_event(xroot, False, SubstructureNotifyMask, &ev);


}

string const & xconnection_t::get_atom_name(Atom a) {
	return atom_to_name(a);
}

Atom xconnection_t::get_atom(char const * name) {
	return name_to_atom(name);
}


void xconnection_t::init_composite_overlay() {
	/* map & passtrough the overlay */
	composite_overlay = XCompositeGetOverlayWindow(_dpy, xroot);
	allow_input_passthrough(composite_overlay);

	/** automaticaly redirect windows, but paint window manually */
	XCompositeRedirectSubwindows(_dpy, xroot, CompositeRedirectManual);
}

void xconnection_t::add_select_input(Window w, long mask) {
	XWindowAttributes wa;
	if (XGetWindowAttributes(_dpy, w, &wa) != 0) {
		XSelectInput(_dpy, w, wa.your_event_mask | mask);
	}
}

string const & xconnection_t::atom_to_name(Atom atom) {

	map<Atom, string>::iterator i = _atom_to_name.find(atom);
	if(i != _atom_to_name.end()) {
		return i->second;
	} else {
		char * name = _get_atom_name(atom);
		if(name == 0) {
			stringstream os;
			os << "UNKNOWN_" << hex << atom;
			_name_to_atom[os.str()] = atom;
			_atom_to_name[atom] = os.str();
		} else {
			_name_to_atom[string(name)] = atom;
			_atom_to_name[atom] = string(name);
			XFree(name);
		}
		return _atom_to_name[atom];
	}
}

Atom const & xconnection_t::name_to_atom(char const * name) {
	static Atom none = None;
	if(name == 0)
		return none;

	map<string, Atom>::iterator i = _name_to_atom.find(string(name));
	if(i != _name_to_atom.end()) {
		return i->second;
	} else {
		Atom a = XInternAtom(dpy, name, False);
		_name_to_atom[string(name)] = a;
		_atom_to_name[a] = string(name);
		return _name_to_atom[string(name)];
	}
}


}

