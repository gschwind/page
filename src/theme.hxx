/*
 * theme.hxx
 *
 *  Created on: 23 mars 2013
 *      Author: gschwind
 */

#ifndef THEME_HXX_
#define THEME_HXX_

#include <glib.h>

#include <set>
#include <list>
#include <string>
#include <vector>

#include <pango/pangocairo.h>

#include "theme_layout.hxx"
#include "split.hxx"
#include "notebook.hxx"
#include "managed_window.hxx"

using namespace std;

namespace page {


class theme_t {

protected:
	theme_layout_t * layout;

public:

	theme_t() {
		layout = 0;
	}

	virtual ~theme_t() {
	}

	virtual void render_split(cairo_t * cr, split_t * s) = 0;
	virtual void render_notebook(cairo_t * cr, notebook_t * n,
			managed_window_t * focuced, bool is_default) = 0;

	virtual void render_floating(managed_window_t * nw, bool is_focus) = 0;

	virtual cairo_font_face_t * get_default_font() = 0;
	virtual PangoFontDescription * get_pango_font() = 0;

	virtual theme_layout_t const * get_theme_layout() const {
		return layout;
	}

	virtual void render_popup_notebook0(cairo_t * cr,
			window_icon_handler_t * icon, unsigned int width,
			unsigned int height, string const & title) = 0;
	virtual void render_popup_move_frame(cairo_t * cr, unsigned int width,
			unsigned int height) = 0;

};

}

#endif /* THEME_HXX_ */
