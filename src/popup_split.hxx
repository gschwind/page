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

struct popup_split_t: public window_overlay_t {

	theme_t * _theme;
	double _current_split;
	split_base_t const * _s_base;

	popup_split_t(theme_t * theme) : window_overlay_t(), _theme(theme) {
		_s_base = nullptr;
		_current_split = 0.5;
	}

	~popup_split_t() {

	}

	void set_position(double pos) {
		_current_split = pos;
	}

	void set_current_split(split_base_t const * s) {
		_s_base = s;
	}

	virtual void render(cairo_t * cr, region const & area) {

		if(not _is_visible)
			return;

		for (auto const & a : area) {
			cairo_save(cr);
			cairo_clip(cr, a);
			_theme->render_popup_split(cr, _s_base, _current_split);
			cairo_restore(cr);
		}

	}


};

}

#endif /* POPUP_SPLIT_HXX_ */
