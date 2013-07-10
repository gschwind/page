/*
 * popup_slipt.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */


#include "popup_split.hxx"


namespace page {

popup_split_t::popup_split_t(box_t<int> const & area) :
		area(area) {

}

void popup_split_t::repair1(cairo_t * cr, box_int_t const & a) {
	box_int_t i = area & a;
	if (!i.is_null()) {
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
		cairo_rectangle(cr, i.x, i.y, i.w, i.h);
		cairo_fill(cr);
		cairo_restore(cr);
	}
}

popup_split_t::~popup_split_t() {

}

box_int_t popup_split_t::get_absolute_extend() {
	return area;
}

region_t<int> popup_split_t::get_area() {
	return region_t<int>(area);
}

void popup_split_t::reconfigure(box_int_t const & a) {
	area = a;
}

bool popup_split_t::is_visible() {
	return true;
}

bool popup_split_t::has_alpha() {
	return true;
}

}
