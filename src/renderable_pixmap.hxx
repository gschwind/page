/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_PIXMAP_HXX_
#define RENDERABLE_PIXMAP_HXX_

#include "tree.hxx"
#include "pixmap.hxx"

namespace page {

using namespace std;

class renderable_pixmap_t : public tree_t {
	tree_t * _parent;

	rect location;
	shared_ptr<pixmap_t> surf;
	region damaged;
	region opaque_region;
	region visible_region;

public:

	renderable_pixmap_t(shared_ptr<pixmap_t> s, rect loc, region damaged = region{}) :
		damaged{damaged},
		surf{s},
		location{loc},
		_parent{nullptr}
	{
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

	virtual region get_opaque_region() {
		return opaque_region;
	}

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

	void clear_damaged() {
		damaged.clear();
	}

	void add_damaged(region const & r) {
		damaged += r;
	}

};



}



#endif /* RENDERABLE_SURFACE_HXX_ */
