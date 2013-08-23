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

#include "managed_window_base.hxx"

using namespace std;

namespace page {

enum split_type_e {
	HORIZONTAL_SPLIT, VERTICAL_SPLIT,
};

class tree_t {
	tree_t * _parent;
	box_int_t _allocation;
public:
	tree_t(tree_t * parent = 0, box_int_t allocation = box_int_t());
	virtual ~tree_t() { }

	tree_t * parent() const {
		return _parent;
	}

	box_int_t const & allocation() const {
		return _allocation;
	}


	virtual void replace(tree_t * src, tree_t * by) = 0;
	virtual void get_childs(list<tree_t *> & lst) = 0;
	virtual void set_allocation(box_int_t const & area) {
		_allocation = area;
	}

	void set_parent(tree_t * parent);
	void set_allocation(int x, int y, int w, int h);

};

struct notebook_base_t : public tree_t {
	virtual ~notebook_base_t() { }
	virtual list<managed_window_base_t const *> clients() const = 0;
	virtual managed_window_base_t const * selected() const = 0;
};

struct split_base_t : public tree_t {
	virtual ~split_base_t() { }
	virtual split_type_e type() const = 0;
	virtual double split() const = 0;
};

struct viewport_base_t : public tree_t {
	virtual ~viewport_base_t() { }
};


}

#endif /* TREE_HXX_ */
