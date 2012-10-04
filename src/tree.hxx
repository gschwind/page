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
#include "renderable.hxx"
#include "window.hxx"

namespace page {
class tree_t : public renderable_t {
protected:
	tree_t * _parent;
	box_t<int> _allocation;
public:
	tree_t(tree_t * parent = 0, box_t<int> allocation = box_t<int>());
	virtual ~tree_t() { }

	virtual void replace(tree_t * src, tree_t * by) = 0;
	virtual void remove(tree_t * src) = 0;
	virtual void reparent(tree_t * parent);
	virtual void close(tree_t * src) = 0;
	virtual window_list_t get_windows() = 0;

	virtual bool add_client(window_t * x) = 0;
	virtual box_int_t get_new_client_size() = 0;
	virtual void remove_client(window_t * x) = 0;
	virtual void activate_client(window_t * x) = 0;
	virtual void iconify_client(window_t * x) = 0;
	virtual void delete_all() = 0;

	virtual void repair1(cairo_t * cr, box_int_t const & area) = 0;
	virtual box_int_t get_absolute_extend() = 0;
	virtual void reconfigure(box_int_t const & area) = 0;

	virtual bool is_visible() {
		return false;
	}

};
}

#endif /* TREE_HXX_ */
