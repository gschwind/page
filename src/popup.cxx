/*
 * popup.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include "popup.hxx"
#include <cstdio>
#include <X11/extensions/Xcomposite.h>

namespace page_next {

#define min(x,y) (((x)>(y)?(y):(x)))
#define max(x,y) (((x)<(y)?(y):(x)))

popup_window_t::popup_window_t(Display * dpy, Window w, XWindowAttributes &wa) :
		area(wa.x, wa.y, wa.width, wa.height), w(w), dpy(dpy), visual(wa.visual) {
	XCompositeRedirectWindow(dpy, w,
			CompositeRedirectManual);
	damage = XDamageCreate(dpy, w, XDamageReportRawRectangles);
	surf = cairo_xlib_surface_create(dpy, w, wa.visual, wa.width, wa.height);
}

popup_window_t::~popup_window_t() {
	XCompositeUnredirectWindow(dpy, w, CompositeRedirectManual);
	XDamageDestroy(dpy, damage);
	cairo_surface_destroy(surf);
}

void popup_window_t::repair1(cairo_t * cr, box_int_t const & _area) {
	int left = max(area.x,_area.x);
	int rigth = min(area.x + area.w, _area.x + _area.w);
	int top = max(area.y, _area.y);
	int bottom = min(area.y + area.h, _area.y + _area.h);

	if ((bottom - top) > 0 && (rigth - left) > 0) {
		cairo_save(cr);
		cairo_set_source_surface(cr, surf, area.x, area.y);
		cairo_rectangle(cr, left, top, rigth - left, bottom - top);
		cairo_clip(cr);
		cairo_paint_with_alpha(cr, 0.9);
		cairo_restore(cr);
	}
}

bool popup_window_t::is_window(Window w) {
	return this->w == w;
}

box_int_t popup_window_t::get_absolute_extend() {
	return area;
}

void popup_window_t::reconfigure(box_int_t const & a) {
	area.x = a.x;
	area.y = a.y;
	area.h = a.h;
	area.w = a.w;

	if (surf != 0) {
		cairo_surface_destroy(surf);
	}

	surf = cairo_xlib_surface_create(dpy, w, visual, area.w, area.h);

}

popup_split_t::popup_split_t(box_t<int> const & area) :
		area(area) {

}

void popup_split_t::repair1(cairo_t * cr, box_int_t const & a) {
	box_int_t i = area & a;
	if (i.w > 0 && i.h > 0) {
		cairo_save(cr);
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
		cairo_rectangle(cr, i.x, i.y, i.w, i.h);
		cairo_fill(cr);
		cairo_restore(cr);
	}
}

popup_split_t::~popup_split_t() {

}

bool popup_split_t::is_window(Window w) {
	return false;
}

box_int_t popup_split_t::get_absolute_extend() {
	return area;
}

void popup_split_t::reconfigure(box_int_t const & a) {
	area = a;
}

popup_notebook_t::popup_notebook_t(int x, int y, int width, int height) :
		area(x, y, width, height) {

}

void popup_notebook_t::repair1(cairo_t * cr, box_int_t const & a) {
	box_int_t i = area & a;
	if (i.w > 0 && i.h > 0) {
		cairo_save(cr);
		cairo_set_line_width(cr, 2.0);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_set_source_rgba(cr, 0.8, 0.8, 0.0, 1.0);
		cairo_rectangle(cr, i.x, i.y, i.w, i.h);
		cairo_clip(cr);
		cairo_rectangle(cr, area.x + 3, area.y + 3, area.w - 6, area.h - 6);
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}

popup_notebook_t::~popup_notebook_t() {

}

bool popup_notebook_t::is_window(Window w) {
	return false;
}

box_int_t popup_notebook_t::get_absolute_extend() {
	return area;
}

void popup_notebook_t::reconfigure(box_int_t const & a) {
	area = a;
}

popup_notebook2_t::popup_notebook2_t(int x, int y, cairo_font_face_t * font,
		cairo_surface_t * icon, std::string & title) :
		font(font), surf(icon), title(title) {
	area.x = x;
	area.y = y;
	area.w = 200;
	area.h = 24;
}

popup_notebook2_t::~popup_notebook2_t() {

}

bool popup_notebook2_t::is_window(Window w) {
	return false;
}

box_int_t popup_notebook2_t::get_absolute_extend() {
	return area;
}

void popup_notebook2_t::reconfigure(box_int_t const & a) {
	area.x = a.x;
	area.y = a.y;
}

void popup_notebook2_t::repair1(cairo_t * cr, box_int_t const & a) {
	box_int_t i = area & a;
	cairo_save(cr);
	cairo_rectangle(cr, i.x, i.y, i.w, i.h);
	cairo_clip(cr);
	cairo_translate(cr, area.x, area.y);
	cairo_set_source_surface(cr, surf, 0.0, 0.0);
	cairo_paint(cr);

	/* draw the name */
	cairo_rectangle(cr, 0.0, 0.0,
			area.w - 17.0, area.h);
	cairo_clip(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_font_size(cr, 13.0);
	cairo_move_to(cr, 20.5, 15.5);
	cairo_show_text(cr, title.c_str());

	cairo_restore(cr);

}
}

