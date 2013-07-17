/*
 * render_context.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include <algorithm>
#include "compositor.hxx"

namespace page {

static void _draw_crossed_box(cairo_t * cr, box_int_t const & box, double r, double g,
		double b) {
	cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
	cairo_set_line_width(cr, 1.0);
	cairo_rectangle(cr, box.x + 0.5, box.y + 0.5, box.w - 1.0, box.h - 1.0);
	cairo_reset_clip(cr);
	cairo_stroke(cr);

	cairo_new_path(cr);
	cairo_move_to(cr, box.x + 0.5, box.y + 0.5);
	cairo_line_to(cr, box.x + box.w - 1.0, box.y + box.h - 1.0);

	cairo_move_to(cr, box.x + box.w - 1.0, box.y + 0.5);
	cairo_line_to(cr, box.x + 0.5, box.y + box.h - 1.0);
	cairo_stroke(cr);
}

compositor_t::compositor_t(xconnection_t * cnx) {
	_cnx = cnx;

	fast_region_surf = 0.0;
	slow_region_surf = 0.0;

	flush_count = 0;
	clock_gettime(CLOCK_MONOTONIC, &last_tic);

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

void compositor_t::draw_box(box_int_t box, double r, double g, double b) {
	cairo_set_source_rgb(composite_overlay_cr, r, g, b);
	cairo_set_line_width(composite_overlay_cr, 1.0);
	cairo_rectangle(composite_overlay_cr, box.x + 0.5, box.y + 0.5, box.w - 1.0, box.h - 1.0);
	cairo_stroke(composite_overlay_cr);
	cairo_surface_flush(composite_overlay_s);
}

void compositor_t::add_damage_area(region_t<int> const & box) {
	pending_damage = pending_damage + box;
}

void compositor_t::render_flush() {
	flush_count += 1;

	clock_gettime(CLOCK_MONOTONIC, &curr_tic);
	if(last_tic.tv_sec + 30 < curr_tic.tv_sec) {

		double t0 = last_tic.tv_sec + last_tic.tv_nsec / 1.0e9;
		double t1 = curr_tic.tv_sec + curr_tic.tv_nsec / 1.0e9;

		printf("render flush per second : %0.2f\n", flush_count / (t1 - t0));

		last_tic = curr_tic;
		flush_count = 0;
	}

	/* list content is bottom window to upper window in stack */
	/* a small optimization, because visible are often few */
	renderable_list_t visible;
	for(renderable_list_t::iterator i = list.begin(); i != list.end(); ++i) {
		if((*i)->is_visible()) {
			visible.push_back((*i));
		}
	}


	/**
	 * Find region where windows are not overlap each other. i.e. region where
	 * only one window will be rendered.
	 * This is often more than 80% of the screen.
	 * This kind of region will be directly rendered.
	 **/
	region_t<int> region_with_not_overlapped_window;

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
		region_with_not_overlapped_window = region_with_not_overlapped_window + r;
		++i;
	}

	/**
	 * Find region where the windows on top is not a window with alpha.
	 * Small area, often dropdown menu.
	 * This kind of area will be directly rendered.
	 **/
	region_t<int> region_with_not_alpha_on_top;

	/**
	 * Walk over all all window from bottom to top one. If window has alpha
	 * channel remove it from region with no alpha, if window do not have alpha
	 * add this window to the area.
	 **/
	i = visible.begin();
	while (i != visible.end()) {
		region_t<int> r = (*i)->get_area();
		if((*i)->has_alpha()) {
			region_with_not_alpha_on_top = region_with_not_alpha_on_top - r;
		} else {
			/* if not has_alpha, add this area */
			region_with_not_alpha_on_top = region_with_not_alpha_on_top + r;
		}
		++i;
	}

	region_t<int> direct_region = region_with_not_overlapped_window;

	direct_region = direct_region & pending_damage;
	region_t<int> slow_region = (pending_damage - direct_region) -
			region_with_not_alpha_on_top;

	/* direct render, area where there is only one window visible */
	{
		cairo_reset_clip(composite_overlay_cr);
		cairo_set_operator(composite_overlay_cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_antialias(composite_overlay_cr, CAIRO_ANTIALIAS_NONE);

		region_t<int>::box_list_t::const_iterator i = direct_region.begin();
		while (i != direct_region.end()) {
			fast_region_surf += (*i).w * (*i).h;
			repair_buffer(visible, composite_overlay_cr, *i);
			// for debuging
			//_draw_crossed_box(composite_overlay_cr, (*i), 0.0, 1.0, 0.0);
			++i;
		}
	}

	/* directly render area where window on the has no alpha */

	region_with_not_alpha_on_top = (region_with_not_alpha_on_top & pending_damage) - direct_region;

	{
		cairo_reset_clip(composite_overlay_cr);
		cairo_set_operator(composite_overlay_cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_antialias(composite_overlay_cr, CAIRO_ANTIALIAS_NONE);

		/* from top to bottom */
		for (renderable_list_t::reverse_iterator i = visible.rbegin(); i != visible.rend();
				++i) {
			renderable_t * r = *i;
			region_t<int> draw_area = region_with_not_alpha_on_top & r->get_area();
			if (!draw_area.empty()) {
				for (region_t<int>::box_list_t::iterator j = draw_area.begin();
						j != draw_area.end(); ++j) {
					if (!(*j).is_null()) {
						r->repair1(composite_overlay_cr, (*j));

						/* this section show direct rendered screen */
						//_draw_crossed_box(composite_overlay_cr, (*j), 0.0, 0.0, 1.0);
					}
				}
			}
			region_with_not_alpha_on_top = region_with_not_alpha_on_top - r->get_area();
		}
	}

	/**
	 * To avoid glitch (blinking) I use back buffer to make the composition.
	 * update back buffer, render area with possible transparency
	 **/
	{
		cairo_reset_clip(pre_back_buffer_cr);
		cairo_set_operator(pre_back_buffer_cr, CAIRO_OPERATOR_OVER);
		region_t<int>::box_list_t::const_iterator i = slow_region.begin();
		while (i != slow_region.end()) {
			slow_region_surf += (*i).w * (*i).h;
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


void compositor_t::repair_buffer(renderable_list_t & visible, cairo_t * cr,
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


void compositor_t::repair_overlay(box_int_t const & area, cairo_surface_t * src) {

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

void compositor_t::add(renderable_t * x) {
	list.push_back(x);
}

void compositor_t::remove(renderable_t * x) {
	list.remove(x);
}

void compositor_t::move_above(renderable_t * r, renderable_t * above) {
	list.remove(r);
	renderable_list_t::iterator i = std::find(list.begin(), list.end(), above);
	if(i != list.end()) {
		list.insert(++i, r);
	} else {
		list.push_back(r);
	}
}

void compositor_t::raise(renderable_t * r) {
	list.remove(r);
	list.push_back(r);
}

void compositor_t::lower(renderable_t * r) {
	list.remove(r);
	list.push_front(r);
}

renderable_list_t compositor_t::get_renderable_list() {
	return list;
}


void compositor_t::process_event(XEvent const & e) {

}

void compositor_t::damage_all() {
	add_damage_area(_cnx->root_size);
}

}

