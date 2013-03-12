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

floating_window_t::floating_window_t(window_t * w, window_t * border) : w(w) {

	this->border = border;

	w->reparent(border->get_xwin(), 0, 0);

	win_surf = border->create_cairo_surface();
	cr = cairo_create(win_surf);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgb(cr, 1.0, 0.0, 1.0);
	cairo_paint(cr);

	_desired_position = w->get_size();
	reconfigure();
	fake_configure();

}

floating_window_t::~floating_window_t() {
	if(cr != 0) {
		cairo_destroy(cr);
	}

	if(win_surf != 0) {
		cairo_surface_destroy(win_surf);
	}
}

void floating_window_t::normalize() {
	border->map();
	w->normalize();
}

void floating_window_t::iconify() {
	border->unmap();
	w->iconify();
}

void floating_window_t::reconfigure() {

	int x = _desired_position.x;
	int y = _desired_position.y;
	int width = _desired_position.w;
	int heigth = _desired_position.h;

	box_int_t size;
	box_int_t subsize;

	size.x = x - 4;
	size.y = y - 26;
	size.w = width + 8;
	size.h = heigth + 24 + 6;

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

void floating_window_t::set_desired_position(box_int_t const & position) {
		_desired_position = position;
}

box_int_t const & floating_window_t::get_desired_position() const {
	return _desired_position;
}

void floating_window_t::fake_configure() {
	w->fake_configure(_desired_position, 0);
}

}

