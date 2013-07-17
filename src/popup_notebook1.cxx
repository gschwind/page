/*
 * popup_notebook1.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include "popup_notebook1.hxx"

namespace page {

popup_notebook1_t::popup_notebook1_t(cairo_font_face_t * font) : title() {
	this->surf = 0;
	this->font = font;

	area.x = 0;
	area.y = 0;
	area.w = 200;
	area.h = 24;
	cache = 0;
}

popup_notebook1_t::~popup_notebook1_t() {
	if(cache != 0)
		cairo_surface_destroy(cache);
}

box_int_t popup_notebook1_t::get_absolute_extend() {
	return area;
}

region_t<int> popup_notebook1_t::get_area() {
	return region_t<int>(area);
}

void popup_notebook1_t::reconfigure(box_int_t const & a) {
	area.x = a.x;
	area.y = a.y;
}

void popup_notebook1_t::show() {
	_show = true;
}

void popup_notebook1_t::hide() {
	_show = false;
}

void popup_notebook1_t::update_data(int x, int y, cairo_surface_t * icon, std::string const & title) {
	this->surf = icon;
	this->title = title;
	area.x = x;
	area.y = y;
}

void popup_notebook1_t::repair1(cairo_t * cr, box_int_t const & a) {
	box_int_t i = area & a;

	if (cache == 0) {
		cache = cairo_surface_create_similar(cairo_get_target(cr),
				CAIRO_CONTENT_COLOR_ALPHA, area.w, area.h);
		cairo_t * tcr = cairo_create(cache);

		cairo_set_source_rgba(tcr, 0.0, 0.0, 0.0, 0.0);
		cairo_set_operator(tcr, CAIRO_OPERATOR_SOURCE);
		cairo_paint(tcr);

		cairo_set_operator(tcr, CAIRO_OPERATOR_OVER);
		if (surf != 0) {
			cairo_rectangle(tcr, 0, 0, area.w, area.h);
			cairo_clip(tcr);
			cairo_set_source_surface(tcr, surf, 0.0, 0.0);
			cairo_paint(tcr);
		}

		cairo_set_source_rgba(tcr, 0.0, 0.0, 0.0, 1.0);
		cairo_set_font_size(tcr, 13.0);
		cairo_move_to(tcr, 20.5, 15.5);
		cairo_show_text(tcr, title.c_str());

		cairo_surface_flush(cache);
		cairo_destroy(tcr);
	}

	cairo_save(cr);
	cairo_rectangle(cr, i.x, i.y, i.w, i.h);
	cairo_clip(cr);
	cairo_set_source_surface(cr, cache, area.x, area.y);
	cairo_rectangle(cr, area.x, area.y, area.w, area.h);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_fill(cr);
	cairo_restore(cr);

}

bool popup_notebook1_t::is_visible() {
	return true;
}

bool popup_notebook1_t::has_alpha() {
	return true;
}

}
