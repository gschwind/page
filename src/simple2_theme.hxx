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

#include <pango/pangocairo.h>
#include <memory>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "utils.hxx"
#include "theme.hxx"
#include "color.hxx"
#include "config_handler.hxx"
#include "renderable.hxx"
#include "pixmap.hxx"

namespace page {

class simple2_theme_t : public theme_t {
public:

	display_t * _cnx;

	std::string notebook_active_font_name;
	std::string notebook_selected_font_name;
	std::string notebook_attention_font_name;
	std::string notebook_normal_font_name;

	std::string floating_active_font_name;
	std::string floating_attention_font_name;
	std::string floating_normal_font_name;

	std::string pango_popup_font_name;

	PangoFontDescription * notebook_active_font;
	PangoFontDescription * notebook_selected_font;
	PangoFontDescription * notebook_attention_font;
	PangoFontDescription * notebook_normal_font;

	PangoFontDescription * floating_active_font;
	PangoFontDescription * floating_attention_font;
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

	cairo_surface_t * left_scroll_arrow_button_s;
	cairo_surface_t * right_scroll_arrow_button_s;

	color_t default_background_color;

	color_t popup_text_color;
	color_t popup_outline_color;
	color_t popup_background_color;

	color_t grip_color;

	color_t notebook_mouse_over_background_color;

	color_t notebook_active_text_color;
	color_t notebook_active_outline_color;
	color_t notebook_active_border_color;
	color_t notebook_active_background_color;

	color_t notebook_selected_text_color;
	color_t notebook_selected_outline_color;
	color_t notebook_selected_border_color;
	color_t notebook_selected_background_color;

	color_t notebook_attention_text_color;
	color_t notebook_attention_outline_color;
	color_t notebook_attention_border_color;
	color_t notebook_attention_background_color;

	color_t notebook_normal_text_color;
	color_t notebook_normal_outline_color;
	color_t notebook_normal_border_color;
	color_t notebook_normal_background_color;

	color_t floating_active_text_color;
	color_t floating_active_outline_color;
	color_t floating_active_border_color;
	color_t floating_active_background_color;

	color_t floating_attention_text_color;
	color_t floating_attention_outline_color;
	color_t floating_attention_border_color;
	color_t floating_attention_background_color;

	color_t floating_normal_text_color;
	color_t floating_normal_outline_color;
	color_t floating_normal_border_color;
	color_t floating_normal_background_color;

	bool has_background;
	std::string background_file;
	std::string scale_mode;

	std::shared_ptr<pixmap_t> backgroun_px;

	simple2_theme_t(display_t * cnx, config_handler_t & conf);

	virtual ~simple2_theme_t();

	rect compute_notebook_close_window_position(
			rect const & allocation, int number_of_client,
			int selected_client_index) const;
	rect compute_notebook_unbind_window_position(
			rect const & allocation, int number_of_client,
			int selected_client_index) const;

	rect compute_notebook_bookmark_position(
			rect const & allocation) const;
	rect compute_notebook_vsplit_position(
			rect const & allocation) const;
	rect compute_notebook_hsplit_position(
			rect const & allocation) const;
	rect compute_notebook_close_position(
			rect const & allocation) const;

	rect compute_notebook_menu_position(
			rect const & allocation) const;

	rect compute_floating_close_position(
			rect const & _allocation) const;
	rect compute_floating_bind_position(
			rect const & _allocation) const;

	static void rounded_i_rect(cairo_t * cr, double x, double y, double w,
			double h, double r);

	void create_background_img();

	virtual void render_notebook(cairo_t * cr, theme_notebook_t const * n) const;
	virtual void render_iconic_notebook(cairo_t * cr, vector<theme_tab_t> const & tabs) const;
	virtual void render_split(cairo_t * cr, theme_split_t const * s) const;
	virtual void render_empty(cairo_t * cr, rect const & area) const;

	virtual void render_thumbnail(cairo_t * cr, rect position, theme_thumbnail_t const & t) const;
	virtual void render_thumbnail_title(cairo_t * cr, rect position, std::string const & title) const;

	virtual void render_floating(theme_managed_window_t * mw) const;


	void render_notebook_selected(
			cairo_t * cr,
			theme_notebook_t const & n,
			theme_tab_t const & data,
			PangoFontDescription const * pango_font,
			color_t const & text_color,
			color_t const & outline_color,
			color_t const & border_color,
			color_t const & background_color,
			double border_width
	) const;

	void render_notebook_normal(
			cairo_t * cr,
			theme_tab_t const & data,
			PangoFontDescription const * pango_font,
			color_t const & text_color,
			color_t const & outline_color,
			color_t const & border_color,
			color_t const & background_color
	) const;

	void render_floating_base(
			theme_managed_window_t * mw,
			PangoFontDescription const * pango_font,
			color_t const & text_color,
			color_t const & outline_color,
			color_t const & border_color,
			color_t const & background_color,
			double border_width
	) const;

	virtual void render_popup_notebook0(cairo_t * cr, icon64 * icon, unsigned int width,
			unsigned int height, std::string const & title) const;
	virtual void render_popup_move_frame(cairo_t * cr, icon64 * icon, unsigned int width,
			unsigned int height, std::string const & title) const;

	virtual void render_popup_split(cairo_t * cr, theme_split_t const * s, double current_split) const;

	virtual void render_menuentry(cairo_t * cr, theme_dropdown_menu_entry_t const & item, rect const & area, bool selected) const;

	void draw_hatched_i_rect(cairo_t * cr, int space, int x, int y, int w,
			int h) const;

	void update();

	static void cairo_rounded_tab3(cairo_t * cr, double x, double y, double w, double h, double radius);

	virtual shared_ptr<pixmap_t> get_background() const;

	virtual color_t const & get_focused_color() const;
	virtual color_t const & get_selected_color() const;
	virtual color_t const & get_normal_color() const;
	virtual color_t const & get_mouse_over_color() const;

};

}

// TODO
//extern "C" page::theme_t * get_theme(GKeyFile * conf);


#endif /* SIMPLE_THEME_HXX_ */
