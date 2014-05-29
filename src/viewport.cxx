/*
 * viewport.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <algorithm>
#include <typeinfo>
#include "notebook.hxx"
#include "viewport.hxx"

namespace page {

viewport_t::viewport_t(theme_t * theme, rectangle const & area) : raw_aera(area), effective_aera(area) {
	_subtree = nullptr;
	_is_visible = true;
	_theme = theme;
	_subtree = new notebook_t(_theme);
	_subtree->set_parent(this);
	_subtree->set_allocation(effective_aera);

}

void viewport_t::replace(tree_t * src, tree_t * by) {
	//printf("replace %p by %p\n", src, by);

	if (_subtree == src) {
		_subtree = by;
		_subtree->set_parent(this);
		_subtree->set_allocation(effective_aera);
	} else {
		throw std::runtime_error("viewport: bad child replacement!");
	}
}

void viewport_t::close(tree_t * src) {

}

void viewport_t::remove(tree_t * src) {
	if(src == _subtree) {
		cout << "WARNING do not use viewport_t::remove to remove subtree element" << endl;
		_subtree = nullptr;
		return;
	}

}

void viewport_t::reconfigure() {
	if(_subtree != nullptr) {
		_subtree->set_allocation(effective_aera);
	}
}

void viewport_t::set_allocation(rectangle const & area) {
	set_effective_area(area);
}

void viewport_t::set_raw_area(rectangle const & area) {
	raw_aera = area;
}

void viewport_t::set_effective_area(rectangle const & area) {
	effective_aera = area;
	if(_subtree != 0) {
		_subtree->set_allocation(effective_aera);
	}
}

rectangle viewport_t::get_absolute_extend() {
	return raw_aera;
}

}
