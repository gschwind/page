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

#include "managed_window_base.hxx"

using namespace std;

namespace page {

enum split_type_e {
	HORIZONTAL_SPLIT, VERTICAL_SPLIT,
};

class tree_t {
	tree_t * _parent;
	rectangle _allocation;
public:
	tree_t(tree_t * parent = 0, rectangle allocation = rectangle());
	virtual ~tree_t() { }

	virtual tree_t * parent() const {
		return _parent;
	}

	rectangle const & allocation() const {
		return _allocation;
	}

	virtual void replace(tree_t * src, tree_t * by) = 0;
	virtual void get_childs(vector<tree_t *> & lst) = 0;
	virtual void set_allocation(rectangle const & area) {
		_allocation = area;
	}

	void set_parent(tree_t * parent);
	void set_allocation(double x, double y, double w, double h);

	virtual string get_node_name() const = 0;

	virtual vector<tree_t *> get_direct_childs() const = 0;

	void print_tree(unsigned level = 0) {
		char clevel[] = "                ";
		if(level < sizeof(clevel))
			clevel[level] = 0;
		printf("%s%s\n", clevel, get_node_name().c_str());
		vector<tree_t *> v = get_direct_childs();
		for(vector<tree_t *>::iterator i = v.begin(); i != v.end(); ++i) {
			(*i)->print_tree(level + 1);
		}
	}

};

struct notebook_base_t : public tree_t {
	virtual ~notebook_base_t() { }
	virtual list<managed_window_base_t const *> clients() const = 0;
	virtual managed_window_base_t const * selected() const = 0;
	virtual bool is_default() const = 0;

	virtual string get_node_name() const {
		char buffer[32];
		snprintf(buffer, 32, "N #%016lx", (uintptr_t)this);
		return string(buffer);
	}

};

struct split_base_t : public tree_t {
	virtual ~split_base_t() { }
	virtual split_type_e type() const = 0;
	virtual double split() const = 0;

	virtual string get_node_name() const {
		char buffer[32];
		snprintf(buffer, 32, "S #%016lx", (uintptr_t)this);
		return string(buffer);
	}

};

struct viewport_base_t : public tree_t {
	virtual ~viewport_base_t() { }


	virtual string get_node_name() const {
		char buffer[32];
		snprintf(buffer, 32, "V #%016lx", (uintptr_t)this);
		return string(buffer);
	}

};


}

#endif /* TREE_HXX_ */
