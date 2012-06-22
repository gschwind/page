/*
 * popup.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include "popup.hxx"

namespace page_next {

#define min(x,y) (((x)>(y)?(y):(x)))
#define max(x,y) (((x)<(y)?(y):(x)))

popup_window_t::popup_window_t(Display * dpy, Window w, XWindowAttributes &wa) :
		wa(wa), w(w), dpy(dpy) {
	damage = XDamageCreate(dpy, w, XDamageReportRawRectangles);
	surf = cairo_xlib_surface_create(dpy, w, wa.visual, wa.width, wa.height);
}

popup_window_t::~popup_window_t() {
	XDamageDestroy(dpy, damage);
	cairo_surface_destroy(surf);
}

void popup_window_t::repair0(cairo_t * cr, cairo_surface_t * s, int x, int y,
		int width, int height) {
	cairo_save(cr);
	cairo_set_source_surface(cr, s, 0, 0);
	cairo_rectangle(cr, wa.x + x, wa.y + y, width, height);
	cairo_fill(cr);
	cairo_set_source_surface(cr, surf, wa.x, wa.y);
	cairo_rectangle(cr, wa.x + x, wa.y + y, width, height);
	cairo_clip(cr);
	cairo_paint(cr);
	cairo_fill(cr);
	cairo_restore(cr);
}

void popup_window_t::repair1(cairo_t * cr, int x, int y, int width,
		int height) {
	int left = max(wa.x,x);
	int rigth = min(wa.x + wa.width, x + width);
	int top = max(wa.y, y);
	int bottom = min(wa.y + wa.height, y + height);

	if ((bottom - top) > 0 && (rigth - left) > 0) {
		cairo_save(cr);
		cairo_set_source_surface(cr, surf, wa.x, wa.y);
		cairo_rectangle(cr, left, top, rigth - left, bottom - top);
		cairo_clip(cr);
		cairo_paint(cr);
		cairo_restore(cr);
	}
}

void popup_window_t::hide(cairo_t * cr, cairo_surface_t * s) {
	cairo_set_source_surface(cr, s, 0, 0);
	cairo_rectangle(cr, wa.x, wa.y, wa.width, wa.height);
	cairo_fill(cr);
}

bool popup_window_t::is_window(Window w) {
	return this->w == w;
}

popup_split_t::popup_split_t(int x, int y, int width, int height) :
		area(x, y, width, height) {

}

void popup_split_t::repair0(cairo_t * cr, cairo_surface_t * s, int x, int y,
		int width, int height) {
	/* never called */
}

void popup_split_t::repair1(cairo_t * cr, int x, int y, int width, int height) {
	int left = max(area.x, x);
	int rigth = min(area.x + area.w, x + width);
	int top = max(area.y, y);
	int bottom = min(area.y + area.h, y + height);

	if ((bottom - top) > 0 && (rigth - left) > 0) {
		cairo_save(cr);
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
		cairo_rectangle(cr, left, top, rigth - left, bottom - top);
		cairo_fill(cr);
		cairo_restore(cr);
	}
}

popup_split_t::~popup_split_t() {

}

bool popup_split_t::is_window(Window w) {
	return false;
}

void popup_split_t::hide(cairo_t * cr, cairo_surface_t * s) {
	cairo_set_source_surface(cr, s, 0, 0);
	cairo_rectangle(cr, area.x, area.y, area.w, area.h);
	cairo_fill(cr);
}

void popup_split_t::update_area(cairo_t * cr, cairo_surface_t * s, int x, int y,
		int width, int height) {
	hide(cr, s);
	area.x = x;
	area.y = y;
	area.w = width;
	area.h = height;
	repair1(cr, area.x, area.y, area.w, area.h);
}


popup_notebook_t::popup_notebook_t(int x, int y, int width, int height) :
		area(x, y, width, height) {

}

void popup_notebook_t::repair0(cairo_t * cr, cairo_surface_t * s, int x, int y,
		int width, int height) {
	/* never called */
}

void popup_notebook_t::repair1(cairo_t * cr, int x, int y, int width, int height) {
	int left = max(area.x, x);
	int rigth = min(area.x + area.w, x + width);
	int top = max(area.y, y);
	int bottom = min(area.y + area.h, y + height);

	if ((bottom - top) > 0 && (rigth - left) > 0) {
		cairo_save(cr);
		cairo_set_line_width(cr, 2.0);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_set_source_rgba(cr, 0.8, 0.8, 0.0, 1.0);
		cairo_rectangle(cr, left, top, rigth - left, bottom - top);
		cairo_clip(cr);
		cairo_rectangle(cr, area.x+3, area.y+3, area.w-6, area.h-6);
		cairo_stroke(cr);
		cairo_restore(cr);
	}
}

popup_notebook_t::~popup_notebook_t() {

}

bool popup_notebook_t::is_window(Window w) {
	return false;
}

void popup_notebook_t::hide(cairo_t * cr, cairo_surface_t * s) {
	cairo_set_source_surface(cr, s, 0, 0);
	cairo_rectangle(cr, area.x, area.y, area.w, area.h);
	cairo_fill(cr);
}

void popup_notebook_t::update_area(cairo_t * cr, cairo_surface_t * s, int x, int y,
		int width, int height) {
	hide(cr, s);
	area.x = x;
	area.y = y;
	area.w = width;
	area.h = height;
	repair1(cr, area.x, area.y, area.w, area.h);
}

}

