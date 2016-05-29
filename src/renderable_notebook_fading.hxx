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
#include "mainloop.hxx"

namespace page {

using namespace std;

class renderable_notebook_fading_t : public tree_t {
	page_context_t * _ctx;

	double _ratio;
	rect _location;
	shared_ptr<pixmap_t> _surface;
	region _damaged;
	region _opaque_region;

	shared_ptr<timeout_t> force_render;

public:

	renderable_notebook_fading_t(page_context_t * ctx, shared_ptr<pixmap_t> surface, int x, int y) :
		_surface{surface},
		_ratio{1.0},
		_ctx{ctx}
	{
		_location = rect(x, y, surface->witdh(), surface->height());
		_opaque_region = region(0, 0, surface->witdh(), surface->height());
		_damaged = _location;

		/* while fading notebook exist, force_render */
		force_render = ctx->mainloop()->add_timeout(1000000000L/120L, []() -> bool { return true; });

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

		region r = region{_location} & area;
		for (auto & c : area.rects()) {
			cairo_reset_clip(cr);
			cairo_clip(cr, c);
			cairo_set_source_surface(cr, _surface->get_cairo_surface(),
					_location.x, _location.y);
			cairo_mask(cr, p0);
		}

		cairo_pattern_destroy(p0);
		cairo_restore(cr);

	}

	void set_ratio(double x) {
		_damaged += _location;
		_ratio = min(max(0.0, x), 1.0);
	}

	virtual region get_opaque_region() {
		return region{};
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
