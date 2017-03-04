/*
 * Copyright (2017) Benoit Gschwind
 *
 * renderable_solid.cxx is part of page-compositor.
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

#include "renderable_solid.hxx"

namespace page {

renderable_solid_t::renderable_solid_t(tree_t * ref, color_t color, rect loc, region damaged) :
	tree_t{ref->_root},
	damaged{damaged},
	color{color},
	location{loc}
{
	opaque_region = region{loc};
	visible_region = region{loc};
}

renderable_solid_t::~renderable_solid_t() { }

/**
 * draw the area of a renderable to the destination surface
 * @param cr the destination surface context
 * @param area the area to redraw
 **/
void renderable_solid_t::render(cairo_t * cr, region const & area) {
	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a);
	region r = visible_region & area;
	for (auto &i : r.rects()) {
		cairo_clip(cr, i);
		cairo_fill(cr);
	}
	cairo_restore(cr);
}

/**
 * Derived class must return opaque region for this object,
 * If unknown it's safe to leave this empty.
 **/
region renderable_solid_t::get_opaque_region() {
	if(color.a > 0.0) {
		return region{};
	} else {
		return opaque_region;
	}
}

/**
 * Derived class must return visible region,
 * If unknow the whole screen can be returned, but draw will be called each time.
 **/
region renderable_solid_t::get_visible_region() {
	return visible_region;
}

region renderable_solid_t::get_damaged() {
	return damaged;
}

void renderable_solid_t::set_opaque_region(region const & r) {
	opaque_region = r;
}

void renderable_solid_t::set_visible_region(region const & r) {
	visible_region = r;
}

}

