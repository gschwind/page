/*
 * tree.cxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#include "tree.hxx"

namespace page {

tree_t::tree_t(tree_t * parent, box_t<int> allocation) :
		_parent(parent), _allocation(allocation) {
}

void tree_t::set_parent(tree_t * parent) {
	_parent = parent;
}

}
