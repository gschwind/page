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
#include "client.hxx"

namespace page_next {
class tree_t {
protected:
	tree_t * _parent;
	box_t<int> _allocation;
public:
	tree_t(tree_t * parent = 0, box_t<int> allocation = box_t<int>());
	virtual ~tree_t() {
	}
	virtual void update_allocation(box_t<int> & alloc) = 0;
	virtual void render() = 0;
	virtual bool process_button_press_event(XEvent const * e) = 0;
	virtual bool add_client(client_t *c) = 0;
	virtual void replace(tree_t * src, tree_t * by) = 0;
	virtual void remove(tree_t * src) = 0;
	virtual void reparent(tree_t * parent);
	virtual void close(tree_t * src) = 0;
	virtual client_list_t * get_clients() = 0;
	virtual void remove_client(client_t * c) = 0;
	virtual void activate_client(client_t * c) = 0;
	virtual void iconify_client(client_t * c) = 0;

};
}

#endif /* TREE_HXX_ */
