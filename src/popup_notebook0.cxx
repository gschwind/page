/*
 * popup_notebook0.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include "popup_notebook0.hxx"

namespace page {

popup_notebook0_t::popup_notebook0_t(xconnection_t * cnx) : window_overlay_t(cnx, 32), wid(_wid) {
	_show = false;
}

popup_notebook0_t::~popup_notebook0_t() {

}

void popup_notebook0_t::reconfigure(box_int_t const & a) {
	move_resize(a);

	cairo_save(_cr);
	cairo_set_line_width(_cr, 2.0);
	cairo_set_antialias(_cr, CAIRO_ANTIALIAS_NONE);

	cairo_set_source_rgba(_cr, 0.0, 0.0, 0.0, 0.0);
	cairo_rectangle(_cr, 0, 0, _area.w, _area.h);
	cairo_paint(_cr);

	//3465A4
	cairo_set_source_rgba(_cr, 0x34 / 255.0, 0x65 / 255.0, 0xA4 / 255.0, 0.15);
	cairo_rectangle(_cr, 3, 3, _area.w - 6, _area.h - 6);
	cairo_fill(_cr);
	cairo_set_source_rgba(_cr, 0x34 / 255.0, 0x65 / 255.0, 0xA4 / 255.0, 1.0);
	cairo_rectangle(_cr, 3, 3, _area.w - 6, _area.h - 6);
	cairo_stroke(_cr);
	cairo_restore(_cr);

	cairo_surface_flush(_surf);

	/* blit result to visible output */
	cairo_set_operator(_wid_cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(_wid_cr, 0, 0, _area.w, _area.h);
	cairo_set_source_surface(_wid_cr, _surf, 0, 0);
	cairo_paint(_wid_cr);
	cairo_surface_flush(_wid_surf);

}

bool popup_notebook0_t::is_visible() {
	return _show;
}

void popup_notebook0_t::show() {
	window_overlay_t::show();
	_show = true;
}

void popup_notebook0_t::hide() {
	window_overlay_t::hide();
	_show = false;
}


}
