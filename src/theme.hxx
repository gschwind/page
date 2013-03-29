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

	theme_t() { layout = 0; }

	virtual ~theme_t() { }

	virtual void render_split(cairo_t * cr, split_t * s) = 0;
	virtual void render_notebook(cairo_t * cr, notebook_t * n, bool is_default) = 0;

	virtual void render_floating(managed_window_t * nw) = 0;

	virtual cairo_font_face_t * get_default_font() = 0;

	virtual theme_layout_t const * get_theme_layout() const {
		return layout;
	}

};

}

#endif /* THEME_HXX_ */
