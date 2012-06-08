/*
 * split.hxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#ifndef SPLIT_HXX_
#define SPLIT_HXX_

#include <cairo.h>

#include "box.hxx"
#include "tree.hxx"

namespace page_next {

enum split_type_t {
	HORIZONTAL_SPLIT, VERTICAL_SPLIT,
};

class split_t: public tree_t {
	Cursor cursor;
	box_t<int> separetion_bar;
	split_type_t _split_type;
	double _split;
	tree_t * _pack0;
	tree_t * _pack1;

public:
	split_t(split_type_t type);
	~split_t();
	void update_allocation(box_t<int> & alloc);
	void render(cairo_t * cr);
	bool process_button_press_event(XEvent const * e);
	bool add_notebook(client_t *c);
	void replace(tree_t * src, tree_t * by);
	cairo_t * get_cairo();
	void close(tree_t * src);
	void remove(tree_t * src);
	std::list<client_t *> * get_clients();
	void remove_client(Window w);
	void activate_client(client_t * c);

	void update_allocation_pack0();
	void update_allocation_pack1();

	void draw_box(GC gc, int x, int y, unsigned int width, unsigned int height);


};

}

#endif /* SPLIT_HXX_ */
