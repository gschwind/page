/*
 * popup_notebook1.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_NOTEBOOK1_HXX_
#define POPUP_NOTEBOOK1_HXX_

#include <string>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

#include "window_overlay.hxx"

namespace page {

struct popup_notebook1_t: public window_overlay_t {

	bool _show;
	std::string _title;
	cairo_font_face_t * _font;
	cairo_surface_t * _icon;


	popup_notebook1_t(xconnection_t * cnx, cairo_font_face_t * font) :
			window_overlay_t(cnx, 32, box_int_t(0, 0, 400, 32)), _title(), _font(
					font) {
		_show = false;
		_icon = 0;
	}

	~popup_notebook1_t() {

	}

	void show() {
		window_overlay_t::show();
		_show = true;
	}

	void hide() {
		window_overlay_t::hide();
		_show = false;
	}

	bool is_visible() {
		return _show;
	}

	void update_data(cairo_surface_t * icon, std::string const & title) {
		_icon = icon;
		_title = title;
		mark_durty();
	}

	void repair_back_buffer() {
		XWindowAttributes const * wa = _cnx->get_window_attributes(_wid);
		assert(wa != 0);

		cairo_t * cr = cairo_create(_back_surf);

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cr);

		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		if (_icon != 0) {
			cairo_rectangle(cr, 0, 0, 16, 16);
			cairo_set_source_surface(cr, _icon, 0.0, 2.0);
			cairo_paint(cr);
		}

		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
		cairo_set_font_size(cr, 18.0);

		cairo_translate(cr, 20, 18);
		cairo_new_path(cr);
		cairo_text_path(cr, _title.c_str());
		cairo_set_line_width(cr, 3.0);
		cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
		cairo_fill(cr);

		cairo_destroy(cr);

	}

};

}


#endif /* POPUP_NOTEBOOK1_HXX_ */
