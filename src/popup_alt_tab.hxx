/*
 * popup_alt_tab.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef POPUP_ALT_TAB_HXX_
#define POPUP_ALT_TAB_HXX_

#include "theme.hxx"
#include "window_overlay.hxx"

namespace page {

class popup_alt_tab_t : public window_overlay_t, public renderable_t {

	theme_t * _theme;

	vector<cycle_window_entry_t *> window_list;
	int selected;

public:

	popup_alt_tab_t(display_t * cnx, theme_t * theme) :
			window_overlay_t(cnx, 32), _theme(theme) {

		selected = 0;
	}

	~popup_alt_tab_t() {
		for(auto i: window_list) {
			delete i;
		}
	}

	void select_next() {
		selected = (selected + 1) % window_list.size();
		mark_durty();
	}

	void update_window(vector<cycle_window_entry_t *> list, int sel) {
		for(vector<cycle_window_entry_t *>::iterator i = window_list.begin();
				i != window_list.end(); ++i) {
			delete *i;
		}

		window_list = list;
		selected = sel;
	}

	void repair_back_buffer() {
		display_t::create_context(__FILE__, __LINE__);
		cairo_t * cr = cairo_create(_back_surf);

		cairo_rectangle(cr, 0, 0, _position.w, _position.h);
		cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

		int n = 0;
		for (vector<cycle_window_entry_t *>::iterator i = window_list.begin();
				i != window_list.end(); ++i) {
			int x = n % 4;
			int y = n / 4;

			if ((*i)->icon != 0) {
				if ((*i)->icon->get_cairo_surface() != 0) {

					cairo_set_source_surface(cr,
							(*i)->icon->get_cairo_surface(), x * 80 + 8, y * 80 + 8);
					cairo_mask_surface(cr,
							(*i)->icon->get_cairo_surface(), x * 80 + 8, y * 80 + 8);

//					cairo_rectangle(cr, x * 80 + 8, y * 80 + 8, 64, 64);
//					cairo_fill(cr);
				}
			}

			if (n == selected) {
				cairo_set_line_width(cr, 2.0);
				::cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
				cairo_rectangle(cr, x * 80 + 8, y * 80 + 8, 64, 64);
				cairo_stroke(cr);
			}

			++n;

		}
		display_t::destroy_context(__FILE__, __LINE__);
		assert(cairo_get_reference_count(cr) == 1);
		cairo_destroy(cr);

	}

	managed_window_base_t * get_selected() {
		return window_list[selected]->id;
	}

	virtual void render(cairo_t * cr, time_t time) {

		if(not _is_visible)
			return;
		display_t::create_context(__FILE__, __LINE__);
		cairo_save(cr);

		cairo_translate(cr, _position.x, _position.y);
		cairo_rectangle(cr, 0, 0, _position.w, _position.h);
		cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

		int n = 0;
		for (vector<cycle_window_entry_t *>::iterator i = window_list.begin();
				i != window_list.end(); ++i) {
			int x = n % 4;
			int y = n / 4;

			if ((*i)->icon != 0) {
				if ((*i)->icon->get_cairo_surface() != 0) {

					cairo_set_source_surface(cr,
							(*i)->icon->get_cairo_surface(), x * 80 + 8, y * 80 + 8);
					cairo_mask_surface(cr,
							(*i)->icon->get_cairo_surface(), x * 80 + 8, y * 80 + 8);

//					cairo_rectangle(cr, x * 80 + 8, y * 80 + 8, 64, 64);
//					cairo_fill(cr);
				}
			}

			if (n == selected) {
				cairo_set_line_width(cr, 2.0);
				::cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
				cairo_rectangle(cr, x * 80 + 8, y * 80 + 8, 64, 64);
				cairo_stroke(cr);
			}

			++n;

		}
		display_t::destroy_context(__FILE__, __LINE__);
		cairo_restore(cr);

	}

};

}



#endif /* POPUP_ALT_TAB_HXX_ */
