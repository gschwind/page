/*
 * page_root.cxx
 *
 *  Created on: 12 août 2015
 *      Author: gschwind
 */

#include <assert.h>

#include "page.hxx"
#include "page_root.hxx"
#include "workspace.hxx"

namespace page {

using namespace std;

page_root_t::page_root_t(page_t * ctx) :
		_ctx{ctx}
{
	_stack_is_locked = true;

	_workspace_stack = make_shared<tree_t>();

	push_back(_workspace_stack);

	_workspace_stack->show();

}

page_root_t::~page_root_t() {

}

string page_root_t::get_node_name() const {
	return _get_node_name<'P'>();
}

void page_root_t::render(cairo_t * cr, region const & area) {
//	auto pix = _ctx->theme()->get_background();
//
//	if (pix != nullptr) {
//		cairo_save(cr);
//		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
//		cairo_set_source_surface(cr, pix->get_cairo_surface(),
//				_root_position.x, _root_position.y);
//		region r = region{_root_position} & area;
//		for (auto &i : r.rects()) {
//			cairo_clip(cr, i);
//			cairo_mask_surface(cr, pix->get_cairo_surface(),
//					_root_position.x, _root_position.y);
//		}
//		cairo_restore(cr);
//	}
}

auto page_root_t::get_visible_region() -> region {
	return region{};
}

auto page_root_t::get_opaque_region() -> region {
	return region{};
}

auto page_root_t::get_damaged() -> region {
	return region{};
}

}
