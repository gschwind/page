/*
 * page.cxx
 *
 *  Created on: 23 févr. 2011
 *      Author: gschwind
 */

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <cstring>

#include <sstream>
#include <limits>
#include <stdint.h>

#include "page.hxx"
#include "box.hxx"
#include "client.hxx"
#include "root.hxx"

namespace page_next {

char const * x_event_name[LASTEvent] = { 0, 0, "KeyPress", "KeyRelease",
		"ButtonPress", "ButtonRelease", "MotionNotify", "EnterNotify",
		"LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose",
		"GraphicsExpose", "NoExpose", "VisibilityNotify", "CreateNotify",
		"DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
		"ReparentNotify", "ConfigureNotify", "ConfigureRequest",
		"GravityNotify", "ResizeRequest", "CirculateNotify",
		"CirculateRequest", "PropertyNotify", "SelectionClear",
		"SelectionRequest", "SelectionNotify", "ColormapNotify",
		"ClientMessage", "MappingNotify", "GenericEvent" };

main_t::main_t() {
	XSetWindowAttributes swa;
	XWindowAttributes wa;
	dpy = XOpenDisplay(0);
	screen = DefaultScreen(dpy);
	xroot = DefaultRootWindow(dpy);
	XGetWindowAttributes(dpy, xroot, &root_wa);
	sx = 0;
	sy = 0;
	sw = root_wa.width;
	sh = root_wa.height;
	XSelectInput(dpy, xroot, SubstructureNotifyMask | SubstructureRedirectMask);

	main_window = XCreateWindow(dpy, xroot, sx, sy, sw, sh, 0, root_wa.depth,
			InputOutput, root_wa.visual, 0, &swa);
	cursor = XCreateFontCursor(dpy, XC_left_ptr);
	XDefineCursor(dpy, main_window, cursor);
	XSelectInput(dpy, main_window, StructureNotifyMask | ButtonPressMask);
	XMapWindow(dpy, main_window);

	printf("Created main window #%lu\n", main_window);

#define ATOM_INIT(name) atoms.name = XInternAtom(dpy, #name, False)

	ATOM_INIT(ATOM);
	ATOM_INIT(CARDINAL);

	ATOM_INIT(WM_STATE);
	ATOM_INIT(WM_NAME);

	ATOM_INIT(_NET_SUPPORTED);
	ATOM_INIT(_NET_WM_NAME);
	ATOM_INIT(_NET_WM_STATE);
	ATOM_INIT(_NET_WM_STATE_FULLSCREEN);

	ATOM_INIT(_NET_WM_WINDOW_TYPE);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_DOCK);

	ATOM_INIT(_NET_WM_STRUT_PARTIAL);

	ATOM_INIT(_NET_WM_USER_TIME);

	box_t<int> a(0, 0, sw, sh);
	tree_root = new root_t(dpy, main_window, a);

}

void main_t::run() {
	scan();
	box_t<int> b(0, 0, sw, sh);
	tree_root->update_allocation(b);
	XMoveResizeWindow(dpy, main_window, sx, sy, sw, sh);
	render();
	running = 1;
	while (running) {
		XEvent e;
		XNextEvent(dpy, &e);
		printf("Event %s #%lu\n", x_event_name[e.type], e.xany.window);
		if (e.type == MapNotify) {
		} else if (e.type == Expose) {
			if (e.xmapping.window == main_window)
				render();
		} else if (e.type == ButtonPress) {
			tree_root->process_button_press_event(&e);
			render();
		} else if (e.type == MapRequest) {
			printf("MapRequest\n");
			process_map_request_event(&e);
		} else if (e.type == MapNotify) {
			process_map_notify_event(&e);
		} else if (e.type == UnmapNotify) {
			process_unmap_notify_event(&e);
		} else if (e.type == PropertyNotify) {
			process_property_notify_event(&e);
		} else if (e.type == DestroyNotify) {
			process_destroy_notify_event(&e);
		}
	}
}

void main_t::render(cairo_t * cr) {
  cairo_save(cr);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_rectangle(cr, 0, 0, wa.width, wa.height);
	cairo_fill(cr);
	cairo_restore(cr);
	tree_root->render(cr);
}

void main_t::render() {
	XGetWindowAttributes(dpy, main_window, &wa);
	box_t<int> b(0, 0, wa.width, wa.height);
	tree_root->update_allocation(b);
	cairo_t * cr = get_cairo();
	render(cr);
	cairo_destroy(cr);
}

void main_t::scan() {
	printf("call %s\n", __PRETTY_FUNCTION__);
	unsigned int i, num;
	Window d1, d2, *wins = 0;
	XWindowAttributes wa;

	/* secure the scan process */
	XGrabServer(dpy);
	XSync(dpy, False);
	/* ask for child of current root window, use Xlib here since gdk
	 * only know windows it have created.
	 */
	if (XQueryTree(dpy, xroot, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; ++i) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (wa.override_redirect)
				continue;
			if ((get_window_state(wins[i]) == IconicState || wa.map_state
					== IsViewable))
				manage(wins[i], &wa);
		}

		if (wins)
			XFree(wins);
	}

	XUngrabServer(dpy);
	XFlush(dpy);
}

client_t * main_t::find_client_by_xwindow(Window w) {
	std::list<client_t *>::iterator i = clients.begin();
	while (i != clients.end()) {
		if ((*i)->xwin == w)
			return (*i);
		++i;
	}

	return 0;
}

client_t * main_t::find_client_by_clipping_window(Window w) {
	std::list<client_t *>::iterator i = clients.begin();
	while (i != clients.end()) {
		if ((*i)->clipping_window == w)
			return (*i);
		++i;
	}

	return 0;
}

bool main_t::manage(Window w, XWindowAttributes * wa) {
	printf("Manage #%lu\n", w);
	if (main_window == w)
		return false;

	client_t * c = find_client_by_xwindow(w);
	if (c != NULL) {
		printf("Window %p is already managed\n", c);
		return false;
	}

	/* do not manage clipping window */
	if (find_client_by_clipping_window(w))
		return false;

	c = new client_t;
	c->xwin = w;
	c->dpy = dpy;
	printf("Map stase : %d\n", wa->map_state);
	/* if the client is mapped, the reparent will unmap the window
	 * The client is mapped if the manage occur on start of
	 * page.
	 */
	if (wa->map_state == IsUnmapped) {
		c->is_map = false;
		c->unmap_pending = 0;
	} else {
		c->is_map = true;
		c->unmap_pending = 1;
	}
	/* before page prepend !! */
	clients.push_back(c);

	update_net_vm_name(*c);
	update_vm_name(*c);
	update_title(*c);
	client_update_size_hints(c);

	if (client_is_dock(c)) {
		printf("IsDock !\n");
		int32_t * partial_struct;
		unsigned int n;
		if (get_all(c->xwin, atoms._NET_WM_STRUT_PARTIAL, atoms.CARDINAL, 32,
				(unsigned char **) &partial_struct, &n)) {

			printf("partial struct %d %d %d %d\n", partial_struct[0],
					partial_struct[1], partial_struct[2], partial_struct[3]);

			sw -= partial_struct[0] + partial_struct[2];
			sh -= partial_struct[1] + partial_struct[3];
			if (sx < partial_struct[0])
				sx = partial_struct[0];
			if (sy < partial_struct[1])
				sy = partial_struct[1];
		}
		return true;

	}

	/* this window will not be destroyed on page close (one bug less) */
	XAddToSaveSet(dpy, w);
	XSetWindowBorderWidth(dpy, w, 0);

	XSetWindowAttributes swa;
	swa.background_pixel = XBlackPixel(dpy, screen);
	swa.border_pixel = XBlackPixel(dpy, screen);
	c->clipping_window = XCreateWindow(dpy, main_window, 0, 0, 1, 1, 0,
			root_wa.depth, InputOutput, root_wa.visual,
			CWBackPixel | CWBorderPixel, &swa);

	printf("XReparentWindow(%p, #%lu, #%lu, %d, %d)\n", dpy, c->xwin,
			c->clipping_window, 0, 0);
	XSelectInput(dpy, c->xwin, StructureNotifyMask | PropertyChangeMask);
	/* this produce an unmap ? */
	XReparentWindow(dpy, c->xwin, c->clipping_window, 0, 0);
	XUnmapWindow(dpy, c->clipping_window);

	if (!tree_root->add_notebook(c)) {
		printf("Fail to add a client\n");
		exit(EXIT_FAILURE);
	}

	printf("Return %s on %p\n", __PRETTY_FUNCTION__, (void *) w);
	return true;
}

long main_t::get_window_state(Window w) {
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, atoms.WM_STATE, 0L, 2L, False,
			atoms.WM_STATE, &real, &format, &n, &extra, (unsigned char **) &p)
			!= Success)
		return -1;

	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

/* inspired from dwm */
bool main_t::get_text_prop(Window w, Atom atom, std::string & text) {
	char **list = NULL;
	int n;
	XTextProperty name;
	XGetTextProperty(dpy, w, &name, atom);
	if (!name.nitems)
		return false;
	text = (char const *) name.value;
	return true;
}

void main_t::update_vm_name(client_t &c) {
	c.wm_name_is_valid = false;
	char **list = NULL;
	int n;
	XTextProperty name;
	XGetTextProperty(c.dpy, c.xwin, &name, atoms.WM_NAME);
	if (!name.nitems)
		return;
	c.wm_name_is_valid = true;
	c.wm_name = (char const *) name.value;
	XFree(name.value);
}

void main_t::update_net_vm_name(client_t &c) {
	c.net_wm_name_is_valid = false;
	char **list = NULL;
	int n;
	XTextProperty name;
	XGetTextProperty(c.dpy, c.xwin, &name, atoms._NET_WM_NAME);
	if (!name.nitems)
		return;
	c.net_wm_name_is_valid = true;
	c.net_wm_name = (char const *) name.value;
	XFree(name.value);
}

/* inspired from dwm */
void main_t::update_title(client_t &c) {
	if (c.net_wm_name_is_valid) {
		c.name = c.net_wm_name;
		return;
	}
	if (c.wm_name_is_valid) {
		c.name = c.wm_name;
	}
	std::stringstream s(std::stringstream::in | std::stringstream::out);
	s << "#" << (c.xwin) << " (noname)";
	c.name = s.str();
}

void main_t::client_update_size_hints(client_t * c) {
	printf("call %s\n", __PRETTY_FUNCTION__);
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(c->dpy, c->xwin, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;

	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else {
		c->basew = 0;
		c->baseh = 0;
	}

	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else {
		c->incw = 0;
		c->inch = 0;
	}

	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else {
		c->maxw = 0;
		c->maxh = 0;
	}

	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else {
		c->minw = 0;
		c->minh = 0;
	}

	if (size.flags & PAspect) {
		if (size.min_aspect.x != 0 && size.max_aspect.y != 0) {
			c->mina = (double) size.min_aspect.y / (double) size.min_aspect.x;
			c->maxa = (double) size.max_aspect.x / (double) size.max_aspect.y;
		}
	} else {
		c->maxa = 0.0;
		c->mina = 0.0;
	}
	c->is_fixed_size = (c->maxw && c->minw && c->maxh && c->minh && c->maxw
			== c->minw && c->maxh == c->minh);

	printf("return %s %d,%d\n", __PRETTY_FUNCTION__, c->width, c->height);
}

bool main_t::get_all(Window win, Atom prop, Atom type, int size,
		unsigned char **data, unsigned int *num) {
	bool ret = false;
	int res;
	unsigned char * xdata = 0;
	Atom ret_type;
	int ret_size;
	unsigned long int ret_items, bytes_left;

	res = XGetWindowProperty(dpy, win, prop, 0L,
			std::numeric_limits<int>::max(), False, type, &ret_type, &ret_size,
			&ret_items, &bytes_left, &xdata);
	if (res == Success) {
		if (ret_size == size && ret_items > 0) {
			unsigned int i;

			*data = (unsigned char *) malloc(ret_items * (size / 8));
			for (i = 0; i < ret_items; ++i)
				switch (size) {
				case 8:
					(*data)[i] = xdata[i];
					break;
				case 16:
					((uint16_t*) *data)[i] = ((uint16_t*) xdata)[i];
					break;
				case 32:
					((uint32_t*) *data)[i] = ((uint32_t*) xdata)[i];
					break;
				default:
					; /* unhandled size */
				}
			*num = ret_items;
			ret = true;
		}
		XFree(xdata);
	}
	return ret;
}

bool main_t::client_is_dock(client_t * c) {
	unsigned int num, i;
	int32_t * val;
	Window t;

	if (get_all(c->xwin, atoms._NET_WM_WINDOW_TYPE, atoms.ATOM, 32,
			(unsigned char **) &val, &num)) {
		/* use the first value that we know about in the array */
		for (i = 0; i < num; ++i) {
			if (val[i] == atoms._NET_WM_WINDOW_TYPE_DOCK)
				return true;
		}
		free(val);
	}

	return false;
}

void main_t::process_map_request_event(XEvent * e) {
	printf("Entering in %s #%p\n", __PRETTY_FUNCTION__,
			(void *) e->xmaprequest.window);
	Window w = e->xmaprequest.window;
	/* secure the map request */
	XGrabServer(dpy);
	XSync(dpy, False);
	XEvent ev;
	if (XCheckTypedWindowEvent(dpy, e->xunmap.window, DestroyNotify, &ev)) {
		/* the window is already destroyed, return */
		XUngrabServer(dpy);
		XFlush(dpy);
		return;
	}

	if (XCheckTypedWindowEvent(dpy, e->xunmap.window, UnmapNotify, &ev)) {
		/* the window is already unmapped, return */
		XUngrabServer(dpy);
		XFlush(dpy);
		return;
	}

	/* should never happen */
	XWindowAttributes wa;
	if (!XGetWindowAttributes(dpy, w, &wa)) {
		XMapWindow(dpy, w);
		return;
	}
	if (wa.override_redirect) {
		XMapWindow(dpy, w);
		return;
	}
	manage(w, &wa);
	XMapWindow(dpy, w);
	render();
	XUngrabServer(dpy);
	XFlush(dpy);
	printf("Return from %s #%p\n", __PRETTY_FUNCTION__,
			(void *) e->xmaprequest.window);
	return;

}

void main_t::process_map_notify_event(XEvent * e) {

}

void main_t::process_unmap_notify_event(XEvent * e) {
	printf("UnmapNotify #%lu #%lu\n", e->xunmap.window, e->xunmap.event);
	client_t * c = find_client_by_xwindow(e->xmap.window);
	/* unmap can be received twice time but are unique per window event.
	 * so this remove multiple events.
	 */
	if (e->xunmap.window != e->xunmap.event) {
		printf("Ignore this unmap #%lu #%lu\n", e->xunmap.window,
				e->xunmap.event);
		return;
	}
	if (!c)
		return;
	if (c->unmap_pending > 0) {
		printf("Expected Unmap\n");
		c->unmap_pending -= 1;
	} else {
		/**
		 * The client initiate an unmap.
		 * That mean the window go in WithDrawn state, PAGE must forget this window.
		 * Unmap is often followed by destroy window, If this is the case we try to
		 * reparent a destroyed window. Therefore we grab the server and check if the
		 * window is not already destroyed.
		 */
		XGrabServer(dpy);
		XSync(dpy, False);
		XEvent ev;
		if (XCheckTypedWindowEvent(dpy, e->xunmap.window, DestroyNotify, &ev)) {
			process_destroy_notify_event(&ev);
		} else {
			tree_root->remove_client(c->xwin);
			clients.remove(c);
			XReparentWindow(c->dpy, c->xwin, xroot, 0, 0);
			XRemoveFromSaveSet(c->dpy, c->xwin);
			XDestroyWindow(c->dpy, c->clipping_window);
			delete c;
			render();
		}
		XUngrabServer(dpy);
		XFlush(dpy);
	}
}

void main_t::process_destroy_notify_event(XEvent * e) {
	printf("DestroyNotify destroy : #%lu, event : #%lu\n", e->xunmap.window,
			e->xunmap.event);
	client_t * c = find_client_by_xwindow(e->xmap.window);
	if (c) {
		tree_root->remove_client(c->xwin);
		clients.remove(c);
		XDestroyWindow(dpy, c->clipping_window);
		delete c;
		render();
	}
}

void main_t::process_property_notify_event(XEvent * ev) {
	printf("Entering in %s on %lu\n", __PRETTY_FUNCTION__, ev->xproperty.window);

	char * name = XGetAtomName(dpy, ev->xproperty.atom);
	printf("Atom Name = \"%s\"\n", name);
	XFree(name);

	client_t * c = find_client_by_xwindow(ev->xproperty.window);
	if (!c)
		return;
	if (c->try_lock_client()) {
		if (ev->xproperty.atom == atoms._NET_WM_USER_TIME) {
			XRaiseWindow(dpy, ev->xproperty.window);
			XSetInputFocus(dpy, ev->xproperty.window, RevertToNone, CurrentTime);
		} else if (ev->xproperty.atom == atoms._NET_WM_NAME) {
			update_net_vm_name(*c);
			update_title(*c);
		} else if (ev->xproperty.atom == atoms.WM_NAME) {
			update_vm_name(*c);
			update_title(*c);
		}
		c->unlock_client();
	}
}

void main_t::update_vm_hints(client_t &c) {

}

}
