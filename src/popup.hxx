/*
 * popup.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_HXX_
#define POPUP_HXX_

#include "X11/extensions/Xdamage.h"
#include <cairo.h>
#include <cairo-xlib.h>
#include <list>
#include "box.hxx"

namespace page_next {

struct popup_t {
	virtual ~popup_t() { }
	virtual void repair0(cairo_t * cr, cairo_surface_t * s, int x, int y, int width, int height) = 0;
	virtual void repair1(cairo_t * cr, int x, int y, int width, int height) = 0;
	virtual void hide(cairo_t * cr, cairo_surface_t * s) = 0;
	virtual bool is_window(Window w) = 0;
	virtual void get_absolute_coord(int relative_x, int relative_y, int &absolute_x, int &absolute_y) = 0;
	virtual void get_extend(short &x, short &y, unsigned short &w, unsigned short &h) = 0;
	virtual void reconfigure(short x, short y, unsigned short w, unsigned short h) = 0;
};

typedef std::list<popup_t *> popup_list_t;

struct popup_window_t : public popup_t{
	Display * dpy;
	XWindowAttributes wa;
	Window w;
	Damage damage;
	cairo_surface_t * surf;

	popup_window_t(Display * dpy, Window w, XWindowAttributes &wa);
	~popup_window_t();
	virtual void repair0(cairo_t * cr, cairo_surface_t * s, int x, int y, int width, int height);
	virtual void repair1(cairo_t * cr, int x, int y, int width, int height);
	virtual void hide(cairo_t * cr, cairo_surface_t * s);
	virtual bool is_window(Window w);
	virtual void get_absolute_coord(int relative_x, int relative_y, int &absolute_x, int &absolute_y);
	virtual void get_extend(short &x, short &y, unsigned short &w, unsigned short &h);
	virtual void reconfigure(short x, short y, unsigned short w, unsigned short h);
};

struct popup_split_t : public popup_t {
	box_t<int> area;
	popup_split_t(int x, int y, int width, int height);
	~popup_split_t();
	virtual void repair0(cairo_t * cr, cairo_surface_t * s, int x, int y, int width, int height);
	virtual void repair1(cairo_t * cr, int x, int y, int width, int height);
	virtual void hide(cairo_t * cr, cairo_surface_t * s);
	virtual bool is_window(Window w);
	void update_area(cairo_t * cr, cairo_surface_t * s, int x, int y, int width, int height);
	virtual void get_absolute_coord(int relative_x, int relative_y, int &absolute_x, int &absolute_y);
	virtual void get_extend(short &x, short &y, unsigned short &w, unsigned short &h);
	virtual void reconfigure(short x, short y, unsigned short w, unsigned short h);
};

struct popup_notebook_t : public popup_t {
	box_t<int> area;
	popup_notebook_t(int x, int y, int width, int height);
	~popup_notebook_t();
	virtual void repair0(cairo_t * cr, cairo_surface_t * s, int x, int y, int width, int height);
	virtual void repair1(cairo_t * cr, int x, int y, int width, int height);
	virtual void hide(cairo_t * cr, cairo_surface_t * s);
	virtual bool is_window(Window w);
	void update_area(cairo_t * cr, cairo_surface_t * s, int x, int y, int width, int height);
	virtual void get_absolute_coord(int relative_x, int relative_y, int &absolute_x, int &absolute_y);
	virtual void get_extend(short &x, short &y, unsigned short &w, unsigned short &h);
	virtual void reconfigure(short x, short y, unsigned short w, unsigned short h);
};


}


#endif /* POPUP_HXX_ */
