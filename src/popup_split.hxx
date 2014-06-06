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

	popup_split_t(display_t * cnx, theme_t * theme) : window_overlay_t(cnx, 32), _theme(theme) {

	}


	~popup_split_t() {

	}

	void repair_back_buffer() {

		cairo_t * cr = cairo_create(_back_surf);

		_theme->render_popup_split(cr, _position.w, _position.h);

		cairo_destroy(cr);

	}

};

}

#endif /* POPUP_SPLIT_HXX_ */
