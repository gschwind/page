/*
 * Copyright (2017) Benoit Gschwind
 *
 * renderable_fadeout_pixmap.cxx is part of page-compositor.
 *
 * page-compositor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * page-compositor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with page-compositor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "renderable_fadeout_pixmap.hxx"

#include "page.hxx"
#include "workspace.hxx"

namespace page {

renderable_fadeout_pixmap_t::renderable_fadeout_pixmap_t(tree_t * ref,
		shared_ptr<pixmap_t> s, int x, int y) :
	renderable_pixmap_t(ref, s, x, y),
	_alpha{0.0}
{
	_start_time = time64_t::now();
}

renderable_fadeout_pixmap_t::~renderable_fadeout_pixmap_t() { }

/**
 * draw the area of a renderable to the destination surface
 * @param cr the destination surface context
 * @param area the area to redraw
 **/
void renderable_fadeout_pixmap_t::render(cairo_t * cr, region const & area)
{
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

void renderable_fadeout_pixmap_t::update_layout(time64_t const time)
{
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

}


