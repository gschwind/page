/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_PIXMAP_HXX_
#define RENDERABLE_PIXMAP_HXX_

#include "pixmap.hxx"

namespace page {

class renderable_pixmap_t : public renderable_t {

	rect location;
	std::shared_ptr<pixmap_t> surf;
	region damaged;
	region opaque_region;
	region visible_region;

public:

	renderable_pixmap_t(std::shared_ptr<pixmap_t> s, rect loc, region damaged) : damaged(damaged), surf(s), location(loc) {
		opaque_region = region(loc);
		visible_region = region(loc);
	}

	virtual ~renderable_pixmap_t() { }

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {
		cairo_save(cr);
		if (surf != nullptr) {
			if (surf->get_cairo_surface() != nullptr) {
				cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
				cairo_set_source_surface(cr, surf->get_cairo_surface(),
						location.x, location.y);
				region r = visible_region & area;
				for (auto &i : r) {
					cairo_clip(cr, i);
					cairo_mask_surface(cr, surf->get_cairo_surface(), location.x, location.y);
				}
			}
		}
		cairo_restore(cr);
	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return opaque_region;
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
