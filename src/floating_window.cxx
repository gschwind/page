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

	set_wished_position(w->get_size());

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

	_base_position.x = _wished_position.x - 4;
	_base_position.y = _wished_position.y - 26;
	_base_position.w = _wished_position.w + 8;
	_base_position.h = _wished_position.h + 24 + 6;

	if(_base_position.x < 0)
		_base_position.x = 0;
	if(_base_position.y < 0)
		_base_position.y = 0;

	_orig_position.x = 4;
	_orig_position.y = 26;
	_orig_position.w = _wished_position.w;
	_orig_position.h = _wished_position.h;

	base->move_resize(_base_position);
	orig->move_resize(_orig_position);

	cairo_xlib_surface_set_size(win_surf, _base_position.w, _base_position.h);

	fake_configure();

}

void floating_window_t::set_wished_position(box_int_t const & position) {
		_wished_position = position;
		reconfigure();
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

bool floating_window_t::check_orig_position(box_int_t const & position) {
	return position == _orig_position;
}

bool floating_window_t::check_base_position(box_int_t const & position) {
	return position == _base_position;
}

}

