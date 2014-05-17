/*
 * tree.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#include "tree.hxx"

#include <cassert>

#include "utils.hxx"

namespace page {

tree_t::tree_t(tree_t * parent, rectangle allocation) :
		_parent(parent), _allocation(allocation), _type(TYPE_UNDEF) {
}

tree_t::~tree_t() {
}

void tree_t::set_parent(tree_t * parent) {
	_parent = parent;
}

void tree_t::set_allocation(double x, double y, double w, double h) {
	set_allocation(rectangle(x, y, w, h));
}

void tree_t::print_tree(unsigned level) {
	char clevel[] = "                ";
	if (level < sizeof(clevel))
		clevel[level] = 0;
	printf("%s%s\n", clevel, get_node_name().c_str());
	list<tree_t *> v = childs();
	for (auto i : v) {
		i->print_tree(level + 1);
	}
}

list<tree_t *> tree_t::get_all_childs() const {
	list<tree_t *> ret;
	list<tree_t *> v = childs();
	for (auto i : v) {
		ret.push_back(i);
		ret.splice(ret.end(), i->get_all_childs());
	}
	return ret;
}

void tree_t::set_allocation(rectangle const & area) {
	_allocation = area;
}

rectangle const & tree_t::allocation() const {
	return _allocation;
}

tree_t * tree_t::parent() const {
	return _parent;
}

tree_type_e tree_t::type() {
	return _type;
}


}
