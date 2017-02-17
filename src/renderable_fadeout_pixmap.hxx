/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_FADEOUT_PIXMAP_HXX_
#define RENDERABLE_FADEOUT_PIXMAP_HXX_

#include "page_context.hxx"
#include "tree.hxx"
#include "pixmap.hxx"
#include "renderable_pixmap.hxx"

#include <cmath>

namespace page {

using namespace std;

class renderable_fadeout_pixmap_t : public renderable_pixmap_t {
	time64_t _start_time;
	double _alpha;

public:

	renderable_fadeout_pixmap_t(page_context_t * ctx, shared_ptr<pixmap_t> s, int x, int y) :
		renderable_pixmap_t(ctx, s, x, y),
		_alpha{0.0}
	{
		_start_time = time64_t::now();
	}

	virtual ~renderable_fadeout_pixmap_t() { }

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
					cairo_paint_with_alpha(cr, _alpha);
				}
			}
		}
		cairo_restore(cr);
		_ctx->schedule_repaint();
	}

	virtual void update_layout(time64_t const time) {
		if (time < (_start_time + time64_t(1000000000L))) {
			double ratio = (static_cast<double>(time - _start_time) / static_cast<double const>(1000000000L));
			ratio = ratio*1.05 - 0.025;
			ratio = min(1.0, max(0.0, ratio));
			_alpha = 1.0 - std::exp(40.0 * std::log(ratio));
		} else {
			_ctx->add_global_damage(_location);
			_parent->remove(shared_from_this());
		}
		_damaged = _location;
	}

};



}

#endif /* RENDERABLE_FADEOUT_PIXMAP_HXX_ */
