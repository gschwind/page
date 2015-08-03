/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_NOTEBOOK_FADING_HXX_
#define RENDERABLE_NOTEBOOK_FADING_HXX_

#include <cairo/cairo.h>

#include <memory>

#include "tree.hxx"
#include "utils.hxx"
#include "renderable.hxx"
#include "pixmap.hxx"

namespace page {

using namespace std;

class renderable_notebook_fading_t : public tree_t {
	tree_t * _parent;

	double _ratio;

	rect _old_client_area;
	shared_ptr<pixmap_t> _old_surface;

public:

	renderable_notebook_fading_t(shared_ptr<pixmap_t> old_surface, rect old_client_area) :
		_old_client_area{old_client_area},
		_old_surface{old_surface},
		_ratio{0.5}
	{

	}

	virtual ~renderable_notebook_fading_t() {

	}

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {

		cairo_save(cr);
		cairo_pattern_t * p0 =
				cairo_pattern_create_rgba(1.0, 1.0, 1.0, 1.0 - _ratio);

		for (auto & c : area) {
			cairo_reset_clip(cr);
			cairo_clip(cr, _old_client_area & c);
			cairo_set_source_surface(cr, _old_surface->get_cairo_surface(),
					_old_client_area.x, _old_client_area.y);
			cairo_mask(cr, p0);
		}

		cairo_pattern_destroy(p0);
		cairo_restore(cr);

	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return region{};
	}

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() {
		return region{_old_client_area};
	}

	virtual region get_damaged() {
		return region{_old_client_area};
	}

	void set_ratio(double x) {
		_ratio = std::min(std::max(0.0, x), 1.0);
	}

	virtual void set_parent(tree_t * t) {
		_parent = t;
	}

	virtual tree_t * parent() const {
		return _parent;
	}


};



}



#endif /* RENDERABLE_SURFACE_HXX_ */
