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
#include "region.hxx"

namespace page {

class notebook_t;

class tree_t {
public:
	tree_t * _parent;
	box_int_t _allocation;
public:
	tree_t(tree_t * parent = 0, box_int_t allocation = box_int_t());
	virtual ~tree_t() { }

	virtual void replace(tree_t * src, tree_t * by) = 0;
	virtual void set_parent(tree_t * parent);
	virtual box_int_t get_absolute_extend() = 0;
	virtual region_t<int> get_area() = 0;

	virtual void set_allocation(box_int_t const & area) = 0;

	virtual void set_allocation(int x, int y, int w, int h) {
		set_allocation(box_int_t(x, y, w, h));
	}

};

}

#endif /* TREE_HXX_ */
