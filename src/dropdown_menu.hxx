/*
 * popup_alt_tab.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef DROPDOWN_MENU_HXX_
#define DROPDOWN_MENU_HXX_

#include "theme.hxx"
#include "window_overlay.hxx"

namespace page {

class dropdown_menu_t : public renderable_t {

	i_rect _position;
	bool _is_durty;
	bool _is_visible;

	theme_t * _theme;
	vector<dropdown_menu_entry_t *> window_list;
	int selected;

public:

	dropdown_menu_t(display_t * cnx, theme_t * theme) : _theme(theme) {
		selected = 0;
		_is_visible = false;
		_is_durty = true;
	}

	~dropdown_menu_t() {
		for(auto i: window_list) {
			delete i;
		}
	}

	void select_next() {
		selected = (selected + 1) % window_list.size();
		_is_durty = true;
	}

	void update_window(vector<dropdown_menu_entry_t *> list, int sel) {
		for(auto i : window_list) {
			delete i;
		}

		window_list = list;
		selected = sel;

		_position.w = 300.0;
		_position.h = 24*window_list.size();
	}

	managed_window_t const * get_selected() {
		return window_list[selected]->id();
	}

	virtual void render(cairo_t * cr, time_t time) {

		if(not _is_visible)
			return;

		cairo_save(cr);

		cairo_identity_matrix(cr);
		cairo_rectangle(cr, _position.x, _position.y, _position.w, _position.h);
		cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

		int n = 0;
		for (auto i : window_list) {
			int x = _position.x;
			int y = _position.y + n * 24;
			i_rect area(x, y, 300, 24);

			if(selected == n) {
				cairo_rectangle(cr, area.x, area.y, area.w, area.h);
				cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 1.0);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_fill(cr);
			}

			_theme->render_menuentry(cr, i, area);
			++n;
		}

		cairo_restore(cr);

	}

	bool need_render(time_t time) {
		return _is_durty;
	}

	void set_selected(int s) {
		if(s >= 0 and s < window_list.size()) {
			selected = s;
			_is_durty = true;
		}
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

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {
		if(not _is_visible)
			return;

		cairo_save(cr);

		for (auto const & a : area) {
			cairo_clip(cr, a);

			cairo_identity_matrix(cr);
			cairo_rectangle(cr, _position.x, _position.y, _position.w,
					_position.h);
			cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_fill(cr);

			cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

			int n = 0;
			for (auto i : window_list) {
				int x = _position.x;
				int y = _position.y + n * 24;
				i_rect area(x, y, 300, 24);

				if (selected == n) {
					cairo_rectangle(cr, area.x, area.y, area.w, area.h);
					cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 1.0);
					cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
					cairo_fill(cr);
				}

				_theme->render_menuentry(cr, i, area);
				++n;
			}

		}

		cairo_restore(cr);
	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return region{_position};
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
