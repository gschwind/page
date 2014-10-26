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
	theme_t * _theme;
	display_t * _cnx;
	vector<ptr<dropdown_menu_entry_t>> _items;
	int _selected;

	xcb_pixmap_t _pix;
	cairo_surface_t * _surf;

	i_rect _position;

	bool _is_durty;

public:

	dropdown_menu_t(display_t * cnx, theme_t * theme, vector<ptr<dropdown_menu_entry_t>> items, int x, int y, int width) : _theme(theme) {
		_selected = -1;

		_is_durty = true;
		_cnx = cnx;

		_items = items;
		_position.x = x;
		_position.y = y;
		_position.w = width;
		_position.h = 24*_items.size();

		_pix = xcb_generate_id(cnx->xcb());
		xcb_create_pixmap(cnx->xcb(), 32, _pix, cnx->root(), _position.w, _position.h);
		_surf = cairo_xcb_surface_create(cnx->xcb(), _pix, cnx->default_visual(), _position.w, _position.h);

		update_backbuffer();
	}

	~dropdown_menu_t() {
		cairo_surface_destroy(_surf);
		xcb_free_pixmap(_cnx->xcb(), _pix);
	}

	client_managed_t const * get_selected() {
		return _items[_selected]->id();
	}

	void update_backbuffer() {

		cairo_t * cr = cairo_create(_surf);

		for (unsigned k = 0; k < _items.size(); ++k) {
			update_items_back_buffer(cr, k);
		}

		cairo_destroy(cr);

	}

	void update_items_back_buffer(cairo_t * cr, int n) {
		if (n >= 0 and n < _items.size()) {
			i_rect area(0, 24 * n, _position.w, 24);
			cairo_rectangle(cr, area.x, area.y, area.w, area.h);
			if (n == _selected) {
				cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 1.0);
			} else {
				cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
			}
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_fill(cr);
			_theme->render_menuentry(cr, _items[n].get(), area);
		}
	}

	void set_selected(int s) {
		if(s >= 0 and s < _items.size() and s != _selected) {
			std::swap(_selected, s);
			cairo_t * cr = cairo_create(_surf);
			update_items_back_buffer(cr, _selected);
			update_items_back_buffer(cr, s);
			cairo_destroy(cr);
			_is_durty = true;
		}
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
		cairo_save(cr);

		for (auto const & a : area) {
			cairo_clip(cr, a);
			cairo_rectangle(cr, _position.x, _position.y, _position.w, _position.h);
			cairo_set_source_surface(cr, _surf, _position.x, _position.y);
			cairo_fill(cr);
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
			_is_durty = false;
			return region{_position};
		} else {
			return region{};
		}
	}

};

}



#endif /* POPUP_ALT_TAB_HXX_ */
