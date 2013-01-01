/*
 * split.hxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#ifndef SPLIT_HXX_
#define SPLIT_HXX_

#include "box.hxx"
#include "tree.hxx"

namespace page {

enum split_type_e {
	HORIZONTAL_SPLIT, VERTICAL_SPLIT,
};

class split_t: public tree_t {

public:
	static int const GRIP_SIZE = 3;

	box_t<int> separetion_bar;
	split_type_e _split_type;
	double _split;
	tree_t * _pack0;
	tree_t * _pack1;

	static Bool drag_and_drop_filter(Display * dpy, XEvent * ev, char * arg);
	void process_drag_and_drop();

public:
	split_t(split_type_e type);
	~split_t();
	void update_allocation(box_t<int> & alloc);
	void replace(tree_t * src, tree_t * by);
	cairo_t * get_cairo();
	void close(tree_t * src);

	notebook_t * get_nearest_notebook();

	bool is_inside(int x, int y);

	window_set_t get_windows();

	bool add_client(window_t *c);
	box_int_t get_new_client_size();
	void remove_client(window_t * c);
	void activate_client(window_t * c);
	void iconify_client(window_t * c);

	void update_allocation_pack0();
	void update_allocation_pack1();
	void delete_all();
	void unmap_all();
	void map_all();

	void compute_slider_area(box_int_t & area, double split);

	virtual box_int_t get_absolute_extend();
	virtual void set_allocation(box_int_t const & area);
	virtual region_t<int> get_area();

};

}

#endif /* SPLIT_HXX_ */
