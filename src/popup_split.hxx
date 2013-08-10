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
		p_window_attribute_t wa = _cnx->get_window_attributes(_wid);
		assert(wa->is_valid);

		cairo_t * cr = cairo_create(_back_surf);

		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 0.5);
		cairo_rectangle(cr, 0, 0, wa->width, wa->height);
		cairo_fill(cr);

		cairo_destroy(cr);

	}

};

}

#endif /* POPUP_SPLIT_HXX_ */
