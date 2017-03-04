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
		_ctx{ctx},
		_current_workspace{0}
{
	_stack_is_locked = true;

	below = make_shared<tree_t>();
	root_subclients = make_shared<tree_t>();
	tooltips = make_shared<tree_t>();
	notifications = make_shared<tree_t>();
	above = make_shared<tree_t>();

	_workspace_stack = make_shared<tree_t>();
	_overlays = make_shared<tree_t>();

	push_back(_workspace_stack);
	push_back(below);
	push_back(root_subclients);
	push_back(tooltips);
	push_back(notifications);
	push_back(above);
	push_back(_overlays);

	below->show();
	root_subclients->show();
	tooltips->show();
	notifications->show();
	above->show();
	_workspace_stack->show();
	_overlays->show();

}

page_root_t::~page_root_t() {

}

string page_root_t::get_node_name() const {
	return _get_node_name<'P'>();
}

void page_root_t::render(cairo_t * cr, region const & area) {
	auto pix = _ctx->theme()->get_background();

	if (pix != nullptr) {
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(cr, pix->get_cairo_surface(),
				_root_position.x, _root_position.y);
		region r = region{_root_position} & area;
		for (auto &i : r.rects()) {
			cairo_clip(cr, i);
			cairo_mask_surface(cr, pix->get_cairo_surface(),
					_root_position.x, _root_position.y);
		}
		cairo_restore(cr);
	}
}

auto page_root_t::get_visible_region() -> region {
	return region{_root_position};
}

auto page_root_t::get_opaque_region() -> region {
	return region{_root_position};
}

auto page_root_t::get_damaged() -> region {
	return region{};
}

}
