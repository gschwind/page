/*
 * compositor_overlay.hxx
 *
 *  Created on: 9 août 2015
 *      Author: gschwind
 */

#ifndef SRC_COMPOSITOR_OVERLAY_HXX_
#define SRC_COMPOSITOR_OVERLAY_HXX_

#include "config.hxx"

#ifdef WITH_PANGO
#include <pango/pango.h>
#include <pango/pangocairo.h>
#endif

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo-xcb.h>

#include "pixmap.hxx"

namespace page {


struct compositor_overlay_t : public tree_t {
	page_context_t * _ctx;

#ifdef WITH_PANGO
	PangoFontDescription * _fps_font_desc;
	PangoFontMap * _fps_font_map;
	PangoContext * _fps_context;
#endif

	shared_ptr<pixmap_t> _back_surf;
	rect _position;

	bool _has_damage;

public:

	compositor_overlay_t(page_context_t * ctx, rect const & pos) : _ctx{ctx}, _position{pos}, _has_damage{false} {

#ifdef WITH_PANGO
		_fps_font_desc = pango_font_description_from_string("Mono 11");
		_fps_font_map = pango_cairo_font_map_new();
		_fps_context = pango_font_map_create_context(_fps_font_map);
#endif

		_back_surf = _ctx->cmp()->create_composite_pixmap(_position.w, _position.h);

	}

	~compositor_overlay_t() {
#ifdef WITH_PANGO
		pango_font_description_free(_fps_font_desc);
		g_object_unref(_fps_context);
		g_object_unref(_fps_font_map);
#endif
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
		return region{_position};
	}

	/**
	 * return currently damaged area (absolute)
	 **/
	virtual region get_damaged()  {
		if(_has_damage)
			return region{_position};
		else
			return region{};
	}



	void show() {
		_is_visible = true;
	}

	void hide() {
		_is_visible = false;
	}

	void update_layout(time64_t const t) {
		_has_damage = false;

		auto child = _parent.lock()->get_all_children();
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

	void _update_back_buffer() {

		cairo_t * cr = cairo_create(_back_surf->get_cairo_surface());

		double fps = _ctx->cmp()->get_fps();
		deque<double> const & _damaged_area = _ctx->cmp()->get_damaged_area_history();
		deque<double> const & _direct_render_area = _ctx->cmp()->get_direct_area_history();
		std::size_t _FPS_WINDOWS = _direct_render_area.size();

		cairo_save(cr);
		cairo_identity_matrix(cr);

		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
		cairo_paint(cr);

		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

		cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
		cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
		cairo_new_path(cr);

		{
			int j = 0;
			auto i = _damaged_area.begin();
			cairo_move_to(cr, 0 * 5.0, 100.0 - std::min((*(i++))*100.0, 100.0));
			while(i != _damaged_area.end())
				cairo_line_to(cr, (j++) * 2.0, 100.0 - std::min((*(i++))*100.0, 100.0));
		}
		cairo_stroke(cr);

		cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
		{
			int j = 0;
			auto i = _direct_render_area.begin();
			cairo_move_to(cr, 0 * 5.0, 100.0 - std::min((*(i++))*100.0, 100.0));
			while(i != _direct_render_area.end())
				cairo_line_to(cr, (j++) * 2.0, 100.0 - std::min((*(i++))*100.0, 100.0));
		}
		cairo_stroke(cr);

		int surf_count;
		int surf_size;

		_ctx->csm()->make_surface_stats(surf_size, surf_count);

		pango_printf(cr, 80*2+20,0,  "fps:        %10.1f", fps);
		pango_printf(cr, 80*2+20,30, "s. count:   %8d", surf_count);
		pango_printf(cr, 80*2+20,50, "s. memory:  %8d KB", surf_size/1024);

		cairo_restore(cr);

		cairo_destroy(cr);
	}

	virtual void render(cairo_t * cr, region const & area) {
		cairo_save(cr);

		region r = _position;
		r &= area;
		for (auto &a : r) {
			cairo_clip(cr, a);
			cairo_set_source_surface(cr, _back_surf->get_cairo_surface(), _position.x, _position.y);
			cairo_mask_surface(cr, _back_surf->get_cairo_surface(), _position.x, _position.y);
		}
		cairo_restore(cr);
	}

	void pango_printf(cairo_t * cr, double x, double y,
			char const * fmt, ...) {

	#ifdef WITH_PANGO

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

		cairo_set_line_width(cr, 3.0);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

		cairo_stroke_preserve(cr);

		cairo_set_line_width(cr, 1.0);
		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		cairo_fill(cr);

		cairo_restore(cr);
	#endif

	}

};



}



#endif /* SRC_COMPOSITOR_OVERLAY_HXX_ */
