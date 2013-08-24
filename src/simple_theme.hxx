/*
 * simple_theme.hxx
 *
 *  Created on: 24 mars 2013
 *      Author: gschwind
 */

#ifndef SIMPLE_THEME_HXX_
#define SIMPLE_THEME_HXX_

#include <stdexcept>
#include <algorithm>
#include <cmath>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo-ft.h>

#include <pango/pangocairo.h>

#include "theme.hxx"
#include "color.hxx"
#include "box.hxx"
#include "config_handler.hxx"

using namespace std;

namespace page {

class simple_theme_t : public theme_t {
public:

	xconnection_t * _cnx;

	PangoFontDescription * pango_font;
	PangoFontDescription * pango_popup_font;

	cairo_surface_t * vsplit_button_s;
	cairo_surface_t * hsplit_button_s;
	cairo_surface_t * close_button_s;
	cairo_surface_t * pop_button_s;
	cairo_surface_t * pops_button_s;
	cairo_surface_t * unbind_button_s;
	cairo_surface_t * bind_button_s;

	cairo_surface_t * background_s;

	color_t grey0;
	color_t grey1;
	color_t grey2;
	color_t grey3;
	color_t grey5;

	color_t plum0;
	color_t plum1;
	color_t plum2;

	color_t c_selected1;
	color_t c_selected2;

	color_t c_selected_highlight1;
	color_t c_selected_highlight2;

	color_t c_normal1;
	color_t c_normal2;

	color_t c_tab_boder_highlight;
	color_t c_tab_boder_normal;

	color_t c_tab_selected_background;
	color_t c_tab_normal_background;

	color_t color_font;
	color_t color_font_selected;

	color_t popup_color;

	bool has_background;
	string background_file;
	string scale_mode;


	simple_theme_t(xconnection_t * cnx, config_handler_t & conf);

	virtual ~simple_theme_t();

	box_int_t compute_notebook_close_window_position(
			box_int_t const & allocation, int number_of_client,
			int selected_client_index) const;
	box_int_t compute_notebook_unbind_window_position(
			box_int_t const & allocation, int number_of_client,
			int selected_client_index) const;

	box_int_t compute_notebook_bookmark_position(
			box_int_t const & allocation) const;
	box_int_t compute_notebook_vsplit_position(
			box_int_t const & allocation) const;
	box_int_t compute_notebook_hsplit_position(
			box_int_t const & allocation) const;
	box_int_t compute_notebook_close_position(
			box_int_t const & allocation) const;

	box_int_t compute_floating_close_position(
			box_int_t const & _allocation) const;
	box_int_t compute_floating_bind_position(
			box_int_t const & _allocation) const;

	static void rounded_rectangle(cairo_t * cr, double x, double y, double w,
			double h, double r);

	void create_background_img();

	virtual void render_notebook(cairo_t * cr, notebook_base_t const * n, box_int_t const & area) const;
	virtual void render_split(cairo_t * cr, split_base_t const * s, box_int_t const & area) const;
	virtual void render_floating(managed_window_base_t * mw) const;


	virtual void render_popup_notebook0(cairo_t * cr, window_icon_handler_t * icon, unsigned int width,
			unsigned int height, string const & title);
	virtual void render_popup_move_frame(cairo_t * cr, window_icon_handler_t * icon, unsigned int width,
			unsigned int height, string const & title);

	virtual vector<box_page_event_t> * compute_page_areas(
			list<tree_t const *> const & page) const;
	virtual vector<box_floating_event_t> * compute_floating_areas(
			managed_window_base_t * mw) const;


	void compute_areas_for_notebook(notebook_base_t const * n,
			vector<box_page_event_t> * l) const;

	void draw_hatched_rectangle(cairo_t * cr, int space, int x, int y, int w,
			int h) const;

	void update();


	box_int_t compute_split_bar_location(split_base_t const * s) const {

		box_int_t ret;
		box_int_t const & alloc = s->allocation();

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
