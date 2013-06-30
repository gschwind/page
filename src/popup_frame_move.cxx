/*
 * popup_frame_move.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include "popup_frame_move.hxx"

namespace page {

popup_frame_move_t::popup_frame_move_t(int x, int y, int width, int height) :
		area(x, y, width, height) {

}

popup_frame_move_t::popup_frame_move_t(box_int_t x) :
		area(x) {

}

void popup_frame_move_t::repair1(cairo_t * cr, box_int_t const & a) {
	box_int_t i = area & a;
	if (i.w > 0 && i.h > 0) {
		cairo_save(cr);
		cairo_set_line_width(cr, 2.0);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

		//3465A4
		cairo_set_source_rgba(cr, 0x34 / 255.0, 0x65 / 255.0, 0xA4 / 255.0, 0.7);
		cairo_rectangle(cr, area.x + 3, area.y + 3, area.w - 6, area.h - 6);
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}

popup_frame_move_t::~popup_frame_move_t() {

}

box_int_t popup_frame_move_t::get_absolute_extend() {
	return area;
}

region_t<int> popup_frame_move_t::get_area() {
	region_t<int> r(area);
	r -= box_int_t(area.x + 6, area.y + 6, area.w - 12, area.h - 12);
	return r;
}

void popup_frame_move_t::reconfigure(box_int_t const & a) {
	area = a;
}

bool popup_frame_move_t::is_visible() {
	return true;
}

}
