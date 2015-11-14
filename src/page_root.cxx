/*
 * page_root.cxx
 *
 *  Created on: 12 août 2015
 *      Author: gschwind
 */

#include <assert.h>
#include "page_root.hxx"

#include "workspace.hxx"

namespace page {

using namespace std;

page_root_t::page_root_t(page_context_t * ctx) :
		_ctx{ctx},
		_current_desktop{0}
{

}

page_root_t::~page_root_t() {

}

string page_root_t::get_node_name() const {
	return _get_node_name<'P'>();
}

void page_root_t::remove(shared_ptr<tree_t> t) {
	assert(has_key(children(), t));
	root_subclients.remove(dynamic_pointer_cast<client_base_t>(t));
	tooltips.remove(dynamic_pointer_cast<client_not_managed_t>(t));
	notifications.remove(dynamic_pointer_cast<client_not_managed_t>(t));
	above.remove(dynamic_pointer_cast<client_not_managed_t>(t));
	below.remove(dynamic_pointer_cast<client_not_managed_t>(t));
	_overlays.remove(t);

	t->clear_parent();
}

void page_root_t::append_children(vector<shared_ptr<tree_t>> & out) const {

	out.insert(out.end(), _desktop_stack.begin(), _desktop_stack.end());

//	for (auto i: _desktop_stack) {
//		out.push_back(i);
//	}

	for(auto x: below) {
		out.push_back(x);
	}

	for(auto x: root_subclients) {
		out.push_back(x);
	}

	for(auto x: tooltips) {
		out.push_back(x);
	}

	for(auto x: notifications) {
		out.push_back(x);
	}

	for(auto x: above) {
		out.push_back(x);
	}

	for(auto x: _overlays) {
		out.push_back(x);
	}

	if(_fps_overlay != nullptr) {
		out.push_back(_fps_overlay);
	}

}

void page_root_t::render(cairo_t * cr, region const & area) {
	auto pix = _ctx->theme()->get_background();

	if (pix != nullptr) {
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(cr, pix->get_cairo_surface(),
				_root_position.x, _root_position.y);
		region r = region{_root_position} & area;
		for (auto &i : r) {
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


void page_root_t::activate() {
	/* has no parent */
}

void page_root_t::activate(shared_ptr<tree_t> t) {
	assert(t != nullptr);
	assert(has_key(children(), t));

	auto w = dynamic_pointer_cast<workspace_t>(t);
	if(w != nullptr) {
		_ctx->switch_to_desktop(w->id());
	}

	/* do nothing, not needed at this level */
	auto x = dynamic_pointer_cast<client_base_t>(t);
	if(has_key(root_subclients, x)) {
		move_back(root_subclients, x);
	}

	auto y = dynamic_pointer_cast<client_not_managed_t>(t);
	if(has_key(tooltips, y)) {
		move_back(tooltips, y);
	}

	if(has_key(notifications, y)) {
		move_back(notifications, y);
	}

	if(has_key(above, y)) {
		move_back(above, y);
	}

	if(has_key(below, y)) {
		move_back(below, y);
	}
}

//void page_root_t::set_allocation(rect const & r) {
//	throw exception_t("page_t::set_allocation should be called");
//}
//
//rect page_root_t::allocation() const {
//	return _root_position;
//}
//
//void page_root_t::replace(shared_ptr<page_component_t> src, shared_ptr<page_component_t> by) {
//	throw exception_t("not implemented: %s", __PRETTY_FUNCTION__);
//}

}
