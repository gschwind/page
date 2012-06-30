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
	virtual void repair1(cairo_t * cr, box_int_t const & area) = 0;
	virtual bool is_window(Window w) = 0;
	virtual box_int_t get_absolute_extend() = 0;
	virtual void reconfigure(box_int_t const & area) = 0;
};

typedef std::list<popup_t *> popup_list_t;

struct popup_window_t : public popup_t{
	Display * dpy;
	box_int_t area;
	Window w;
	Damage damage;
	cairo_surface_t * surf;
	Visual * visual;

	popup_window_t(Display * dpy, Window w, XWindowAttributes &wa);
	~popup_window_t();
	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual bool is_window(Window w);
	virtual box_int_t get_absolute_extend();
	virtual void reconfigure(box_int_t const & area);
};

struct popup_split_t : public popup_t {
	box_t<int> area;
	popup_split_t(box_t<int> const & area);
	~popup_split_t();
	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual bool is_window(Window w);
	virtual box_int_t get_absolute_extend();
	virtual void reconfigure(box_int_t const & area);
};

struct popup_notebook_t : public popup_t {
	box_t<int> area;
	popup_notebook_t(int x, int y, int width, int height);
	~popup_notebook_t();
	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual bool is_window(Window w);
	virtual box_int_t get_absolute_extend();
	virtual void reconfigure(box_int_t const & area);
};


}


#endif /* POPUP_HXX_ */
