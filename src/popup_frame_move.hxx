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

	popup_frame_move_t(xconnection_t * cnx, theme_t * theme) :
			window_overlay_t(cnx, 32), _theme(theme) {
	}

	~popup_frame_move_t() {

	}

	void repair_back_buffer() {
		XWindowAttributes const * wa = _cnx->get_window_attributes(_wid);
		assert(wa != 0);

		cairo_t * cr = cairo_create(_back_surf);

		cairo_rectangle(cr, 0, 0, wa->width, wa->height);
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_fill(cr);

		_theme->render_popup_move_frame(cr, wa->width, wa->height);

		cairo_destroy(cr);

	}

};

}


#endif /* POPUP_FRAME_MOVE_HXX_ */
