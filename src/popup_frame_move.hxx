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

struct popup_frame_move_t: public window_overlay_t {

	theme_t * _theme;
	window_icon_handler_t * icon;
	string title;

	popup_frame_move_t(theme_t * theme) :
			window_overlay_t(), _theme(theme) {

		icon = nullptr;
	}

	~popup_frame_move_t() {
		if (icon != nullptr)
			delete icon;
	}

	void update_window(client_base_t * c, string title) {
		if (icon != nullptr)
			delete icon;
		icon = new window_icon_handler_t(c, 64, 64);
		this->title = title;
	}

	virtual void render(cairo_t * cr, time_t time) {

		if(not _is_visible)
			return;

		cairo_save(cr);
		cairo_translate(cr, _position.x, _position.y);
		_theme->render_popup_move_frame(cr, icon, _position.w, _position.h, title);
		cairo_restore(cr);
	}

};

}


#endif /* POPUP_FRAME_MOVE_HXX_ */
