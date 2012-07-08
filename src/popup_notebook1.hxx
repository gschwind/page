/*
 * popup_notebook1.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_NOTEBOOK1_HXX_
#define POPUP_NOTEBOOK1_HXX_

#include <string>
#include "renderable.hxx"

namespace page {

struct popup_notebook1_t: public renderable_t {
	box_t<int> area;
	cairo_surface_t * surf;
	std::string title;
	cairo_font_face_t * font;
	popup_notebook1_t(int x, int y, cairo_font_face_t * font,
			cairo_surface_t * icon, std::string const & title);
	virtual ~popup_notebook1_t();
	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual box_int_t get_absolute_extend();
	virtual void reconfigure(box_int_t const & area);
};

}


#endif /* POPUP_NOTEBOOK1_HXX_ */
