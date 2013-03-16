/*
 * managed_window.cxx
 *
 *  Created on: 16 mars 2013
 *      Author: gschwind
 */

#include <cairo.h>
#include <cairo-xlib.h>
#include "managed_window.hxx"
#include "notebook.hxx"

namespace page {

managed_window_t::managed_window_t(managed_window_type_e initial_type, window_t * w, window_t * border) : _orig(w) {

	set_managed_type(initial_type);

	this->_base = border;

	w->reparent(border->get_xwin(), 0, 0);

	_surf = border->create_cairo_surface();
	_cr = cairo_create(_surf);
	cairo_set_operator(_cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgb(_cr, 1.0, 0.0, 1.0);
	cairo_paint(_cr);

	set_wished_position(w->get_size());

}

managed_window_t::~managed_window_t() {
	if(_cr != 0) {
		cairo_destroy(_cr);
	}

	if(_surf != 0) {
		cairo_surface_destroy(_surf);
	}
}

void managed_window_t::normalize() {
	_base->map();
	_orig->normalize();
}

void managed_window_t::iconify() {
	_base->unmap();
	_orig->iconify();
}

void managed_window_t::reconfigure() {

	_base_position.x = _wished_position.x - _margin_left;
	_base_position.y = _wished_position.y - _margin_top;
	_base_position.w = _wished_position.w + _margin_left + _margin_right;
	_base_position.h = _wished_position.h + _margin_top + _margin_bottom;

	if(_base_position.x < 0)
		_base_position.x = 0;
	if(_base_position.y < 0)
		_base_position.y = 0;

	_orig_position.x = _margin_left;
	_orig_position.y = _margin_top;
	_orig_position.w = _wished_position.w;
	_orig_position.h = _wished_position.h;

	_base->move_resize(_base_position);
	_orig->move_resize(_orig_position);

	cairo_xlib_surface_set_size(_surf, _base_position.w, _base_position.h);

	fake_configure();

}

void managed_window_t::set_wished_position(box_int_t const & position) {
		_wished_position = position;
		reconfigure();
}

box_int_t const & managed_window_t::get_wished_position() const {
	return _wished_position;
}

void managed_window_t::fake_configure() {
	_orig->fake_configure(_wished_position, 0);
}

void managed_window_t::delete_window(Time t) {
	_orig->delete_window(t);
}

window_t * managed_window_t::get_orig() {
	return _orig;
}

window_t * managed_window_t::get_base() {
	return _base;
}

bool managed_window_t::check_orig_position(box_int_t const & position) {
	return position == _orig_position;
}

bool managed_window_t::check_base_position(box_int_t const & position) {
	return position == _base_position;
}

void managed_window_t::set_managed_type(managed_window_type_e type) {
	_type = type;
	switch(type) {
	case MANAGED_FLOATING:
		_margin_top = 26;
		_margin_bottom = 4;
		_margin_left = 4;
		_margin_right = 4;
		break;
	case MANAGED_NOTEBOOK:
	case MANAGED_FULLSCREEN:
	default:
		_margin_top = 0;
		_margin_bottom = 0;
		_margin_left = 0;
		_margin_right = 0;
		break;
	}
}

string managed_window_t::get_title() {
	return _orig->get_title();
}

cairo_t * managed_window_t::get_cairo_context() {
	return _cr;
}

void managed_window_t::focus() {
	_orig->focus();
}

}


