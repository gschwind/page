/*
 * viewport.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include <cassert>
#include <algorithm>
#include <typeinfo>
#include "notebook.hxx"
#include "viewport.hxx"

namespace page {

viewport_t::viewport_t(box_t<int> const & area) : raw_aera(area), effective_aera(area), fullscreen_client(0) {
	_subtree = 0;
	_is_visible = true;
}

viewport_t::viewport_t() {
	_subtree = 0;
	_is_visible = true;
	fullscreen_client = 0;
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
	if (_is_visible && fullscreen_client == 0) {
		region_t<int> r(effective_aera);
//		list<notebook_t *> lst;
//		get_notebooks(lst);
//		for (list<notebook_t *>::iterator i = lst.begin(); i != lst.end();
//				++i) {
//			if ((*i)->get_selected() != 0)
//				r = r - ((*i)->get_selected())->get_base_position();
//		}

		return r;
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

void viewport_t::get_childs(list<tree_t *> & lst) {
	if(_subtree != 0) {
		_subtree->get_childs(lst);
		lst.push_back(_subtree);
	}
}

//notebook_t * viewport_t::new_notebook() {
//	notebook_t * n = new notebook_t();
//	return n;
//}

void viewport_t::get_notebooks(list<notebook_t *> & l) {
	list<tree_t *> lt;
	get_childs(lt);
	for (list<tree_t *>::iterator i = lt.begin(); i != lt.end(); ++i)
		l.push_back(dynamic_cast<notebook_t *>(*i));
	l.remove(0);
}

void viewport_t::get_splits(list<split_t *> & l) {
	list<tree_t *> lt;
	get_childs(lt);
	for (list<tree_t *>::iterator i = lt.begin(); i != lt.end(); ++i)
		l.push_back(dynamic_cast<split_t *>(*i));
	l.remove(0);
}

}
