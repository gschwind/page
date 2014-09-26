/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_NOTEBOOK_FADING_HXX_
#define RENDERABLE_NOTEBOOK_FADING_HXX_

#include <cairo/cairo.h>

#include "pixmap.hxx"
#include "utils.hxx"
#include "region.hxx"
#include "renderable.hxx"
#include "box.hxx"

namespace page {

using namespace std;

class renderable_notebook_fading_t : public renderable_t {

	double ratio;

	i_rect prev_pos;
	i_rect next_pos;

	ptr<pixmap_t> prev_surf;
	ptr<pixmap_t> next_surf;

public:

	renderable_notebook_fading_t(ptr<pixmap_t> prev, ptr<pixmap_t> next, i_rect prev_pos, i_rect next_pos) : prev_surf(prev), next_surf(next), prev_pos(prev_pos), next_pos(next_pos), ratio(0.5) {

	}

	virtual ~renderable_notebook_fading_t() { }

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {

		for (auto &cl : area) {
			/** render old surface if one is available **/
			if (prev_surf != nullptr) {
				cairo_surface_t * s = prev_surf->get_cairo_surface();
				region r(prev_pos);
				r -= next_pos;

				cairo_save(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_pattern_t * p0 = cairo_pattern_create_rgba(1.0, 1.0, 1.0,
						1.0 - ratio);
				for (auto &a : r) {
					cairo_clip(cr, cl & a);
					cairo_set_source_surface(cr, s, prev_pos.x, prev_pos.y);
					cairo_mask(cr, p0);
				}
				cairo_pattern_destroy(p0);

				region r1(prev_pos);
				r1 &= next_pos;
				for (auto &a : r1) {
					cairo_clip(cr, cl & a);
					cairo_set_source_surface(cr, s, prev_pos.x, prev_pos.y);
					cairo_paint(cr);
				}

				cairo_restore(cr);
			}

			/** render selected surface if available **/
			if (next_surf != nullptr) {
				cairo_surface_t * s = next_surf->get_cairo_surface();
				region r(next_pos);
				r -= prev_pos;

				cairo_save(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_pattern_t * p0 = cairo_pattern_create_rgba(1.0, 1.0, 1.0,
						ratio);
				cairo_set_source_surface(cr, s, next_pos.x, next_pos.y);
				cairo_clip(cr, cl & next_pos);
				cairo_mask(cr, p0);
				cairo_pattern_destroy(p0);

				cairo_restore(cr);
			}

		}

	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return region();
	}

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() {
		region ret;
		ret += next_pos;
		ret += prev_pos;
		return ret;
	}

	virtual region get_damaged() {
		region ret;
		ret += next_pos;
		ret += prev_pos;
		return ret;
	}

	void set_ratio(double x) {
		ratio = std::min(std::max(0.0, x), 1.0);
	}


};



}



#endif /* RENDERABLE_SURFACE_HXX_ */
