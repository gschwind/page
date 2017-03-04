/*
 * Copyright (2017) Benoit Gschwind
 *
 * renderable_pixmap.cxx is part of page-compositor.
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

#include "renderable_pixmap.hxx"

#include "workspace.hxx"

namespace page {

renderable_pixmap_t::renderable_pixmap_t(tree_t * ref, shared_ptr<pixmap_t> s, int x, int y) :
	tree_t{ref->_root},
	_ctx{ref->_root->_ctx},
	_surf{s}
{
	_location = rect(x, y, s->witdh(), s->height());
	if(s->format() == PIXMAP_RGB) {
		_opaque_region = region(0, 0, s->witdh(), s->height());
	}
	_damaged = _location;
}

renderable_pixmap_t::~renderable_pixmap_t() { }

/**
 * draw the area of a renderable to the destination surface
 * @param cr the destination surface context
 * @param area the area to redraw
 **/
void renderable_pixmap_t::render(cairo_t * cr, region const & area) {
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

region renderable_pixmap_t::get_opaque_region() {
	region ret = _opaque_region;
	ret.translate(_location.x, _location.y);
	return ret;
}

region renderable_pixmap_t::get_visible_region() {
	return region{_location};
}

region renderable_pixmap_t::get_damaged() {
	return _damaged;
}

void renderable_pixmap_t::render_finished() {
	_damaged.clear();
}

void renderable_pixmap_t::set_opaque_region(region const & r) {
	_opaque_region = r;
}

void renderable_pixmap_t::move(int x, int y) {
	_damaged += _location;
	_location.x = x;
	_location.y = y;
	_damaged += _location;
}

}



