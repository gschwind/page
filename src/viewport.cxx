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

viewport_t::viewport_t(theme_t * theme, i_rect const & area) :
		_raw_aera(area),
		_effective_aera(area),
		_parent(nullptr)
{
	_subtree = nullptr;
	_is_visible = true;
	_theme = theme;
	_subtree = new notebook_t(_theme);
	_subtree->set_parent(this);
	_subtree->set_allocation(_effective_aera);

}

void viewport_t::replace(page_component_t * src, page_component_t * by) {
	//printf("replace %p by %p\n", src, by);

	if (_subtree == src) {
		_subtree = by;
		_subtree->set_parent(this);
		_subtree->set_allocation(_effective_aera);
	} else {
		throw std::runtime_error("viewport: bad child replacement!");
	}
}

void viewport_t::remove(tree_t * src) {
	if(src == _subtree) {
		cout << "WARNING do not use viewport_t::remove to remove subtree element" << endl;
		_subtree = nullptr;
		return;
	}

}

void viewport_t::set_allocation(i_rect const & area) {
	_effective_aera = area;
	if(_subtree != nullptr)
		_subtree->set_allocation(_effective_aera);
}

void viewport_t::set_raw_area(i_rect const & area) {
	_raw_aera = area;
}

i_rect const & viewport_t::raw_area() const {
	return _raw_aera;
}

}
