/*
 * popup_frame_move.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include "popup_frame_move.hxx"

namespace page {

popup_frame_move_t::popup_frame_move_t(xconnection_t * cnx) :
		window_overlay_t(cnx, 32), wid(_wid) {
}

popup_frame_move_t::~popup_frame_move_t() {

}

void popup_frame_move_t::reconfigure(box_int_t const & a) {
	move_resize(a);

	/* update back buffer */
	cairo_set_line_width(_cr, 2.0);
	cairo_set_antialias(_cr, CAIRO_ANTIALIAS_NONE);
	cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);

	cairo_set_source_rgba(_cr, 0.0, 0.0, 0.0, 0.0);
	cairo_rectangle(_cr, 0, 0, _area.w, _area.h);
	cairo_paint(_cr);

	//3465A4

	cairo_set_source_rgba(_cr, 0x34 / 255.0, 0x65 / 255.0, 0xA4 / 255.0, 0.7);
	cairo_rectangle(_cr, 3, 3, _area.w - 6, _area.h - 6);
	cairo_stroke(_cr);

	cairo_surface_flush(_surf);

	/* blit back buffer to window */
	cairo_set_operator(_wid_cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(_wid_cr, _surf, 0, 0);
	cairo_rectangle(_wid_cr, 0, 0, _area.w, _area.h);
	cairo_paint(_wid_cr);

	cairo_surface_flush(_wid_surf);

}

}
