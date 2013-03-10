/*
 * viewport.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include <cassert>
#include <algorithm>
#include "notebook.hxx"
#include "viewport.hxx"

namespace page {

viewport_t::viewport_t(box_t<int> const & area) : raw_aera(area), effective_aera(area), fullscreen_client(0) {
	_subtree = 0;
	_is_visible = true;
}

void viewport_t::replace(tree_t * src, tree_t * by) {
	printf("replace %p by %p\n", src, by);

	if (_subtree == src) {
		_subtree = by;
		_subtree->set_parent(this);
		_subtree->set_allocation(effective_aera);
	}
}

void viewport_t::close(tree_t * src) {

}

void viewport_t::remove(tree_t * src) {

}

void viewport_t::reconfigure() {
	if(_subtree != 0) {
		_subtree->set_allocation(effective_aera);
	}
}

void viewport_t::set_allocation(box_int_t const & area) {
	raw_aera = area;
	reconfigure();
	//fix_allocation();
	//_subtree->reconfigure(effective_aera);
}

box_int_t viewport_t::get_absolute_extend() {
	return raw_aera;
}

region_t<int> viewport_t::get_area() {
	if(_is_visible && fullscreen_client == 0) {
		return _subtree->get_area();
	} else {
		return box_int_t();
	}
}

//tab_window_set_t viewport_t::get_windows() {
//	assert(_subtree != 0);
//	return _subtree->get_windows();
//}

//notebook_t * viewport_t::get_nearest_notebook() {
//	if(_subtree != 0)
//		return _subtree->get_nearest_notebook();
//	return 0;
//}

}
