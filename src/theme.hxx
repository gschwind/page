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

#include "tree.hxx"
#include "theme_layout.hxx"

using namespace std;

namespace page {


class theme_t {

protected:
	theme_layout_t * _layout;

public:

	theme_t() {
		_layout = 0;
	}

	virtual ~theme_t() {
	}

	virtual void render_split(cairo_t * cr, split_base_t * s) const = 0;
	virtual void render_notebook(cairo_t * cr, notebook_base_t * n) const = 0;

	virtual void render_floating(managed_window_base_t * nw) const = 0;

	virtual theme_layout_t const * layout() const {
		return _layout;
	}

	virtual void render_popup_notebook0(cairo_t * cr,
			window_icon_handler_t * icon, unsigned int width,
			unsigned int height, string const & title) = 0;
	virtual void render_popup_move_frame(cairo_t * cr,
			window_icon_handler_t * icon, unsigned int width,
			unsigned int height, string const & title) = 0;

	virtual void update() = 0;

};

}

#endif /* THEME_HXX_ */
