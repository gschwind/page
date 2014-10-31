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

#include <cairo/cairo.h>

#include <memory>

#include "renderable.hxx"
#include "icon_handler.hxx"

namespace page {

struct cycle_window_entry_t {
	std::shared_ptr<icon64> icon;
	std::string title;
	client_managed_t * id;

private:
	cycle_window_entry_t(cycle_window_entry_t const &);
	cycle_window_entry_t & operator=(cycle_window_entry_t const &);

public:
	cycle_window_entry_t(client_managed_t * mw, std::string title, std::shared_ptr<icon64> icon) :
			icon(icon), title(title), id(mw) {
	}

	~cycle_window_entry_t() { }

};

class popup_alt_tab_t : public renderable_t {
	theme_t * _theme;
	i_rect _position;
	std::vector<std::shared_ptr<cycle_window_entry_t>> _client_list;
	int _selected;
	bool _is_visible;
	bool _is_durty;

public:

	popup_alt_tab_t(display_t * cnx, theme_t * theme, std::vector<std::shared_ptr<cycle_window_entry_t>> client_list, int selected) :
		_theme{theme},
		_selected{selected},
		_client_list{client_list},
		_is_durty{true},
		_is_visible{false}
	{

	}

	void mark_durty() {
		_is_durty = true;
	}

	void move_resize(i_rect const & area) {
		_position = area;
	}

	void move(int x, int y) {
		_position.x = x;
		_position.y = y;
	}

	void show() {
		_is_visible = true;
	}

	void hide() {
		_is_visible = false;
	}

	bool is_visible() {
		return _is_visible;
	}

	i_rect const & position() {
		return _position;
	}

	~popup_alt_tab_t() {

	}

	void select_next() {
		_selected = (_selected + 1) % _client_list.size();
	}

	client_managed_t * get_selected() {
		return _client_list[_selected]->id;
	}

	void render(cairo_t * cr, region const & area) {

		for (auto &a : area) {
			cairo_save(cr);
			cairo_clip(cr, a);
			cairo_translate(cr, _position.x, _position.y);
			cairo_rectangle(cr, 0, 0, _position.w, _position.h);
			cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_fill(cr);

			cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

			int n = 0;
			for (auto i : _client_list) {
				int x = n % 4;
				int y = n / 4;

				if (i->icon != nullptr) {
					if (i->icon->get_cairo_surface() != nullptr) {
						cairo_set_source_surface(cr,
								i->icon->get_cairo_surface(), x * 80 + 8,
								y * 80 + 8);
						cairo_mask_surface(cr, i->icon->get_cairo_surface(),
								x * 80 + 8, y * 80 + 8);

					}
				}

				if (n == _selected) {
					/** draw a buttyfull yelow box **/
					cairo_set_line_width(cr, 2.0);
					::cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
					cairo_rectangle(cr, x * 80 + 8, y * 80 + 8, 64, 64);
					cairo_stroke(cr);
				}

				++n;

			}

			cairo_restore(cr);
		}
	}


	bool need_render(time_t time) {
		return false;
	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return region{};
	}

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() {
		return region{_position};
	}

	/**
	 * return currently damaged area (absolute)
	 **/
	virtual region get_damaged()  {
		if(_is_durty) {
			return region{_position};
		} else {
			return region{};
		}
	}


};

}



#endif /* POPUP_ALT_TAB_HXX_ */
