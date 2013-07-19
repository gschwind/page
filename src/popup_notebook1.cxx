/*
 * popup_notebook1.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include "popup_notebook1.hxx"

namespace page {

popup_notebook1_t::popup_notebook1_t(xconnection_t * cnx, cairo_font_face_t * font) : window_overlay_t(cnx, 32, box_int_t(0, 0, 200, 32)), wid(_wid), title(), font(font) {
	_show = false;
}

popup_notebook1_t::~popup_notebook1_t() {

}

void popup_notebook1_t::show() {
	window_overlay_t::show();
	_show = true;
}

void popup_notebook1_t::hide() {
	window_overlay_t::hide();
	_show = false;
}

void popup_notebook1_t::update_data(cairo_surface_t * icon, std::string const & title) {
	rebuild_cairo();

	this->title = title;

	/* clear */
	cairo_set_source_rgba(_cr, 0.0, 0.0, 0.0, 0.0);
	cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(_cr);

	cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
	if (icon != 0) {
		cairo_rectangle(_cr, 0, 0, 16, 16);
		cairo_set_source_surface(_cr, _surf, 0.0, 0.0);
		cairo_paint(_cr);

	}

	cairo_set_source_rgba(_cr, 0.0, 0.0, 0.0, 1.0);
	cairo_set_font_size(_cr, 13.0);
	cairo_move_to(_cr, 20.5, 15.5);
	cairo_set_font_face(_cr, font);
	cairo_show_text(_cr, title.c_str());

	cairo_surface_flush(_surf);

	cairo_set_operator(_wid_cr, CAIRO_OPERATOR_SOURCE);

	cairo_set_source_surface(_wid_cr, _surf, 0, 0);
	cairo_rectangle(_wid_cr, 0, 0, _area.w, _area.h);
	cairo_paint(_wid_cr);

	cairo_set_source_rgba(_wid_cr, 1.0, 1.0, 0.0, 0.5);
	cairo_rectangle(_wid_cr, 0, 0, 16, 16);
	cairo_paint(_wid_cr);

	cairo_surface_flush(_wid_surf);

}

bool popup_notebook1_t::is_visible() {
	return _show;
}

}
