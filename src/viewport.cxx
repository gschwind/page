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

viewport_t::viewport_t(display_t * cnx, theme_t * theme, i_rect const & area) :
		_cnx(cnx),
		_raw_aera(area),
		_effective_area(area),
		_parent(nullptr),
		_is_hidden(false),
		_is_durty(true),
		_win(XCB_NONE),
		_pix(XCB_NONE),
		_back_surf(nullptr)
{
	_page_area = i_rect{0, 0, _effective_area.w, _effective_area.h};
	_subtree = nullptr;
	_theme = theme;
	_subtree = new notebook_t(_theme);
	_subtree->set_parent(this);
	_subtree->set_allocation(_page_area);

	create_window();

}

void viewport_t::replace(page_component_t * src, page_component_t * by) {
	//printf("replace %p by %p\n", src, by);

	if (_subtree == src) {
		_subtree->set_parent(nullptr);
		_subtree = by;
		_subtree->set_parent(this);
		_subtree->set_allocation(_effective_area);
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
	_effective_area = area;
	_page_area = i_rect{0, 0, _effective_area.w, _effective_area.h};
	if(_subtree != nullptr)
		_subtree->set_allocation(_effective_area);
	update_renderable();
}

void viewport_t::set_raw_area(i_rect const & area) {
	_raw_aera = area;
}

i_rect const & viewport_t::raw_area() const {
	return _raw_aera;
}

void viewport_t::get_all_children(std::vector<tree_t *> & out) const {
	if (_subtree != nullptr) {
		out.push_back(_subtree);
		_subtree->get_all_children(out);
	}
}

}
