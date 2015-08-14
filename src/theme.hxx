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

#include <typeinfo>

#include <cairo/cairo.h>

#include "color.hxx"

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

struct theme_thumbnail_t {
	std::shared_ptr<pixmap_t> pix;
	std::shared_ptr<pixmap_t> title;
};

class theme_t {

public:

	struct {
		margin_t margin;

		unsigned tab_height;
		unsigned iconic_tab_width;
		unsigned selected_close_width;
		unsigned selected_unbind_width;

		unsigned menu_button_width;
		unsigned close_width;
		unsigned hsplit_width;
		unsigned vsplit_width;
		unsigned mark_width;

		unsigned left_scroll_arrow_width;
		unsigned right_scroll_arrow_width;

	} notebook;

	struct {
		margin_t margin;
		unsigned width;
	} split;

	struct {
		margin_t margin;
		unsigned int title_height;
		unsigned int close_width;
		unsigned int bind_width;
	} floating;


	theme_t() { }
	virtual ~theme_t() { }

	virtual void render_split(cairo_t * cr, theme_split_t const * s) const = 0;
	virtual void render_notebook(cairo_t * cr, theme_notebook_t const * n) const = 0;
	virtual void render_empty(cairo_t * cr, rect const & area) const = 0;

	virtual void render_iconic_notebook(cairo_t * cr, vector<theme_tab_t> const & tabs) const = 0;

	virtual void render_thumbnail(cairo_t * cr, rect position, theme_thumbnail_t const & t) const = 0;
	virtual void render_thumbnail_title(cairo_t * cr, rect position, std::string const & title) const = 0;


	virtual void render_floating(theme_managed_window_t * nw) const = 0;

	virtual void render_popup_notebook0(cairo_t * cr,
			icon64 * icon, unsigned int width,
			unsigned int height, std::string const & title) const = 0;
	virtual void render_popup_move_frame(cairo_t * cr,
			icon64 * icon, unsigned int width,
			unsigned int height, std::string const & title) const = 0;

	virtual void render_popup_split(cairo_t * cr, theme_split_t const * s, double current_split) const = 0;
	virtual void render_menuentry(cairo_t * cr, theme_dropdown_menu_entry_t const & item, rect const & area, bool selected) const = 0;
	virtual void update() = 0;

	virtual std::shared_ptr<pixmap_t> get_background() const = 0;

	virtual color_t const & get_focused_color() const = 0;
	virtual color_t const & get_selected_color() const = 0;
	virtual color_t const & get_normal_color() const = 0;
	virtual color_t const & get_mouse_over_color() const = 0;

};

}

#endif /* THEME_HXX_ */
