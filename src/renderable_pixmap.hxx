/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_PIXMAP_HXX_
#define RENDERABLE_PIXMAP_HXX_

#include "page_context.hxx"
#include "tree.hxx"
#include "pixmap.hxx"

namespace page {

using namespace std;

class renderable_pixmap_t : public tree_t {
	page_context_t * _ctx;

	rect _location;
	shared_ptr<pixmap_t> _surf;
	region _damaged;
	region _opaque_region;

public:

	renderable_pixmap_t(page_context_t * ctx, shared_ptr<pixmap_t> s, int x, int y) :
		_ctx{ctx},
		_surf{s}
	{
		_location = rect(x, y, s->witdh(), s->height());
		_opaque_region = region(0, 0, s->witdh(), s->height());
		_damaged = _location;
	}

	virtual ~renderable_pixmap_t() { }

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {
		cairo_save(cr);
		if (_surf != nullptr) {
			if (_surf->get_cairo_surface() != nullptr) {
				cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
				cairo_set_source_surface(cr, _surf->get_cairo_surface(),
						_location.x, _location.y);
				region r = region{_location} & area;
				for (auto &i : r.rects()) {
					cairo_clip(cr, i);
					cairo_mask_surface(cr, _surf->get_cairo_surface(), _location.x, _location.y);
				}
			}
		}
		cairo_restore(cr);
	}

	virtual region get_opaque_region() {
		region ret = _opaque_region;
		ret.translate(_location.x, _location.y);
		return ret;
	}

	virtual region get_visible_region() {
		return region{_location};
	}

	virtual region get_damaged() {
		return _damaged;
	}

	virtual void render_finished() {
		_damaged.clear();
	}

	void set_opaque_region(region const & r) {
		_opaque_region = r;
	}

	void move(int x, int y) {
		_damaged += _location;
		_location.x = x;
		_location.y = y;
		_damaged += _location;
	}

};



}



#endif /* RENDERABLE_SURFACE_HXX_ */
