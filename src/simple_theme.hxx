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
#include <ft2build.h>
#include <freetype/freetype.h>
#include FT_FREETYPE_H

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo-ft.h>

#include "box.hxx"
#include "split.hxx"
#include "notebook.hxx"
#include "viewport.hxx"
#include "renderable_window.hxx"
#include "managed_window.hxx"
#include "theme_layout.hxx"
#include "theme.hxx"

using namespace std;

namespace page {

class simple_theme_layout_t : public theme_layout_t {

public:

	simple_theme_layout_t();

	virtual list<box_int_t> compute_client_tab(box_int_t const & allocation,
			int number_of_client, int selected_client_index) const;
	virtual box_int_t compute_notebook_close_window_position(
			box_int_t const & allocation, int number_of_client,
			int selected_client_index) const;
	virtual box_int_t compute_notebook_unbind_window_position(
			box_int_t const & allocation, int number_of_client,
			int selected_client_index) const;

	virtual box_int_t compute_notebook_bookmark_position(
			box_int_t const & allocation, int number_of_client,
			int selected_client_index) const;
	virtual box_int_t compute_notebook_vsplit_position(
			box_int_t const & allocation, int number_of_client,
			int selected_client_index) const;
	virtual box_int_t compute_notebook_hsplit_position(
			box_int_t const & allocation, int number_of_client,
			int selected_client_index) const;
	virtual box_int_t compute_notebook_close_position(
			box_int_t const & allocation, int number_of_client,
			int selected_client_index) const;

	virtual box_int_t compute_floating_close_position(
			box_int_t const & _allocation) const;
	virtual box_int_t compute_floating_bind_position(
			box_int_t const & _allocation) const;



};



class simple_theme_t : public theme_t {
public:

	bool ft_is_loaded;
	FT_Library library; /* handle to library */
	FT_Face face; /* handle to face object */
	cairo_font_face_t * font;
	FT_Face face_bold; /* handle to face object */
	cairo_font_face_t * font_bold;

	cairo_surface_t * vsplit_button_s;
	cairo_surface_t * hsplit_button_s;
	cairo_surface_t * close_button_s;
	cairo_surface_t * pop_button_s;
	cairo_surface_t * pops_button_s;
	cairo_surface_t * unbind_button_s;
	cairo_surface_t * bind_button_s;

	simple_theme_t(std::string conf_img_dir,
			std::string font, std::string font_bold);

	static void rounded_rectangle(cairo_t * cr, double x, double y, double w,
			double h, double r);
	void draw_unselected_tab(cairo_t * cr, managed_window_t * nw, box_int_t location);
	void draw_selected_tab(cairo_t * cr, managed_window_t * nw, box_int_t location);

	virtual void render_notebook(cairo_t * cr, notebook_t * n, bool is_default);
	virtual void render_split(cairo_t * cr, split_t * s);
	virtual void render_floating(managed_window_t * mw);
	virtual cairo_font_face_t * get_default_font();

};

}





#endif /* SIMPLE_THEME_HXX_ */
