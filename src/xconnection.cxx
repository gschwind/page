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

xconnection_t::xconnection_t() {
	old_error_handler = XSetErrorHandler(error_handler);

	dpy = XOpenDisplay(0);
	if (dpy == NULL) {
		throw std::runtime_error("Could not open display");
	} else {
		cnx_printf("Open display : Success\n");
	}

	/** for testing **/
	//XSynchronize(dpy, True);

	connection_fd = ConnectionNumber(dpy);

	grab_count = 0;
	_screen = DefaultScreen(dpy);
	xroot = DefaultRootWindow(dpy) ;

	select_input(xroot, SubstructureNotifyMask);

	// Check if composite is supported.
	if (XQueryExtension(dpy, COMPOSITE_NAME, &composite_opcode,
			&composite_event, &composite_error)) {
		int major = 0, minor = 0; // The highest version we support
		XCompositeQueryVersion(dpy, &major, &minor);
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
	if (!XQueryExtension(dpy, DAMAGE_NAME, &damage_opcode, &damage_event,
			&damage_error)) {
		throw std::runtime_error("Damage extension is not supported");
	} else {
		int major = 0, minor = 0;
		XDamageQueryVersion(dpy, &major, &minor);
		cnx_printf("Damage Extension version %d.%d found\n", major, minor);
		cnx_printf("Damage error %d, Damage event %d\n", damage_error,
				damage_event);
		extension_request_name_map[damage_opcode] = xdamage_func;
	}

	if (!XQueryExtension(dpy, SHAPENAME, &xshape_opcode, &xshape_event,
			&xshape_error)) {
		throw std::runtime_error(SHAPENAME " extension is not supported");
	} else {
		int major = 0, minor = 0;
		XShapeQueryVersion(dpy, &major, &minor);
		cnx_printf("Shape Extension version %d.%d found\n", major, minor);
	}

	if (!XQueryExtension(dpy, RANDR_NAME, &xrandr_opcode, &xrandr_event,
			&xrandr_error)) {
		throw std::runtime_error(RANDR_NAME " extension is not supported");
	} else {
		int major = 0, minor = 0;
		XRRQueryVersion(dpy, &major, &minor);
		cnx_printf(RANDR_NAME " Extension version %d.%d found\n", major, minor);
	}

	open_connections[dpy] = this;

	_A = atom_handler_t(dpy);

}

void xconnection_t::grab() {
	if (grab_count == 0) {
		unsigned long serial = XNextRequest(dpy);
		cnx_printf(">%08lu XGrabServer\n", serial);
		XGrabServer(dpy);
		serial = XNextRequest(dpy);
		cnx_printf(">%08lu XSync\n", serial);
		XSync(dpy, False);
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
		unsigned long serial = XNextRequest(dpy);
		cnx_printf(">%08lu XUngrabServer\n", serial);
		XUngrabServer(dpy);
		serial = XNextRequest(dpy);
		cnx_printf(">%08lu XFlush\n", serial);
		XFlush(dpy);
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
	XserverRegion region = XFixesCreateRegion(dpy, NULL, 0);
	// Shape for the entire of window.
	XFixesSetWindowShapeRegion(dpy, w, ShapeBounding, 0, 0, 0);
	// input shape was introduced by Keith Packard to define an input area of window
	// by default is the ShapeBounding which is used.
	// here we set input area an empty region.
	XFixesSetWindowShapeRegion(dpy, w, ShapeInput, 0, 0, region);
	XFixesDestroyRegion(dpy, region);
}

void xconnection_t::unmap(Window w) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu X_UnmapWindow: win = %lu\n", serial, w);
	XUnmapWindow(dpy, w);
	event_t e;
	e.serial = serial;
	e.type = UnmapNotify;
	pending.push_back(e);
}

void xconnection_t::reparentwindow(Window w, Window parent, int x, int y) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf("Reparent serial: #%lu win: #%lu, parent: #%lu\n", serial, w, parent);
	XReparentWindow(dpy, w, parent, x, y);
	event_t e;
	e.serial = serial;
	e.type = UnmapNotify;
	pending.push_back(e);
}

void xconnection_t::map_window(Window w) {
	unsigned long serial = XNextRequest(dpy);
	//cnx_printf(">%08lu X_MapWindow: win = %lu\n", serial, w);
	XMapWindow(dpy, w);
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
	XNextEvent(dpy, ev);
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
	snprintf(net_wm_cm, sizeof(net_wm_cm), "_NET_WM_CM_S%d", _screen);
	a_cm = XInternAtom(dpy, net_wm_cm, False);

	/** read if there is a compositor **/
	current_cm = XGetSelectionOwner(dpy, a_cm);
	if (current_cm != None) {
		printf("Another composite manager is running\n");
		return false;
	} else {

		/** become the compositor **/
		XSetSelectionOwner(dpy, a_cm, w, CurrentTime);

		/** check is we realy are the current compositor **/
		if (XGetSelectionOwner(dpy, a_cm) != w) {
            printf("Could not acquire window manager selection on screen %d\n", _screen);
            return false;
        }

		printf("Composite manager is registered on screen %d\n", _screen);
		return true;
	}

}

bool xconnection_t::register_wm(bool replace, Window w) {
    Atom wm_sn_atom;
    Window current_wm_sn_owner;

    static char wm_sn[] = "WM_Sxx";
    snprintf(wm_sn, sizeof(wm_sn), "WM_S%d", _screen);
    wm_sn_atom = XInternAtom(dpy, wm_sn, FALSE);

    current_wm_sn_owner = XGetSelectionOwner(dpy, wm_sn_atom);
    if (current_wm_sn_owner == w)
        current_wm_sn_owner = None;
    if (current_wm_sn_owner != None) {
        if (!replace) {
            printf("A window manager is already running on screen %d",
                      _screen);
            return false;
        } else {
            /* We want to find out when the current selection owner dies */
            XSelectInput(dpy, current_wm_sn_owner, StructureNotifyMask);
            XSync(dpy, FALSE);

            XSetSelectionOwner(dpy, wm_sn_atom, w, CurrentTime);

            if (XGetSelectionOwner(dpy, wm_sn_atom) != w) {
                printf("Could not acquire window manager selection on screen %d",
                          _screen);
                return false;
            }

            /* Wait for old window manager to go away */
            if (current_wm_sn_owner != None) {
              unsigned long wait = 0;
              const unsigned long timeout = G_USEC_PER_SEC * 15; /* wait for 15s max */

              XEvent ev;

              while (wait < timeout) {
                  /* Checks the local queue and incoming events for this event */
                  if (XCheckTypedWindowEvent(dpy, current_wm_sn_owner, DestroyNotify, &ev) == True)
                      break;
                  g_usleep(G_USEC_PER_SEC / 10);
                  wait += G_USEC_PER_SEC / 10;
              }

              if (wait >= timeout) {
                  printf("The WM on screen %d is not exiting", _screen);
                  return false;
              }
            }

        }

    } else {
        XSetSelectionOwner(dpy, wm_sn_atom, w, CurrentTime);

        if (XGetSelectionOwner(dpy, wm_sn_atom) != w) {
            printf("Could not acquire window manager selection on screen %d",
                      _screen);
            return false;
        }
    }

    return true;
}


void xconnection_t::add_to_save_set(Window w) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XAddToSaveSet: win = %lu\n", serial, w);
	XAddToSaveSet(dpy, w);
}

void xconnection_t::remove_from_save_set(Window w) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XRemoveFromSaveSet: win = %lu\n", serial, w);
	XRemoveFromSaveSet(dpy, w);
}

void xconnection_t::select_input(Window w, long int mask) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XSelectInput: win = %lu, mask = %08lx\n", serial, w,
			(unsigned long) mask);
	_acache.mark_durty(w);
	/** add StructureNotifyMask and PropertyNotify to manage properties cache **/
	mask |= StructureNotifyMask | PropertyChangeMask;
	XSelectInput(dpy, w, mask);
}

void xconnection_t::move_resize(Window w, box_int_t const & size) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XMoveResizeWindow: win = %lu, %dx%d+%d+%d\n", serial, w,
			size.w, size.h, size.x, size.y);

	XWindowAttributes * wa = _acache.get_window_attributes(dpy, w);
	wa->x = size.x;
	wa->y = size.y;
	wa->width = size.w;
	wa->height = size.h;
	XMoveResizeWindow(dpy, w, size.x, size.y, size.w, size.h);

}

void xconnection_t::set_window_border_width(Window w, unsigned int width) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XSetWindowBorderWidth: win = %lu, width = %u\n", serial, w,
			width);
	XSetWindowBorderWidth(dpy, w, width);
	event_t e;
	e.serial = serial;
	e.type = ConfigureNotify;
	pending.push_back(e);
}

void xconnection_t::raise_window(Window w) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XRaiseWindow: win = %lu\n", serial, w);
	XRaiseWindow(dpy, w);
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

		process_cache_event(e);

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

void xconnection_t::process_cache_event(XEvent const & e) {
	if(e.type == PropertyNotify) {
		_pcache.mark_durty(e.xproperty.window, e.xproperty.atom);
	} else if (e.type == ConfigureNotify) {
		update_process_configure_notify_event(e.xconfigure);
	} else if (e.type == xshape_event + ShapeNotify) {
		//XShapeEvent const * ev = reinterpret_cast<XShapeEvent const *>(&e);
		//get_window_t(ev->window)->mark_durty_shape_region();
	} else if (e.type == DestroyNotify) {
		_acache.mark_durty(e.xdestroywindow.window);
		_pcache.erase(e.xdestroywindow.window);
	} else if (e.type == CreateNotify) {
		/** select default inputs **/
		select_input(e.xcreatewindow.window, 0);
	} else if (e.type == MapNotify) {
		XWindowAttributes * wa = _acache.get_window_attributes(dpy, e.xmap.window);
		if (wa != 0) {
			wa->map_state = IsViewable;
		}
	} else if (e.type == UnmapNotify) {
		XWindowAttributes * wa = _acache.get_window_attributes(dpy, e.xunmap.window);
		if (wa != 0) {
			wa->map_state = IsUnmapped;
		}
	}


}

int xconnection_t::change_property(Window w, atom_e property, atom_e type,
		int format, int mode, unsigned char const * data, int nelements) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XChangeProperty: win = %lu\n", serial, w);
	return XChangeProperty(dpy, w, A(property), A(type), format, mode, data,
			nelements);
}

Status xconnection_t::get_window_attributes(Window w,
		XWindowAttributes * window_attributes_return) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XGetWindowAttributes: win = %lu\n", serial, w);
	return XGetWindowAttributes(dpy, w, window_attributes_return);
}

Status xconnection_t::get_text_property(Window w,
		XTextProperty * text_prop_return, atom_e property) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XGetTextProperty: win = %lu\n", serial, w);
	return XGetTextProperty(dpy, w, text_prop_return, A(property));
}

int xconnection_t::lower_window(Window w) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XLowerWindow: win = %lu\n", serial, w);
	return XLowerWindow(dpy, w);
}

int xconnection_t::configure_window(Window w, unsigned int value_mask,
		XWindowChanges * values) {
	unsigned long serial = XNextRequest(dpy);
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
	return XConfigureWindow(dpy, w, value_mask, values);
}

char * xconnection_t::_get_atom_name(Atom atom) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XGetAtomName: atom = %lu\n", serial, atom);
	return XGetAtomName(dpy, atom);
}

Status xconnection_t::send_event(Window w, Bool propagate, long event_mask,
		XEvent* event_send) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XSendEvent: win = %lu\n", serial, w);
	return XSendEvent(dpy, w, propagate, event_mask, event_send);
}

int xconnection_t::set_input_focus(Window focus, int revert_to, Time time) {
	unsigned long serial = XNextRequest(dpy);
	printf(">%08lu XSetInputFocus: win = %lu, time = %lu\n", serial, focus, time);
	fflush(stdout);
	return XSetInputFocus(dpy, focus, revert_to, time);
}

XWMHints * xconnection_t::_get_wm_hints(Window w) {
	unsigned long serial = XNextRequest(dpy);
	cnx_printf(">%08lu XGetWMHints: win = %lu\n", serial, w);
	return XGetWMHints(dpy, w);
}

void xconnection_t::fake_configure(Window w, box_int_t location, int border_width) {
	XEvent ev;
	ev.xconfigure.type = ConfigureNotify;
	ev.xconfigure.display = dpy;
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
	composite_overlay = XCompositeGetOverlayWindow(dpy, xroot);
	allow_input_passthrough(composite_overlay);

	/** automaticaly redirect windows, but paint window manually */
	XCompositeRedirectSubwindows(dpy, xroot, CompositeRedirectManual);
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

