/*
 * popup_notebook0.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_NOTEBOOK0_HXX_
#define POPUP_NOTEBOOK0_HXX_

#include "window_overlay.hxx"

namespace page {

struct popup_notebook0_t : public window_overlay_t {

	theme_t * _theme;

	bool _show;

	popup_notebook0_t(xconnection_t * cnx, theme_t * theme) :
			window_overlay_t(cnx, 32), _theme(theme) {
		_show = false;
	}

	~popup_notebook0_t() {

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

	void repair_back_buffer() {
		XWindowAttributes const * wa = _cnx->get_window_attributes(_wid);
		assert(wa != 0);

		cairo_t * cr = cairo_create(_back_surf);

		_theme->render_popup_notebook0(cr, wa->width, wa->height);

//		cairo_set_line_width(cr, 2.0);
//		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
//
//		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
//		cairo_rectangle(cr, 0, 0, wa->width, wa->height);
//		cairo_paint(cr);
//
//		cairo_set_source_rgba(cr, 0x34 / 255.0, 0x65 / 255.0, 0xA4 / 255.0, 0.15);
//		cairo_rectangle(cr, 3, 3, wa->width - 6, wa->height - 6);
//		cairo_fill(cr);
//		cairo_set_source_rgba(cr, 0x34 / 255.0, 0x65 / 255.0, 0xA4 / 255.0, 1.0);
//		cairo_rectangle(cr, 3, 3, wa->width - 6, wa->height - 6);
//		cairo_stroke(cr);

		cairo_destroy(cr);

	}

};

}


#endif /* POPUP_NOTEBOOK0_HXX_ */
