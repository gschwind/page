/*
 * tree.hxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

#ifndef TREE_HXX_
#define TREE_HXX_

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <list>
#include "client.hxx"

namespace page_next {

struct box_t {
	int x, y;
	int width, height;

	inline bool is_inside(int _x, int _y) {
		return (x < _x && _x < x + width && y < _y && _y < y + height);
	}

};

enum split_type_t {
	HORIZONTAL_SPLIT, VERTICAL_SPLIT,
};

class tree_t {

private:

	struct vtable_t {
		void (tree_t::*_update_allocation)(box_t & alloc);
		void (tree_t::*_render)(cairo_t * cr);
		bool (tree_t::*_process_button_press_event)(XEvent const * e);
		void (tree_t::*_add_notebook)(client_t * c);
		bool (tree_t::*_is_selected)(int x, int y);
	};

	struct shared_t {
		Display * _dpy;
		Window _w;
		std::list<tree_t *> note_link;
		Cursor cursor;
		cairo_surface_t * close_img;
		cairo_surface_t * vsplit_img;
		cairo_surface_t * hsplit_img;
		vtable_t vtable_split;
		vtable_t vtable_notebook;
	};

	shared_t * _shared;

	box_t _allocation;
	int _selected;
	std::list<client_t *> _clients;
	tree_t * _parent;
	double _split;

	split_type_t _split_type;

	tree_t * _pack0;
	tree_t * _pack1;

	/* disable copy */
	tree_t(tree_t const &);
	tree_t &operator=(tree_t const &);

	void split_update_allocation(box_t & alloc);
	void split_render(cairo_t * cr);
	bool split_process_button_press_event(XEvent const * e);
	void split_add_notebook(client_t * c);
	bool split_is_selected(int x, int y);

	void notebook_update_allocation(box_t & alloc);
	void notebook_render(cairo_t * cr);
	bool notebook_process_button_press_event(XEvent const * e);
	void notebook_add_notebook(client_t * c);
	bool notebook_is_selected(int x, int y);

	vtable_t _vtable;

	bool is_selected(int x, int y);

	tree_t(shared_t * s, tree_t * parent);
public:

	tree_t(Display * dpy, Window w);
	~tree_t();

	void update_allocation(box_t & alloc);
	void render(cairo_t * cr);
	bool process_button_press_event(XEvent const * e);
	void add_notebook(client_t * c);

	void mutate_to_split(split_type_t type);
	void mutate_to_notebook(tree_t * pack);
	cairo_t * get_cairo();

};

}

#endif /* TREE_HXX_ */
