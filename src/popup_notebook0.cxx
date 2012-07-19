/*
 * popup_notebook0.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include "popup_notebook0.hxx"

namespace page {

popup_notebook0_t::popup_notebook0_t(int x, int y, int width, int height) :
		area(x, y, width, height) {

}

void popup_notebook0_t::repair1(cairo_t * cr, box_int_t const & a) {
	box_int_t i = area & a;
	if (i.w > 0 && i.h > 0) {
		cairo_save(cr);
		cairo_set_line_width(cr, 2.0);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_set_source_rgba(cr, 0.8, 0.8, 0.0, 0.75);
		cairo_rectangle(cr, i.x, i.y, i.w, i.h);
		cairo_clip(cr);
		cairo_rectangle(cr, area.x + 3, area.y + 3, area.w - 6, area.h - 6);
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}

popup_notebook0_t::~popup_notebook0_t() {

}

box_int_t popup_notebook0_t::get_absolute_extend() {
	return area;
}

void popup_notebook0_t::reconfigure(box_int_t const & a) {
	area = a;
}

bool popup_notebook0_t::is_visible() {
	return true;
}

}
