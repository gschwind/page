/*
 * popup_split.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef POPUP_SPLIT_HXX_
#define POPUP_SPLIT_HXX_

#include "window_overlay.hxx"
#include "box.hxx"
#include "renderable.hxx"

namespace page {

struct popup_split_t : public renderable_t {

	theme_t * _theme;
	double _current_split;
	split_t const * _s_base;

	i_rect _position;

	bool _has_alpha;
	bool _is_durty;
	bool _is_visible;

public:

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


	popup_split_t(theme_t * theme) : _theme(theme) {
		_s_base = nullptr;
		_current_split = 0.5;
		_has_alpha = true;
		_is_durty = true;
		_is_visible = false;
	}

	~popup_split_t() {

	}

	void set_position(double pos) {
		_current_split = pos;
	}

	void set_current_split(split_t const * s) {
		_s_base = s;
	}

	virtual void render(cairo_t * cr, region const & area) {

		theme_split_t ts;
		ts.split = _s_base->split();
		ts.type = _s_base->type();
		ts.allocation = _s_base->allocation();

		for (auto const & a : area) {
			cairo_save(cr);
			cairo_clip(cr, a);
			_theme->render_popup_split(cr, &ts, _current_split);
			cairo_restore(cr);
		}

	}


};

}

#endif /* POPUP_SPLIT_HXX_ */
