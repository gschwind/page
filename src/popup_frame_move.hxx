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

	popup_frame_move_t(display_t * cnx, theme_t * theme) :
			window_overlay_t(cnx, 32), _theme(theme) {

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

	void repair_back_buffer() {
		display_t::create_context(__FILE__, __LINE__);
		cairo_t * cr = cairo_create(_back_surf);

		cairo_rectangle(cr, 0, 0, _position.w, _position.h);
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_fill(cr);

		_theme->render_popup_move_frame(cr, icon, _position.w, _position.h,
				title);
		display_t::destroy_context(__FILE__, __LINE__);
		assert(cairo_get_reference_count(cr) == 1);
		cairo_destroy(cr);

	}

};

}


#endif /* POPUP_FRAME_MOVE_HXX_ */
