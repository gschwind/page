/*
 * popup_slipt.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */


#include "popup_split.hxx"


namespace page {

popup_split_t::popup_split_t(xconnection_t * cnx) : window_overlay_t(cnx, 32), wid(_wid) {

}


popup_split_t::~popup_split_t() {

}

void popup_split_t::move_resize(box_int_t const & a) {
	window_overlay_t::move_resize(a);

	cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(_cr, 0.0, 0.0, 0.0, 0.5);
	cairo_rectangle(_cr, 0, 0, _area.w, _area.h);
	cairo_fill(_cr);

	cairo_surface_flush(_surf);

	cairo_set_operator(_wid_cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(_wid_cr, _surf, 0.0, 0.0);
	cairo_rectangle(_wid_cr, 0, 0, _area.w, _area.h);
	cairo_fill(_wid_cr);

	cairo_surface_flush(_wid_surf);

}

}
