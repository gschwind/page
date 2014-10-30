/*
 * theme.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef THEME_HXX_
#define THEME_HXX_

#include <cairo/cairo.h>

#include "leak_checker.hxx"
#include "icon_handler.hxx"

#include "theme_split.hxx"
#include "theme_managed_window.hxx"
#include "theme_tab.hxx"
#include "theme_notebook.hxx"

namespace page {

struct margin_t {
	int top;
	int bottom;
	int left;
	int right;
};

struct theme_alttab_entry_t {
	std::shared_ptr<icon64> icon;
	std::string label;
};

struct theme_dropdown_menu_entry_t {
	std::shared_ptr<icon16> icon;
	std::string label;
};


class theme_t {

public:

	struct {
		margin_t margin;

		unsigned tab_height;
		unsigned selected_close_width;
		unsigned selected_unbind_width;

		unsigned menu_button_width;
		unsigned close_width;
		unsigned hsplit_width;
		unsigned vsplit_width;
		unsigned mark_width;

	} notebook;

	struct {
		margin_t margin;
		unsigned width;
	} split;

	struct {
		margin_t margin;
		unsigned int close_width;
		unsigned int bind_width;
	} floating;


	virtual ~theme_t() { }


	virtual void render_split(cairo_t * cr, theme_split_t const * s,
			i_rect const & area) const = 0;
	virtual void render_notebook(cairo_t * cr, theme_notebook_t const * n,
			i_rect const & area) const = 0;

	virtual void render_floating(theme_managed_window_t * nw) const = 0;

	virtual void render_popup_notebook0(cairo_t * cr,
			icon64 * icon, unsigned int width,
			unsigned int height, std::string const & title) = 0;
	virtual void render_popup_move_frame(cairo_t * cr,
			icon64 * icon, unsigned int width,
			unsigned int height, std::string const & title) = 0;

	virtual void render_popup_split(cairo_t * cr, theme_split_t const * s, double current_split) = 0;
	virtual void render_menuentry(cairo_t * cr, theme_dropdown_menu_entry_t const & item, i_rect const & area, bool selected) = 0;
	virtual void update() = 0;


};

}

#endif /* THEME_HXX_ */
