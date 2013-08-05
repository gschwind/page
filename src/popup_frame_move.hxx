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

	popup_frame_move_t(xconnection_t * cnx) :
			window_overlay_t(cnx, 32) {
	}

	~popup_frame_move_t() {

	}

	void repair_back_buffer() {
		XWindowAttributes const * wa = _cnx->get_window_attributes(_wid);
		assert(wa != 0);

		cairo_t * cr = cairo_create(_back_surf);

		cairo_set_line_width(cr, 2.0);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
		cairo_rectangle(cr, 0, 0, wa->width, wa->height);
		cairo_paint(cr);

		//3465A4
		cairo_set_source_rgba(cr, 0x34 / 255.0, 0x65 / 255.0, 0xA4 / 255.0, 0.7);
		cairo_rectangle(cr, 3, 3, wa->width - 6, wa->height - 6);
		cairo_stroke(cr);

		cairo_destroy(cr);

	}

};

}


#endif /* POPUP_FRAME_MOVE_HXX_ */
