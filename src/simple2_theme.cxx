/*
 * simple_theme.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <cairo/cairo-xcb.h>
#include <string>
#include <algorithm>
#include <iostream>

#include "config.hxx"
#include "renderable_solid.hxx"
#include "renderable_pixmap.hxx"
#include "simple2_theme.hxx"
#include "box.hxx"
#include "color.hxx"
#include "pixmap.hxx"

#include "blur_image_surface.hxx"

namespace page {

using namespace std;

inline void print_cairo_status(cairo_t * cr, char const * file, int line) {
	cairo_status_t s = cairo_status(cr);
	if (s != CAIRO_STATUS_SUCCESS) {
		printf("Cairo status %s:%d = %s\n", file, line,
				cairo_status_to_string(s));
		//abort();
	}
}




#define CHECK_CAIRO(x) do { \
	x;\
	print_cairo_status(cr, __FILE__, __LINE__); \
} while(false)

rect simple2_theme_t::compute_notebook_bookmark_position(
		rect const & allocation) const {
	return rect(allocation.x + allocation.w - 4 * 17 - 2, allocation.y + 2,
			16, 16).floor();
}

rect simple2_theme_t::compute_notebook_vsplit_position(
		rect const & allocation) const {

	return rect(allocation.x + allocation.w - 3 * 17 - 2, allocation.y + 2,
			16, 16).floor();
}

rect simple2_theme_t::compute_notebook_hsplit_position(
		rect const & allocation) const {

	return rect(allocation.x + allocation.w - 2 * 17 - 2, allocation.y + 2,
			16, 16).floor();

}

rect simple2_theme_t::compute_notebook_close_position(
		rect const & allocation) const {

	return rect(allocation.x + allocation.w - 1 * 17 - 2, allocation.y + 2,
			16, 16).floor();
}

rect simple2_theme_t::compute_notebook_menu_position(
		rect const & allocation) const {

	return rect(allocation.x, allocation.y,
			40, 20).floor();

}


simple2_theme_t::simple2_theme_t(display_t * cnx, config_handler_t & conf) {

	notebook.margin.top = 4;
	notebook.margin.bottom = 4;
	notebook.margin.left = 4;
	notebook.margin.right = 4;

	notebook.tab_height = 22;
	notebook.iconic_tab_width = 33;
	notebook.selected_close_width = 48;
	notebook.selected_unbind_width = 20;

	notebook.menu_button_width = 40;
	notebook.close_width = 17;
	notebook.hsplit_width = 17;
	notebook.vsplit_width = 17;
	notebook.mark_width = 28;

	notebook.left_scroll_arrow_width = 16;
	notebook.right_scroll_arrow_width = 16;

	split.margin.top = 0;
	split.margin.bottom = 0;
	split.margin.left = 0;
	split.margin.right = 0;

	floating.margin.top = 6;
	floating.margin.bottom = 6;
	floating.margin.left = 6;
	floating.margin.right = 6;
	floating.title_height = 22;
	floating.close_width = 48;
	floating.bind_width = 20;

	split.margin.top = 0;
	split.margin.bottom = 0;
	split.margin.left = 0;
	split.margin.right = 0;
	split.width = 10;

	_cnx = cnx;

	std::string conf_img_dir = conf.get_string("default", "theme_dir");

	default_background_color.set(
			conf.get_string("simple_theme", "default_background_color"));

	popup_text_color.set(conf.get_string("simple_theme", "popup_text_color"));
	popup_outline_color.set(
			conf.get_string("simple_theme", "popup_outline_color"));
	popup_background_color.set(
			conf.get_string("simple_theme", "popup_background_color"));

	grip_color.set(conf.get_string("simple_theme", "grip_color"));

	notebook_mouse_over_background_color.set(conf.get_string("simple_theme", "notebook_mouse_over_background_color"));

	notebook_active_text_color.set(
			conf.get_string("simple_theme", "notebook_active_text_color"));
	notebook_active_outline_color.set(
			conf.get_string("simple_theme", "notebook_active_outline_color"));
	notebook_active_border_color.set(
			conf.get_string("simple_theme", "notebook_active_border_color"));
	notebook_active_background_color.set(
			conf.get_string("simple_theme",
					"notebook_active_background_color"));

	notebook_selected_text_color.set(
			conf.get_string("simple_theme", "notebook_selected_text_color"));
	notebook_selected_outline_color.set(
			conf.get_string("simple_theme", "notebook_selected_outline_color"));
	notebook_selected_border_color.set(
			conf.get_string("simple_theme", "notebook_selected_border_color"));
	notebook_selected_background_color.set(
			conf.get_string("simple_theme",
					"notebook_selected_background_color"));

	notebook_attention_text_color.set(
			conf.get_string("simple_theme", "notebook_attention_text_color"));
	notebook_attention_outline_color.set(
			conf.get_string("simple_theme", "notebook_attention_outline_color"));
	notebook_attention_border_color.set(
			conf.get_string("simple_theme", "notebook_attention_border_color"));
	notebook_attention_background_color.set(
			conf.get_string("simple_theme",
					"notebook_attention_background_color"));

	notebook_normal_text_color.set(
			conf.get_string("simple_theme", "notebook_normal_text_color"));
	notebook_normal_outline_color.set(
			conf.get_string("simple_theme", "notebook_normal_outline_color"));
	notebook_normal_border_color.set(
			conf.get_string("simple_theme", "notebook_normal_border_color"));
	notebook_normal_background_color.set(
			conf.get_string("simple_theme",
					"notebook_normal_background_color"));

	floating_active_text_color.set(
			conf.get_string("simple_theme", "floating_active_text_color"));
	floating_active_outline_color.set(
			conf.get_string("simple_theme", "floating_active_outline_color"));
	floating_active_border_color.set(
			conf.get_string("simple_theme", "floating_active_border_color"));
	floating_active_background_color.set(
			conf.get_string("simple_theme",
					"floating_active_background_color"));

	floating_attention_text_color.set(
			conf.get_string("simple_theme", "floating_attention_text_color"));
	floating_attention_outline_color.set(
			conf.get_string("simple_theme", "floating_attention_outline_color"));
	floating_attention_border_color.set(
			conf.get_string("simple_theme", "floating_attention_border_color"));
	floating_attention_background_color.set(
			conf.get_string("simple_theme",
					"floating_attention_background_color"));

	floating_normal_text_color.set(
			conf.get_string("simple_theme", "floating_normal_text_color"));
	floating_normal_outline_color.set(
			conf.get_string("simple_theme", "floating_normal_outline_color"));
	floating_normal_border_color.set(
			conf.get_string("simple_theme", "floating_normal_border_color"));
	floating_normal_background_color.set(
			conf.get_string("simple_theme",
					"floating_normal_background_color"));

	notebook.margin.top =
			conf.get_long("simple_theme",
					"notebook_margin_top");
	notebook.margin.bottom =
			conf.get_long("simple_theme",
					"notebook_margin_bottom");
	notebook.margin.left =
			conf.get_long("simple_theme",
					"notebook_margin_left");
	notebook.margin.right =
			conf.get_long("simple_theme",
					"notebook_margin_right");

	vsplit_button_s = nullptr;
	hsplit_button_s = nullptr;
	close_button_s = nullptr;
	close_button1_s = nullptr;
	pop_button_s = nullptr;
	pops_button_s = nullptr;
	unbind_button_s = nullptr;
	bind_button_s = nullptr;
	left_scroll_arrow_button_s = nullptr;
	right_scroll_arrow_button_s = nullptr;

	has_background = conf.has_key("simple_theme", "background_png");
	if(has_background)
		background_file = conf.get_string("simple_theme", "background_png");
	scale_mode = "center";

	if(conf.has_key("simple_theme", "scale_mode"))
		scale_mode = conf.get_string("simple_theme", "scale_mode");

	create_background_img();

	/* open icons */
	if (hsplit_button_s == nullptr) {
		std::string filename = conf_img_dir + "/hsplit_button.png";
		printf("Load: %s\n", filename.c_str());
		hsplit_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (hsplit_button_s == nullptr)
			throw std::runtime_error("file not found!");
	}

	if (vsplit_button_s == nullptr) {
		std::string filename = conf_img_dir + "/vsplit_button.png";
		printf("Load: %s\n", filename.c_str());
		vsplit_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (vsplit_button_s == nullptr)
			throw std::runtime_error("file not found!");
	}

	if (close_button_s == nullptr) {
		std::string filename = conf_img_dir + "/window-close-4.png";
		printf("Load: %s\n", filename.c_str());
		close_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (close_button_s == nullptr)
			throw std::runtime_error("file not found!");
	}

	if (close_button1_s == nullptr) {
		std::string filename = conf_img_dir + "/window-close-2.png";
		printf("Load: %s\n", filename.c_str());
		close_button1_s = cairo_image_surface_create_from_png(filename.c_str());
		if (close_button1_s == nullptr)
			throw std::runtime_error("file not found!");
	}

	if (pop_button_s == nullptr) {
		std::string filename = conf_img_dir + "/pop.png";
		printf("Load: %s\n", filename.c_str());
		pop_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (pop_button_s == nullptr)
			throw std::runtime_error("file not found!");
	}

	if (pops_button_s == nullptr) {
		std::string filename = conf_img_dir + "/pops.png";
		printf("Load: %s\n", filename.c_str());
		pops_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (pop_button_s == nullptr)
			throw std::runtime_error("file not found!");
	}

	if (unbind_button_s == nullptr) {
		std::string filename = conf_img_dir + "/media-eject.png";
		printf("Load: %s\n", filename.c_str());
		unbind_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (unbind_button_s == nullptr)
			throw std::runtime_error("file not found!");
	}

	if (bind_button_s == nullptr) {
		std::string filename = conf_img_dir + "/view-restore.png";
		printf("Load: %s\n", filename.c_str());
		bind_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (bind_button_s == nullptr)
			throw std::runtime_error("file not found!");
	}

	if (left_scroll_arrow_button_s == nullptr) {
		std::string filename = conf_img_dir + "/go-previous.png";
		printf("Load: %s\n", filename.c_str());
		left_scroll_arrow_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (left_scroll_arrow_button_s == nullptr)
			throw std::runtime_error("file not found!");
	}

	if (right_scroll_arrow_button_s == nullptr) {
		std::string filename = conf_img_dir + "/go-next.png";
		printf("Load: %s\n", filename.c_str());
		right_scroll_arrow_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (right_scroll_arrow_button_s == nullptr)
			throw std::runtime_error("file not found!");
	}

	notebook_active_font_name = conf.get_string("simple_theme", "notebook_active_font").c_str();
	notebook_selected_font_name = conf.get_string("simple_theme", "notebook_selected_font").c_str();
	notebook_attention_font_name = conf.get_string("simple_theme", "notebook_attention_font").c_str();
	notebook_normal_font_name = conf.get_string("simple_theme", "notebook_normal_font").c_str();

	floating_active_font_name = conf.get_string("simple_theme", "floating_active_font").c_str();
	floating_attention_font_name = conf.get_string("simple_theme", "floating_attention_font").c_str();
	floating_normal_font_name = conf.get_string("simple_theme", "floating_normal_font").c_str();

	pango_popup_font_name = conf.get_string("simple_theme", "pango_popup_font").c_str();

	notebook_active_font = pango_font_description_from_string(notebook_active_font_name.c_str());
	notebook_selected_font = pango_font_description_from_string(notebook_selected_font_name.c_str());
	notebook_attention_font = pango_font_description_from_string(notebook_attention_font_name.c_str());
	notebook_normal_font = pango_font_description_from_string(notebook_normal_font_name.c_str());

	floating_active_font = pango_font_description_from_string(floating_active_font_name.c_str());
	floating_attention_font = pango_font_description_from_string(floating_attention_font_name.c_str());
	floating_normal_font = pango_font_description_from_string(floating_normal_font_name.c_str());

	pango_popup_font = pango_font_description_from_string(pango_popup_font_name.c_str());

	pango_font_map = pango_cairo_font_map_new();
	pango_context = pango_font_map_create_context(pango_font_map);


}

simple2_theme_t::~simple2_theme_t() {

	warn(cairo_surface_get_reference_count(hsplit_button_s) == 1);
	cairo_surface_destroy(hsplit_button_s);
	warn(cairo_surface_get_reference_count(vsplit_button_s) == 1);
	cairo_surface_destroy(vsplit_button_s);
	warn(cairo_surface_get_reference_count(close_button_s) == 1);
	cairo_surface_destroy(close_button_s);
	warn(cairo_surface_get_reference_count(pop_button_s) == 1);
	cairo_surface_destroy(pop_button_s);
	warn(cairo_surface_get_reference_count(pops_button_s) == 1);
	cairo_surface_destroy(pops_button_s);
	warn(cairo_surface_get_reference_count(unbind_button_s) == 1);
	cairo_surface_destroy(unbind_button_s);
	warn(cairo_surface_get_reference_count(bind_button_s) == 1);
	cairo_surface_destroy(bind_button_s);
	warn(cairo_surface_get_reference_count(left_scroll_arrow_button_s) == 1);
	cairo_surface_destroy(left_scroll_arrow_button_s);
	warn(cairo_surface_get_reference_count(right_scroll_arrow_button_s) == 1);
	cairo_surface_destroy(right_scroll_arrow_button_s);

	pango_font_description_free(notebook_active_font);
	pango_font_description_free(notebook_selected_font);
	pango_font_description_free(notebook_attention_font);
	pango_font_description_free(notebook_normal_font);
	pango_font_description_free(floating_active_font);
	pango_font_description_free(floating_attention_font);
	pango_font_description_free(floating_normal_font);
	pango_font_description_free(pango_popup_font);

	g_object_unref(pango_context);
	g_object_unref(pango_font_map);

}

void simple2_theme_t::rounded_i_rect(cairo_t * cr, double x, double y,
		double w, double h, double r) {

	CHECK_CAIRO(cairo_save(cr));

	CHECK_CAIRO(cairo_new_path(cr));
	CHECK_CAIRO(cairo_move_to(cr, x, y + h));
	CHECK_CAIRO(cairo_line_to(cr, x, y + r));
	CHECK_CAIRO(cairo_arc(cr, x + r, y + r, r, 2.0 * (M_PI_2), 3.0 * (M_PI_2)));
	CHECK_CAIRO(cairo_move_to(cr, x + r, y));
	CHECK_CAIRO(cairo_line_to(cr, x + w - r, y));
	CHECK_CAIRO(cairo_arc(cr, x + w - r, y + r, r, 3.0 * (M_PI_2), 4.0 * (M_PI_2)));
	CHECK_CAIRO(cairo_move_to(cr, x + w, y + h));
	CHECK_CAIRO(cairo_line_to(cr, x + w, y + r));
	CHECK_CAIRO(cairo_stroke(cr));

	CHECK_CAIRO(cairo_restore(cr));
}

void simple2_theme_t::render_notebook(cairo_t * cr, theme_notebook_t const * n) const {

	CHECK_CAIRO(cairo_save(cr));
	CHECK_CAIRO(cairo_reset_clip(cr));

	CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
	CHECK_CAIRO(cairo_new_path(cr));

	CHECK_CAIRO(cairo_save(cr));
	cairo_clip(cr, n->allocation);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	if (backgroun_px != nullptr) {
		CHECK_CAIRO(cairo_set_source_surface(cr, backgroun_px->get_cairo_surface(), -n->root_x, -n->root_y));
	} else {
		CHECK_CAIRO(cairo_set_source_color_alpha(cr, default_background_color));
	}
	CHECK_CAIRO(cairo_paint(cr));
	CHECK_CAIRO(cairo_restore(cr));

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));

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

//	{
//		CHECK_CAIRO(cairo_save(cr));
//		/** bottom shadow **/
//		rect b { n->allocation };
//		b.y += notebook.tab_height;
//		b.h -= notebook.tab_height;
//		draw_outer_graddien(cr, b, color_t{0.0, 0.0, 0.0, 0.25}, 5);
//		/* borders lines */
//		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
//		cairo_reset_clip(cr);
//		cairo_rectangle(cr, b.x + 0.5, b.y - 0.5, b.w - 1.0, b.h);
//		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
//		cairo_stroke(cr);
//		CHECK_CAIRO(cairo_restore(cr));
//	}

	if (n->has_selected_client) {
		if (not n->selected_client.is_iconic) {

			/* clear selected tab background */
			rect a{n->selected_client.position};

			a.x+=2;
			a.w-=4;
			a.h += 1;

			CHECK_CAIRO(cairo_save(cr));
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			if (backgroun_px != nullptr) {
				CHECK_CAIRO(cairo_set_source_surface(cr, backgroun_px->get_cairo_surface(), -n->root_x, -n->root_y));
			} else {
				CHECK_CAIRO(cairo_set_source_color_alpha(cr, default_background_color));
			}
			CHECK_CAIRO(cairo_rectangle(cr, a.x, a.y, a.w, a.h));
			CHECK_CAIRO(cairo_fill(cr));
			CHECK_CAIRO(cairo_restore(cr));

			render_notebook_selected(cr, *n, n->selected_client,
					notebook_selected_font, notebook_selected_text_color,
					notebook_selected_outline_color,
					notebook_selected_border_color,
					n->selected_client.tab_color, 1.0);
		}
	}

	CHECK_CAIRO(cairo_reset_clip(cr));
	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
	CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));

	{

		cairo_save(cr);
		rect btext = compute_notebook_menu_position(n->allocation);

		if(n->button_mouse_over == NOTEBOOK_BUTTON_EXPOSAY) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			CHECK_CAIRO(cairo_rectangle(cr, btext.x, btext.y, btext.w, btext.h));
			cairo_fill(cr);
		}

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));

		char buf[32];
		snprintf(buf, 32, "%d", n->client_count);

		/* draw title */
		CHECK_CAIRO(cairo_set_source_color_alpha(cr, notebook_normal_outline_color));

		CHECK_CAIRO(cairo_translate(cr, btext.x + 15, btext.y + 4));

		//CHECK_CAIRO(cairo_new_path(cr));

		CHECK_CAIRO(cairo_set_line_width(cr, 3.0));
		CHECK_CAIRO(cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND));
		CHECK_CAIRO(cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL));

		{
			PangoLayout * pango_layout = pango_layout_new(pango_context);
			pango_layout_set_font_description(pango_layout,
					notebook_selected_font);
			pango_cairo_update_layout(cr, pango_layout);
			pango_layout_set_text(pango_layout, buf, -1);
			pango_layout_set_width(pango_layout, btext.w * PANGO_SCALE);
			pango_layout_set_wrap(pango_layout, PANGO_WRAP_CHAR);
			pango_layout_set_ellipsize(pango_layout, PANGO_ELLIPSIZE_END);
			pango_cairo_layout_path(cr, pango_layout);
			g_object_unref(pango_layout);
		}

		CHECK_CAIRO(cairo_stroke_preserve(cr));

		CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
		CHECK_CAIRO(cairo_set_source_color_alpha(cr, notebook_selected_text_color));
		CHECK_CAIRO(cairo_fill(cr));

		cairo_restore(cr);

	}


	{
		rect b = compute_notebook_bookmark_position(n->allocation);

		if(n->button_mouse_over == NOTEBOOK_BUTTON_MASK) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
			cairo_fill(cr);
		}

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		//CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
		if (n->is_default) {
			CHECK_CAIRO(cairo_set_source_surface(cr, pops_button_s, b.x, b.y));
			CHECK_CAIRO(cairo_mask_surface(cr, pops_button_s, b.x, b.y));
		} else {
			CHECK_CAIRO(cairo_set_source_surface(cr, pop_button_s, b.x, b.y));
			CHECK_CAIRO(cairo_mask_surface(cr, pop_button_s, b.x, b.y));
		}

	}

	{
		rect b = compute_notebook_vsplit_position(n->allocation);

		if(n->button_mouse_over == NOTEBOOK_BUTTON_VSPLIT) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
			cairo_fill(cr);
		}

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		CHECK_CAIRO(cairo_set_source_surface(cr, vsplit_button_s, b.x, b.y));
		CHECK_CAIRO(cairo_mask_surface(cr, vsplit_button_s, b.x, b.y));

		if(not n->can_vsplit) {
			::cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.5);
			CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
			cairo_fill(cr);
		}
	}

	{
		rect b = compute_notebook_hsplit_position(n->allocation);

		if(n->button_mouse_over == NOTEBOOK_BUTTON_HSPLIT) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
			cairo_fill(cr);
		}

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		CHECK_CAIRO(cairo_set_source_surface(cr, hsplit_button_s, b.x, b.y));
		CHECK_CAIRO(cairo_mask_surface(cr, hsplit_button_s, b.x, b.y));

		if(not n->can_hsplit) {
			::cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.5);
			CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
			cairo_fill(cr);
		}
	}

	{
		rect b = compute_notebook_close_position(n->allocation);

		if(n->button_mouse_over == NOTEBOOK_BUTTON_CLOSE) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
			cairo_fill(cr);
		}

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		CHECK_CAIRO(cairo_set_source_surface(cr, close_button_s, b.x, b.y));
		CHECK_CAIRO(cairo_mask_surface(cr, close_button_s, b.x, b.y));
	}

	if(n->has_scroll_arrow) {
		rect b = n->left_arrow_position;

		if(n->button_mouse_over == NOTEBOOK_BUTTON_LEFT_SCROLL_ARROW) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
			cairo_fill(cr);
		}

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		CHECK_CAIRO(cairo_set_source_surface(cr, left_scroll_arrow_button_s, b.x, b.y+3));
		CHECK_CAIRO(cairo_mask_surface(cr, left_scroll_arrow_button_s, b.x, b.y+3));
	}

	if(n->has_scroll_arrow) {
		rect b = n->right_arrow_position;

		if(n->button_mouse_over == NOTEBOOK_BUTTON_RIGHT_SCROLL_ARROW) {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
			CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
			cairo_fill(cr);
		}

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		CHECK_CAIRO(cairo_set_source_surface(cr, right_scroll_arrow_button_s, b.x, b.y+3));
		CHECK_CAIRO(cairo_mask_surface(cr, right_scroll_arrow_button_s, b.x, b.y+3));
	}



	CHECK_CAIRO(cairo_restore(cr));

}


static void draw_notebook_border(cairo_t * cr, rect const & alloc,
		rect const & tab_area, color_t const & color,
		double border_width) {

	double half = border_width / 2.0;

	CHECK_CAIRO(cairo_rectangle_arc_corner(cr, tab_area.x + 0.5, tab_area.y + 0.5, tab_area.w - 1.0, tab_area.h - 1.0, 7.0, CAIRO_CORNER_TOP));

//	CHECK_CAIRO(cairo_new_path(cr));
//
//	CHECK_CAIRO(cairo_move_to(cr, tab_area.x + half, tab_area.y + tab_area.h + half));
//	/** tab left **/
//	CHECK_CAIRO(cairo_line_to(cr, tab_area.x + half, tab_area.y + half));
//	/** tab top **/
//	CHECK_CAIRO(cairo_line_to(cr, tab_area.x + tab_area.w - half, tab_area.y + half));
//	/** tab right **/
//	CHECK_CAIRO(cairo_line_to(cr, tab_area.x + tab_area.w - half,)
//			tab_area.y + tab_area.h + half);
//	/** tab bottom **/
//	CHECK_CAIRO(cairo_line_to(cr, tab_area.x + half, tab_area.y + tab_area.h + half));

	CHECK_CAIRO(cairo_set_line_width(cr, border_width));
	CHECK_CAIRO(cairo_set_source_color_alpha(cr, color));
	CHECK_CAIRO(cairo_stroke(cr));
}


void simple2_theme_t::render_notebook_selected(
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
	tab_area.h -= 1;

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
	CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE));

	/** draw the tab background **/
	rect b = tab_area;

	CHECK_CAIRO(cairo_save(cr));

	/** clip tabs **/
	CHECK_CAIRO(cairo_rectangle_arc_corner(cr, b.x, b.y, b.w, b.h+30.0, 6.5, CAIRO_CORNER_ALL));
	CHECK_CAIRO(cairo_clip(cr));

	CHECK_CAIRO(cairo_set_source_color_alpha(cr, background_color));
	cairo_pattern_t * gradient;
	gradient = cairo_pattern_create_linear(0.0, b.y-2.0, 0.0, b.y+b.h+20.0);
	CHECK_CAIRO(cairo_pattern_add_color_stop_rgba(gradient, 0.0, 1.0, 1.0, 1.0, 1.0));
	CHECK_CAIRO(cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 1.0, 1.0, 0.0));
	CHECK_CAIRO(cairo_mask(cr, gradient));

	cairo_reset_clip(cr);

	/* draw black outline */
	CHECK_CAIRO(cairo_rounded_tab3(cr, b.x+0.5, b.y+0.5, b.w-1.0, b.h+30.0, 5.5));
	cairo_set_line_width(cr, 1.0);
	::cairo_set_source_rgb(cr, data.tab_color.r*border_factor, data.tab_color.g*border_factor, data.tab_color.b*border_factor);
	CHECK_CAIRO(cairo_stroke(cr));

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));

	cairo_reset_clip(cr);
	CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y+5.5, 1.0, 40.0));
	cairo_clip(cr);

	::cairo_set_source_rgb(cr, data.tab_color.r*border_factor, data.tab_color.g*border_factor, data.tab_color.b*border_factor);
	CHECK_CAIRO(cairo_mask(cr, gradient));

	cairo_reset_clip(cr);
	CHECK_CAIRO(cairo_rectangle(cr, b.x+b.w-1.0, b.y+5.5, 1.0, 40.0));
	cairo_clip(cr);

	::cairo_set_source_rgb(cr, data.tab_color.r*border_factor, data.tab_color.g*border_factor, data.tab_color.b*border_factor);
	CHECK_CAIRO(cairo_mask(cr, gradient));


	cairo_pattern_destroy(gradient);

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));

	/** clip tabs **/
	cairo_reset_clip(cr);
	CHECK_CAIRO(cairo_rectangle_arc_corner(cr, b.x, b.y, b.w, b.h+30.0, 6.5, CAIRO_CORNER_ALL));
	CHECK_CAIRO(cairo_clip(cr));

	rect xncclose;
	xncclose.x = b.x + b.w - notebook.selected_close_width;
	xncclose.y = b.y;
	xncclose.w = notebook.selected_close_width - 10;
	xncclose.h = b.h - 4;

	if(n.button_mouse_over == NOTEBOOK_BUTTON_CLIENT_CLOSE) {
		CHECK_CAIRO(::cairo_set_source_rgb(cr, 0xcc/255.0*1.2, 0x44/255.0*1.2, 0.0*1.2));
	} else {
		CHECK_CAIRO(::cairo_set_source_rgb(cr, 0xcc/255.0, 0x44/255.0, 0.0));
	}
	CHECK_CAIRO(cairo_rectangle(cr, xncclose.x, xncclose.y, xncclose.w, xncclose.h));
	CHECK_CAIRO(cairo_fill(cr));

	CHECK_CAIRO(cairo_new_path(cr));
	CHECK_CAIRO(cairo_move_to(cr, xncclose.x+0.5, xncclose.y+0.5));
	CHECK_CAIRO(cairo_line_to(cr, xncclose.x+0.5, xncclose.y+0.5 + xncclose.h-1.0));
	CHECK_CAIRO(cairo_line_to(cr, xncclose.x+xncclose.w+0.5, xncclose.y+0.5 + xncclose.h-1.0));
	CHECK_CAIRO(cairo_line_to(cr, xncclose.x+xncclose.w+0.5, xncclose.y+0.5));
	::cairo_set_source_rgb(cr, data.tab_color.r*border_factor, data.tab_color.g*border_factor, data.tab_color.b*border_factor);
	cairo_stroke(cr);

	CHECK_CAIRO(cairo_restore(cr));
	CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT));

	/** draw application icon **/
	rect bicon = tab_area;
	bicon.h = 16;
	bicon.w = 16;
	bicon.x += 10;
	bicon.y += 2;

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
	if (data.icon != nullptr) {
		if (data.icon->get_cairo_surface() != 0) {
			CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
			CHECK_CAIRO(cairo_set_source_surface(cr, data.icon->get_cairo_surface(),
					bicon.x, bicon.y));
			CHECK_CAIRO(cairo_mask_surface(cr, data.icon->get_cairo_surface(),
					bicon.x, bicon.y));
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
	CHECK_CAIRO(cairo_translate(cr, btext.x + 2, btext.y));

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

		CHECK_CAIRO(cairo_set_line_width(cr, 3.0));
		CHECK_CAIRO(cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND));
		CHECK_CAIRO(cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL));
		CHECK_CAIRO(cairo_set_source_color_alpha(cr, outline_color));

		CHECK_CAIRO(cairo_stroke_preserve(cr));

		CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
		CHECK_CAIRO(cairo_set_source_color_alpha(cr, text_color));
		CHECK_CAIRO(cairo_fill(cr));
	}

	cairo_restore(cr);

	/** draw close button **/

	rect ncclose;
	ncclose.x = tab_area.x + tab_area.w - (notebook.selected_close_width-10)/2 - 18;
	ncclose.y = tab_area.y;
	ncclose.w = 16;
	ncclose.h = 16;

	CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
	//CHECK_CAIRO(cairo_rectangle(cr, ncclose.x, ncclose.y, ncclose.w, ncclose.h));
	CHECK_CAIRO(cairo_set_source_surface(cr, close_button1_s, ncclose.x, ncclose.y));
	CHECK_CAIRO(cairo_mask_surface(cr, close_button1_s, ncclose.x, ncclose.y));

	/** draw unbind button **/
	rect ncub;
	ncub.x = tab_area.x + tab_area.w - notebook.selected_unbind_width
			- notebook.selected_close_width;
	ncub.y = tab_area.y;
	ncub.w = 16;
	ncub.h = 16;

	if(n.button_mouse_over == NOTEBOOK_BUTTON_CLIENT_UNBIND) {
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.2);
		CHECK_CAIRO(cairo_rectangle(cr, ncub.x, ncub.y, ncub.w, ncub.h));
		cairo_fill(cr);
	}

	CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
	CHECK_CAIRO(cairo_set_source_surface(cr, unbind_button_s, ncub.x, ncub.y));
	CHECK_CAIRO(cairo_mask_surface(cr, unbind_button_s, ncub.x, ncub.y));

	rect area = data.position;
	area.y += notebook.margin.top - 4;
	area.h -= notebook.margin.top - 4;

}



void simple2_theme_t::render_notebook_normal(
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

	CHECK_CAIRO(cairo_save(cr));

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
	CHECK_CAIRO(cairo_set_line_width(cr, 1.0));

	/** draw the tab background **/
	rect b = tab_area;
	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
	CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE));

	/** clip tabs **/
	CHECK_CAIRO(cairo_rectangle_arc_corner(cr, b.x, b.y, b.w, b.h, 6.5, CAIRO_CORNER_TOP));
	CHECK_CAIRO(cairo_clip(cr));

	CHECK_CAIRO(cairo_set_source_color_alpha(cr, background_color));
	cairo_pattern_t * gradient;
	gradient = cairo_pattern_create_linear(0.0, b.y-2.0, 0.0, b.y+b.h+20.0);
	CHECK_CAIRO(cairo_pattern_add_color_stop_rgba(gradient, 0.0, 1.0, 1.0, 1.0, 1.0));
	CHECK_CAIRO(cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 1.0, 1.0, 0.0));
	CHECK_CAIRO(cairo_mask(cr, gradient));
	cairo_pattern_destroy(gradient);

	cairo_reset_clip(cr);

	CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
	CHECK_CAIRO(cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND));
	CHECK_CAIRO(cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL));

	CHECK_CAIRO(cairo_set_source_rgba(cr, data.tab_color.r*border_factor, data.tab_color.g*border_factor, data.tab_color.b*border_factor, 1.0));
	CHECK_CAIRO(cairo_rectangle_arc_corner(cr, b.x+0.5, b.y+0.5, b.w-1.0, b.h-1.0, 5.5, CAIRO_CORNER_TOP));
	CHECK_CAIRO(cairo_stroke(cr));

	CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT));

	/* draw application icon */
	rect bicon = tab_area;
	bicon.h = 16;
	bicon.w = 16;
	bicon.x += 6;
	bicon.y += 2;

	CHECK_CAIRO(cairo_save(cr));
	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
	if (data.icon != nullptr) {
		if (data.icon->get_cairo_surface() != nullptr) {
			CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
			CHECK_CAIRO(cairo_set_source_surface(cr, data.icon->get_cairo_surface(),
					bicon.x, bicon.y));
			CHECK_CAIRO(cairo_mask_surface(cr, data.icon->get_cairo_surface(),
					bicon.x, bicon.y));
		}
	}

	CHECK_CAIRO(cairo_restore(cr));

	rect btext = tab_area;
	btext.h -= 0;
	btext.w -= 1 * 16 + 12;
	btext.x += 3 + 16 + 6;
	btext.y += 1;

	if (btext.w > 10) {

		CHECK_CAIRO(cairo_save(cr));

		/* draw title */
		CHECK_CAIRO(cairo_set_source_color_alpha(cr, outline_color));

		CHECK_CAIRO(cairo_translate(cr, btext.x + 2, btext.y));

		//CHECK_CAIRO(cairo_new_path(cr));

		CHECK_CAIRO(cairo_set_line_width(cr, 3.0));
		CHECK_CAIRO(cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND));
		CHECK_CAIRO(cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL));

		{
			PangoLayout * pango_layout = pango_layout_new(pango_context);
			pango_layout_set_font_description(pango_layout, pango_font);
			pango_cairo_update_layout(cr, pango_layout);
			pango_layout_set_text(pango_layout, data.title.c_str(), -1);
			pango_layout_set_width(pango_layout, btext.w * PANGO_SCALE);
			pango_layout_set_wrap(pango_layout, PANGO_WRAP_CHAR);
			pango_layout_set_ellipsize(pango_layout, PANGO_ELLIPSIZE_END);
			pango_cairo_layout_path(cr, pango_layout);
			g_object_unref(pango_layout);
		}

		CHECK_CAIRO(cairo_stroke_preserve(cr));

		CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
		CHECK_CAIRO(cairo_set_source_color_alpha(cr, text_color));
		CHECK_CAIRO(cairo_fill(cr));

		CHECK_CAIRO(cairo_restore(cr));

	}

	cairo_new_path(cr);
	cairo_move_to(cr, data.position.x+0.5, data.position.y+data.position.h - 0.5);
	cairo_line_to(cr, data.position.x+data.position.w-0.5, data.position.y+data.position.h - 0.5);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_stroke(cr);

	CHECK_CAIRO(cairo_restore(cr));

}

void simple2_theme_t::render_split(cairo_t * cr,
		theme_split_t const * s) const {

	CHECK_CAIRO(cairo_save(cr));
	CHECK_CAIRO(cairo_set_line_width(cr, 1.0));

	rect sarea = s->allocation;
	if (backgroun_px != nullptr) {
		CHECK_CAIRO(cairo_set_source_surface(cr, backgroun_px->get_cairo_surface(), -s->root_x, -s->root_y));
	} else {
		CHECK_CAIRO(cairo_set_source_color_alpha(cr, default_background_color));
	}
	CHECK_CAIRO(cairo_rectangle(cr, sarea.x, sarea.y, sarea.w, sarea.h));
	CHECK_CAIRO(cairo_fill(cr));

	if(s->has_mouse_over) {
		cairo_save(cr);
		cairo_clip(cr, sarea);
		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		cairo_paint_with_alpha(cr, 0.2);
		cairo_restore(cr);
	}

	rect grip0;
	rect grip1;
	rect grip2;

	if (sarea.w > sarea.h) {
		/* vertical splitting of area, i.e. horizontal grip */
		grip1.x = sarea.x + (sarea.w / 2.0) - 4.0;
		grip1.y = sarea.y + (sarea.h / 2.0) - 4.0;
		grip1.w = 8.0;
		grip1.h = 8.0;
		grip0 = grip1;
		grip0.x -= 16;
		grip2 = grip1;
		grip2.x += 16;
	} else {
		grip1.x = sarea.x + (sarea.w / 2.0) - 4.0;
		grip1.y = sarea.y + (sarea.h / 2.0) - 4.0;
		grip1.w = 8.0;
		grip1.h = 8.0;
		grip0 = grip1;
		grip0.y -= 16;
		grip2 = grip1;
		grip2.y += 16;
	}

	CHECK_CAIRO(cairo_set_source_color_alpha(cr, grip_color));
	CHECK_CAIRO(cairo_rectangle(cr, grip0.x, grip0.y, grip0.w, grip0.h));
	CHECK_CAIRO(cairo_fill(cr));
	CHECK_CAIRO(cairo_set_source_color_alpha(cr, grip_color));
	CHECK_CAIRO(cairo_rectangle(cr, grip1.x, grip1.y, grip1.w, grip1.h));
	CHECK_CAIRO(cairo_fill(cr));
	CHECK_CAIRO(cairo_set_source_color_alpha(cr, grip_color));
	CHECK_CAIRO(cairo_rectangle(cr, grip2.x, grip2.y, grip2.w, grip2.h));
	CHECK_CAIRO(cairo_fill(cr));

	CHECK_CAIRO(cairo_restore(cr));
}

void simple2_theme_t::render_empty(cairo_t * cr, rect const & area) const {
	CHECK_CAIRO(cairo_save(cr));
	cairo_clip(cr, area);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	if (backgroun_px != nullptr) {
		CHECK_CAIRO(cairo_set_source_surface(cr, backgroun_px->get_cairo_surface(), 0.0, 0.0));
	} else {
		CHECK_CAIRO(cairo_set_source_color_alpha(cr, default_background_color));
	}
	CHECK_CAIRO(cairo_paint(cr));
	CHECK_CAIRO(cairo_restore(cr));
}

void simple2_theme_t::render_thumbnail(cairo_t * cr, rect position, theme_thumbnail_t const & t) const {

	double y_text_offset = 0;

	if (t.pix != nullptr) {
		cairo_surface_t * src = t.pix->get_cairo_surface();
		cairo_save(cr);

		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

		rect thumbnail_pos = position;
		thumbnail_pos.h -= 20;

		double src_width = t.pix->witdh();
		double src_height = t.pix->height();

		double x_ratio = thumbnail_pos.w / src_width;
		double y_ratio = thumbnail_pos.h / src_height;

		double x_offset, y_offset;

		if (x_ratio < y_ratio) {

			double yp = thumbnail_pos.h / x_ratio;

			x_offset = 0;
			y_offset = floor((yp - src_height) / 2.0);

			cairo_translate(cr, thumbnail_pos.x, thumbnail_pos.y);
			cairo_scale(cr, x_ratio, x_ratio);
			cairo_set_source_surface(cr, src, x_offset, y_offset);
			cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
			cairo_paint(cr);

			y_text_offset = src_height * x_ratio + (thumbnail_pos.h - (src_height * x_ratio)) / 2.0;

		} else {
			double xp = thumbnail_pos.w / y_ratio;

			y_offset = 0;
			x_offset = floor((xp - src_width) / 2.0);
			cairo_translate(cr, thumbnail_pos.x, thumbnail_pos.y);
			cairo_scale(cr, y_ratio, y_ratio);
			cairo_set_source_surface(cr, src, x_offset, y_offset);
			cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
			cairo_paint(cr);

			y_text_offset = src_height * y_ratio + (thumbnail_pos.h - (src_height * y_ratio)) / 2.0;
		}

		cairo_restore(cr);
	} else {
		y_text_offset = position.h - 20;
	}

	rect btext = position;
	btext.y += y_text_offset;
	btext.h = 20;

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(cr, t.title->get_cairo_surface(), btext.x, btext.y);
	cairo_paint(cr);
	cairo_restore(cr);

}


void simple2_theme_t::render_thumbnail_title(cairo_t * cr, rect btext, std::string const & title) const {

	CHECK_CAIRO(cairo_save(cr));

	/* clear background */
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
	cairo_paint(cr);

	CHECK_CAIRO(cairo_set_source_color_alpha(cr, notebook_normal_outline_color));
	CHECK_CAIRO(cairo_translate(cr, btext.x + 2, btext.y));

	CHECK_CAIRO(cairo_set_line_width(cr, 3.0));
	CHECK_CAIRO(cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND));
	CHECK_CAIRO(cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL));

	{
		PangoLayout * pango_layout = pango_layout_new(pango_context);
		pango_layout_set_font_description(pango_layout, notebook_normal_font);
		pango_cairo_update_layout(cr, pango_layout);
		pango_layout_set_text(pango_layout, title.c_str(), -1);
		pango_layout_set_width(pango_layout, btext.w * PANGO_SCALE);
		pango_layout_set_wrap(pango_layout, PANGO_WRAP_CHAR);
		pango_layout_set_ellipsize(pango_layout, PANGO_ELLIPSIZE_MIDDLE);
		pango_layout_set_alignment(pango_layout, PANGO_ALIGN_CENTER);
		pango_cairo_layout_path(cr, pango_layout);
		g_object_unref(pango_layout);
	}

	CHECK_CAIRO(cairo_stroke_preserve(cr));

	CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
	CHECK_CAIRO(cairo_set_source_color_alpha(cr, notebook_normal_text_color));
	CHECK_CAIRO(cairo_fill(cr));
	CHECK_CAIRO(cairo_restore(cr));

}

void simple2_theme_t::render_floating(theme_managed_window_t * mw) const {

	if (mw->focuced) {
		render_floating_base(mw,
				floating_active_font,
				floating_active_text_color,
				floating_active_outline_color, floating_active_border_color,
				floating_active_background_color, 1.0);
	} else if (mw->demand_attention) {
		render_floating_base(mw,
				floating_attention_font,
				floating_attention_text_color,
				floating_attention_outline_color,
				floating_attention_border_color,
				floating_attention_background_color, 1.0);
	} else {
		render_floating_base(mw,
				floating_normal_font,
				floating_normal_text_color,
				floating_normal_outline_color, floating_normal_border_color,
				floating_normal_background_color, 1.0);
	}
}


void simple2_theme_t::render_floating_base(
		theme_managed_window_t * mw,
		PangoFontDescription const * pango_font,
		color_t const & text_color,
		color_t const & outline_color,
		color_t const & border_color,
		color_t const & background_color,
		double border_width
) const {
	double const border_factor = 0.6;
	rect allocation = mw->position;

	if (mw->cairo_top != nullptr) {
		cairo_t * cr = mw->cairo_top;

		rect tab_area(0, 0, allocation.w, floating.title_height);

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
		::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
		cairo_rectangle(cr, tab_area.x, tab_area.y, tab_area.w, tab_area.h+floating.margin.top);
		cairo_fill(cr);

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
		CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE));

		/** draw the tab background **/
		rect b = tab_area;

		CHECK_CAIRO(cairo_save(cr));

		/** clip tabs **/
		CHECK_CAIRO(cairo_rectangle_arc_corner(cr, b.x, b.y, b.w, b.h+30.0, 8.0, CAIRO_CORNER_ALL));
		CHECK_CAIRO(cairo_clip(cr));

		CHECK_CAIRO(cairo_set_source_color_alpha(cr, background_color));
		cairo_pattern_t * gradient;
		gradient = cairo_pattern_create_linear(0.0, b.y-2.0, 0.0, b.y+b.h);
		CHECK_CAIRO(cairo_pattern_add_color_stop_rgba(gradient, 0.0, 1.0, 1.0, 1.0, (background_color.a)));
		CHECK_CAIRO(cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 1.0, 1.0, (background_color.a)*0.8));
		CHECK_CAIRO(cairo_mask(cr, gradient));
		cairo_pattern_destroy(gradient);

		cairo_set_line_width(cr, 1.0);

		/* draw black outline */
		::cairo_set_source_rgb(cr, background_color.r*border_factor, background_color.g*border_factor, background_color.b*border_factor);
		CHECK_CAIRO(cairo_rectangle_arc_corner(cr, b.x+0.5, b.y+0.5, b.w-1.0, b.h+30.0, 7.0, CAIRO_CORNER_TOP));
		CHECK_CAIRO(cairo_stroke(cr));

		rect xncclose;
		xncclose.x = b.x + b.w - floating.close_width;
		xncclose.y = b.y;
		xncclose.w = floating.close_width - 10;
		xncclose.h = floating.title_height - 4;

		CHECK_CAIRO(cairo_rectangle(cr, xncclose.x, xncclose.y, xncclose.w, xncclose.h+0.5));
		CHECK_CAIRO(::cairo_set_source_rgb(cr, 0xcc/255.0, 0x44/255.0, 0.0));
		CHECK_CAIRO(cairo_fill(cr));

//		CHECK_CAIRO(cairo_new_path(cr));
//		CHECK_CAIRO(cairo_move_to(cr, xncclose.x+0.5, xncclose.y+0.5));
//		CHECK_CAIRO(cairo_line_to(cr, xncclose.x+xncclose.w+0.5, xncclose.y+0.5));
//		::cairo_set_source_rgb(cr, 0xcc/255.0*.5, 0x44/255.0*.5, 0.0);
//		cairo_stroke(cr);

		CHECK_CAIRO(cairo_new_path(cr));
		CHECK_CAIRO(cairo_move_to(cr, xncclose.x+0.5, xncclose.y+0.5));
		CHECK_CAIRO(cairo_line_to(cr, xncclose.x+0.5, xncclose.y+0.5 + xncclose.h-1.0));
		CHECK_CAIRO(cairo_line_to(cr, xncclose.x+xncclose.w+0.5, xncclose.y+0.5 + xncclose.h-1.0));
		CHECK_CAIRO(cairo_line_to(cr, xncclose.x+xncclose.w+0.5, xncclose.y+0.5));
		::cairo_set_source_rgb(cr, background_color.r*border_factor, background_color.g*border_factor, background_color.b*border_factor);
		cairo_stroke(cr);

//		CHECK_CAIRO(cairo_new_path(cr));
//		CHECK_CAIRO(cairo_move_to(cr, xncclose.x+0.5, xncclose.y+0.5));
//		CHECK_CAIRO(cairo_line_to(cr, xncclose.x+xncclose.w+0.5, xncclose.y+0.5));
//		::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
//		cairo_stroke(cr);

		CHECK_CAIRO(cairo_restore(cr));
		CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT));

		/** draw application icon **/
		rect bicon = tab_area;
		bicon.h = 16;
		bicon.w = 16;
		bicon.x += 10;
		bicon.y += 4;

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
		if (mw->icon != 0) {
			if (mw->icon->get_cairo_surface() != nullptr) {
				CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
				CHECK_CAIRO(cairo_set_source_surface(cr, mw->icon->get_cairo_surface(),
						bicon.x, bicon.y));
				CHECK_CAIRO(cairo_mask_surface(cr, mw->icon->get_cairo_surface(),
						bicon.x, bicon.y));
			}
		}

		/** draw application title **/
		rect btext = tab_area;
		btext.h -= 0;
		btext.w -= 3 * 16 + 12 + 30;
		btext.x += 7 + 16 + 6;
		btext.y += 4;

		cairo_save(cr);
		/** draw title **/
		CHECK_CAIRO(cairo_translate(cr, btext.x + 2, btext.y));

		{

			{
				PangoLayout * pango_layout = pango_layout_new(pango_context);
				pango_layout_set_font_description(pango_layout, pango_font);
				pango_cairo_update_layout(cr, pango_layout);
				pango_layout_set_text(pango_layout, mw->title.c_str(), -1);
				pango_layout_set_wrap(pango_layout, PANGO_WRAP_CHAR);
				pango_layout_set_ellipsize(pango_layout, PANGO_ELLIPSIZE_END);
				pango_layout_set_width(pango_layout, btext.w * PANGO_SCALE);
				pango_cairo_layout_path(cr, pango_layout);
				g_object_unref(pango_layout);
			}

			CHECK_CAIRO(cairo_set_line_width(cr, 3.0));
			CHECK_CAIRO(cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND));
			CHECK_CAIRO(cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL));
			CHECK_CAIRO(cairo_set_source_color_alpha(cr, outline_color));

			CHECK_CAIRO(cairo_stroke_preserve(cr));

			CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
			CHECK_CAIRO(cairo_set_source_color_alpha(cr, text_color));
			CHECK_CAIRO(cairo_fill(cr));
		}

		cairo_restore(cr);

		/** draw close button **/

		rect ncclose;
		ncclose.x = tab_area.x + tab_area.w - (floating.close_width-10)/2 - 18;
		ncclose.y = tab_area.y;
		ncclose.w = floating.close_width;
		ncclose.h = floating.title_height - 4;

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		//CHECK_CAIRO(cairo_rectangle(cr, ncclose.x, ncclose.y, ncclose.w, ncclose.h));
		CHECK_CAIRO(cairo_set_source_surface(cr, close_button1_s, ncclose.x, ncclose.y));
		CHECK_CAIRO(cairo_mask_surface(cr, close_button1_s, ncclose.x, ncclose.y));

		/** draw unbind button **/
		rect ncub;
		ncub.x = tab_area.x + tab_area.w - floating.bind_width - floating.close_width;
		ncub.y = tab_area.y;
		ncub.w = floating.bind_width;
		ncub.h = floating.title_height;

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		//CHECK_CAIRO(cairo_rectangle(cr, ncub.x, ncub.y, ncub.w, ncub.h));
		CHECK_CAIRO(cairo_set_source_surface(cr, bind_button_s, ncub.x, ncub.y));
		CHECK_CAIRO(cairo_mask_surface(cr, bind_button_s, ncub.x, ncub.y));

		warn(cairo_get_reference_count(cr) == 1);
		cairo_destroy(cr);
	}

	if (mw->cairo_bottom != nullptr) {
		cairo_t * cr = mw->cairo_bottom;

		rect b(0, 0, allocation.w, floating.margin.bottom);

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
		::cairo_set_source_rgba(cr, background_color.r, background_color.g, background_color.b, (background_color.a)*0.8);
		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_fill(cr);

		::cairo_set_source_rgb(cr, background_color.r*border_factor, background_color.g*border_factor, background_color.b*border_factor);
		cairo_set_line_width(cr, 1.0);

		// bottom border
		cairo_new_path(cr);
		cairo_move_to(cr,b.x+0.5, b.y);
		cairo_line_to(cr, b.x+0.5, b.y+b.h-0.5);
		cairo_line_to(cr, b.x+b.w-0.5, b.y+b.h-0.5);
		cairo_line_to(cr,b.x+b.w-0.5, b.y);
		cairo_stroke(cr);

		warn(cairo_get_reference_count(cr) == 1);
		cairo_destroy(cr);

	}

	if (mw->cairo_right != nullptr) {
		cairo_t * cr = mw->cairo_right;

		rect b(0, 0, floating.margin.right,
				allocation.h - floating.margin.top - floating.title_height
						- floating.margin.bottom);

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
		::cairo_set_source_rgba(cr, background_color.r, background_color.g, background_color.b, (background_color.a)*0.8);
		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_fill(cr);

		::cairo_set_source_rgb(cr, background_color.r*border_factor, background_color.g*border_factor, background_color.b*border_factor);
		cairo_set_line_width(cr, 1.0);

		cairo_new_path(cr);
		cairo_move_to(cr,b.x+b.w-0.5, b.y);
		cairo_line_to(cr, b.x+b.w-0.5, b.y+b.h);
		cairo_stroke(cr);

		warn(cairo_get_reference_count(cr) == 1);
		cairo_destroy(cr);
	}

	if (mw->cairo_left != nullptr) {
		cairo_t * cr = mw->cairo_left;

		rect b(0, 0, floating.margin.left,
				allocation.h - floating.margin.top - floating.title_height
						- floating.margin.bottom);

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		::cairo_set_source_rgba(cr, background_color.r, background_color.g, background_color.b, (background_color.a)*0.8);
		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_fill(cr);

		::cairo_set_source_rgb(cr, background_color.r*border_factor, background_color.g*border_factor, background_color.b*border_factor);
		cairo_set_line_width(cr, 1.0);

		cairo_new_path(cr);
		cairo_move_to(cr,b.x+0.5, b.y);
		cairo_line_to(cr, b.x+0.5, b.y+b.h);
		cairo_stroke(cr);

		warn(cairo_get_reference_count(cr) == 1);
		cairo_destroy(cr);

	}

}

void simple2_theme_t::render_popup_notebook0(cairo_t * cr, icon64 * icon, unsigned int width,
		unsigned int height, string const & title) const {

	cairo_save(cr);
	cairo_set_line_width(cr, 6.0);
	cairo_rectangle_arc_corner(cr, 3, 3, width - 6, height - 6, 10.0, CAIRO_CORNER_ALL);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_stroke_preserve(cr);
	cairo_set_line_width(cr, 4.0);
	cairo_set_source_color(cr, popup_background_color);
	cairo_stroke(cr);

	if (icon != nullptr) {
		double x_offset = (width - 64) / 2.0;
		double y_offset = (height - 64) / 2.0;

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		CHECK_CAIRO(cairo_set_source_surface(cr, icon->get_cairo_surface(), x_offset, y_offset));
		CHECK_CAIRO(cairo_mask_surface(cr, icon->get_cairo_surface(), x_offset, y_offset));

		/* draw title */
		CHECK_CAIRO(cairo_translate(cr, 0.0, y_offset + 68.0));

		{
			PangoLayout * pango_layout = pango_layout_new(pango_context);
			pango_layout_set_font_description(pango_layout, pango_popup_font);
			pango_cairo_update_layout(cr, pango_layout);
			pango_layout_set_text(pango_layout, title.c_str(), -1);
			pango_layout_set_width(pango_layout, width * PANGO_SCALE);
			pango_layout_set_alignment(pango_layout, PANGO_ALIGN_CENTER);
			pango_layout_set_wrap(pango_layout, PANGO_WRAP_CHAR);
			pango_cairo_layout_path(cr, pango_layout);
			g_object_unref(pango_layout);
		}

		CHECK_CAIRO(cairo_set_line_width(cr, 5.0));
		CHECK_CAIRO(cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND));
		CHECK_CAIRO(cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL));

		CHECK_CAIRO(cairo_set_source_color_alpha(cr, popup_outline_color));

		CHECK_CAIRO(cairo_stroke_preserve(cr));

		CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
		CHECK_CAIRO(cairo_set_source_color_alpha(cr, popup_text_color));
		CHECK_CAIRO(cairo_fill(cr));

	}

	cairo_restore(cr);

}

void simple2_theme_t::render_popup_move_frame(cairo_t * cr, icon64 * icon, unsigned int width,
		unsigned int height, std::string const & title) const {
	render_popup_notebook0(cr, icon, width, height, title);
}

void simple2_theme_t::draw_hatched_i_rect(cairo_t * cr, int space, int x, int y, int w, int h) const {

	CHECK_CAIRO(cairo_save(cr));
	int left_bound = x;
	int right_bound = x + w;
	int top_bound = y;
	int bottom_bound = y + h;

	int count = (right_bound - left_bound) / space + (bottom_bound - top_bound) / space;

	CHECK_CAIRO(cairo_new_path(cr));

	for(int i = 0; i <= count; ++i) {

		/** clip left/right **/

		int x0 = left_bound;
		int y0 = top_bound + space * i;
		int x1 = right_bound;
		int y1 = y0 - (right_bound - left_bound);

		/** clip top **/
		if(y1 < top_bound) {
			y1 = top_bound;
			x1 = left_bound + (y0 - top_bound);
		}

		/** clip bottom **/
		if(y0 > bottom_bound) {
			x0 = left_bound + (y0 - (bottom_bound - top_bound));
			y0 = bottom_bound;
		}

		CHECK_CAIRO(cairo_move_to(cr, x0, y0));
		CHECK_CAIRO(cairo_line_to(cr, x1, y1));

	}

//	CHECK_CAIRO(cairo_set_source_rgb(cr, 1.0, 0.0, 0.0));
	CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
	CHECK_CAIRO(cairo_stroke(cr));

	CHECK_CAIRO(cairo_rectangle(cr, left_bound, top_bound, right_bound - left_bound, bottom_bound - top_bound));
//	CHECK_CAIRO(cairo_set_source_rgb(cr, 1.0, 0.0, 0.0));
	CHECK_CAIRO(cairo_set_line_width(cr, 3.0));
	CHECK_CAIRO(cairo_stroke(cr));

	CHECK_CAIRO(cairo_restore(cr));

}

void simple2_theme_t::update() {
	create_background_img();
}

void simple2_theme_t::create_background_img() {

	if (has_background) {

		cairo_surface_t * tmp = cairo_image_surface_create_from_png(
				background_file.c_str());

		xcb_get_geometry_cookie_t ck = xcb_get_geometry(_cnx->xcb(), _cnx->root());
		xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(_cnx->xcb(), ck, 0);

		cairo_surface_t * image_background_s = cairo_image_surface_create(CAIRO_FORMAT_RGB24, geometry->width, geometry->height);

		/**
		 * WARNING: transform order and set_source_surface have huge
		 * Consequence.
		 **/

		double src_width = cairo_image_surface_get_width(tmp);
		double src_height = cairo_image_surface_get_height(tmp);

		if (src_width > 0 and src_height > 0) {

			if (scale_mode == "stretch") {

				cairo_t * cr = cairo_create(image_background_s);

				CHECK_CAIRO(::cairo_set_source_rgb(cr, 0.5, 0.5, 0.5));
				CHECK_CAIRO(cairo_rectangle(cr, 0, 0, geometry->width, geometry->height));
				CHECK_CAIRO(cairo_fill(cr));

				double x_ratio = geometry->width / src_width;
				double y_ratio = geometry->height / src_height;
				CHECK_CAIRO(cairo_scale(cr, x_ratio, y_ratio));
				CHECK_CAIRO(cairo_set_source_surface(cr, tmp, 0, 0));
				CHECK_CAIRO(cairo_rectangle(cr, 0, 0, src_width, src_height));
				CHECK_CAIRO(cairo_fill(cr));

				warn(cairo_get_reference_count(cr) == 1);
				cairo_destroy(cr);

			} else if (scale_mode == "zoom") {

				cairo_t * cr = cairo_create(image_background_s);

				CHECK_CAIRO(::cairo_set_source_rgb(cr, 0.5, 0.5, 0.5));
				CHECK_CAIRO(cairo_rectangle(cr, 0, 0, geometry->width, geometry->height));
				CHECK_CAIRO(cairo_fill(cr));

				double x_ratio = geometry->width / (double)src_width;
				double y_ratio = geometry->height / (double)src_height;

				double x_offset;
				double y_offset;

				if (x_ratio > y_ratio) {

					double yp = geometry->height / x_ratio;

					x_offset = 0;
					y_offset = (yp - src_height) / 2.0;

					CHECK_CAIRO(cairo_scale(cr, x_ratio, x_ratio));
					CHECK_CAIRO(cairo_set_source_surface(cr, tmp, x_offset, y_offset));
					CHECK_CAIRO(cairo_rectangle(cr, 0, 0, src_width, yp));
					CHECK_CAIRO(cairo_fill(cr));

				} else {

					double xp = geometry->width / y_ratio;

					x_offset = (xp - src_width) / 2.0;
					y_offset = 0;

					CHECK_CAIRO(cairo_scale(cr, y_ratio, y_ratio));
					CHECK_CAIRO(cairo_set_source_surface(cr, tmp, x_offset, y_offset));
					CHECK_CAIRO(cairo_rectangle(cr, 0, 0, xp, src_height));
					CHECK_CAIRO(cairo_fill(cr));
				}

				warn(cairo_get_reference_count(cr) == 1);
				cairo_destroy(cr);

			} else if (scale_mode == "center") {

				cairo_t * cr = cairo_create(image_background_s);

				CHECK_CAIRO(::cairo_set_source_rgb(cr, 0.5, 0.5, 0.5));
				CHECK_CAIRO(cairo_rectangle(cr, 0, 0, geometry->width, geometry->height));
				CHECK_CAIRO(cairo_fill(cr));

				double x_offset = (geometry->width - src_width) / 2.0;
				double y_offset = (geometry->height - src_height) / 2.0;

				CHECK_CAIRO(cairo_set_source_surface(cr, tmp, x_offset, y_offset));
				CHECK_CAIRO(cairo_rectangle(cr, max<double>(0.0, x_offset),
						max<double>(0.0, y_offset),
						min<double>(src_width, geometry->width),
						min<double>(src_height, geometry->height)));
				CHECK_CAIRO(cairo_fill(cr));

				warn(cairo_get_reference_count(cr) == 1);
				cairo_destroy(cr);

			} else if (scale_mode == "scale" || scale_mode == "span") {

				cairo_t * cr = cairo_create(image_background_s);

				CHECK_CAIRO(::cairo_set_source_rgb(cr, 0.5, 0.5, 0.5));
				CHECK_CAIRO(cairo_rectangle(cr, 0, 0, geometry->width, geometry->height));
				CHECK_CAIRO(cairo_fill(cr));

				double x_ratio = geometry->width / src_width;
				double y_ratio = geometry->height / src_height;

				double x_offset, y_offset;

				if (x_ratio < y_ratio) {

					double yp = geometry->height / x_ratio;

					x_offset = 0;
					y_offset = (yp - src_height) / 2.0;

					CHECK_CAIRO(cairo_scale(cr, x_ratio, x_ratio));
					CHECK_CAIRO(cairo_set_source_surface(cr, tmp, x_offset, y_offset));
					CHECK_CAIRO(cairo_rectangle(cr, x_offset, y_offset, src_width, src_height));
					CHECK_CAIRO(cairo_fill(cr));

				} else {
					double xp = geometry->width / y_ratio;

					y_offset = 0;
					x_offset = (xp - src_width) / 2.0;

					CHECK_CAIRO(cairo_scale(cr, y_ratio, y_ratio));
					CHECK_CAIRO(cairo_set_source_surface(cr, tmp, x_offset, y_offset));
					CHECK_CAIRO(cairo_rectangle(cr, x_offset, y_offset, src_width, src_height));
					CHECK_CAIRO(cairo_fill(cr));
				}

				warn(cairo_get_reference_count(cr) == 1);
				cairo_destroy(cr);

			} else if (scale_mode == "tile") {

				cairo_t * cr = cairo_create(image_background_s);
				CHECK_CAIRO(::cairo_set_source_rgb(cr, 0.5, 0.5, 0.5));
				CHECK_CAIRO(cairo_rectangle(cr, 0, 0, geometry->width, geometry->height));
				CHECK_CAIRO(cairo_fill(cr));

				for (double x = 0; x < geometry->width; x += src_width) {
					for (double y = 0; y < geometry->height; y += src_height) {
						CHECK_CAIRO(cairo_identity_matrix(cr));
						CHECK_CAIRO(cairo_translate(cr, x, y));
						CHECK_CAIRO(cairo_set_source_surface(cr, tmp, 0, 0));
						CHECK_CAIRO(cairo_rectangle(cr, 0, 0, geometry->width, geometry->height));
						CHECK_CAIRO(cairo_fill(cr));
					}
				}

				warn(cairo_get_reference_count(cr) == 1);
				cairo_destroy(cr);

			}

		}

		warn(cairo_surface_get_reference_count(tmp) == 1);
		cairo_surface_destroy(tmp);

		/* copy background to pixmap */
		backgroun_px = make_shared<pixmap_t>(_cnx, PIXMAP_RGB, geometry->width, geometry->height);

		cairo_t * cr = cairo_create(backgroun_px->get_cairo_surface());
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_surface(cr, image_background_s, 0.0, 0.0);
		cairo_paint(cr);
		cairo_destroy(cr);

		cairo_surface_destroy(image_background_s);

		free(geometry);

	}

}

void simple2_theme_t::render_popup_split(cairo_t * cr, theme_split_t const * s,
		double current_split) const {

	cairo_save(cr);

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
	CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE));

	rect const & alloc = s->allocation;

	rect rect0;
	rect rect1;

	if(s->type == HORIZONTAL_SPLIT) {

		rect0.x = alloc.x + 5.0;
		rect0.w = alloc.w - 10.0;
		rect1.x = alloc.x + 5.0;
		rect1.w = alloc.w - 10.0;

		rect0.y = alloc.y + 5.0 + notebook.margin.top;
		rect0.h = alloc.h * current_split - 5.0 - 5.0  - notebook.margin.top;

		rect1.y = alloc.y + alloc.h * current_split + 5.0;
		rect1.h = alloc.h * (1.0-current_split) - 5.0 - 5.0;

	} else {

		rect0.y = alloc.y + 5.0 + notebook.margin.top;
		rect0.h = alloc.h - 10.0 - notebook.margin.top;
		rect1.y = alloc.y + 5.0 + notebook.margin.top;
		rect1.h = alloc.h - 10.0 - notebook.margin.top;

		rect0.x = alloc.x + 5.0;
		rect0.w = alloc.w * current_split - 5.0 - 5.0;

		rect1.x = alloc.x + alloc.w * current_split + 5.0;
		rect1.w = alloc.w * (1.0-current_split) - 5.0 - 5.0;

	}

	CHECK_CAIRO(cairo_set_line_width(cr, 6.0));
	CHECK_CAIRO(::cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0));
	CHECK_CAIRO(cairo_rectangle(cr, rect0.x, rect0.y, rect0.w, rect0.h));
	CHECK_CAIRO(cairo_stroke(cr));

	CHECK_CAIRO(::cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0));
	CHECK_CAIRO(cairo_rectangle(cr, rect1.x, rect1.y, rect1.w, rect1.h));
	CHECK_CAIRO(cairo_stroke(cr));

	CHECK_CAIRO(cairo_set_line_width(cr, 4.0));
	CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 1.0));
	CHECK_CAIRO(cairo_rectangle(cr, rect0.x, rect0.y, rect0.w, rect0.h));
	CHECK_CAIRO(cairo_stroke(cr));

	CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 1.0));
	CHECK_CAIRO(cairo_rectangle(cr, rect1.x, rect1.y, rect1.w, rect1.h));
	CHECK_CAIRO(cairo_stroke(cr));

	cairo_restore(cr);

}

void simple2_theme_t::render_menuentry(cairo_t * cr, theme_dropdown_menu_entry_t const & item, rect const & area, bool selected) const {

	cairo_save(cr);

	cairo_rectangle(cr, area.x, area.y, area.w, area.h);
	if (selected) {
		::cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 1.0);
	} else {
		::cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
	}
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_fill(cr);

	CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT));

	/** draw application icon **/
	rect bicon = area;
	bicon.h = 16;
	bicon.w = 16;
	bicon.x += 5;
	bicon.y += 5;

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
	if (item.icon != nullptr) {
		if (item.icon->get_cairo_surface() != 0) {
			CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
			CHECK_CAIRO(cairo_set_source_surface(cr, item.icon->get_cairo_surface(),
					bicon.x, bicon.y));
			CHECK_CAIRO(cairo_mask_surface(cr, item.icon->get_cairo_surface(),
					bicon.x, bicon.y));
		}
	}

	/** draw application title **/
	rect btext = area;
	btext.h -= 0;
	btext.w -= 0;
	btext.x += 24;
	btext.y += 3;

	/** draw title **/
	CHECK_CAIRO(cairo_translate(cr, btext.x + 2, btext.y));

	{

		{
			PangoLayout * pango_layout = pango_layout_new(pango_context);
			pango_layout_set_font_description(pango_layout, notebook_normal_font);
			pango_cairo_update_layout(cr, pango_layout);
			pango_layout_set_text(pango_layout, item.label.c_str(), -1);
			pango_layout_set_wrap(pango_layout, PANGO_WRAP_CHAR);
			pango_layout_set_ellipsize(pango_layout, PANGO_ELLIPSIZE_END);
			pango_layout_set_width(pango_layout, btext.w * PANGO_SCALE);
			pango_cairo_layout_path(cr, pango_layout);
			g_object_unref(pango_layout);
		}

		CHECK_CAIRO(cairo_set_line_width(cr, 3.0));
		CHECK_CAIRO(cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND));
		CHECK_CAIRO(cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL));
		CHECK_CAIRO(cairo_set_source_color_alpha(cr, notebook_normal_outline_color));

		CHECK_CAIRO(cairo_stroke_preserve(cr));

		CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
		CHECK_CAIRO(cairo_set_source_color_alpha(cr, notebook_normal_text_color));
		CHECK_CAIRO(cairo_fill(cr));
	}

	cairo_restore(cr);

}

void simple2_theme_t::cairo_rounded_tab3(cairo_t * cr, double x, double y, double w, double h, double radius) {

	CHECK_CAIRO(cairo_new_path(cr));
	CHECK_CAIRO(cairo_move_to(cr, x, y + radius));
	CHECK_CAIRO(cairo_arc(cr, x + radius, y + radius, radius, 2.0 * M_PI_2, 3.0 * M_PI_2));
	CHECK_CAIRO(cairo_line_to(cr, x + w - radius, y));
	CHECK_CAIRO(cairo_arc(cr, x + w - radius, y + radius, radius, 3.0 * M_PI_2, 4.0 * M_PI_2));

}

shared_ptr<pixmap_t> simple2_theme_t::get_background() const {
	return backgroun_px;
}

color_t const & simple2_theme_t::get_focused_color() const {
	return notebook_active_background_color;
}

color_t const & simple2_theme_t::get_selected_color() const {
	return notebook_selected_background_color;
}

color_t const & simple2_theme_t::get_normal_color() const {
	return notebook_normal_background_color;
}

color_t const & simple2_theme_t::get_mouse_over_color() const {
	return notebook_mouse_over_background_color;
}

void simple2_theme_t::render_iconic_notebook(cairo_t * cr, vector<theme_tab_t> const & tabs) const {
	for (auto const & i : tabs) {
		render_notebook_normal(cr, i,
				notebook_normal_font,
				notebook_normal_text_color,
				notebook_normal_outline_color,
				notebook_normal_border_color,
				i.tab_color);
	}
}


}

