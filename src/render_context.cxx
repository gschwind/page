/*
 * render_context.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include <algorithm>
#include "render_context.hxx"

namespace page {

render_context_t::render_context_t(xconnection_t & cnx) :
		_cnx(cnx) {
	composite_overlay_s = cairo_xlib_surface_create(_cnx.dpy,
			_cnx.composite_overlay, _cnx.root_wa.visual, _cnx.root_wa.width,
			_cnx.root_wa.height);
	composite_overlay_cr = cairo_create(composite_overlay_s);

	back_buffer_s = cairo_surface_create_similar(composite_overlay_s,
			CAIRO_CONTENT_COLOR, cnx.root_wa.width, _cnx.root_wa.height);
	back_buffer_cr = cairo_create(back_buffer_s);

	pre_back_buffer_s = cairo_surface_create_similar(composite_overlay_s,
			CAIRO_CONTENT_COLOR, cnx.root_wa.width, _cnx.root_wa.height);
	pre_back_buffer_cr = cairo_create(pre_back_buffer_s);
}

void render_context_t::draw_box(box_int_t box, double r, double g, double b) {
	cairo_set_source_rgb(composite_overlay_cr, r, g, b);
	cairo_set_line_width(composite_overlay_cr, 1.0);
	cairo_rectangle(composite_overlay_cr, box.x + 0.5, box.y + 0.5,
			box.w - 1.0, box.h - 1.0);
	cairo_stroke(composite_overlay_cr);
	cairo_surface_flush(composite_overlay_s);
}

void render_context_t::add_damage_area(box_int_t const & box) {
	pending_damage += box;
}

void render_context_t::add_damage_overlay_area(box_int_t const & box) {
	pending_overlay_damage += box;
}


bool render_context_t::z_comp(renderable_t * x, renderable_t * y) {
	return x->z < y->z;
}

void render_context_t::render_flush() {
	list.sort(z_comp);

	region_t<int>::box_list_t area = pending_damage.get_area();
	/* update back buffer */
	{
		cairo_reset_clip(pre_back_buffer_cr);
		region_t<int>::box_list_t::const_iterator i = area.begin();
		while (i != area.end()) {
			repair_pre_back_buffer(*i);
			++i;
		}
		cairo_surface_flush(pre_back_buffer_s);
	}

	region_t<int>::box_list_t area_overlay = pending_overlay_damage.get_area();
	/* update back buffer */
	{
		cairo_reset_clip(back_buffer_cr);
		region_t<int>::box_list_t::const_iterator i = area_overlay.begin();
		while (i != area_overlay.end()) {
			repair_back_buffer(*i);
			++i;
		}
		i = area.begin();
		while (i != area.end()) {
			repair_back_buffer(*i);
			++i;
		}
		cairo_surface_flush(back_buffer_s);
	}

	/* update front buffer */
	{
		region_t<int>::box_list_t::const_iterator i = area.begin();
		while (i != area.end()) {
			repair_overlay(*i);
			++i;
		}
		i = area_overlay.begin();
		while (i != area_overlay.end()) {
			repair_overlay(*i);
			++i;
		}
		cairo_surface_flush(composite_overlay_s);
	}
	pending_damage.clear();
	pending_overlay_damage.clear();

}

void render_context_t::repair_pre_back_buffer(box_int_t const & area) {
	for (renderable_list_t::iterator i = list.begin(); i != list.end(); ++i) {
		if (!(*i)->is_visible())
			continue;
		renderable_t * r = *i;
		box_int_t clip = area & r->get_absolute_extend();
		if (clip.w > 0 && clip.h > 0) {
			r->repair1(pre_back_buffer_cr, clip);
		}
	}
}

void render_context_t::repair_back_buffer(box_int_t const & area) {

	cairo_reset_clip(back_buffer_cr);
	cairo_set_source_surface(back_buffer_cr, pre_back_buffer_s, 0.0, 0.0);
	cairo_set_operator(back_buffer_cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(back_buffer_cr, area.x, area.y, area.w, area.h);
	cairo_fill(back_buffer_cr);

	cairo_set_operator(back_buffer_cr, CAIRO_OPERATOR_OVER);
	for (renderable_list_t::iterator i = overlay_componant.begin(); i != overlay_componant.end(); ++i) {
		if (!(*i)->is_visible())
			continue;
		renderable_t * r = *i;
		box_int_t clip = area & r->get_absolute_extend();
		if (clip.w > 0 && clip.h > 0) {
			r->repair1(back_buffer_cr, clip);
		}
	}
}

void render_context_t::repair_overlay(box_int_t const & area) {

	cairo_reset_clip(composite_overlay_cr);
	cairo_set_operator(composite_overlay_cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(composite_overlay_cr, back_buffer_s, 0, 0);
	cairo_rectangle(composite_overlay_cr, area.x, area.y, area.w, area.h);
	cairo_fill(composite_overlay_cr);

	/* for debug purpose */
	if (false) {
		static int color = 0;
		switch (color % 3) {
		case 0:
			cairo_set_source_rgb(composite_overlay_cr, 1.0, 0.0, 0.0);
			break;
		case 1:
			cairo_set_source_rgb(composite_overlay_cr, 1.0, 1.0, 0.0);
			break;
		case 2:
			cairo_set_source_rgb(composite_overlay_cr, 1.0, 0.0, 1.0);
			break;
		}
		++color;
		cairo_set_line_width(composite_overlay_cr, 1.0);
		cairo_rectangle(composite_overlay_cr, area.x + 0.5, area.y + 0.5,
				area.w - 1.0, area.h - 1.0);
		cairo_stroke(composite_overlay_cr);
	}

	//cairo_surface_flush(composite_overlay_s);

}

void render_context_t::add(renderable_t * x) {
	list.push_back(x);
}

void render_context_t::remove(renderable_t * x) {
	list.remove(x);
}

void render_context_t::overlay_add(renderable_t * x) {
	overlay_componant.push_back(x);
}

void render_context_t::overlay_remove(renderable_t * x) {
	overlay_componant.remove(x);
}

}

