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

#include <glib.h>

#include <set>
#include <list>
#include <string>
#include <vector>



#include "theme_managed_window.hxx"
#include "theme_split.hxx"
#include "theme_notebook.hxx"

#include "floating_event.hxx"
#include "page_event.hxx"
#include "leak_checker.hxx"
#include "icon_handler.hxx"

using namespace std;

namespace page {

struct margin_t {
	int top;
	int bottom;
	int left;
	int right;
};

struct cycle_window_entry_t : public leak_checker {
	icon64 * icon;
	string title;
	client_managed_t * id;

private:
	cycle_window_entry_t(cycle_window_entry_t const &);
	cycle_window_entry_t & operator=(cycle_window_entry_t const &);

public:
	cycle_window_entry_t(client_managed_t * mw, string title,
			icon64 * icon) :
			icon(icon), title(title), id(mw) {
	}

	~cycle_window_entry_t() {
		if(icon != nullptr) {
			delete icon;
		}
	}

};

struct theme_dropdown_menu_entry_t {
	ptr<icon16> icon;
	string label;
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


	virtual ~theme_t() {
	}

//	virtual vector<page_event_t> * compute_page_areas(
//			list<tree_t const *> const & page) const = 0;
//	virtual vector<floating_event_t> * compute_floating_areas(
//			theme_managed_window_t * mw) const = 0;

	virtual void render_split(cairo_t * cr, theme_split_t const * s,
			i_rect const & area) const = 0;
	virtual void render_notebook(cairo_t * cr, theme_notebook_t const * n,
			i_rect const & area) const = 0;

	virtual void render_floating(theme_managed_window_t * nw) const = 0;

	virtual void render_popup_notebook0(cairo_t * cr,
			icon64 * icon, unsigned int width,
			unsigned int height, string const & title) = 0;
	virtual void render_popup_move_frame(cairo_t * cr,
			icon64 * icon, unsigned int width,
			unsigned int height, string const & title) = 0;

	virtual void render_popup_split(cairo_t * cr, theme_split_t const * s, double current_split) = 0;
	virtual void render_menuentry(cairo_t * cr, theme_dropdown_menu_entry_t const & item, i_rect const & area, bool selected) = 0;
	virtual void update() = 0;


};

}

#endif /* THEME_HXX_ */
