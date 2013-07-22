/*
 * popup_notebook1.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_NOTEBOOK1_HXX_
#define POPUP_NOTEBOOK1_HXX_

#include "window_overlay.hxx"
#include <string>
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>
#include "renderable.hxx"

namespace page {

struct popup_notebook1_t: public window_overlay_t {

	Window const & wid;

	bool _show;
	std::string _title;
	cairo_font_face_t * _font;
	cairo_surface_t * _icon;

	popup_notebook1_t(xconnection_t * cnx, cairo_font_face_t * font);
	virtual ~popup_notebook1_t();

	void update_data(cairo_surface_t * icon, std::string const & title);

	void show();
	void hide();

	bool is_visible();
	void expose();

private:
	void move_resize(box_int_t const & a);


};

}


#endif /* POPUP_NOTEBOOK1_HXX_ */
