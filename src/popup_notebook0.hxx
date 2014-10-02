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

	icon64 * icon;
	string title;

	bool _show;

	popup_notebook0_t(theme_t * theme) :
			window_overlay_t(), _theme(theme) {

		icon = nullptr;

		_show = false;
	}

	~popup_notebook0_t() {
		if(icon != nullptr)
			delete icon;
	}

	void update_window(client_base_t * c, string title) {
		if (icon != nullptr)
			delete icon;
		icon = new icon64(*c);
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

	virtual void render(cairo_t * cr, region const & area) {

		if(not _is_visible)
			return;

		for (auto &a : area) {
			cairo_save(cr);
			cairo_clip(cr, a);
			cairo_translate(cr, _position.x, _position.y);
			_theme->render_popup_notebook0(cr, icon, _position.w, _position.h,
					title);
			cairo_restore(cr);
		}
	}

};

}


#endif /* POPUP_NOTEBOOK0_HXX_ */
