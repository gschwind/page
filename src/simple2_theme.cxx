/*
 * simple_theme.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#include "simple2_theme.hxx"
#include "box.hxx"
#include "color.hxx"
#include <string>
#include <algorithm>
#include <iostream>

#include <viewport_base.hxx>

using namespace std;

namespace page {

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


inline void cairo_set_source_rgba(cairo_t * cr, color_t const & color) {
	CHECK_CAIRO(::cairo_set_source_rgba(cr, color.r, color.g, color.b, color.a));
}

inline void cairo_set_source_rgb(cairo_t * cr, color_t const & color) {
	CHECK_CAIRO(::cairo_set_source_rgba(cr, color.r, color.g, color.b, 1.0));
}

i_rect simple2_theme_t::compute_notebook_bookmark_position(
		i_rect const & allocation) const {
	return i_rect(allocation.x + allocation.w - 4 * 17 - 2, allocation.y + 2,
			16, 16).floor();
}

i_rect simple2_theme_t::compute_notebook_vsplit_position(
		i_rect const & allocation) const {

	return i_rect(allocation.x + allocation.w - 3 * 17 - 2, allocation.y + 2,
			16, 16).floor();
}

i_rect simple2_theme_t::compute_notebook_hsplit_position(
		i_rect const & allocation) const {

	return i_rect(allocation.x + allocation.w - 2 * 17 - 2, allocation.y + 2,
			16, 16).floor();

}

i_rect simple2_theme_t::compute_notebook_close_position(
		i_rect const & allocation) const {

	return i_rect(allocation.x + allocation.w - 1 * 17 - 2, allocation.y + 2,
			16, 16).floor();
}

i_rect simple2_theme_t::compute_notebook_menu_position(
		i_rect const & allocation) const {

	return i_rect(allocation.x, allocation.y,
			40, 20).floor();

}


simple2_theme_t::simple2_theme_t(display_t * cnx, config_handler_t & conf) {

	notebook.margin.top = 22;
	notebook.margin.bottom = 4;
	notebook.margin.left = 4;
	notebook.margin.right = 4;

	notebook.tab_height = 22;
	notebook.selected_close_width = 35;
	notebook.selected_unbind_width = 20;

	notebook.menu_button_width;
	notebook.close_width = 17;
	notebook.hsplit_width = 17;
	notebook.vsplit_width = 17;
	notebook.mark_width = 17;

	split.margin.top = 0;
	split.margin.bottom = 0;
	split.margin.left = 0;
	split.margin.right = 0;

	floating.margin.top = 28;
	floating.margin.bottom = 6;
	floating.margin.left = 6;
	floating.margin.right = 6;
	floating.close_width = 35;
	floating.bind_width = 20;


	split.margin.top = 0;
	split.margin.bottom = 0;
	split.margin.left = 0;
	split.margin.right = 0;
	split.width = 10;



	_cnx = cnx;

	string conf_img_dir = conf.get_string("default", "theme_dir");

	default_background_color.set(
			conf.get_string("simple_theme", "default_background_color"));

	popup_text_color.set(conf.get_string("simple_theme", "popup_text_color"));
	popup_outline_color.set(
			conf.get_string("simple_theme", "popup_outline_color"));
	popup_background_color.set(
			conf.get_string("simple_theme", "popup_background_color"));

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

	background_s = nullptr;

	vsplit_button_s = nullptr;
	hsplit_button_s = nullptr;
	close_button_s = nullptr;
	close_button1_s = nullptr;
	pop_button_s = nullptr;
	pops_button_s = nullptr;
	unbind_button_s = nullptr;
	bind_button_s = nullptr;

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
		std::string filename = conf_img_dir + "/window-close-1.png";
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
		std::string filename = conf_img_dir + "/pop_selected.png";
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


	notebook_active_font_name = conf.get_string("simple_theme", "notebook_active_font").c_str();
	notebook_selected_font_name = conf.get_string("simple_theme", "notebook_selected_font").c_str();
	notebook_normal_font_name = conf.get_string("simple_theme", "notebook_normal_font").c_str();

	floating_active_font_name = conf.get_string("simple_theme", "floating_active_font").c_str();
	floating_normal_font_name = conf.get_string("simple_theme", "floating_normal_font").c_str();

	pango_popup_font_name = conf.get_string("simple_theme", "pango_popup_font").c_str();

	notebook_active_font = pango_font_description_from_string(notebook_active_font_name.c_str());
	notebook_selected_font = pango_font_description_from_string(notebook_selected_font_name.c_str());
	notebook_normal_font = pango_font_description_from_string(notebook_normal_font_name.c_str());

	floating_active_font = pango_font_description_from_string(floating_active_font_name.c_str());
	floating_normal_font = pango_font_description_from_string(floating_normal_font_name.c_str());

	pango_popup_font = pango_font_description_from_string(pango_popup_font_name.c_str());

	pango_font_map = pango_cairo_font_map_new();
	pango_context = pango_font_map_create_context(pango_font_map);


}

simple2_theme_t::~simple2_theme_t() {

	if(background_s != nullptr) {
		warn(cairo_surface_get_reference_count(background_s) == 1);
		cairo_surface_destroy(background_s);
		background_s = nullptr;
		XFreePixmap(_cnx->dpy(), background_p);
	}
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

	pango_font_description_free(notebook_active_font);
	pango_font_description_free(notebook_selected_font);
	pango_font_description_free(notebook_normal_font);
	pango_font_description_free(floating_active_font);
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

void simple2_theme_t::render_notebook(cairo_t * cr, theme_notebook_t const * n,
		i_rect const & area) const {

	CHECK_CAIRO(cairo_save(cr));

	CHECK_CAIRO(cairo_reset_clip(cr));
	CHECK_CAIRO(cairo_identity_matrix(cr));
	CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
	CHECK_CAIRO(cairo_new_path(cr));

	/** hope to optimize things **/
	CHECK_CAIRO(cairo_rectangle(cr, area.x, area.y, area.w, area.h));
	CHECK_CAIRO(cairo_clip(cr));

	/** draw background **/
	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
	CHECK_CAIRO(cairo_rectangle(cr, n->allocation.x, n->allocation.y, n->allocation.w,
			n->allocation.h));
	if (background_s != nullptr) {
		CHECK_CAIRO(cairo_set_source_surface(cr, background_s, 0.0, 0.0));
	} else {
		CHECK_CAIRO(cairo_set_source_rgba(cr, default_background_color));
	}
	CHECK_CAIRO(cairo_fill(cr));

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));

	for (auto &i : n->clients_tab) {

		/** skip out of scope boxes **/
		if (i.selected) {

			if (i.focuced) {
				render_notebook_selected(cr, *n, i,
						notebook_active_font,
						notebook_active_text_color,
						notebook_active_outline_color,
						notebook_active_border_color,
						notebook_active_background_color, 1.0);
			} else {
				render_notebook_selected(cr, *n, i,
						notebook_selected_font,
						notebook_selected_text_color,
						notebook_selected_outline_color,
						notebook_selected_border_color,
						notebook_selected_background_color, 1.0);
			}

		} else {
			render_notebook_normal(cr, i,
					notebook_normal_font,
					notebook_normal_text_color,
					notebook_normal_outline_color,
					notebook_normal_border_color,
					notebook_normal_background_color);
		}

	}

	CHECK_CAIRO(cairo_reset_clip(cr));
	CHECK_CAIRO(cairo_identity_matrix(cr));
	CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));

	{

		cairo_save(cr);
		i_rect btext = compute_notebook_menu_position(n->allocation);

		char buf[32];
		snprintf(buf, 32, "%lu", n->clients_tab.size());

#ifdef WITH_PANGO

		/* draw title */
		CHECK_CAIRO(cairo_set_source_rgba(cr, notebook_normal_outline_color));

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
		CHECK_CAIRO(cairo_set_source_rgba(cr, notebook_selected_text_color));
		CHECK_CAIRO(cairo_fill(cr));

#endif

		cairo_restore(cr);

	}


	{
		i_rect b = compute_notebook_bookmark_position(n->allocation);

		CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
		CHECK_CAIRO(cairo_set_source_rgb(cr, notebook_normal_background_color));
		CHECK_CAIRO(cairo_fill(cr));

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
		i_rect b = compute_notebook_vsplit_position(n->allocation);

		CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
		CHECK_CAIRO(cairo_set_source_rgb(cr, notebook_normal_background_color));
		CHECK_CAIRO(cairo_fill(cr));

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		//CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
		CHECK_CAIRO(cairo_set_source_surface(cr, vsplit_button_s, b.x, b.y));
		CHECK_CAIRO(cairo_mask_surface(cr, vsplit_button_s, b.x, b.y));
	}

	{
		i_rect b = compute_notebook_hsplit_position(n->allocation);

		CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
		CHECK_CAIRO(cairo_set_source_rgb(cr, notebook_normal_background_color));
		CHECK_CAIRO(cairo_fill(cr));

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		//CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
		CHECK_CAIRO(cairo_set_source_surface(cr, hsplit_button_s, b.x, b.y));
		CHECK_CAIRO(cairo_mask_surface(cr, hsplit_button_s, b.x, b.y));
	}

	{
		i_rect b = compute_notebook_close_position(n->allocation);

		CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
		CHECK_CAIRO(cairo_set_source_rgb(cr, notebook_normal_background_color));
		CHECK_CAIRO(cairo_fill(cr));

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		//CHECK_CAIRO(cairo_rectangle(cr, b.x, b.y, b.w, b.h));
		CHECK_CAIRO(cairo_set_source_surface(cr, close_button_s, b.x, b.y));
		CHECK_CAIRO(cairo_mask_surface(cr, close_button_s, b.x, b.y));
	}

	CHECK_CAIRO(cairo_restore(cr));

}


static void draw_notebook_border(cairo_t * cr, i_rect const & alloc,
		i_rect const & tab_area, color_t const & color,
		double border_width) {

	double half = border_width / 2.0;

	CHECK_CAIRO(simple2_theme_t::cairo_rounded_tab(cr, tab_area.x + 0.5, tab_area.y + 0.5, tab_area.w - 1.0, tab_area.h - 1.0, 7.0));

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
	CHECK_CAIRO(cairo_set_source_rgba(cr, color));
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

	i_rect tab_area = data.position;

	tab_area.x += 2;
	tab_area.w -= 4;
	tab_area.y += 0;
	tab_area.h -= 1;

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
	CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT));

	/** draw the tab background **/
	i_rect b = tab_area;

	CHECK_CAIRO(cairo_save(cr));

	/** clip tabs **/
	CHECK_CAIRO(cairo_rounded_tab2(cr, b.x, b.y, b.w, b.h+30.0, 6.0));
	CHECK_CAIRO(cairo_clip(cr));

	CHECK_CAIRO(cairo_set_source_rgba(cr, background_color));
	cairo_pattern_t * gradient;
	gradient = cairo_pattern_create_linear(0.0, b.y-2.0, 0.0, b.y+b.h+20.0);
	CHECK_CAIRO(cairo_pattern_add_color_stop_rgba(gradient, 0.0, 1.0, 1.0, 1.0, 1.0));
	CHECK_CAIRO(cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 1.0, 1.0, 0.0));
	CHECK_CAIRO(cairo_mask(cr, gradient));
	cairo_pattern_destroy(gradient);

	i_rect xncclose;
	xncclose.x = b.x + b.w - notebook.selected_close_width;
	xncclose.y = b.y;
	xncclose.w = 35;
	xncclose.h = b.h-3;

	CHECK_CAIRO(cairo_rectangle(cr, xncclose.x, xncclose.y, xncclose.w, xncclose.h+0.5));
	CHECK_CAIRO(::cairo_set_source_rgb(cr, 0xcc/255.0, 0x44/255.0, 0.0));
	CHECK_CAIRO(cairo_fill(cr));

	double radius = 6.5;
	CHECK_CAIRO(cairo_new_path(cr));
	CHECK_CAIRO(cairo_move_to(cr, xncclose.x+0.5, xncclose.y+0.5));
	CHECK_CAIRO(cairo_line_to(cr, xncclose.x+0.5, xncclose.y+0.5 + xncclose.h-1.0));
	CHECK_CAIRO(cairo_line_to(cr, xncclose.x+xncclose.w+0.5, xncclose.y+0.5 + xncclose.h-1.0));

	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_stroke(cr);

	radius = 5.5;
	CHECK_CAIRO(cairo_new_path(cr));
	CHECK_CAIRO(cairo_move_to(cr, xncclose.x+1.5, xncclose.y-1.5 + xncclose.h));
	CHECK_CAIRO(cairo_line_to(cr, xncclose.x+1.5, xncclose.y+.5));
	CHECK_CAIRO(cairo_line_to(cr, xncclose.x + xncclose.w -0.5 - radius, xncclose.y+.5));
	CHECK_CAIRO(cairo_arc(cr, xncclose.x + xncclose.w - 0.5 - radius, xncclose.y+.5 + radius, radius, 3.0 * M_PI_2, 4.0 * M_PI_2));
	CHECK_CAIRO(cairo_line_to(cr, xncclose.x + xncclose.w -0.5, xncclose.y + xncclose.h - 1.5));
	CHECK_CAIRO(cairo_close_path(cr));
	::cairo_set_source_rgb(cr, 0.9, 0.4, 0.3);
	cairo_stroke(cr);

	CHECK_CAIRO(cairo_restore(cr));
	CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT));

	/** draw application icon **/
	i_rect bicon = tab_area;
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

#ifdef WITH_PANGO
	/** draw application title **/
	i_rect btext = tab_area;
	btext.h -= 0;
	btext.w -= 3 * 16 + 12 + 30;
	btext.x += 7 + 16 + 6;
	btext.y += 1;

	/** draw title **/
	CHECK_CAIRO(cairo_translate(cr, btext.x + 2, btext.y));

	{

		{
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
		CHECK_CAIRO(cairo_set_source_rgba(cr, outline_color));

		CHECK_CAIRO(cairo_stroke_preserve(cr));

		CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
		CHECK_CAIRO(cairo_set_source_rgba(cr, text_color));
		CHECK_CAIRO(cairo_fill(cr));
	}

#endif

	CHECK_CAIRO(cairo_identity_matrix(cr));

	/** draw close button **/

	i_rect ncclose;
	ncclose.x = tab_area.x + tab_area.w - notebook.selected_close_width + 10;
	ncclose.y = tab_area.y;
	ncclose.w = 16;
	ncclose.h = 16;

	CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
	//CHECK_CAIRO(cairo_rectangle(cr, ncclose.x, ncclose.y, ncclose.w, ncclose.h));
	CHECK_CAIRO(cairo_set_source_surface(cr, close_button1_s, ncclose.x, ncclose.y));
	CHECK_CAIRO(cairo_mask_surface(cr, close_button1_s, ncclose.x, ncclose.y));

	/** draw unbind button **/
	i_rect ncub;
	ncub.x = tab_area.x + tab_area.w - notebook.selected_unbind_width
			- notebook.selected_close_width;
	ncub.y = tab_area.y;
	ncub.w = 16;
	ncub.h = 16;

	CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
	//CHECK_CAIRO(cairo_rectangle(cr, ncub.x, ncub.y, ncub.w, ncub.h));
	CHECK_CAIRO(cairo_set_source_surface(cr, unbind_button_s, ncub.x, ncub.y));
	CHECK_CAIRO(cairo_mask_surface(cr, unbind_button_s, ncub.x, ncub.y));

	i_rect area = data.position;
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

	i_rect tab_area = data.position;
	tab_area.x += 2;
	tab_area.w -= 4;
	tab_area.y += 1;
	tab_area.h -= 1;

	CHECK_CAIRO(cairo_save(cr));

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
	CHECK_CAIRO(cairo_set_line_width(cr, 1.0));

	/** draw the tab background **/
	i_rect b = tab_area;
	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
	CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT));

	/** clip tabs **/
	CHECK_CAIRO(cairo_rounded_tab2(cr, b.x, b.y, b.w, b.h+1.0, 6.0));
	CHECK_CAIRO(cairo_clip(cr));

	CHECK_CAIRO(cairo_set_source_rgba(cr, background_color));
	cairo_pattern_t * gradient;
	gradient = cairo_pattern_create_linear(0.0, b.y-2.0, 0.0, b.y+b.h+20.0);
	CHECK_CAIRO(cairo_pattern_add_color_stop_rgba(gradient, 0.0, 1.0, 1.0, 1.0, 1.0));
	CHECK_CAIRO(cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 1.0, 1.0, 0.0));
	CHECK_CAIRO(cairo_mask(cr, gradient));
	cairo_pattern_destroy(gradient);

	CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT));

	i_rect bicon = tab_area;
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

	i_rect btext = tab_area;
	btext.h -= 0;
	btext.w -= 1 * 16 + 12;
	btext.x += 3 + 16 + 6;
	btext.y += 1;

	CHECK_CAIRO(cairo_save(cr));

#ifdef WITH_PANGO

	/* draw title */
	CHECK_CAIRO(cairo_set_source_rgba(cr, outline_color));

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
	CHECK_CAIRO(cairo_set_source_rgba(cr, text_color));
	CHECK_CAIRO(cairo_fill(cr));

#endif

	CHECK_CAIRO(cairo_restore(cr));
	CHECK_CAIRO(cairo_restore(cr));

}

void simple2_theme_t::render_split(cairo_t * cr,
		theme_split_t const * s,
		i_rect const & area) const {

	CHECK_CAIRO(cairo_save(cr));

	CHECK_CAIRO(cairo_reset_clip(cr));
	CHECK_CAIRO(cairo_identity_matrix(cr));
	CHECK_CAIRO(cairo_set_line_width(cr, 1.0));

	CHECK_CAIRO(cairo_rectangle(cr, area.x, area.y, area.w, area.h));
	CHECK_CAIRO(cairo_clip(cr));

	i_rect sarea = s->allocation;
	if (background_s != nullptr) {
		CHECK_CAIRO(cairo_set_source_surface(cr, background_s, 0.0, 0.0));
	} else {
		CHECK_CAIRO(cairo_set_source_rgba(cr, default_background_color));
	}
	CHECK_CAIRO(cairo_rectangle(cr, sarea.x, sarea.y, sarea.w, sarea.h));
	CHECK_CAIRO(cairo_fill(cr));

	i_rect grip0;
	i_rect grip1;
	i_rect grip2;

	if (sarea.w > sarea.h) {
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

	CHECK_CAIRO(::cairo_set_source_rgb(cr, 1.0, 0.0, 0.0));
	CHECK_CAIRO(cairo_rectangle(cr, grip0.x, grip0.y, grip0.w, grip0.h));
	CHECK_CAIRO(cairo_fill(cr));
	CHECK_CAIRO(::cairo_set_source_rgb(cr, 1.0, 0.0, 0.0));
	CHECK_CAIRO(cairo_rectangle(cr, grip1.x, grip1.y, grip1.w, grip1.h));
	CHECK_CAIRO(cairo_fill(cr));
	CHECK_CAIRO(::cairo_set_source_rgb(cr, 1.0, 0.0, 0.0));
	CHECK_CAIRO(cairo_rectangle(cr, grip2.x, grip2.y, grip2.w, grip2.h));
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

	i_rect allocation = mw->position;

	if (mw->cairo_top != nullptr) {
		cairo_t * cr = mw->cairo_top;

		i_rect tab_area(0, 0, allocation.w, floating.margin.top);

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
		::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
		cairo_rectangle(cr, tab_area.x, tab_area.y, tab_area.w, tab_area.h);
		cairo_fill(cr);

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
		CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE));

		/** draw the tab background **/
		i_rect b = tab_area;

		CHECK_CAIRO(cairo_save(cr));

		/** clip tabs **/
		CHECK_CAIRO(cairo_rounded_tab(cr, b.x, b.y, b.w, b.h+30.0, 8.0));
		CHECK_CAIRO(cairo_clip(cr));

		CHECK_CAIRO(cairo_set_source_rgba(cr, background_color));
		cairo_pattern_t * gradient;
		gradient = cairo_pattern_create_linear(0.0, b.y-2.0, 0.0, b.y+b.h);
		CHECK_CAIRO(cairo_pattern_add_color_stop_rgba(gradient, 0.0, 1.0, 1.0, 1.0, (background_color.a)));
		CHECK_CAIRO(cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 1.0, 1.0, (background_color.a)*0.5));
		CHECK_CAIRO(cairo_mask(cr, gradient));
		cairo_pattern_destroy(gradient);

		/* draw black outline */
		CHECK_CAIRO(cairo_rounded_tab(cr, b.x+0.5, b.y+0.5, b.w-1.0, b.h+30.0, 7.5));
		cairo_set_line_width(cr, 1.0);
		::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		CHECK_CAIRO(cairo_stroke(cr));

		i_rect xncclose;
		xncclose.x = b.x + b.w - floating.close_width;
		xncclose.y = b.y;
		xncclose.w = 35;
		xncclose.h = notebook.margin.top-3;

		CHECK_CAIRO(cairo_rectangle(cr, xncclose.x, xncclose.y, xncclose.w, xncclose.h+0.5));
		CHECK_CAIRO(::cairo_set_source_rgb(cr, 0xcc/255.0, 0x44/255.0, 0.0));
		CHECK_CAIRO(cairo_fill(cr));

		double radius = 6.5;
		CHECK_CAIRO(cairo_new_path(cr));
		CHECK_CAIRO(cairo_move_to(cr, xncclose.x+0.5, xncclose.y+0.5 + xncclose.h-1.0));
		CHECK_CAIRO(cairo_line_to(cr, xncclose.x+0.5, xncclose.y+0.5));
		CHECK_CAIRO(cairo_line_to(cr, xncclose.x + xncclose.w -.5 - radius, xncclose.y+0.5));
		CHECK_CAIRO(cairo_arc(cr, xncclose.x + xncclose.w -.5 - radius, xncclose.y+0.5 + radius, radius, 3.0 * M_PI_2, 4.0 * M_PI_2));
		CHECK_CAIRO(cairo_line_to(cr, xncclose.x + xncclose.w -.5, xncclose.y + xncclose.h -.5));
		CHECK_CAIRO(cairo_close_path(cr));

		::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_stroke(cr);

		radius = 6.5;
		CHECK_CAIRO(cairo_new_path(cr));
		CHECK_CAIRO(cairo_move_to(cr, xncclose.x+1.5, xncclose.y-1.5 + xncclose.h));
		CHECK_CAIRO(cairo_line_to(cr, xncclose.x+1.5, xncclose.y+.5));
		CHECK_CAIRO(cairo_line_to(cr, xncclose.x + xncclose.w -0.5 - radius, xncclose.y+.5));
		CHECK_CAIRO(cairo_arc(cr, xncclose.x + xncclose.w - 0.5 - radius, xncclose.y+.5 + radius, radius, 3.0 * M_PI_2, 4.0 * M_PI_2));
		CHECK_CAIRO(cairo_line_to(cr, xncclose.x + xncclose.w -0.5, xncclose.y + xncclose.h -1.5));
		CHECK_CAIRO(cairo_close_path(cr));
		::cairo_set_source_rgb(cr, 0.9, 0.4, 0.3);
		cairo_stroke(cr);

		CHECK_CAIRO(cairo_restore(cr));
		CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT));

		/** draw application icon **/
		i_rect bicon = tab_area;
		bicon.h = 16;
		bicon.w = 16;
		bicon.x += 10;
		bicon.y += floating.margin.top - 22;

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

	#ifdef WITH_PANGO
		/** draw application title **/
		i_rect btext = tab_area;
		btext.h -= 0;
		btext.w -= 3 * 16 + 12 + 30;
		btext.x += 7 + 16 + 6;
		btext.y += floating.margin.top - 23;

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
			CHECK_CAIRO(cairo_set_source_rgba(cr, outline_color));

			CHECK_CAIRO(cairo_stroke_preserve(cr));

			CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
			CHECK_CAIRO(cairo_set_source_rgba(cr, text_color));
			CHECK_CAIRO(cairo_fill(cr));
		}

	#endif

		CHECK_CAIRO(cairo_identity_matrix(cr));

		/** draw close button **/

		i_rect ncclose;
		ncclose.x = tab_area.x + tab_area.w - floating.close_width + 10;
		ncclose.y = tab_area.y;
		ncclose.w = 16;
		ncclose.h = 16;

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		//CHECK_CAIRO(cairo_rectangle(cr, ncclose.x, ncclose.y, ncclose.w, ncclose.h));
		CHECK_CAIRO(cairo_set_source_surface(cr, close_button1_s, ncclose.x, ncclose.y));
		CHECK_CAIRO(cairo_mask_surface(cr, close_button1_s, ncclose.x, ncclose.y));

		/** draw unbind button **/
		i_rect ncub;
		ncub.x = tab_area.x + tab_area.w - floating.bind_width - floating.close_width;
		ncub.y = tab_area.y;
		ncub.w = 16;
		ncub.h = 16;

		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		//CHECK_CAIRO(cairo_rectangle(cr, ncub.x, ncub.y, ncub.w, ncub.h));
		CHECK_CAIRO(cairo_set_source_surface(cr, bind_button_s, ncub.x, ncub.y));
		CHECK_CAIRO(cairo_mask_surface(cr, bind_button_s, ncub.x, ncub.y));

		warn(cairo_get_reference_count(cr) == 1);
		cairo_destroy(cr);
	}

	if (mw->cairo_bottom != nullptr) {
		cairo_t * cr = mw->cairo_bottom;

		i_rect b(0, 0, allocation.w, floating.margin.bottom);

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
		::cairo_set_source_rgba(cr, background_color.r, background_color.g, background_color.b, (background_color.a)*0.5);
		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_fill(cr);

		::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_new_path(cr);
		cairo_move_to(cr,b.x, b.y+b.h-0.5);
		cairo_line_to(cr, b.x+b.w, b.y+b.h-0.5);
		cairo_set_line_width(cr, 1.0);
		cairo_stroke(cr);

		::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_new_path(cr);
		cairo_move_to(cr,b.x+0.5, b.y);
		cairo_line_to(cr, b.x+0.5, b.y+b.h);
		cairo_set_line_width(cr, 1.0);
		cairo_stroke(cr);

		::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_new_path(cr);
		cairo_move_to(cr,b.x+b.w-0.5, b.y);
		cairo_line_to(cr, b.x+b.w-0.5, b.y+b.h);
		cairo_set_line_width(cr, 1.0);
		cairo_stroke(cr);

		warn(cairo_get_reference_count(cr) == 1);
		cairo_destroy(cr);

	}

	if (mw->cairo_right != nullptr) {
		cairo_t * cr = mw->cairo_right;

		i_rect b(0, 0, floating.margin.right,
				allocation.h - floating.margin.top
						- floating.margin.bottom);

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
		::cairo_set_source_rgba(cr, background_color.r, background_color.g, background_color.b, (background_color.a)*0.5);
		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_fill(cr);

		::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_new_path(cr);
		cairo_move_to(cr,b.x+b.w-0.5, b.y);
		cairo_line_to(cr, b.x+b.w-0.5, b.y+b.h);
		cairo_set_line_width(cr, 1.0);
		cairo_stroke(cr);

		warn(cairo_get_reference_count(cr) == 1);
		cairo_destroy(cr);
	}

	if (mw->cairo_left != nullptr) {
		cairo_t * cr = mw->cairo_left;

		i_rect b(0, 0, floating.margin.left,
				allocation.h - floating.margin.top
						- floating.margin.bottom);

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		::cairo_set_source_rgba(cr, background_color.r, background_color.g, background_color.b, (background_color.a)*0.5);
		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_fill(cr);

		::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_new_path(cr);
		cairo_move_to(cr,b.x+0.5, b.y);
		cairo_line_to(cr, b.x+0.5, b.y+b.h);
		cairo_set_line_width(cr, 1.0);
		cairo_stroke(cr);

		warn(cairo_get_reference_count(cr) == 1);
		cairo_destroy(cr);

	}

}

void simple2_theme_t::render_popup_notebook0(cairo_t * cr, icon64 * icon, unsigned int width,
		unsigned int height, string const & title) {


	CHECK_CAIRO(cairo_rectangle(cr, 1, 1, width - 2, height - 2));
	CHECK_CAIRO(cairo_clip(cr));
	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
	CHECK_CAIRO(cairo_set_source_rgba(cr, popup_background_color));
	cairo_pattern_t * p = cairo_pattern_create_rgba(1.0, 1.0, 1.0, 0.5);
	CHECK_CAIRO(cairo_mask(cr, p));
	cairo_pattern_destroy(p);
	cairo_reset_clip(cr);
	CHECK_CAIRO(cairo_set_source_rgba(cr, popup_background_color));
	//draw_hatched_i_rect(cr, 40, 1, 1, width - 2, height - 2);

	if (icon != nullptr) {
		double x_offset = (width - 64) / 2.0;
		double y_offset = (height - 64) / 2.0;

		CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
		CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
		CHECK_CAIRO(cairo_set_source_surface(cr, icon->get_cairo_surface(), x_offset, y_offset));
		CHECK_CAIRO(cairo_mask_surface(cr, icon->get_cairo_surface(), x_offset, y_offset));

#ifdef WITH_PANGO

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

		CHECK_CAIRO(cairo_set_source_rgba(cr, popup_outline_color));

		CHECK_CAIRO(cairo_stroke_preserve(cr));

		CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
		CHECK_CAIRO(cairo_set_source_rgba(cr, popup_text_color));
		CHECK_CAIRO(cairo_fill(cr));

#endif

	}

}

void simple2_theme_t::render_popup_move_frame(cairo_t * cr, icon64 * icon, unsigned int width,
		unsigned int height, string const & title) {
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

	if(background_s != nullptr) {
		warn(cairo_surface_get_reference_count(background_s) == 1);
		cairo_surface_destroy(background_s);
		background_s = nullptr;
	}

	create_background_img();

}

void simple2_theme_t::create_background_img() {

	if (has_background) {

		cairo_surface_t * tmp = cairo_image_surface_create_from_png(
				background_file.c_str());

		XWindowAttributes wa;
		XGetWindowAttributes(_cnx->dpy(), _cnx->root(), &wa);


		background_p = XCreatePixmap(_cnx->dpy(), _cnx->root(), wa.width, wa.height, wa.depth);

		background_s = cairo_xlib_surface_create(_cnx->dpy(), background_p, wa.visual, wa.width,
				wa.height);

		/**
		 * WARNING: transform order and set_source_surface have huge
		 * Consequence.
		 **/

		double src_width = cairo_image_surface_get_width(tmp);
		double src_height = cairo_image_surface_get_height(tmp);

		if (src_width > 0 and src_height > 0) {

			if (scale_mode == "stretch") {

				cairo_t * cr = cairo_create(background_s);

				CHECK_CAIRO(::cairo_set_source_rgb(cr, 0.5, 0.5, 0.5));
				CHECK_CAIRO(cairo_rectangle(cr, 0, 0, wa.width, wa.height));
				CHECK_CAIRO(cairo_fill(cr));

				double x_ratio = wa.width / src_width;
				double y_ratio = wa.height / src_height;
				CHECK_CAIRO(cairo_scale(cr, x_ratio, y_ratio));
				CHECK_CAIRO(cairo_set_source_surface(cr, tmp, 0, 0));
				CHECK_CAIRO(cairo_rectangle(cr, 0, 0, src_width, src_height));
				CHECK_CAIRO(cairo_fill(cr));

				warn(cairo_get_reference_count(cr) == 1);
				cairo_destroy(cr);

			} else if (scale_mode == "zoom") {

				cairo_t * cr = cairo_create(background_s);

				CHECK_CAIRO(::cairo_set_source_rgb(cr, 0.5, 0.5, 0.5));
				CHECK_CAIRO(cairo_rectangle(cr, 0, 0, wa.width, wa.height));
				CHECK_CAIRO(cairo_fill(cr));

				double x_ratio = wa.width / (double)src_width;
				double y_ratio = wa.height / (double)src_height;

				double x_offset;
				double y_offset;

				if (x_ratio > y_ratio) {

					double yp = wa.height / x_ratio;

					x_offset = 0;
					y_offset = (yp - src_height) / 2.0;

					CHECK_CAIRO(cairo_scale(cr, x_ratio, x_ratio));
					CHECK_CAIRO(cairo_set_source_surface(cr, tmp, x_offset, y_offset));
					CHECK_CAIRO(cairo_rectangle(cr, 0, 0, src_width, yp));
					CHECK_CAIRO(cairo_fill(cr));

				} else {

					double xp = wa.width / y_ratio;

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

				cairo_t * cr = cairo_create(background_s);

				CHECK_CAIRO(::cairo_set_source_rgb(cr, 0.5, 0.5, 0.5));
				CHECK_CAIRO(cairo_rectangle(cr, 0, 0, wa.width, wa.height));
				CHECK_CAIRO(cairo_fill(cr));

				double x_offset = (wa.width - src_width) / 2.0;
				double y_offset = (wa.height - src_height) / 2.0;

				CHECK_CAIRO(cairo_set_source_surface(cr, tmp, x_offset, y_offset));
				CHECK_CAIRO(cairo_rectangle(cr, max<double>(0.0, x_offset),
						max<double>(0.0, y_offset),
						min<double>(src_width, wa.width),
						min<double>(src_height, wa.height)));
				CHECK_CAIRO(cairo_fill(cr));

				warn(cairo_get_reference_count(cr) == 1);
				cairo_destroy(cr);

			} else if (scale_mode == "scale" || scale_mode == "span") {

				cairo_t * cr = cairo_create(background_s);

				CHECK_CAIRO(::cairo_set_source_rgb(cr, 0.5, 0.5, 0.5));
				CHECK_CAIRO(cairo_rectangle(cr, 0, 0, wa.width, wa.height));
				CHECK_CAIRO(cairo_fill(cr));

				double x_ratio = wa.width / src_width;
				double y_ratio = wa.height / src_height;

				double x_offset, y_offset;

				if (x_ratio < y_ratio) {

					double yp = wa.height / x_ratio;

					x_offset = 0;
					y_offset = (yp - src_height) / 2.0;

					CHECK_CAIRO(cairo_scale(cr, x_ratio, x_ratio));
					CHECK_CAIRO(cairo_set_source_surface(cr, tmp, x_offset, y_offset));
					CHECK_CAIRO(cairo_rectangle(cr, x_offset, y_offset, src_width, src_height));
					CHECK_CAIRO(cairo_fill(cr));

				} else {
					double xp = wa.width / y_ratio;

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

				cairo_t * cr = cairo_create(background_s);
				CHECK_CAIRO(::cairo_set_source_rgb(cr, 0.5, 0.5, 0.5));
				CHECK_CAIRO(cairo_rectangle(cr, 0, 0, wa.width, wa.height));
				CHECK_CAIRO(cairo_fill(cr));

				for (double x = 0; x < wa.width; x += src_width) {
					for (double y = 0; y < wa.height; y += src_height) {
						CHECK_CAIRO(cairo_identity_matrix(cr));
						CHECK_CAIRO(cairo_translate(cr, x, y));
						CHECK_CAIRO(cairo_set_source_surface(cr, tmp, 0, 0));
						CHECK_CAIRO(cairo_rectangle(cr, 0, 0, wa.width, wa.height));
						CHECK_CAIRO(cairo_fill(cr));
					}
				}

				warn(cairo_get_reference_count(cr) == 1);
				cairo_destroy(cr);

			}

		}

		warn(cairo_surface_get_reference_count(tmp) == 1);
		cairo_surface_destroy(tmp);

	} else {
		background_s = nullptr;
	}
}

void simple2_theme_t::render_popup_split(cairo_t * cr, theme_split_t const * s,
		double current_split) {

	cairo_save(cr);

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
	CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE));

	i_rect const & alloc = s->allocation;

	i_rect rect0;
	i_rect rect1;

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

void simple2_theme_t::render_menuentry(cairo_t * cr, dropdown_menu_entry_t * w, i_rect const & area) {

	cairo_save(cr);

	CHECK_CAIRO(cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT));

	/** draw application icon **/
	i_rect bicon = area;
	bicon.h = 16;
	bicon.w = 16;
	bicon.x += 5;
	bicon.y += 5;

	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_OVER));
	if ((w)->icon() != nullptr) {
		if ((w)->icon()->get_cairo_surface() != 0) {
			CHECK_CAIRO(::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
			CHECK_CAIRO(cairo_set_source_surface(cr, (w)->icon()->get_cairo_surface(),
					bicon.x, bicon.y));
			CHECK_CAIRO(cairo_mask_surface(cr, (w)->icon()->get_cairo_surface(),
					bicon.x, bicon.y));
		}
	}

#ifdef WITH_PANGO
	/** draw application title **/
	i_rect btext = area;
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
			pango_layout_set_text(pango_layout, (w)->title().c_str(), -1);
			pango_layout_set_wrap(pango_layout, PANGO_WRAP_CHAR);
			pango_layout_set_ellipsize(pango_layout, PANGO_ELLIPSIZE_END);
			pango_layout_set_width(pango_layout, btext.w * PANGO_SCALE);
			pango_cairo_layout_path(cr, pango_layout);
			g_object_unref(pango_layout);
		}

		CHECK_CAIRO(cairo_set_line_width(cr, 3.0));
		CHECK_CAIRO(cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND));
		CHECK_CAIRO(cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL));
		CHECK_CAIRO(cairo_set_source_rgba(cr, notebook_normal_outline_color));

		CHECK_CAIRO(cairo_stroke_preserve(cr));

		CHECK_CAIRO(cairo_set_line_width(cr, 1.0));
		CHECK_CAIRO(cairo_set_source_rgba(cr, notebook_normal_text_color));
		CHECK_CAIRO(cairo_fill(cr));
	}

#endif

	cairo_restore(cr);

}

void simple2_theme_t::cairo_rounded_tab(cairo_t * cr, double x, double y, double w, double h, double radius) {

	CHECK_CAIRO(cairo_new_path(cr));
	CHECK_CAIRO(cairo_move_to(cr, x, y + h));
	CHECK_CAIRO(cairo_line_to(cr, x, y + radius));
	CHECK_CAIRO(cairo_arc(cr, x + radius, y + radius, radius, 2.0 * M_PI_2, 3.0 * M_PI_2));
	CHECK_CAIRO(cairo_line_to(cr, x + w - radius, y));
	CHECK_CAIRO(cairo_arc(cr, x + w - radius, y + radius, radius, 3.0 * M_PI_2, 4.0 * M_PI_2));
	CHECK_CAIRO(cairo_line_to(cr, x + w, y + h));
	CHECK_CAIRO(cairo_close_path(cr));

}

void simple2_theme_t::cairo_rounded_tab2(cairo_t * cr, double x, double y, double w, double h, double radius) {

	CHECK_CAIRO(cairo_new_path(cr));
	CHECK_CAIRO(cairo_move_to(cr, x, y + h - radius));
	CHECK_CAIRO(cairo_line_to(cr, x, y + radius));
	CHECK_CAIRO(cairo_arc(cr, x + radius, y + radius, radius, 2.0 * M_PI_2, 3.0 * M_PI_2));
	CHECK_CAIRO(cairo_line_to(cr, x + w - radius, y));
	CHECK_CAIRO(cairo_arc(cr, x + w - radius, y + radius, radius, 3.0 * M_PI_2, 4.0 * M_PI_2));
	CHECK_CAIRO(cairo_line_to(cr, x + w, y + h - radius));
	CHECK_CAIRO(cairo_arc(cr, x + w - radius, y + h - radius, radius, 0.0 * M_PI_2, 1.0 * M_PI_2));
	CHECK_CAIRO(cairo_line_to(cr, x + radius, y + h));
	CHECK_CAIRO(cairo_arc(cr, x + radius, y + h - radius, radius, 1.0 * M_PI_2, 2.0 * M_PI_2));
	CHECK_CAIRO(cairo_close_path(cr));

}

}

