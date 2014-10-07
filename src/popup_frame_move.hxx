/*
 * popup_frame_move.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef POPUP_FRAME_MOVE_HXX_
#define POPUP_FRAME_MOVE_HXX_

#include "window_overlay.hxx"

namespace page {

struct popup_frame_move_t: public renderable_t {

	theme_t * _theme;
	icon64 * icon;
	string title;

protected:
	i_rect _position;

	bool _has_alpha;
	bool _is_durty;
	bool _is_visible;

public:
	popup_frame_move_t(theme_t * theme) :
			_theme(theme), _position { -1, -1, 1, 1 } {

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



	~popup_frame_move_t() {
		if (icon != nullptr)
			delete icon;
	}

	void update_window(client_base_t * c, string title) {
		if (icon != nullptr)
			delete icon;
		icon = new icon64(*c);
		this->title = title;
	}

	virtual void render(cairo_t * cr, region const & area) {

		if(not _is_visible)
			return;

		for (auto & a : area) {
			cairo_save(cr);
			cairo_clip(cr, a);
			cairo_translate(cr, _position.x, _position.y);
			_theme->render_popup_move_frame(cr, icon, _position.w, _position.h,
					title);
			cairo_restore(cr);
		}
	}

};

}


#endif /* POPUP_FRAME_MOVE_HXX_ */
