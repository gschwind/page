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

namespace page {

long int const ClientEventMask = (StructureNotifyMask | PropertyChangeMask);

renderable_window_t::renderable_window_t(window_t * w) : w(w), damage(None), opacity(1.0), window_surf(0) {
	create_render_context();
}

void renderable_window_t::create_render_context() {
	if (window_surf == 0 && damage == None) {

		/* create the cairo surface */
		window_surf = w->create_cairo_surface();
		if(!window_surf)
			printf("WARNING CAIRO FAIL\n");
		/* track update */
		damage = XDamageCreate(w->get_display(), w->get_xwin(), XDamageReportNonEmpty);
		if (damage) {
			XserverRegion region = XFixesCreateRegion(w->get_display(), 0, 0);
			XDamageSubtract(w->get_display(), damage, None, region);
			XFixesDestroyRegion(w->get_display(), region);
		} else
			printf("DAMAGE FAIL.\n");
	}

}

void renderable_window_t::set_opacity(double x) {
	opacity = x;
	if (x > 1.0)
		x = 1.0;
	if (x < 0.0)
		x = 0.0;
}

void renderable_window_t::destroy_render_context() {
	if (damage != None) {
		XDamageDestroy(w->get_display(), damage);
		damage = None;
	}

	if (window_surf != 0) {
		cairo_surface_destroy(window_surf);
		window_surf = 0;
	}
}

renderable_window_t::~renderable_window_t() {
	destroy_render_context();
}


void renderable_window_t::repair1(cairo_t * cr, box_int_t const & area) {
	if(window_surf == 0)
		return;
	assert(window_surf != 0);

	box_int_t size = w->get_size();
	box_int_t clip = area & size;

	cairo_xlib_surface_set_size(window_surf, size.w, size.h);
	cairo_surface_mark_dirty(window_surf);
	//printf("repair window %s %dx%d+%d+%d\n", get_title().c_str(), clip.w, clip.h, clip.x, clip.y);
	if (clip.w > 0 && clip.h > 0) {
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_reset_clip(cr);
		cairo_set_source_surface(cr, window_surf, size.x, size.y);
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

box_int_t renderable_window_t::get_absolute_extend() {
	return w->get_size();
}

region_t<int> renderable_window_t::get_area() {
	return region_t<int>(w->get_size());
}

void renderable_window_t::mark_dirty() {
	//cairo_surface_flush(window_surf);
	cairo_surface_mark_dirty(window_surf);
}

void renderable_window_t::mark_dirty_retangle(box_int_t const & area) {
	//cairo_surface_flush(window_surf);
	cairo_surface_mark_dirty_rectangle(window_surf, area.x, area.y, area.w,
			area.h);
}

bool renderable_window_t::is_visible() {
	return (w->is_map() && !w->is_input_only() && !w->is_hidden());
}

void renderable_window_t::reconfigure(box_int_t const & area) {
	cairo_xlib_surface_set_size(window_surf, area.w, area.h);
}

}

