/*
 * floating_window.cxx
 *
 *  Created on: 15 févr. 2013
 *      Author: gschwind
 */

#include <cairo.h>
#include <cairo-xlib.h>
#include "floating_window.hxx"
#include "notebook.hxx"

namespace page {

floating_window_t::floating_window_t(window_t * w, Window parent) : w(w) {
	XWindowAttributes xwa;
	w->cnx.get_window_attributes(parent, &xwa);
	w->cnx.reparentwindow(w->get_xwin(), parent, 0, 0);

	/* create window handler */
	border = new window_t(w->cnx, parent, xwa);
	border->select_input(ButtonPressMask | ButtonReleaseMask | StructureNotifyMask);

	unsigned int width = w->wa.width;
	unsigned int heigth = w->wa.height;

	box_int_t size = w->get_size();
	box_int_t subsize = size;

	size.w = width + 8;
	size.h = heigth + 24 + 4;

	subsize.x = 4;
	subsize.y = 24;
	subsize.w = width;
	subsize.h = heigth;

	border->move_resize(size);
	w->move_resize(subsize);

	win_surf = cairo_xlib_surface_create(border->cnx.dpy, border->get_xwin(), border->wa.visual, border->wa.width, border->wa.height);
	cr = cairo_create(win_surf);

}

void floating_window_t::paint() {
	box_int_t x = border->get_size();
	cairo_xlib_surface_set_size(win_surf, x.w, x.h);
}

floating_window_t::~floating_window_t() {
	delete border;
}

void floating_window_t::map() {
	w->map();
	border->map();
}

void floating_window_t::unmap() {
	border->unmap();
}

void floating_window_t::reconfigure(box_int_t const & area) {

	unsigned int width = area.w;
	unsigned int heigth = area.h;

	box_int_t size = area;
	box_int_t subsize = size;

	size.x -= 2;
	size.y -= 11;
	size.w = width + 2;
	size.h = heigth + 12;

	subsize.x = 1;
	subsize.y = 11;
	subsize.w = width;
	subsize.h = heigth;

	border->move_resize(size);
	w->move_resize(subsize);
}


}

