/*
 * renderable_window.cxx
 *
 *  Created on: 15 févr. 2013
 *      Author: gschwind
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

#include "renderable_window.hxx"
#include "compositor.hxx"

namespace page {

long int const ClientEventMask = (StructureNotifyMask | PropertyChangeMask);

renderable_window_t::renderable_window_t() : position(_position) {
	_dpy = 0;
	wid = None;
	visual = 0;
	_position = box_int_t(-100, -100, 1, 1);
	damage = None;
	_is_map = false;
	_has_alpha = false;
	_surf = 0;
	c_class = InputOnly;
	_region = region_t<int>();
	has_shape = false;
}

void renderable_window_t::update(Display * dpy, Window w, XWindowAttributes & wa) {
	_dpy = dpy;
	wid = w;
	visual = wa.visual;
	_position = box_int_t(wa.x, wa.y, wa.width, wa.height);

	XRenderPictFormat * format = XRenderFindVisualFormat(dpy, visual);
	_has_alpha = ( format->type == PictTypeDirect && format->direct.alphaMask );
	c_class = wa.c_class;

	update_map_state(wa.map_state != IsUnmapped);

	if (c_class == InputOutput) {
		//printf("Create damage\n");
		damage = XDamageCreate(_dpy, wid, XDamageReportNonEmpty);
		if (damage != None) {
			XserverRegion region = XFixesCreateRegion(_dpy, 0, 0);
			XDamageSubtract(_dpy, damage, None, region);
			XFixesDestroyRegion(_dpy, region);
		} else {
			//printf("Damage fail\n");
		}

		read_shape();
		XShapeInputSelected(dpy, wid);

		_surf = cairo_xlib_surface_create(_dpy, wid, visual, _position.w, _position.h);

	}

}

void renderable_window_t::init_cairo() {
	//_surf = cairo_xlib_surface_create(_dpy, wid, visual, position.w, position.h);
}

void renderable_window_t::destroy_cairo() {
	//cairo_surface_destroy(_surf);
}


renderable_window_t::~renderable_window_t() {
	//printf("Destroy Damage\n");
	if (damage != None) {
		XDamageDestroy(_dpy, damage);
		damage = None;
	}

	if (_surf != 0) {
		cairo_surface_destroy(_surf);
	}
}


void renderable_window_t::repair1(cairo_t * cr, box_int_t const & area) {

	box_int_t size = _position;
	box_int_t clip = area & size;

	//cairo_xlib_surface_set_size(_surf, size.w, size.h);
	//cairo_surface_mark_dirty(_surf);
	//printf("repair window %s %dx%d+%d+%d\n", get_title().c_str(), clip.w, clip.h, clip.x, clip.y);
	if (clip.w > 0 && clip.h > 0) {
		cairo_save(cr);
		cairo_reset_clip(cr);
		cairo_identity_matrix(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(cr, _surf, size.x, size.y);
		cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
		cairo_clip(cr);
		cairo_paint(cr);
//		cairo_reset_clip(cr);
//		cairo_rectangle(cr, clip.x - 0.5, clip.y - 0.5, clip.w - 0.5, clip.h - 0.5);
//		cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
//		cairo_stroke(cr);
		cairo_restore(cr);
	}

//	cairo_save(cr);
//	cairo_reset_clip(cr);
//	cairo_set_source_surface(cr, window_surf, size.x, size.y);
//	cairo_rectangle(cr, size.x, size.y, size.w, size.h);
//	cairo_clip(cr);
//	cairo_paint_with_alpha(cr, opacity);
//	cairo_restore(cr);
}


bool renderable_window_t::has_alpha() {
	return _has_alpha;
}

void renderable_window_t::update_map_state(bool is_map) {
	if (is_map == _is_map)
		return;

	_is_map = is_map;

	if(c_class != InputOutput || _dpy == 0) {
		damage = None;
		return;
	}

	if (_is_map) {

	} else {

	}
}

void renderable_window_t::read_shape() {
	if(c_class != InputOutput) {
		has_shape = false;
		return;
	}

	int count, ordering;
	XRectangle * recs = XShapeGetRectangles(_dpy, wid, ShapeBounding, &count, &ordering);

	_region.clear();

	if(recs != NULL) {
		has_shape = true;
		for(int i = 0; i < count; ++i) {
			_region = _region + box_int_t(recs[i]);
		}
		/* In doubt */
		XFree(recs);
	} else {
		has_shape = false;
	}
}

region_t<int> renderable_window_t::get_region() {
	region_t<int> region = _position;
	if (has_shape) {
		region_t<int> shape_region = _region;
		shape_region.translate(_position.x, _position.y);
		region = region & shape_region;
	}
	return region;
}

void renderable_window_t::update_position(box_int_t const & position) {
	_position = position;
	read_shape();

	if(_surf)
		cairo_xlib_surface_set_size(_surf, _position.w, _position.h);
}


}


