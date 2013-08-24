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

#include "floating_event.hxx"
#include "page_event.hxx"
#include "tree.hxx"

using namespace std;

namespace page {

struct margin_t {
	int top;
	int bottom;
	int left;
	int right;
};

class theme_t {

public:

	margin_t notebook_margin;
	margin_t split_margin;
	margin_t floating_margin;

	unsigned char split_width;

	theme_t() {

		notebook_margin.top = 0;
		notebook_margin.bottom = 0;
		notebook_margin.left = 0;
		notebook_margin.right = 0;

		split_margin.top = 0;
		split_margin.bottom = 0;
		split_margin.left = 0;
		split_margin.right = 0;

		floating_margin.top = 0;
		floating_margin.bottom = 0;
		floating_margin.left = 0;
		floating_margin.right = 0;

		split_width = 0;
	}

	virtual ~theme_t() {
	}

	virtual vector<page_event_t> * compute_page_areas(
			list<tree_t const *> const & page) const = 0;
	virtual vector<floating_event_t> * compute_floating_areas(
			managed_window_base_t * mw) const = 0;

	virtual void render_split(cairo_t * cr, split_base_t const * s,
			box_int_t const & area) const = 0;
	virtual void render_notebook(cairo_t * cr, notebook_base_t const * n,
			box_int_t const & area) const = 0;

	virtual void render_floating(managed_window_base_t * nw) const = 0;

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
