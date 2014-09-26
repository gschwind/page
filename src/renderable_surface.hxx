/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_SURFACE_HXX_
#define RENDERABLE_SURFACE_HXX_

#include <cairo/cairo.h>

#include "region.hxx"
#include "renderable.hxx"
#include "box.hxx"

namespace page {



class renderable_surface_t : public renderable_t {

	i_rect location;
	cairo_surface_t * surf;
	region _damaged;

public:

	renderable_surface_t(cairo_surface_t * s, i_rect loc) : surf(s), location(loc) {
		location.round();
	}

	virtual ~renderable_surface_t() { }

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {
		cairo_set_source_surface(cr, surf, location.x, location.y);
		region r = region(location) & area;
		for(auto &i: r) {
			i.round(); // force integer
			cairo_rectangle(cr, i.x, i.y, i.w, i.h);
			cairo_fill(cr);
		}
	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return region(location);
	}

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() {
		return region(location);
	}

	virtual region get_damaged() {
		return _damaged;
	}

	void clear_damaged() {
		_damaged.clear();
	}

	void add_damaged(region const & damaged) {
		_damaged += damaged;
	}


};



}



#endif /* RENDERABLE_SURFACE_HXX_ */
