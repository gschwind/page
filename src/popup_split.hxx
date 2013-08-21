/*
 * popup_split.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_SPLIT_HXX_
#define POPUP_SPLIT_HXX_

#include "window_overlay.hxx"
#include "box.hxx"
#include "renderable.hxx"

namespace page {

struct popup_split_t: public window_overlay_t {

	popup_split_t(xconnection_t * cnx) : window_overlay_t(cnx, 32) {

	}


	~popup_split_t() {

	}

	void repair_back_buffer() {

		cairo_t * cr = cairo_create(_back_surf);

		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
//		cairo_rectangle(cr, 0, 0, wa->width, wa->height);
//		cairo_fill(cr);

		cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
		cairo_new_path(cr);

		if(_position.w > _position.h) {
			cairo_move_to(cr, 0.0, _position.h / 2);
			cairo_line_to(cr, _position.w, _position.h / 2);

			cairo_move_to(cr, 1.0, 1.0);
			cairo_line_to(cr, 1.0, _position.h - 1.0);

			cairo_move_to(cr, _position.w - 1.0, 1.0);
			cairo_line_to(cr, _position.w - 1.0, _position.h);

		} else {
			cairo_move_to(cr, _position.w / 2, 1.0);
			cairo_line_to(cr, _position.w / 2, _position.h - 1.0);

			cairo_move_to(cr, 1.0, 1.0);
			cairo_line_to(cr, _position.w - 1.0, 1.0);

			cairo_move_to(cr, 1.0, _position.h - 1.0);
			cairo_line_to(cr, _position.w - 1.0, _position.h - 1.0);

		}

		cairo_set_line_width(cr, 2.0);
		cairo_stroke(cr);

		cairo_destroy(cr);

	}

};

}

#endif /* POPUP_SPLIT_HXX_ */
