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

viewport_t::viewport_t(page_context_t * ctx, rect const & area, bool keep_focus) :
		_ctx{ctx},
		_raw_aera{area},
		_effective_area{area},
		_parent{nullptr},
		_is_hidden{false},
		_is_durty{true},
		_win{XCB_NONE},
		_pix{XCB_NONE},
		_back_surf{nullptr}
{
	_page_area = rect{0, 0, _effective_area.w, _effective_area.h};
	create_window();
	_subtree = nullptr;
	_subtree = new notebook_t{_ctx, keep_focus};
	_subtree->set_parent(this);
	_subtree->set_allocation(_page_area);
}

void viewport_t::replace(page_component_t * src, page_component_t * by) {
	//printf("replace %p by %p\n", src, by);

	if (_subtree == src) {
		_subtree->set_parent(nullptr);
		_subtree = by;
		_subtree->set_parent(this);
		_subtree->set_allocation(_page_area);
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

void viewport_t::set_allocation(rect const & area) {
	_effective_area = area;
	_page_area = rect{0, 0, _effective_area.w, _effective_area.h};
	if(_subtree != nullptr)
		_subtree->set_allocation(_page_area);
	update_renderable();
}

void viewport_t::set_raw_area(rect const & area) {
	_raw_aera = area;
}

rect const & viewport_t::raw_area() const {
	return _raw_aera;
}


}
