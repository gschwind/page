/*
 * tiny_theme.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef TINY_THEME_HXX_
#define TINY_THEME_HXX_

#include <pango/pangocairo.h>

#include <memory>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "utils.hxx"
#include "theme.hxx"
#include "simple2_theme.hxx"
#include "color.hxx"
#include "config_handler.hxx"
#include "renderable.hxx"
#include "pixmap.hxx"

namespace page {

using namespace std;

class tiny_theme_t : public simple2_theme_t {

	rect compute_notebook_bookmark_position(rect const & allocation) const;
	rect compute_notebook_vsplit_position(rect const & allocation) const;
	rect compute_notebook_hsplit_position(rect const & allocation) const;
	rect compute_notebook_close_position(rect const & allocation) const;
	rect compute_notebook_menu_position(rect const & allocation) const;

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

public:
	tiny_theme_t(display_t * cnx, config_handler_t & conf);
	virtual ~tiny_theme_t();

	virtual void render_notebook(cairo_t * cr, theme_notebook_t const * n) const;

	virtual void render_iconic_notebook(
			cairo_t * cr,
			vector<theme_tab_t> const & tabs
	) const;

};

}

#endif /* SIMPLE_THEME_HXX_ */
