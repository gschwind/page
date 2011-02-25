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

namespace page_next {

class root_t {
	tree_t *note0, *note1;
	tree_t *vpan;

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

	Window x_main_window;

	root_t(root_t const &);
	root_t &operator=(root_t const &);
public:
	root_t();
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

	void update_allocation(box_t & alloc) {
	}

};

}

#endif /* PAGE_HXX_ */
