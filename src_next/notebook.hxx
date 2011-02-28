/*
 * notebook.hxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#ifndef NOTEBOOK_HXX_
#define NOTEBOOK_HXX_

#include <list>
#include <cairo.h>

#include "box.hxx"
#include "client.hxx"
#include "tree.hxx"
#include "split.hxx"

namespace page_next {

class notebook_t: public tree_t {
	static std::list<notebook_t *> notebooks;
	Cursor cursor;

	cairo_surface_t * close_img;
	cairo_surface_t * vsplit_img;
	cairo_surface_t * hsplit_img;

	box_t<int> button_close;
	box_t<int> button_vsplit;
	box_t<int> button_hsplit;

	int group;
	std::list<client_t *> _clients;
	std::list<client_t *>::iterator _selected;
public:
	notebook_t(int group = 0);
	void update_allocation(box_t<int> & allocation);
	void render(cairo_t * cr);
	bool process_button_press_event(XEvent const * e);
	void add_notebook(client_t *c);
	void split(split_type_t type);
	void update_client_mapping();
	cairo_t * get_cairo();
	void replace(tree_t * src, tree_t * by);
};

}

#endif /* NOTEBOOK_HXX_ */
