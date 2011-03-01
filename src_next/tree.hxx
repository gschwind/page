/*
 * tree.hxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#ifndef TREE_HXX_
#define TREE_HXX_

#include <X11/Xlib.h>
#include <cairo.h>
#include <list>
#include "box.hxx"
#include "client.hxx"

namespace page_next {
class tree_t {
protected:
	tree_t * _parent;
	box_t<int> _allocation;
	Display * _dpy;
	Window _w;
public:
	tree_t();
	tree_t(Display * dpy, Window w, tree_t * parent = 0, box_t<int> allocation =
			box_t<int>());
	virtual void update_allocation(box_t<int> & alloc) = 0;
	virtual void render(cairo_t * cr) = 0;
	virtual bool process_button_press_event(XEvent const * e) = 0;
	virtual void add_notebook(client_t *c) = 0;
	virtual void replace(tree_t * src, tree_t * by) = 0;
	virtual void remove(tree_t * src) = 0;
	virtual void reparent(tree_t * parent);
	virtual void close(tree_t * src) = 0;
	virtual std::list<client_t *> * get_clients() = 0;

};
}

#endif /* TREE_HXX_ */
