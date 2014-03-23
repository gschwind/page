/*
 * tree.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "tree.hxx"

namespace page {

tree_t::tree_t(tree_t * parent, rectangle allocation) :
		_parent(parent), _allocation(allocation) {
}

void tree_t::set_parent(tree_t * parent) {
	_parent = parent;
}

void tree_t::set_allocation(double x, double y, double w, double h) {
	set_allocation(rectangle(x, y, w, h));
}

}
