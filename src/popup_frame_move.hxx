/*
 * popup_frame_move.hxx
 *
 * copyright (2012) Benoit Gschwind
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

	popup_frame_move_t(xconnection_t * cnx, theme_t * theme) :
			window_overlay_t(cnx, 32), _theme(theme) {

		icon = 0;
	}

	~popup_frame_move_t() {

	}

	void update_window(Window w, string title) {
		if (icon != 0)
			delete icon;
		icon = new window_icon_handler_t(_cnx, w, 64, 64);
		this->title = title;
	}

	void repair_back_buffer() {

		cairo_t * cr = cairo_create(_back_surf);

		cairo_rectangle(cr, 0, 0, _position.w, _position.h);
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_fill(cr);

		_theme->render_popup_move_frame(cr, icon, _position.w, _position.h, title);

		cairo_destroy(cr);

	}

};

}


#endif /* POPUP_FRAME_MOVE_HXX_ */
