/*
 * page.hxx
 *
 *  Created on: 23 févr. 2011
 *      Author: gschwind
 */

#ifndef PAGE_HXX_
#define PAGE_HXX_

#include <X11/X.h>
#include <X11/cursorfont.h>
#include <assert.h>
#include <unistd.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include <stdio.h>

#include <list>
#include <string>
#include <iostream>

#include "tree.hxx"
#include "atoms.hxx"
#include "client.hxx"

namespace page_next {

enum {
	AtomType,
	WMProtocols,
	WMDelete,
	WMState,
	WMLast,
	NetSupported,
	NetWMName,
	NetWMState,
	NetWMFullscreen,
	NetWMType,
	NetWMTypeDock,
	NetLast,
	AtomLast
}; /* EWMH atoms */

class main_t {
	tree_t * tree_root;
	std::list<client_t *> clients;

	Cursor cursor;
	XWindowAttributes wa;
	int running;
	int selected;
	Display *dpy;
	int screen;
	Window xroot;
	/* size of default root window */
	int sw, sh, sx, sy;
	int start_x, end_x;
	int start_y, end_y;

	atoms_t atoms;

	Window x_main_window;

	main_t(main_t const &);
	main_t &operator=(main_t const &);
public:
	main_t();
	void render(cairo_t * cr);
	void render();
	void run();

	Window get_window() {
		return x_main_window;
	}

	Display * get_dpy() {
		return dpy;
	}

	cairo_t * get_cairo() {
		cairo_surface_t * surf;
		XGetWindowAttributes(this->dpy, this->x_main_window, &(this->wa));
		surf = cairo_xlib_surface_create(this->dpy, this->x_main_window,
				this->wa.visual, this->wa.width, this->wa.height);
		cairo_t * cr = cairo_create(surf);
		return cr;
	}

	void scan();
	long get_window_state(Window w);
	bool manage(Window w, XWindowAttributes * wa);
	client_t * find_client_by_xwindow(Window w);
	client_t * find_client_by_clipping_window(Window w);
	bool get_text_prop(Window w, Atom atom, std::string & text);
	void update_title(client_t * c);
	void client_update_size_hints(client_t * ths);
	bool client_is_dock(client_t * c);
	bool get_all(Window win, Atom prop, Atom type, int size,
			unsigned char **data, unsigned int *num);

	void process_map_request_event(XEvent * e);
	void process_map_notify_event(XEvent * e);
	void process_unmap_notify_event(XEvent * e);
};

}

#endif /* PAGE_HXX_ */
