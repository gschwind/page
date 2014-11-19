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

class popup_alt_tab_t {
	display_t * _dpy;
	theme_t * _theme;

	xcb_window_t _wid;
	xcb_pixmap_t _pix;

	cairo_surface_t * _surf;

	i_rect _position;
	std::vector<std::shared_ptr<cycle_window_entry_t>> _client_list;
	int _selected;
	bool _is_visible;
	bool _is_durty;

public:

	popup_alt_tab_t(display_t * cnx, theme_t * theme, std::vector<std::shared_ptr<cycle_window_entry_t>> client_list, int selected) :
		_dpy{cnx},
		_theme{theme},
		_selected{selected},
		_client_list{client_list},
		_is_durty{true},
		_is_visible{false}
	{

		_position.x = 0;
		_position.y = 0;
		_position.w = 80 * 4;
		_position.h = 80 * (_client_list.size()/4 + 1);

		xcb_colormap_t cmap = xcb_generate_id(_dpy->xcb());
		xcb_create_colormap(_dpy->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, _dpy->root(), _dpy->root_visual()->visual_id);

		uint32_t value_mask = 0;
		uint32_t value[5];

		value_mask |= XCB_CW_BACK_PIXEL;
		value[0] = _dpy->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_BORDER_PIXEL;
		value[1] = _dpy->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_OVERRIDE_REDIRECT;
		value[2] = True;

		value_mask |= XCB_CW_EVENT_MASK;
		value[3] = XCB_EVENT_MASK_EXPOSURE;

		value_mask |= XCB_CW_COLORMAP;
		value[4] = cmap;

		_wid = xcb_generate_id(_dpy->xcb());
		xcb_create_window(_dpy->xcb(), _dpy->root_depth(), _wid, _dpy->root(),
				_position.x, _position.y, _position.w, _position.h, 0,
				XCB_WINDOW_CLASS_INPUT_OUTPUT, _dpy->root_visual()->visual_id,
				value_mask, value);

		_pix = xcb_generate_id(_dpy->xcb());
		xcb_create_pixmap(_dpy->xcb(), _dpy->root_depth(), _pix, _wid,
				_position.w, _position.h);
		_surf = cairo_xcb_surface_create(_dpy->xcb(), _pix, _dpy->root_visual(),
				_position.w, _position.h);

		update_backbuffer();

		_dpy->map(_wid);

	}

	void mark_durty() {
		_is_durty = true;
	}

	void move(int x, int y) {
		_position.x = x;
		_position.y = y;
		_dpy->move_resize(_wid, _position);
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
		xcb_free_pixmap(_dpy->xcb(), _pix);
		xcb_destroy_window(_dpy->xcb(), _wid);
	}

	void select_next() {
		_selected = (_selected + 1) % _client_list.size();
		update_backbuffer();
		expose(i_rect{0,0,_position.w,_position.h});
	}

	client_managed_t * get_selected() {
		return _client_list[_selected]->id;
	}

	void update_backbuffer() {

		i_rect a{0,0,_position.w,_position.h};
		cairo_t * cr = cairo_create(_surf);
		//cairo_clip(cr, a);
		//cairo_translate(cr, _position.x, _position.y);
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
				/** draw a beautiful yellow box **/
				cairo_set_line_width(cr, 2.0);
				::cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
				cairo_rectangle(cr, x * 80 + 8, y * 80 + 8, 64, 64);
				cairo_stroke(cr);
			}

			++n;

		}
		cairo_destroy(cr);
	}

	void expose(region const & r) {
		cairo_surface_t * surf = cairo_xcb_surface_create(_dpy->xcb(), _wid, _dpy->root_visual(), _position.w, _position.h);
		cairo_t * cr = cairo_create(surf);
		for(auto a: r) {
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_set_source_surface(cr, _surf, 0.0, 0.0);
			cairo_rectangle(cr, a.x, a.y, a.w, a.h);
			cairo_fill(cr);
		}
		cairo_destroy(cr);
		cairo_surface_destroy(surf);
	}

	bool need_render(time_t time) {
		return false;
	}

	xcb_window_t id() {
		return _wid;
	}


};

}



#endif /* POPUP_ALT_TAB_HXX_ */
