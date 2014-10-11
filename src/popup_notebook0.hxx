/*
 * popup_notebook0.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef POPUP_NOTEBOOK0_HXX_
#define POPUP_NOTEBOOK0_HXX_

#include "window_overlay.hxx"
#include "window_icon_handler.hxx"

namespace page {

struct popup_notebook0_t : public renderable_t {
	theme_t * _theme;
	ptr<icon64> icon;
	string title;
	bool _show;

protected:
	i_rect _position;

	bool _has_alpha;
	bool _is_durty;
	bool _is_visible;

public:
	popup_notebook0_t(theme_t * theme) :
			_theme(theme), _position { -1, -1, 1, 1 } {
				icon = nullptr;

				_show = false;
		icon = nullptr;
		_has_alpha = true;
		_is_durty = true;
		_is_visible = false;
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

	i_rect const & position() {
		return _position;
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

	~popup_notebook0_t() {

	}

	void update_window(client_managed_t * c) {
		icon = ptr<icon64>{new icon64(*c)};
		this->title = c->title();
	}

	void show() {
		_is_visible = true;
		_show = true;
	}

	void hide() {
		_is_visible = false;
		_show = false;
	}

	bool is_visible() {
		return _show;
	}

	virtual void render(cairo_t * cr, region const & area) {

		if(not _is_visible)
			return;

		for (auto &a : area) {
			cairo_save(cr);
			cairo_clip(cr, a);
			cairo_translate(cr, _position.x, _position.y);
			_theme->render_popup_notebook0(cr, icon.get(), _position.w, _position.h,
					title);
			cairo_restore(cr);
		}
	}

};

}


#endif /* POPUP_NOTEBOOK0_HXX_ */
