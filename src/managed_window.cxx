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

managed_window_t::managed_window_t(managed_window_type_e initial_type,
		window_t * orig, window_t * base, window_t * deco,
		theme_layout_t const * theme) :
		orig(orig), base(base), deco(deco) {

	set_theme(theme);
	init_managed_type(initial_type);

	orig->reparent(base->id, 0, 0);

	_surf = deco->create_cairo_surface();
	assert(_surf != 0);
	_cr = cairo_create(_surf);
	assert(_cr != 0);

	_back_surf = cairo_surface_create_similar(_surf, CAIRO_CONTENT_COLOR,
			base->get_size().w, base->get_size().h);

	_back_cr = cairo_create(_back_surf);

	_floating_wished_position = orig->get_size();
	set_wished_position(orig->get_size());

	icon = 0;

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
	orig->normalize();
	deco->map();
	base->map();
}

void managed_window_t::iconify() {
	base->unmap();
	deco->unmap();
	orig->iconify();
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

	base->move_resize(_base_position);
	deco->move_resize(box_int_t(0, 0, _base_position.w, _base_position.h));
	orig->move_resize(_orig_position);

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
	orig->fake_configure(_wished_position, 0);
}

void managed_window_t::delete_window(Time t) {
	orig->delete_window(t);
}

bool managed_window_t::check_orig_position(box_int_t const & position) {
	return position == _orig_position;
}

bool managed_window_t::check_base_position(box_int_t const & position) {
	return position == _base_position;
}

void managed_window_t::init_managed_type(managed_window_type_e type) {
	switch(type) {
	case MANAGED_FLOATING:
		_margin_top = theme->floating_margin.top;
		_margin_bottom = theme->floating_margin.bottom;
		_margin_left = theme->floating_margin.left;
		_margin_right = theme->floating_margin.right;
		break;
	case MANAGED_NOTEBOOK:
	case MANAGED_FULLSCREEN:
		_margin_top = 0;
		_margin_bottom = 0;
		_margin_left = 0;
		_margin_right = 0;
		break;
	}

	_type = type;

}

void managed_window_t::set_managed_type(managed_window_type_e type) {
	switch(type) {
	case MANAGED_FLOATING:
		_margin_top = theme->floating_margin.top;
		_margin_bottom = theme->floating_margin.bottom;
		_margin_left = theme->floating_margin.left;
		_margin_right = theme->floating_margin.right;
		_wished_position = _floating_wished_position;
		break;
	case MANAGED_NOTEBOOK:
	case MANAGED_FULLSCREEN:
		if(_type == MANAGED_FLOATING)
			_floating_wished_position = _wished_position;
		_margin_top = 0;
		_margin_bottom = 0;
		_margin_left = 0;
		_margin_right = 0;
		break;
	}

	_type = type;
	reconfigure();

}

string managed_window_t::get_title() {
	return orig->get_title();
}

cairo_t * managed_window_t::get_cairo_context() {
	return _cr;
}

void managed_window_t::focus(Time t) {
	if(!orig->is_map())
		return;

	/** when focus a window, disable all button grab **/
	base->ungrab_all_buttons();

	orig->icccm_focus(t);

}

box_int_t managed_window_t::get_base_position() const {
	return _base_position;
}

managed_window_type_e managed_window_t::get_type() {
	return _type;
}

window_icon_handler_t * managed_window_t::get_icon() {
	if(icon == 0) {
		icon = new window_icon_handler_t(orig);
	}
	return icon;
}

void managed_window_t::update_icon() {
	if(icon != 0)
		delete icon;
	icon = new window_icon_handler_t(orig);
}

void managed_window_t::set_theme(theme_layout_t const * theme) {
	this->theme = theme;
}

cairo_t * managed_window_t::get_cairo() {
	return _back_cr;
}

bool managed_window_t::is(managed_window_type_e type) {
	return _type == type;
}

void managed_window_t::expose() {
	cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(_cr, 0, 0, deco->get_size().w, deco->get_size().h);
	cairo_set_source_surface(_cr, _back_surf, 0, 0);
	cairo_paint(_cr);
	cairo_surface_flush(_surf);
}

}


