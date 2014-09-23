/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_PIXMAP_HXX_
#define RENDERABLE_PIXMAP_HXX_

#include <cairo/cairo.h>

#include "pixmap.hxx"
#include "utils.hxx"
#include "region.hxx"
#include "renderable.hxx"
#include "box.hxx"

namespace page {



class renderable_pixmap_t : public renderable_t {

	rectangle location;
	ptr<pixmap_t> surf;

public:

	renderable_pixmap_t(ptr<pixmap_t> s, rectangle loc) : surf(s), location(loc) {
		location.round();
	}

	virtual ~renderable_pixmap_t() { }

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {
		if (surf != nullptr) {
			if (surf->get_cairo_surface() != nullptr) {
				cairo_set_source_surface(cr, surf->get_cairo_surface(),
						location.x, location.y);
				region r = region(location) & area;
				for (auto &i : r) {
					i.round(); // force integer
					cairo_rectangle(cr, i.x, i.y, i.w, i.h	);
					cairo_fill(cr);
				}
			}
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


};



}



#endif /* RENDERABLE_SURFACE_HXX_ */
