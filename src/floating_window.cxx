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

floating_window_t::floating_window_t(window_t * w, window_t * border) : orig(w) {

	this->base = border;

	w->reparent(border->get_xwin(), 0, 0);

	win_surf = border->create_cairo_surface();
	cr = cairo_create(win_surf);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgb(cr, 1.0, 0.0, 1.0);
	cairo_paint(cr);

	_wished_position = w->get_size();
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
	base->map();
	orig->normalize();
}

void floating_window_t::iconify() {
	base->unmap();
	orig->iconify();
}

void floating_window_t::reconfigure() {

	int x = _wished_position.x;
	int y = _wished_position.y;
	int width = _wished_position.w;
	int heigth = _wished_position.h;

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

	base->move_resize(size);
	orig->move_resize(subsize);

	cairo_xlib_surface_set_size(win_surf, size.w, size.h);

}

void floating_window_t::set_wished_position(box_int_t const & position) {
		_wished_position = position;
}

box_int_t const & floating_window_t::get_wished_position() const {
	return _wished_position;
}

void floating_window_t::fake_configure() {
	orig->fake_configure(_wished_position, 0);
}

void floating_window_t::delete_window(Time t) {
	orig->delete_window(t);
}

window_t * floating_window_t::get_orig() {
	return orig;
}

window_t * floating_window_t::get_base() {
	return base;
}

}

