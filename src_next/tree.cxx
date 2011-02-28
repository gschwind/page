/*
 * tree.cxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#include "tree.hxx"

namespace page_next {

tree_t::tree_t() {
	_parent = 0;
}

tree_t::tree_t(Display * dpy, Window w, tree_t * parent, box_t<int> allocation) :
	_dpy(dpy), _w(w), _parent(parent), _allocation(allocation) {
}

void tree_t::reparent(tree_t * parent) {
	_parent = parent;
	_dpy = _parent->_dpy;
	_w = _parent->_w;
}

}
