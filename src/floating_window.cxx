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

	/* create window handler */
	border = new window_t(w->cnx, parent, xwa);
	border->select_input(ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | PropertyChangeMask | SubstructureRedirectMask);
	border->read_all();

	w->cnx.reparentwindow(w->get_xwin(), parent, 0, 0);

	win_surf = cairo_xlib_surface_create(border->cnx.dpy, border->get_xwin(), border->visual, border->position.w, border->position.h);
	cr = cairo_create(win_surf);

	box_int_t x = w->get_size();
	reconfigure(x);

}

floating_window_t::~floating_window_t() {
	delete border;
}

void floating_window_t::map() {
	border->map();
	w->normalize();
}

void floating_window_t::unmap() {
	border->unmap();
	w->iconify();
}

void floating_window_t::reconfigure(box_int_t const & area) {

	int x = area.x;
	int y = area.y;
	int width = area.w;
	int heigth = area.h;

	box_int_t size;
	box_int_t subsize;

	size.x = x - 4;
	size.y = y - 26;
	size.w = width + 8;
	size.h = heigth + 24 + 4;

	if(size.x < 0)
		size.x = 0;
	if(size.y < 0)
		size.y = 0;

	subsize.x = 4;
	subsize.y = 26;
	subsize.w = width;
	subsize.h = heigth;

	border->move_resize(size);
	w->move_resize(subsize);

	cairo_xlib_surface_set_size(win_surf, size.w, size.h);

}

box_int_t floating_window_t::get_position() {
	box_int_t size = border->get_size();
	box_int_t subsize = w->get_size();
	return box_int_t(size.x + subsize.x, size.y + subsize.y, subsize.w, subsize.h);
}


}

