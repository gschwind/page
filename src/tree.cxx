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

tree_t::tree_t(tree_t * parent, box_t<int> allocation) :
		_parent(parent), _allocation(allocation) {
}

void tree_t::set_parent(tree_t * parent) {
	_parent = parent;
}

void tree_t::set_allocation(int x, int y, int w, int h) {
	set_allocation(box_int_t(x, y, w, h));
}

}
