/*
 * page.cxx
 *
 *  Created on: 23 févr. 2011
 *      Author: gschwind
 */

#include "page.hxx"
#include "box.hxx"
#include "client.hxx"

#include <X11/Xlib.h>
#include <stdlib.h>

#include <sstream>
#include <limits>
#include <stdint.h>

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

root_t::root_t() {
	XSetWindowAttributes swa;
	dpy = XOpenDisplay(0);
	screen = DefaultScreen(dpy);
	xroot = DefaultRootWindow(dpy);
	assert(dpy);
	// Create the window
	this->x_main_window = XCreateWindow(this->dpy, this->xroot, 0, 0, 800, 600,
			0, DefaultDepth(this->dpy, this->screen), InputOutput,
			DefaultVisual(this->dpy, this->screen), 0, &swa);
	XSelectInput(this->dpy, this->x_main_window, StructureNotifyMask
			| ButtonPressMask);
	XMapWindow(this->dpy, this->x_main_window);

#define ATOM_INIT(name) atoms.name = XInternAtom(dpy, #name, False)

	ATOM_INIT(ATOM);
	ATOM_INIT(CARDINAL);

	ATOM_INIT(WM_STATE);

	ATOM_INIT(_NET_SUPPORTED);
	ATOM_INIT(_NET_WM_NAME);
	ATOM_INIT(_NET_WM_STATE);
	ATOM_INIT(_NET_WM_STATE_FULLSCREEN);

	ATOM_INIT(_NET_WM_WINDOW_TYPE);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_DOCK);

	ATOM_INIT(_NET_WM_STRUT_PARTIAL);

	box_t<int> allocation;
	allocation.x = 0;
	allocation.y = 0;
	allocation.w = 800;
	allocation.h = 600;

	tree_root = new tree_t(dpy, x_main_window);
	tree_root->update_allocation(allocation);

}

void root_t::run() {
	scan();
	running = 1;
	while (running) {
		XEvent e;
		XNextEvent(this->dpy, &e);
		printf("Event %s\n", x_event_name[e.type]);
		if (e.type == MapNotify || e.type == Expose) {
			render();
		} else if (e.type == ButtonPress) {
			tree_root->process_button_press_event(&e);
			render();
		} else if (e.type == MapRequest) {
			process_map_request_event(&e);
		}
	}
}

void root_t::render(cairo_t * cr) {
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_rectangle(cr, 0, 0, wa.width, wa.height);
	cairo_fill(cr);
	tree_root->render(cr);
}

void root_t::render() {
	cairo_t * cr = get_cairo();
	render(cr);
	cairo_destroy(cr);
}

void root_t::scan() {
	printf("call %s\n", __PRETTY_FUNCTION__);
	unsigned int i, num;
	Window d1, d2, *wins = 0;
	XWindowAttributes wa;

	/* ask for child of current root window, use Xlib here since gdk only know windows it
	 * have created. */
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
}

client_t * root_t::find_client_by_xwindow(Window w) {
	std::list<client_t *>::iterator i = clients.begin();
	while (i != clients.end()) {
		if ((*i)->xwin == w)
			return (*i);
		++i;
	}

	return 0;
}

client_t * root_t::find_client_by_clipping_window(Window w) {
	std::list<client_t *>::iterator i = clients.begin();
	while (i != clients.end()) {
		if ((*i)->clipping_window == w)
			return (*i);
		++i;
	}

	return 0;
}

void root_t::manage(Window w, XWindowAttributes * wa) {
	fprintf(stderr, "Call %s on %p\n", __PRETTY_FUNCTION__, (void *) w);
	if (x_main_window == w)
		return;

	client_t * c = find_client_by_xwindow(w);
	if (c != NULL) {
		printf("Window %p is already managed\n", c);
		return;
	}

	/* do not manage clipping window */
	if (find_client_by_clipping_window(w))
		return;

	c = new client_t;
	c->xwin = w;
	c->dpy = dpy;
	c->is_mapped = false;
	/* before page prepend !! */
	clients.push_back(c);

	update_title(c);
	client_update_size_hints(c);

	if (client_is_dock(c)) {
		printf("IsDock !\n");
		/* partial struct need to be read ! */
		int32_t * partial_struct;
		unsigned int n;
		get_all(c->xwin, atoms._NET_WM_STRUT_PARTIAL, atoms.CARDINAL, 32,
				(unsigned char **) &partial_struct, &n);

		sw -= partial_struct[0] + partial_struct[1];
		sh -= partial_struct[2] + partial_struct[3];
		if (sx < partial_struct[0])
			sx = partial_struct[0];
		if (sy < partial_struct[2])
			sy = partial_struct[2];

		XMoveResizeWindow(dpy, x_main_window, sx, sy, sw, sh);
		box_t<int> b;
		b.x = 0;
		b.y = 0;
		b.h = sh;
		b.w = sw;
		tree_root->update_allocation(b);

		return;
	}

	/* this window will not be destroyed on page close (one bug less)
	 * TODO check gdk equivalent */
	XAddToSaveSet(dpy, w);

	//gdk_window_foreign_new(c->xwin);
	c->clipping_window = XCreateSimpleWindow(dpy, x_main_window, 0, 0, 1, 1, 0,
			XBlackPixel(dpy, 0), XBlackPixel(dpy, 0));
	XReparentWindow(dpy, c->xwin, c->clipping_window, 0, 0);
	XMapWindow(dpy, c->xwin);
	tree_root->add_notebook(c);

	fprintf(stderr, "Return %s on %p\n", __FUNCTION__, (void *) w);
}

long root_t::get_window_state(Window w) {
	printf("call %s\n", __FUNCTION__);
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
bool root_t::get_text_prop(Window w, Atom atom, std::string & text) {
	char **list = NULL;
	int n;
	XTextProperty name;
	if (text.length() == 0)
		return false;
	XGetTextProperty(dpy, w, &name, atom);
	if (!name.nitems)
		return false;
	if (name.encoding == XA_STRING) {
		text = (char const *) name.value;
	} else {
		if (XmbTextPropertyToTextList(dpy, &name, &list, &n) == Success) {
			if (n > 0) {
				if (list[0]) {
					text = list[0];
				}
			}
			XFreeStringList(list);
		}
	}
	XFree(name.value);
	return true;
}

/* inspired from dwm */
void root_t::update_title(client_t * c) {
	if (!get_text_prop(c->xwin, atoms._NET_WM_NAME, c->name))
		if (!get_text_prop(c->xwin, XA_WM_NAME, c->name)) {
			std::stringstream s(std::stringstream::in | std::stringstream::out);
			s << static_cast<int> (c->xwin) << " (noname)";
			c->name = s.str();
		}
	if (c->name[0] == '\0') { /* hack to mark broken clients */
		std::ostringstream s;
		s << static_cast<int> (c->xwin) << " (broken)";
		c->name = s.str();
	}
}

void root_t::client_update_size_hints(client_t * c) {
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

bool root_t::get_all(Window win, Atom prop, Atom type, int size,
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

bool root_t::client_is_dock(client_t * c) {
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

void root_t::process_map_request_event(XEvent * e) {
	printf("Entering in %s #%p\n", __PRETTY_FUNCTION__,
			(void *) e->xmaprequest.window);
	Window w = e->xmaprequest.window;
	/* should never happen */

	static XWindowAttributes wa;
	if (!XGetWindowAttributes(dpy, w, &wa))
		return;
	if (wa.override_redirect)
		return;
	manage(w, &wa);
	render();

	printf("Return from %s #%p\n", __FUNCTION__, (void *) e->xmaprequest.window);
	return;

}

}
