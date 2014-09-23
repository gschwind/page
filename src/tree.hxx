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
#include "time.hxx"
#include "renderable.hxx"

using namespace std;

namespace page {

/**
 * Currently the type of tree branch or leaf are not fully handle by
 * derivative class and cannot be fully handled by RTTI. Thus page use
 * this enumerate, until proper sub-typing will be implemented.
 **/
enum tree_type_e {
	TYPE_UNDEF,             /* Undefine type */
	TYPE_PAGE,              /* Root of tree (page class) */
	TYPE_VIEWPORT,          /* viewport class */
	TYPE_SPLIT,             /* split class */
	TYPE_NOTEBOOK,          /* notebook class */
	TYPE_MANAGED_CLIENT,    /* managed window, sub typing to fullscreen and floating ? */
	TYPE_DOCK,              /* unmanaged class (WM_TYPE_DOCK) */
	TYPE_DROPDOWN_MENU,     /* unmanaged class (WM_TYPE_DROPDOWN_MENU) */
	TYPE_TOOLTIPS,          /* unmanaged class (WM_TYPE_TOOLTIPS) */
	TYPE_OTHER_OVERLAY      /* unmanaged class (ALL unknown type) */
};

class tree_t {
protected:
	tree_t * _parent;
	rectangle _allocation;
	tree_type_e _type;
public:
	tree_t(tree_t * parent = nullptr, rectangle allocation = rectangle());

	virtual ~tree_t();
	virtual tree_t * parent() const;
	virtual void replace(tree_t * src, tree_t * by) = 0;
	virtual void set_allocation(rectangle const & area);
	virtual string get_node_name() const = 0;
	virtual list<tree_t *> childs() const = 0;
	virtual void raise_child(tree_t * t = nullptr) = 0;
	virtual void remove(tree_t * t) = 0;

	rectangle const & allocation() const;
	void set_parent(tree_t * parent);
	void set_allocation(double x, double y, double w, double h);
	list<tree_t *> get_all_childs() const;
	void print_tree(unsigned level = 0);
	tree_type_e type();

	template<char const c>
	string _get_node_name() const {
		char buffer[64];
		snprintf(buffer, 64, "%c #%016lx #%016lx", c, (uintptr_t) _parent,
				(uintptr_t) this);
		return string(buffer);
	}

	virtual vector<ptr<renderable_t>> prepare_render(page::time_t const & time) = 0;

};


}

#endif /* TREE_HXX_ */
