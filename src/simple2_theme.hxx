/*
 * simple_theme.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef SIMPLE2_THEME_HXX_
#define SIMPLE2_THEME_HXX_

#include "config.hxx"

#include <stdexcept>
#include <algorithm>
#include <cmath>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo-ft.h>

#ifdef WITH_PANGO
#include <pango/pangocairo.h>
#endif

#include "theme.hxx"
#include "color.hxx"
#include "box.hxx"
#include "config_handler.hxx"

using namespace std;

namespace page {

class simple2_theme_t : public theme_t {
public:

	display_t * _cnx;

	string notebook_active_font_name;
	string notebook_selected_font_name;
	string notebook_normal_font_name;

	string floating_active_font_name;
	string floating_normal_font_name;

	string pango_popup_font_name;

	PangoFontDescription * notebook_active_font;
	PangoFontDescription * notebook_selected_font;
	PangoFontDescription * notebook_normal_font;

	PangoFontDescription * floating_active_font;
	PangoFontDescription * floating_normal_font;

	PangoFontDescription * pango_popup_font;

	PangoFontMap * pango_font_map;
	PangoContext * pango_context;


	cairo_surface_t * vsplit_button_s;
	cairo_surface_t * hsplit_button_s;
	cairo_surface_t * close_button_s;
	cairo_surface_t * close_button1_s;
	cairo_surface_t * pop_button_s;
	cairo_surface_t * pops_button_s;
	cairo_surface_t * unbind_button_s;
	cairo_surface_t * bind_button_s;

	cairo_surface_t * background_s;

	color_t default_background_color;

	color_t popup_text_color;
	color_t popup_outline_color;
	color_t popup_background_color;

	color_t notebook_active_text_color;
	color_t notebook_active_outline_color;
	color_t notebook_active_border_color;
	color_t notebook_active_background_color;

	color_t notebook_selected_text_color;
	color_t notebook_selected_outline_color;
	color_t notebook_selected_border_color;
	color_t notebook_selected_background_color;

	color_t notebook_normal_text_color;
	color_t notebook_normal_outline_color;
	color_t notebook_normal_border_color;
	color_t notebook_normal_background_color;

	color_t floating_active_text_color;
	color_t floating_active_outline_color;
	color_t floating_active_border_color;
	color_t floating_active_background_color;

	color_t floating_normal_text_color;
	color_t floating_normal_outline_color;
	color_t floating_normal_border_color;
	color_t floating_normal_background_color;

	bool has_background;
	string background_file;
	string scale_mode;


	simple2_theme_t(display_t * cnx, config_handler_t & conf);

	virtual ~simple2_theme_t();

	rectangle compute_notebook_close_window_position(
			rectangle const & allocation, int number_of_client,
			int selected_client_index) const;
	rectangle compute_notebook_unbind_window_position(
			rectangle const & allocation, int number_of_client,
			int selected_client_index) const;

	rectangle compute_notebook_bookmark_position(
			rectangle const & allocation) const;
	rectangle compute_notebook_vsplit_position(
			rectangle const & allocation) const;
	rectangle compute_notebook_hsplit_position(
			rectangle const & allocation) const;
	rectangle compute_notebook_close_position(
			rectangle const & allocation) const;

	rectangle compute_floating_close_position(
			rectangle const & _allocation) const;
	rectangle compute_floating_bind_position(
			rectangle const & _allocation) const;

	static void rounded_rectangle(cairo_t * cr, double x, double y, double w,
			double h, double r);

	void create_background_img();

	virtual void render_notebook(cairo_t * cr, notebook_base_t const * n, rectangle const & area) const;
	virtual void render_split(cairo_t * cr, split_base_t const * s, rectangle const & area) const;
	virtual void render_floating(managed_window_base_t * mw) const;


	void render_notebook_selected(
			cairo_t * cr,
			page_event_t const & data,
			PangoFontDescription const * pango_font,
			color_t const & text_color,
			color_t const & outline_color,
			color_t const & border_color,
			color_t const & background_color,
			double border_width
	) const;

	void render_notebook_normal(
			cairo_t * cr,
			page_event_t const & data,
			PangoFontDescription const * pango_font,
			color_t const & text_color,
			color_t const & outline_color,
			color_t const & border_color,
			color_t const & background_color
	) const;

	void render_floating_base(
			managed_window_base_t * mw,
			PangoFontDescription const * pango_font,
			color_t const & text_color,
			color_t const & outline_color,
			color_t const & border_color,
			color_t const & background_color,
			double border_width
	) const;

	virtual void render_popup_notebook0(cairo_t * cr, window_icon_handler_t * icon, unsigned int width,
			unsigned int height, string const & title);
	virtual void render_popup_move_frame(cairo_t * cr, window_icon_handler_t * icon, unsigned int width,
			unsigned int height, string const & title);

	virtual void render_popup_split(cairo_t * cr, split_base_t const * s, double current_split);

	virtual vector<page_event_t> * compute_page_areas(
			list<tree_t const *> const & page) const;
	virtual vector<floating_event_t> * compute_floating_areas(
			managed_window_base_t * mw) const;


	void compute_areas_for_notebook(notebook_base_t const * n,
			vector<page_event_t> * l) const;

	void draw_hatched_rectangle(cairo_t * cr, int space, int x, int y, int w,
			int h) const;

	void update();

	static void cairo_rounded_tab(cairo_t * cr, double x, double y, double w, double h, double radius);


	rectangle compute_split_bar_location(split_base_t const * s) const {

		rectangle ret;
		rectangle const & alloc = s->allocation();

		if (s->type() == VERTICAL_SPLIT) {

			int w = alloc.w - 2 * split_margin.left
					- 2 * split_margin.right - split_width;
			int w0 = floor(w * s->split() + 0.5);

			ret.x = alloc.x + split_margin.left + w0
					+ split_margin.right;
			ret.y = alloc.y;

		} else {

			int h = alloc.h - 2 * split_margin.top
					- 2 * split_margin.bottom - split_width;
			int h0 = floor(h * s->split() + 0.5);

			ret.x = alloc.x;
			ret.y = alloc.y + split_margin.top + h0
					+ split_margin.bottom;
		}

		if (s->type() == VERTICAL_SPLIT) {
			ret.w = split_width;
			ret.h = alloc.h;
		} else {
			ret.w = alloc.w;
			ret.h = split_width;
		}

		return ret;

	}



};

}

// TODO
//extern "C" page::theme_t * get_theme(GKeyFile * conf);


#endif /* SIMPLE_THEME_HXX_ */
