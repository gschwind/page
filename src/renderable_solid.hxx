/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_SOLID_HXX_
#define RENDERABLE_SOLID_HXX_

#include "tree.hxx"
#include "utils.hxx"
#include "renderable.hxx"
#include "color.hxx"

namespace page {

class renderable_solid_t : public tree_t {
	rect location;
	color_t color;
	region damaged;
	region opaque_region;
	region visible_region;

public:

	renderable_solid_t(color_t color, rect loc, region damaged) :
		damaged{damaged},
		color{color},
		location{loc}
	{
		opaque_region = region{loc};
		visible_region = region{loc};
	}

	virtual ~renderable_solid_t() { }

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a);
		region r = visible_region & area;
		for (auto &i : r.rects()) {
			cairo_clip(cr, i);
			cairo_fill(cr);
		}
		cairo_restore(cr);
	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		if(color.a > 0.0) {
			return region{};
		} else {
			return opaque_region;
		}
	}

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() {
		return visible_region;
	}

	virtual region get_damaged() {
		return damaged;
	}

	void set_opaque_region(region const & r) {
		opaque_region = r;
	}

	void set_visible_region(region const & r) {
		visible_region = r;
	}

};



}



#endif /* RENDERABLE_SURFACE_HXX_ */
