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
#include "renderable.hxx"

namespace page {

struct popup_notebook1_t: public renderable_t {
	box_t<int> area;
	bool _show;
	cairo_surface_t * surf;
	std::string title;
	cairo_font_face_t * font;
	cairo_surface_t * cache;

	popup_notebook1_t(cairo_font_face_t * font);
	virtual ~popup_notebook1_t();

	void update_data(int x, int y, cairo_surface_t * icon, std::string const & title);

	void show();
	void hide();

	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual box_int_t get_absolute_extend();
	virtual region_t<int> get_area();
	virtual void reconfigure(box_int_t const & area);
	virtual bool is_visible();
	virtual bool has_alpha();
};

}


#endif /* POPUP_NOTEBOOK1_HXX_ */
