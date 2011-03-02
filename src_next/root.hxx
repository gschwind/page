/*
 * root.hxx
 *
 *  Created on: Feb 27, 2011
 *      Author: gschwind
 */

#ifndef ROOT_HXX_
#define ROOT_HXX_

#include <cairo.h>
#include "tree.hxx"

namespace page_next {

class root_t: public tree_t {
	tree_t * _pack0;
public:
	root_t(Display * dpy, Window w, box_t<int> &allocation);
	void update_allocation(box_t<int> & allocation);

	void render(cairo_t * cr);
	bool process_button_press_event(XEvent const * e);
	void add_notebook(client_t *c);
	void replace(tree_t * src, tree_t * by);
	void close(tree_t * src);
	void remove(tree_t * src);
	std::list<client_t *> *  get_clients();
	void remove_client(Window w);
};

}

#endif /* ROOT_HXX_ */
