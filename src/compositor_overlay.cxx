/*
 * Copyright (2017) Benoit Gschwind
 *
 * compositor_overlay.cxx is part of page-compositor.
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

#include "compositor_overlay.hxx"

#include "page.hxx"
#include "pixmap.hxx"
#include "view.hxx"

namespace page {

compositor_overlay_t::compositor_overlay_t(tree_t * ref, rect const & pos) :
	tree_t{ref->_root},
	_ctx{ref->_root->_ctx},
	_position{pos},
	_has_damage{false},
	render_max{100000000}
{
	_fps_font_desc = pango_font_description_from_string("Mono 11");
	_fps_font_map = pango_cairo_font_map_new();
	_fps_context = pango_font_map_create_context(_fps_font_map);
	_back_surf = make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGBA, _position.w, _position.h);

	render_times.push_back(0);

}

compositor_overlay_t::~compositor_overlay_t() {
	pango_font_description_free(_fps_font_desc);
	g_object_unref(_fps_context);
	g_object_unref(_fps_font_map);
}

/**
 * Derived class must return opaque region for this object,
 * If unknown it's safe to leave this empty.
 **/
region compositor_overlay_t::get_opaque_region() {
	return region{};
}

/**
 * Derived class must return visible region,
 * If unknow the whole screen can be returned, but draw will be called each time.
 **/
region compositor_overlay_t::get_visible_region() {
	return region{_position};
}

/**
 * return currently damaged area (absolute)
 **/
region compositor_overlay_t::get_damaged()  {
	if(_has_damage)
		return region{_position};
	else
		return region{};
}



void compositor_overlay_t::show() {
	_is_visible = true;
}

void compositor_overlay_t::hide() {
	_is_visible = false;
}

void compositor_overlay_t::update_layout(time64_t const t) {
	_has_damage = false;

	frame_start = t;

	auto child = _ctx->get_current_workspace()->gather_children_root_first<view_t>();
	for(auto & c: child) {
		if(not c->is_visible())
			continue;
		if(not c->get_damaged().empty()) {
			_has_damage = true;
			break;
		}
	}

	if(_has_damage)
		_update_back_buffer();
}

void compositor_overlay_t::_update_back_buffer() {

	cairo_t * cr = cairo_create(_back_surf->get_cairo_surface());

	double fps = _ctx->cmp()->get_fps();
	deque<double> const & _damaged_area = _ctx->cmp()->get_damaged_area_history();
	deque<double> const & _direct_render_area = _ctx->cmp()->get_direct_area_history();
	std::size_t _FPS_WINDOWS = _direct_render_area.size();

	cairo_identity_matrix(cr);

	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.7);
	cairo_paint(cr);

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

	cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
	cairo_new_path(cr);

	{
		int j = 0;
		auto i = _damaged_area.begin();
		cairo_move_to(cr, 0 * 5.0, 95.0 - std::min((*(i++))*90.0, 90.0));
		while(i != _damaged_area.end())
			cairo_line_to(cr, (j++) * 2.0, 95.0 - std::min((*(i++))*90.0, 90.0));
	}
	cairo_stroke(cr);

	cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
	{
		int j = 0;
		auto i = _direct_render_area.begin();
		cairo_move_to(cr, 0 * 5.0, 95.0 - std::min((*(i++))*90.0, 90.0));
		while(i != _direct_render_area.end())
			cairo_line_to(cr, (j++) * 2.0, 95.0 - std::min((*(i++))*90.0, 90.0));
	}
	cairo_stroke(cr);

	cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
	{
		render_max = 1000L;
		for (auto v: render_times) {
			if (render_max < v)
				render_max = v;
		}

		int j = 0;
		auto i = render_times.begin();
		cairo_move_to(cr, 0 * 5.0, 95.0 - std::min(static_cast<double>(*(i++))*90.0/render_max, 90.0));
		while(i != render_times.end())
			cairo_line_to(cr, (j++) * 2.0, 95.0 - std::min(static_cast<double>(*(i++))*90.0/render_max, 90.0));
	}
	cairo_stroke(cr);


	int surf_count;
	int surf_size;

	_ctx->make_surface_stats(surf_size, surf_count);

	pango_printf(cr, 80*2+20,0,  "version: %s", VERSION);
	pango_printf(cr, 80*2+20,30,  "fps:       %8.1f", fps);
	pango_printf(cr, 80*2+20,50, "s. count:  %6d", surf_count);
	pango_printf(cr, 80*2+20,80, "s. memory: %6d KB", surf_size/1024);

	pango_printf(cr, 0, 0, "render: %d", render_max);

	cairo_destroy(cr);
}

void compositor_overlay_t::render(cairo_t * cr, region const & area) {
	cairo_save(cr);

	region r = _position;
	r &= area;
	for (auto &a : r.rects()) {
		cairo_clip(cr, a);
		cairo_set_source_surface(cr, _back_surf->get_cairo_surface(), _position.x, _position.y);
		cairo_mask_surface(cr, _back_surf->get_cairo_surface(), _position.x, _position.y);
	}
	cairo_restore(cr);
}

void compositor_overlay_t::render_finished()
{
	frame_end = time64_t::now();

	if(render_times.size() > 80)
		render_times.pop_back();
	render_times.push_front((frame_end - frame_start).microseconds());

}

void compositor_overlay_t::pango_printf(cairo_t * cr, double x, double y,
		char const * fmt, ...) {

	va_list l;
	va_start(l, fmt);

	cairo_save(cr);
	cairo_move_to(cr, x, y);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

	static char buffer[4096];
	vsnprintf(buffer, 4096, fmt, l);

	PangoLayout * pango_layout = pango_layout_new(_fps_context);
	pango_layout_set_font_description(pango_layout, _fps_font_desc);
	pango_cairo_update_layout(cr, pango_layout);
	pango_layout_set_text(pango_layout, buffer, -1);
	pango_cairo_layout_path(cr, pango_layout);
	g_object_unref(pango_layout);

	cairo_set_line_width(cr, 5.0);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

	cairo_stroke_preserve(cr);

	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_fill(cr);

	cairo_restore(cr);
}

}


