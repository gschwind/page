/*
 * popup_notebook0.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_NOTEBOOK0_HXX_
#define POPUP_NOTEBOOK0_HXX_

#include "window_overlay.hxx"
#include "window_icon_handler.hxx"

namespace page {

struct popup_notebook0_t : public window_overlay_t {

	xconnection_t * _cnx;
	theme_t * _theme;

	window_icon_handler_t * icon;

	bool _show;

	popup_notebook0_t(xconnection_t * cnx, theme_t * theme) :
			window_overlay_t(cnx, 32), _theme(theme) {

		_cnx = cnx;
		icon = 0;

		_show = false;
	}

	~popup_notebook0_t() {
		if(icon != 0)
			delete icon;
	}

	void update_window(Window w) {
		if (icon != 0)
			delete icon;
		icon = new window_icon_handler_t(_cnx, w, 64, 64);
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

		cairo_rectangle(cr, 0, 0, wa->width, wa->height);
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_fill(cr);

		_theme->render_popup_notebook0(cr, icon,  wa->width, wa->height);

		cairo_destroy(cr);

	}

};

}


#endif /* POPUP_NOTEBOOK0_HXX_ */
