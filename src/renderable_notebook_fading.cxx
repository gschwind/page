/*
 * Copyright (2017) Benoit Gschwind
 *
 * renderable_notebook_fading.cxx is part of page-compositor.
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

#include "renderable_notebook_fading.hxx"
#include "workspace.hxx"

namespace page {

renderable_notebook_fading_t::renderable_notebook_fading_t(tree_t * ref, shared_ptr<pixmap_t> surface, int x, int y) :
	tree_t{ref->_root},
	_surface{surface},
	_ratio{1.0},
	_ctx{ref->_root->_ctx}
{
	_location = rect(x, y, surface->witdh(), surface->height());
	_opaque_region = region(0, 0, surface->witdh(), surface->height());
	_damaged = _location;
}

renderable_notebook_fading_t::~renderable_notebook_fading_t() {

}

/**
 * draw the area of a renderable to the destination surface
 * @param cr the destination surface context
 * @param area the area to redraw
 **/
void renderable_notebook_fading_t::render(cairo_t * cr, region const & area) {

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

void renderable_notebook_fading_t::set_ratio(double x) {
	_damaged += region{_location};
	_ratio = min(max(0.0, x), 1.0);
}

region renderable_notebook_fading_t::get_opaque_region() {
	return region{};
}

region renderable_notebook_fading_t::get_visible_region() {
	return region{_location};
}

region renderable_notebook_fading_t::get_damaged() {
	return _damaged;
}

void renderable_notebook_fading_t::render_finished() {
	_damaged.clear();
}

void renderable_notebook_fading_t::set_opaque_region(region const & r) {
	_opaque_region = r;
}

void renderable_notebook_fading_t::move(int x, int y) {
	_damaged += _location;
	_location.x = x;
	_location.y = y;
	_damaged += _location;
}

void renderable_notebook_fading_t::update_pixmap(shared_ptr<pixmap_t> surface, int x, int y) {
	_location = rect(x, y, surface->witdh(), surface->height());
	_surface = surface;
}

double renderable_notebook_fading_t::ratio() {
	return _ratio;
}

shared_ptr<pixmap_t> renderable_notebook_fading_t::surface() {
	return _surface;
}

}


