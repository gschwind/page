/*
 * popup_notebook1.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include "popup_notebook1.hxx"

namespace page {

popup_notebook1_t::popup_notebook1_t(xconnection_t * cnx, cairo_font_face_t * font) : window_overlay_t(cnx, 32, box_int_t(0, 0, 400, 32)), wid(_wid), _title(), _font(font) {
	_show = false;
	_icon = 0;
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

	_icon = icon;
	_title = title;

	/* clear */
	cairo_set_source_rgba(_cr, 0.0, 0.0, 0.0, 0.0);
	cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(_cr);

	cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
	if (_icon != 0) {
		cairo_rectangle(_cr, 0, 0, 16, 16);
		cairo_set_source_surface(_cr, _icon, 0.0, 2.0);
		cairo_paint(_cr);
	}

	cairo_set_source_rgba(_cr, 0.0, 0.0, 0.0, 1.0);
	cairo_set_font_size(_cr, 18.0);

	cairo_save(_cr);
	cairo_translate(_cr, 20, 18);
	cairo_new_path(_cr);
	cairo_text_path(_cr, title.c_str());
	cairo_set_line_width(_cr, 3.0);
	cairo_set_line_cap (_cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(_cr, CAIRO_LINE_JOIN_BEVEL);
	cairo_fill(_cr);
	cairo_restore(_cr);

	cairo_surface_flush(_surf);

	expose();
}

void popup_notebook1_t::expose() {
	cairo_set_operator(_wid_cr, CAIRO_OPERATOR_SOURCE);

	cairo_set_source_surface(_wid_cr, _surf, 0, 0);
	cairo_rectangle(_wid_cr, 0, 0, _area.w, _area.h);
	cairo_paint(_wid_cr);

	cairo_surface_flush(_wid_surf);
}

bool popup_notebook1_t::is_visible() {
	return _show;
}

}
