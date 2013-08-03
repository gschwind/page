/*
 * composite_window.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */


#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <cairo.h>
#include <cairo-xlib.h>

#include <iostream>
#include <sstream>
#include <cassert>
#include <stdint.h>

#include "composite_window.hxx"
#include "compositor.hxx"

namespace page {

long int const ClientEventMask = (StructureNotifyMask | PropertyChangeMask);

composite_window_t::composite_window_t() {
	_dpy = 0;
	_wid = None;
	_visual = 0;
	_position = box_int_t(-100, -100, 1, 1);
	_damage = None;
	_is_map = false;
	_has_alpha = false;
	_surf = 0;
	_c_class = InputOnly;
	_region = _region_t();
	_has_shape = false;
}

void composite_window_t::update(Display * dpy, Window w, XWindowAttributes & wa) {
	_dpy = dpy;
	_wid = w;
	_visual = wa.visual;
	_position = box_int_t(wa.x, wa.y, wa.width, wa.height);

	XRenderPictFormat * format = XRenderFindVisualFormat(dpy, _visual);
	_has_alpha = ( format->type == PictTypeDirect && format->direct.alphaMask );
	_c_class = wa.c_class;

	update_map_state(wa.map_state != IsUnmapped);

	if (_c_class == InputOutput) {
		read_shape();
		XShapeInputSelected(dpy, _wid);
	}
}

void composite_window_t::init_cairo() {
	_surf = cairo_xlib_surface_create(_dpy, _wid, _visual, _position.w, _position.h);
}

void composite_window_t::destroy_cairo() {
	cairo_surface_destroy(_surf);
}


composite_window_t::~composite_window_t() {

}

void composite_window_t::draw_to(cairo_t * cr, box_int_t const & area) {

	box_int_t size = _position;
	box_int_t clip = area & size;

	if (clip.w > 0 && clip.h > 0) {
		cairo_save(cr);
		cairo_reset_clip(cr);
		cairo_identity_matrix(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(cr, _surf, size.x, size.y);
		cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
		cairo_clip(cr);
		cairo_paint(cr);
		cairo_restore(cr);
	}

}


bool composite_window_t::has_alpha() {
	return _has_alpha;
}

void composite_window_t::update_map_state(bool is_map) {
	if (is_map == _is_map)
		return;

	_is_map = is_map;

	if(_c_class != InputOutput || _dpy == 0) {
		_damage = None;
		return;
	}

	if (_is_map) {

	} else {

	}
}

void composite_window_t::read_shape() {
	if(_c_class != InputOutput) {
		_has_shape = false;
		return;
	}

	int count = 0, ordering = 0;
	XRectangle * recs = XShapeGetRectangles(_dpy, _wid, ShapeBounding, &count,
			&ordering);

	_region.clear();

	if(count > 0) {
		_has_shape = true;
		for(int i = 0; i < count; ++i) {
			_region = _region + box_int_t(recs[i]);
		}
		/* In doubt */
		XFree(recs);
	} else {
		_has_shape = false;
	}
}

region_t<int> composite_window_t::get_region() {
	region_t<int> region = _position;
	if (_has_shape) {
		_region_t shape_region = _region;
		shape_region.translate(_position.x, _position.y);
		region = region & shape_region;
	}
	return region;
}

void composite_window_t::update_position(box_int_t const & position) {
	_position = position;
	read_shape();
}

void composite_window_t::create_damage() {
	if (_c_class == InputOutput) {
		_damage = XDamageCreate(_dpy, _wid, XDamageReportNonEmpty);
		if (_damage != None) {
			XserverRegion region = XFixesCreateRegion(_dpy, 0, 0);
			XDamageSubtract(_dpy, _damage, None, region);
			XFixesDestroyRegion(_dpy, region);
		}
	}
}

void composite_window_t::destroy_damage() {
	if(_damage != None) {
		XDamageDestroy(_dpy, _damage);
		_damage = None;
	}
}


}


