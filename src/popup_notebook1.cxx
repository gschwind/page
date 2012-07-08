/*
 * popup_notebook1.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include "popup_notebook1.hxx"

namespace page {

popup_notebook1_t::popup_notebook1_t(int x, int y, cairo_font_face_t * font,
		cairo_surface_t * icon, std::string const & title) :
		font(font), surf(icon), title(title) {
	area.x = x;
	area.y = y;
	area.w = 200;
	area.h = 24;
}

popup_notebook1_t::~popup_notebook1_t() {

}

box_int_t popup_notebook1_t::get_absolute_extend() {
	return area;
}

void popup_notebook1_t::reconfigure(box_int_t const & a) {
	area.x = a.x;
	area.y = a.y;
}

void popup_notebook1_t::repair1(cairo_t * cr, box_int_t const & a) {
	box_int_t i = area & a;
	cairo_save(cr);

	if (surf != 0) {
		cairo_rectangle(cr, i.x, i.y, i.w, i.h);
		cairo_clip(cr);
		cairo_translate(cr, area.x, area.y);
		cairo_set_source_surface(cr, surf, 0.0, 0.0);
		cairo_paint(cr);
	}

	/* draw the name */
	cairo_rectangle(cr, 0.0, 0.0, area.w - 17.0, area.h);
	cairo_clip(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_font_size(cr, 13.0);
	cairo_move_to(cr, 20.5, 15.5);
	cairo_show_text(cr, title.c_str());

	cairo_restore(cr);

}

}
