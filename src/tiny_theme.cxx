/*
 * tiny_theme.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <cairo/cairo-xcb.h>

#include "config.hxx"
#include "renderable_solid.hxx"
#include "renderable_pixmap.hxx"
#include "tiny_theme.hxx"
#include "box.hxx"
#include "color.hxx"
#include <string>
#include <algorithm>
#include <iostream>

namespace page {

using namespace std;

tiny_theme_t::tiny_theme_t(display_t * cnx, config_handler_t & conf) :
	simple2_theme_t(cnx, conf)
{
	notebook.tab_height = 15;
	notebook.margin.top = 0;
	notebook.margin.bottom = 1;
	notebook.margin.left = 1;
	notebook.margin.right = 1;

	string conf_img_dir = conf.get_string("default", "theme_dir");

	{
	cairo_surface_destroy(pop_button_s);
	string filename = conf_img_dir + "/tiny_pop.png";
	printf("Load: %s\n", filename.c_str());
	pop_button_s = cairo_image_surface_create_from_png(filename.c_str());
	if (pop_button_s == nullptr)
		throw std::runtime_error("file not found!");
	}

	{
	cairo_surface_destroy(pops_button_s);
	string filename = conf_img_dir + "/tiny_pops.png";
	printf("Load: %s\n", filename.c_str());
	pops_button_s = cairo_image_surface_create_from_png(filename.c_str());
	if (pops_button_s == nullptr)
		throw std::runtime_error("file not found!");
	}

	{
	cairo_surface_destroy(vsplit_button_s);
	string filename = conf_img_dir + "/tiny_vsplit_button.png";
	printf("Load: %s\n", filename.c_str());
	vsplit_button_s = cairo_image_surface_create_from_png(filename.c_str());
	if (vsplit_button_s == nullptr)
		throw std::runtime_error("file not found!");
	}

	{
	cairo_surface_destroy(hsplit_button_s);
	string filename = conf_img_dir + "/tiny_hsplit_button.png";
	printf("Load: %s\n", filename.c_str());
	hsplit_button_s = cairo_image_surface_create_from_png(filename.c_str());
	if (hsplit_button_s == nullptr)
		throw std::runtime_error("file not found!");
	}

	{
	cairo_surface_destroy(close_button_s);
	string filename = conf_img_dir + "/window-close-3.png";
	printf("Load: %s\n", filename.c_str());
	close_button_s = cairo_image_surface_create_from_png(filename.c_str());
	if (close_button_s == nullptr)
		throw std::runtime_error("file not found!");
	}

}


tiny_theme_t::~tiny_theme_t() {

}

rect tiny_theme_t::compute_notebook_bookmark_position(
		rect const & allocation) const {
	return rect(
			allocation.x + allocation.w - 4 * 17 - 2,
			allocation.y + 1,
			16,
			notebook.tab_height - 1
		);
}

rect tiny_theme_t::compute_notebook_vsplit_position(
		rect const & allocation) const {

	return rect(
			allocation.x + allocation.w - 3 * 17 - 2,
			allocation.y + 1,
			16,
			notebook.tab_height - 1
		);
}

rect tiny_theme_t::compute_notebook_hsplit_position(
		rect const & allocation) const {

	return rect(
			allocation.x + allocation.w - 2 * 17 - 2,
			allocation.y + 1,
			16,
			notebook.tab_height - 1
		);

}

rect tiny_theme_t::compute_notebook_close_position(
		rect const & allocation) const {

	return rect(
			allocation.x + allocation.w - 1 * 17 - 2,
			allocation.y + 1,
			16,
			notebook.tab_height - 1
		);
}

rect tiny_theme_t::compute_notebook_menu_position(
		rect const & allocation) const {

	return rect(
			allocation.x,
			allocation.y,
			40,
			notebook.tab_height
		);

}

void tiny_theme_t::render_notebook(cairo_t * cr, theme_notebook_t const * n) const {

	cairo_save(cr);
	cairo_reset_clip(cr);

	cairo_set_line_width(cr, 1.0);
	cairo_new_path(cr);

	if(backgroun_px != nullptr) {
		rect tab_area{n->allocation};
		tab_area.h = notebook.tab_height;

		cairo_save(cr);
		cairo_clip(cr, tab_area);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_surface(cr, backgroun_px->get_cairo_surface(), -n->root_x, -n->root_y);
		cairo_paint(cr);
		cairo_restore(cr);

		rect body_area{n->allocation};
		body_area.y += notebook.tab_height;
		body_area.h -= notebook.tab_height;
		cairo_save(cr);
		cairo_clip(cr, body_area);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_surface(cr, backgroun_px->get_cairo_surface(), -n->root_x, -n->root_y);
		cairo_paint(cr);
		cairo_restore(cr);
	} else {
		rect tab_area{n->allocation};
		tab_area.h = notebook.tab_height;

		cairo_save(cr);
		cairo_clip(cr, tab_area);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_color(cr, default_background_color);
		cairo_paint(cr);
		cairo_restore(cr);

		rect body_area{n->allocation};
		body_area.y += notebook.tab_height;
		body_area.h -= notebook.tab_height;
		cairo_save(cr);
		cairo_clip(cr, body_area);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_color(cr, default_background_color);
		cairo_paint(cr);
		cairo_restore(cr);
	}

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	if (n->has_selected_client) {
		if (n->selected_client.is_iconic) {

			render_notebook_normal(cr, n->selected_client,
					notebook_normal_font,
					notebook_normal_text_color,
					notebook_normal_outline_color,
					notebook_normal_border_color,
					n->selected_client.tab_color);

		}

	}

	if (n->has_selected_client) {
		if (not n->selected_client.is_iconic) {

			/* clear selected tab background */
			rect a{n->selected_client.position};

			a.x+=2;
			a.w-=4;
			a.h += 1;

			cairo_save(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			if (backgroun_px != nullptr) {
				cairo_set_source_surface(cr, backgroun_px->get_cairo_surface(), -n->root_x, -n->root_y);
			} else {
				cairo_set_source_color(cr, default_background_color);
			}
			cairo_rectangle(cr, a.x, a.y, a.w, a.h);
			cairo_fill(cr);
			cairo_restore(cr);

			render_notebook_selected(cr, *n, n->selected_client,
					notebook_selected_font, notebook_selected_text_color,
					notebook_selected_outline_color,
					notebook_selected_border_color,
					n->selected_client.tab_color, 1.0);
		}
	}

	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);

	if(n->has_selected_client) {

		cairo_save(cr);
		rect btext = compute_notebook_menu_position(n->allocation);

		if(n->button_mouse_over == NOTEBOOK_BUTTON_EXPOSAY) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			cairo_rectangle(cr, btext.x, btext.y, btext.w, btext.h);
			cairo_fill(cr);
		}

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);

		char buf[32];
		snprintf(buf, 32, "%d", n->client_count);

		/* draw title */
		cairo_set_source_color_alpha(cr, notebook_normal_outline_color);

		cairo_translate(cr, btext.x, btext.y);

		cairo_set_line_width(cr, 3.0);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);

		{
			PangoLayout * pango_layout = pango_layout_new(pango_context);
			pango_layout_set_font_description(pango_layout,
					notebook_selected_font);
			pango_cairo_update_layout(cr, pango_layout);
			pango_layout_set_text(pango_layout, buf, -1);
			pango_layout_set_width(pango_layout, btext.w * PANGO_SCALE);
			pango_layout_set_wrap(pango_layout, PANGO_WRAP_CHAR);
			pango_layout_set_ellipsize(pango_layout, PANGO_ELLIPSIZE_END);
			pango_layout_set_alignment(pango_layout, PANGO_ALIGN_CENTER);
			pango_cairo_layout_path(cr, pango_layout);
			g_object_unref(pango_layout);
		}

		cairo_stroke_preserve(cr);

		cairo_set_line_width(cr, 1.0);
		cairo_set_source_color_alpha(cr, notebook_selected_text_color);
		cairo_fill(cr);

		cairo_restore(cr);

	}


	{
		rect b = compute_notebook_bookmark_position(n->allocation);

		cairo_save(cr);
		cairo_clip(cr, b);

		if(n->button_mouse_over == NOTEBOOK_BUTTON_MASK) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			cairo_paint(cr);
		}

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
		if (n->is_default) {
			cairo_set_source_surface(cr, pops_button_s, b.x, b.y);
			cairo_mask_surface(cr, pops_button_s, b.x, b.y);
		} else {
			cairo_set_source_surface(cr, pop_button_s, b.x, b.y);
			cairo_mask_surface(cr, pop_button_s, b.x, b.y);
		}

		cairo_restore(cr);
	}

	{
		rect b = compute_notebook_vsplit_position(n->allocation);

		cairo_save(cr);
		cairo_clip(cr, b);

		if(n->button_mouse_over == NOTEBOOK_BUTTON_VSPLIT) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			cairo_paint(cr);
		}

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
		cairo_set_source_surface(cr, vsplit_button_s, b.x, b.y);
		cairo_mask_surface(cr, vsplit_button_s, b.x, b.y);

		if(not n->can_vsplit) {
			cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.5);
			cairo_paint(cr);
		}

		cairo_restore(cr);
	}

	{
		rect b = compute_notebook_hsplit_position(n->allocation);

		cairo_save(cr);
		cairo_clip(cr, b);

		if(n->button_mouse_over == NOTEBOOK_BUTTON_HSPLIT) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			cairo_paint(cr);
		}

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
		cairo_set_source_surface(cr, hsplit_button_s, b.x, b.y);
		cairo_mask_surface(cr, hsplit_button_s, b.x, b.y);

		if(not n->can_hsplit) {
			cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.5);
			cairo_paint(cr);
		}

		cairo_restore(cr);
	}

	{
		rect b = compute_notebook_close_position(n->allocation);

		cairo_save(cr);
		cairo_clip(cr, b);

		if(n->button_mouse_over == NOTEBOOK_BUTTON_CLOSE) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			cairo_paint(cr);
		}

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
		cairo_set_source_surface(cr, close_button_s, b.x, b.y);
		cairo_mask_surface(cr, close_button_s, b.x, b.y);

		cairo_restore(cr);
	}

	if(n->has_scroll_arrow) {
		rect b = n->left_arrow_position;

		cairo_save(cr);
		cairo_clip(cr, b);

		if(n->button_mouse_over == NOTEBOOK_BUTTON_LEFT_SCROLL_ARROW) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			cairo_paint(cr);
		}

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
		cairo_set_source_surface(cr, left_scroll_arrow_button_s, b.x, b.y+3);
		cairo_mask_surface(cr, left_scroll_arrow_button_s, b.x, b.y+3);

		cairo_restore(cr);
	}

	if(n->has_scroll_arrow) {
		rect b = n->right_arrow_position;

		cairo_save(cr);
		cairo_clip(cr, b);

		if(n->button_mouse_over == NOTEBOOK_BUTTON_RIGHT_SCROLL_ARROW) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			cairo_paint(cr);
		}

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
		cairo_set_source_surface(cr, right_scroll_arrow_button_s, b.x, b.y+3);
		cairo_mask_surface(cr, right_scroll_arrow_button_s, b.x, b.y+3);

		cairo_restore(cr);
	}

	cairo_restore(cr);

}

void tiny_theme_t::render_notebook_selected(
		cairo_t * cr,
		theme_notebook_t const & n,
		theme_tab_t const & data,
		PangoFontDescription const * pango_font,
		color_t const & text_color,
		color_t const & outline_color,
		color_t const & border_color,
		color_t const & background_color,
		double border_width
) const {
	double const border_factor = 0.6;

	rect tab_area = data.position;

	tab_area.x += 2;
	tab_area.w -= 4;
	tab_area.y += 0;
	tab_area.h += 0;

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

	/** draw the tab background **/
	rect b = tab_area;

	cairo_save(cr);

	/** clip tabs **/
	cairo_rectangle_arc_corner(cr, b.x, b.y, b.w, b.h+30.0, 6.5, CAIRO_CORNER_TOP);
	cairo_clip(cr);

	if(has_background) {
		cairo_set_source_surface(cr, backgroun_px->get_cairo_surface(), -n.root_x, -n.root_y);
	} else {
		cairo_set_source_color(cr, default_background_color);
	}

	cairo_paint(cr);

	cairo_set_source_color_alpha(cr, background_color);
	cairo_pattern_t * gradient;
	gradient = cairo_pattern_create_linear(0.0, b.y-2.0, 0.0, b.y+b.h+20.0);
	cairo_pattern_add_color_stop_rgba(gradient, 0.0, 1.0, 1.0, 1.0, 1.0);
	cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 1.0, 1.0, 0.0);
	cairo_mask(cr, gradient);

	cairo_reset_clip(cr);

	/* draw black outline */
	cairo_rounded_tab3(cr, b.x+0.5, b.y+0.5, b.w-1.0, b.h+30.0, 5.5);
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgb(cr, data.tab_color.r*border_factor, data.tab_color.g*border_factor, data.tab_color.b*border_factor);
	cairo_stroke(cr);

	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

	cairo_reset_clip(cr);
	cairo_rectangle(cr, b.x, b.y+5.5, 1.0, 40.0);
	cairo_clip(cr);

	cairo_set_source_rgb(cr, data.tab_color.r*border_factor, data.tab_color.g*border_factor, data.tab_color.b*border_factor);
	cairo_mask(cr, gradient);

	cairo_reset_clip(cr);
	cairo_rectangle(cr, b.x+b.w-1.0, b.y+5.5, 1.0, 40.0);
	cairo_clip(cr);

	cairo_set_source_rgb(cr, data.tab_color.r*border_factor, data.tab_color.g*border_factor, data.tab_color.b*border_factor);
	cairo_mask(cr, gradient);


	cairo_pattern_destroy(gradient);

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

	/** clip tabs **/
	cairo_reset_clip(cr);
	cairo_rectangle_arc_corner(cr, b.x, b.y, b.w, b.h+30.0, 6.5, CAIRO_CORNER_ALL);
	cairo_clip(cr);

	rect xncclose;
	xncclose.x = b.x + b.w - notebook.selected_close_width;
	xncclose.y = b.y;
	xncclose.w = notebook.selected_close_width - 10;
	xncclose.h = b.h + 2;

	if(n.button_mouse_over == NOTEBOOK_BUTTON_CLIENT_CLOSE) {
		cairo_set_source_color(cr, color_t{0xcc, 0x44, 0x00} * 1.2);
	} else {
		cairo_set_source_color(cr, color_t{0xcc, 0x44, 0x00});
	}
	cairo_rectangle(cr, xncclose.x, xncclose.y, xncclose.w, xncclose.h);
	cairo_fill(cr);

	cairo_new_path(cr);
	cairo_move_to(cr, xncclose.x+0.5, xncclose.y+0.5);
	cairo_line_to(cr, xncclose.x+0.5, xncclose.y+0.5 + xncclose.h-1.0);
	cairo_line_to(cr, xncclose.x+xncclose.w+0.5, xncclose.y+0.5 + xncclose.h-1.0);
	cairo_line_to(cr, xncclose.x+xncclose.w+0.5, xncclose.y+0.5);
	cairo_set_source_rgb(cr, data.tab_color.r*border_factor, data.tab_color.g*border_factor, data.tab_color.b*border_factor);
	cairo_stroke(cr);

	cairo_restore(cr);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

	/** draw application icon **/
	rect bicon = tab_area;
	bicon.h = 16;
	bicon.w = 16;
	bicon.x += 10;
	bicon.y += 2;

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	if (data.icon != nullptr) {
		if (data.icon->get_cairo_surface() != 0) {
			cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
			cairo_set_source_surface(cr, data.icon->get_cairo_surface(),
					bicon.x, bicon.y);
			cairo_mask_surface(cr, data.icon->get_cairo_surface(),
					bicon.x, bicon.y);
		}
	}

	/** draw application title **/
	rect btext = tab_area;
	btext.h -= 0;
	btext.w -= 3 * 16 + 12 + 30;
	btext.x += 7 + 16 + 6;
	btext.y += 1;

	cairo_save(cr);

	/** draw title **/
	cairo_translate(cr, btext.x + 2, btext.y);

	{

		{
			cairo_new_path(cr);
			PangoLayout * pango_layout = pango_layout_new(pango_context);
			pango_layout_set_font_description(pango_layout, pango_font);
			pango_cairo_update_layout(cr, pango_layout);
			pango_layout_set_text(pango_layout, data.title.c_str(), -1);
			pango_layout_set_wrap(pango_layout, PANGO_WRAP_CHAR);
			pango_layout_set_ellipsize(pango_layout, PANGO_ELLIPSIZE_END);
			pango_layout_set_width(pango_layout, btext.w * PANGO_SCALE);
			pango_cairo_layout_path(cr, pango_layout);
			g_object_unref(pango_layout);
		}

		cairo_set_line_width(cr, 3.0);
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
		cairo_set_source_color_alpha(cr, outline_color);

		cairo_stroke_preserve(cr);

		cairo_set_line_width(cr, 1.0);
		cairo_set_source_color_alpha(cr, text_color);
		cairo_fill(cr);
	}

	cairo_restore(cr);

	/** draw close button **/

	rect ncclose;
	ncclose.x = tab_area.x + tab_area.w - (notebook.selected_close_width-10)/2 - 18;
	ncclose.y = tab_area.y;
	ncclose.w = 16;
	ncclose.h = 16;

	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
	cairo_set_source_surface(cr, close_button1_s, ncclose.x, ncclose.y);
	cairo_mask_surface(cr, close_button1_s, ncclose.x, ncclose.y);

	/** draw unbind button **/
	rect ncub;
	ncub.x = tab_area.x + tab_area.w - notebook.selected_unbind_width
			- notebook.selected_close_width;
	ncub.y = tab_area.y;
	ncub.w = 16;
	ncub.h = 16;

	if(n.button_mouse_over == NOTEBOOK_BUTTON_CLIENT_UNBIND) {
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.2);
		cairo_rectangle(cr, ncub.x, ncub.y, ncub.w, ncub.h);
		cairo_fill(cr);
	}

	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
	cairo_set_source_surface(cr, unbind_button_s, ncub.x, ncub.y);
	cairo_mask_surface(cr, unbind_button_s, ncub.x, ncub.y);

	rect area = data.position;
	area.y += notebook.margin.top - 4;
	area.h -= notebook.margin.top - 4;

}

void tiny_theme_t::render_iconic_notebook(
		cairo_t * cr,
		vector<theme_tab_t> const & tabs
) const {
	for (auto const & i : tabs) {
		render_notebook_normal(cr, i,
				notebook_normal_font,
				notebook_normal_text_color,
				notebook_normal_outline_color,
				notebook_normal_border_color,
				i.tab_color);
	}
}

void tiny_theme_t::render_notebook_normal(
		cairo_t * cr,
		theme_tab_t const & data,
		PangoFontDescription const * pango_font,
		color_t const & text_color,
		color_t const & outline_color,
		color_t const & border_color,
		color_t const & background_color
) const {
	double const border_factor = 0.6;

	rect tab_area = data.position;
	tab_area.x += 2;
	tab_area.w -= 4;
	tab_area.y += 2;
	tab_area.h -= 2;

	cairo_save(cr);

	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_line_width(cr, 1.0);

	/** draw the tab background **/
	rect b = tab_area;
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

	/** clip tabs **/
	cairo_rectangle_arc_corner(cr, b.x, b.y, b.w, b.h, 6.5, CAIRO_CORNER_TOP);
	cairo_clip(cr);

	cairo_set_source_color_alpha(cr, background_color);
	cairo_pattern_t * gradient;
	gradient = cairo_pattern_create_linear(0.0, b.y-2.0, 0.0, b.y+b.h+20.0);
	cairo_pattern_add_color_stop_rgba(gradient, 0.0, 1.0, 1.0, 1.0, 1.0);
	cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 1.0, 1.0, 0.0);
	cairo_mask(cr, gradient);
	cairo_pattern_destroy(gradient);

	cairo_reset_clip(cr);

	cairo_set_line_width(cr, 1.0);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);

	cairo_set_source_color(cr, data.tab_color*border_factor);
	cairo_rectangle_arc_corner(cr, b.x+0.5, b.y+0.5, b.w-1.0, b.h, 5.5, CAIRO_CORNER_TOP);
	cairo_stroke(cr);

	cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

	/* draw application icon */
	rect bicon = tab_area;
	bicon.h = 16;
	bicon.w = 16;
	bicon.x += 6;
	bicon.y += 2;

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	if (data.icon != nullptr) {
		if (data.icon->get_cairo_surface() != nullptr) {
			cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
			cairo_set_source_surface(cr, data.icon->get_cairo_surface(),
					bicon.x, bicon.y);
			cairo_mask_surface(cr, data.icon->get_cairo_surface(),
					bicon.x, bicon.y);
		}
	}
	cairo_restore(cr);

	cairo_new_path(cr);
	cairo_move_to(cr, data.position.x+0.5, data.position.y+data.position.h - 0.5);
	cairo_line_to(cr, data.position.x+data.position.w-0.5, data.position.y+data.position.h - 0.5);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_stroke(cr);

	cairo_restore(cr);

}


}

