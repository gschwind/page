/*
 * render_context.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include <algorithm>
#include "render_context.hxx"

namespace page {

render_context_t::render_context_t(xconnection_t * cnx) {
	_cnx = cnx;
	composite_overlay_s = cairo_xlib_surface_create(_cnx->dpy, _cnx->composite_overlay, _cnx->root_wa.visual, _cnx->root_wa.width, _cnx->root_wa.height);
	composite_overlay_cr = cairo_create(composite_overlay_s);

	/* clean up surface */
	cairo_set_operator(composite_overlay_cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(composite_overlay_cr, 0.0, 0.0, 0.0, 1.0);
	cairo_reset_clip(composite_overlay_cr);
	cairo_paint(composite_overlay_cr);

	pre_back_buffer_s = cairo_surface_create_similar(composite_overlay_s, CAIRO_CONTENT_COLOR_ALPHA, _cnx->root_wa.width, _cnx->root_wa.height);
	pre_back_buffer_cr = cairo_create(pre_back_buffer_s);

	/* clean up surface */
	cairo_set_operator(pre_back_buffer_cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(pre_back_buffer_cr, 0.0, 0.0, 0.0, 1.0);
	cairo_reset_clip(pre_back_buffer_cr);
	cairo_paint(pre_back_buffer_cr);

}

void render_context_t::draw_box(box_int_t box, double r, double g, double b) {
	cairo_set_source_rgb(composite_overlay_cr, r, g, b);
	cairo_set_line_width(composite_overlay_cr, 1.0);
	cairo_rectangle(composite_overlay_cr, box.x + 0.5, box.y + 0.5, box.w - 1.0, box.h - 1.0);
	cairo_stroke(composite_overlay_cr);
	cairo_surface_flush(composite_overlay_s);
}

void render_context_t::add_damage_area(region_t<int> const & box) {
	pending_damage = pending_damage + box;
}

void render_context_t::render_flush() {
	/* a small optimization, because visible are often few */
	renderable_list_t visible;
	for(renderable_list_t::iterator i = list.begin(); i != list.end(); ++i) {
		if((*i)->is_visible()) {
			visible.push_back((*i));
		}
	}

	/* fast region are region that can be rendered directly on front buffer */
	/* slow region are region that will be rendered in back buffer before the front */
	region_t<int> fast_region;

	/* This loops check area that have only one window over it, if this is
	 * the case we will be able to render it directly.
	 */
	renderable_list_t::iterator i = visible.begin();
	while (i != visible.end()) {
		region_t<int> r = (*i)->get_area();
		renderable_list_t::iterator j = visible.begin();
		while (j != visible.end()) {
			if (i != j) {
				r = r - (*j)->get_area();
			}
			++j;
		}
		fast_region = fast_region + r;
		++i;
	}

	fast_region = fast_region & pending_damage;
	region_t<int> slow_region = pending_damage - fast_region;

	/* direct render */
	{
		cairo_reset_clip(composite_overlay_cr);
		cairo_set_operator(composite_overlay_cr, CAIRO_OPERATOR_SOURCE);
		region_t<int>::box_list_t::const_iterator i = fast_region.begin();
		while (i != fast_region.end()) {
			repair_buffer(visible, composite_overlay_cr, *i);
			/* this section show direct rendered screen */
//			cairo_set_source_rgb(composite_overlay_cr, 0.0, 1.0, 0.0);
//			cairo_set_line_width(composite_overlay_cr, 1.0);
//			cairo_rectangle(composite_overlay_cr, (*i).x + 0.5, (*i).y + 0.5, (*i).w - 1.0, (*i).h - 1.0);
//			cairo_clip(composite_overlay_cr);
//			cairo_paint_with_alpha(composite_overlay_cr, 0.1);
//			cairo_reset_clip(composite_overlay_cr);
//			cairo_stroke(composite_overlay_cr);
			++i;
		}
	}

	/* update back buffer */
	{
		cairo_reset_clip(pre_back_buffer_cr);
		cairo_set_operator(pre_back_buffer_cr, CAIRO_OPERATOR_OVER);
		region_t<int>::box_list_t::const_iterator i = slow_region.begin();
		while (i != slow_region.end()) {
			repair_buffer(visible, pre_back_buffer_cr, *i);
			++i;
		}
	}

	{
		cairo_surface_flush(pre_back_buffer_s);
		region_t<int>::box_list_t::const_iterator i = slow_region.begin();
		while (i != slow_region.end()) {
			repair_overlay(*i, pre_back_buffer_s);
			++i;
		}
	}
	pending_damage.clear();
}


void render_context_t::repair_buffer(renderable_list_t & visible, cairo_t * cr,
		box_int_t const & area) {
	for (renderable_list_t::iterator i = visible.begin(); i != visible.end();
			++i) {
		renderable_t * r = *i;
		region_t<int>::box_list_t barea = r->get_area();
		for (region_t<int>::box_list_t::iterator j = barea.begin();
				j != barea.end(); ++j) {
			box_int_t clip = area & (*j);
			if (!clip.is_null()) {
				r->repair1(cr, clip);
			}
		}
	}
}


void render_context_t::repair_overlay(box_int_t const & area, cairo_surface_t * src) {

	cairo_reset_clip(composite_overlay_cr);
	cairo_identity_matrix(composite_overlay_cr);
	cairo_set_operator(composite_overlay_cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(composite_overlay_cr, src, 0, 0);
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
		//++color;
		cairo_set_line_width(composite_overlay_cr, 1.0);
		cairo_rectangle(composite_overlay_cr, area.x + 0.5, area.y + 0.5, area.w - 1.0, area.h - 1.0);
		cairo_clip(composite_overlay_cr);
		cairo_paint_with_alpha(composite_overlay_cr, 0.3);
		cairo_reset_clip(composite_overlay_cr);
		//cairo_stroke(composite_overlay_cr);
	}

	// Never flush composite_overlay_s because this surface is never a source surface.
	//cairo_surface_flush(composite_overlay_s);

}

void render_context_t::add(renderable_t * x) {
	list.push_back(x);
}

void render_context_t::remove(renderable_t * x) {
	list.remove(x);
}

void render_context_t::move_above(renderable_t * r, renderable_t * above) {
	list.remove(r);
	renderable_list_t::iterator i = std::find(list.begin(), list.end(), above);
	if(i != list.end()) {
		list.insert(++i, r);
	} else {
		list.push_back(r);
	}
}

void render_context_t::raise(renderable_t * r) {
	list.remove(r);
	list.push_back(r);
}

void render_context_t::lower(renderable_t * r) {
	list.remove(r);
	list.push_front(r);
}

renderable_list_t render_context_t::get_renderable_list() {
	return list;
}


void render_context_t::process_event(XEvent const & e) {

}

void render_context_t::damage_all() {
	add_damage_area(_cnx->root_size);
}

}

