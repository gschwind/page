/*
 * tab_window.cxx
 *
 *  Created on: 9 mars 2013
 *      Author: gschwind
 */

#include "tab_window.hxx"

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

namespace page {

tab_window_t::tab_window_t(window_t * w, window_t * border) : w(w) {

	this->border = border;

	w->reparent(border->get_xwin(), 0, 0);

	win_surf = border->create_cairo_surface();
	cr = cairo_create(win_surf);
//	cairo_reset_clip(cr);
//	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
//	cairo_set_source_rgba(cr, 1.0, 0.0, 1.0, 1.0);
//	cairo_paint(cr);

}

tab_window_t::~tab_window_t() {
	if(cr != 0) {
		cairo_destroy(cr);
	}

	if(win_surf != 0) {
		cairo_surface_destroy(win_surf);
	}
}

void tab_window_t::reconfigure() {
	border->move_resize(box_int_t(_wished_position.x, _wished_position.y, _wished_position.w, _wished_position.h));
	w->move_resize(box_int_t(0, 0, _wished_position.w, _wished_position.h));
	cairo_xlib_surface_set_size(win_surf, _wished_position.w, _wished_position.h);
}

void tab_window_t::iconify() {
	w->iconify();
	border->unmap();
}

void tab_window_t::normalize() {
	border->map();
	w->normalize();
}


box_int_t const & tab_window_t::get_wished_position() {
	return _wished_position;
}

void tab_window_t::set_wished_position(box_int_t const & position) {
	_wished_position = position;
}

void tab_window_t::fake_configure() {
	w->fake_configure(_wished_position, 0);
}

}

