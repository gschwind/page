/*
 * tree.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef TREE_HXX_
#define TREE_HXX_

#include <stdint.h>

#include <X11/Xlib.h>
#include <cairo.h>
#include <list>
#include <vector>
#include "region.hxx"

using namespace std;

namespace page {

class tree_t {
protected:
	tree_t * _parent;
	rectangle _allocation;
public:
	tree_t(tree_t * parent = nullptr, rectangle allocation = rectangle());

	virtual ~tree_t();
	virtual tree_t * parent() const;
	virtual void replace(tree_t * src, tree_t * by) = 0;
	virtual void set_allocation(rectangle const & area);
	virtual string get_node_name() const = 0;
	virtual list<tree_t *> childs() const = 0;
	virtual void raise_child(tree_t * t = nullptr) = 0;

	rectangle const & allocation() const;
	void set_parent(tree_t * parent);
	void set_allocation(double x, double y, double w, double h);
	list<tree_t *> get_all_childs() const;
	void print_tree(unsigned level = 0);

};


}

#endif /* TREE_HXX_ */
