/*
 * popup_notebook0.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef POPUP_NOTEBOOK0_HXX_
#define POPUP_NOTEBOOK0_HXX_

#include "window_overlay.hxx"
#include "window_icon_handler.hxx"

namespace page {

struct popup_notebook0_t : public window_overlay_t {

	theme_t * _theme;

	window_icon_handler_t * icon;
	string title;

	bool _show;

	popup_notebook0_t(xconnection_t * cnx, theme_t * theme) :
			window_overlay_t(cnx, 32), _theme(theme) {

		icon = 0;

		_show = false;
	}

	~popup_notebook0_t() {
		if(icon != 0)
			delete icon;
	}

	void update_window(client_base_t * c, string title) {
		if (icon != 0)
			delete icon;
		icon = new window_icon_handler_t(c, 64, 64);
		this->title = title;
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

		cairo_t * cr = cairo_create(_back_surf);

		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(cr, 0, 0, _position.w, _position.h);
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
		cairo_fill(cr);

		_theme->render_popup_notebook0(cr, icon,  _position.w, _position.h, title);

		cairo_destroy(cr);

	}

};

}


#endif /* POPUP_NOTEBOOK0_HXX_ */
