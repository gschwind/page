/*
 * split.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <cstdio>
#include <cairo-xlib.h>
#include <X11/cursorfont.h>
#include <cmath>

#include "split.hxx"
#include "grab_handlers.hxx"

namespace page {

using namespace std;

split_t::split_t(page_context_t * ctx, split_type_e type) :
		_ctx{ctx},
		_type{type},
		_ratio{0.5}
{

}

split_t::~split_t() {

}

void split_t::set_allocation(rect const & allocation) {
	_allocation = allocation;
	update_allocation();
}

void split_t::update_allocation_pack0() {

	rect b;
	if (_type == VERTICAL_SPLIT) {

		rect bpack0;
		rect bpack1;

		int w = allocation().w - 2 * _ctx->theme()->split.margin.left - 2 * _ctx->theme()->split.margin.right - _ctx->theme()->split.width;
		int w0 = floor(w * _ratio + 0.5);
		int w1 = w - w0;

		bpack0.x = allocation().x + _ctx->theme()->split.margin.left;
		bpack0.y = allocation().y + _ctx->theme()->split.margin.top;
		bpack0.w = w0;
		bpack0.h = allocation().h - _ctx->theme()->split.margin.top - _ctx->theme()->split.margin.bottom;

		bpack1.x = allocation().x + _ctx->theme()->split.margin.left + w0 + _ctx->theme()->split.margin.right + _ctx->theme()->split.width + _ctx->theme()->split.margin.left;
		bpack1.y = allocation().y + _ctx->theme()->split.margin.top;
		bpack1.w = w1;
		bpack1.h = allocation().h - _ctx->theme()->split.margin.top - _ctx->theme()->split.margin.bottom;

		if(_pack0)
			_pack0->set_allocation(bpack0);
		if(_pack1)
			_pack1->set_allocation(bpack1);

	} else {
		b.x = allocation().x + _ctx->theme()->split.margin.left;
		b.y = allocation().y + _ctx->theme()->split.margin.top;
		b.w = allocation().w - _ctx->theme()->split.margin.left - _ctx->theme()->split.margin.right;
		b.h = floor(allocation().h * _ratio + 0.5) - _ctx->theme()->split.width - _ctx->theme()->split.margin.top - _ctx->theme()->split.margin.bottom;
		_pack0->set_allocation(b);
	}

}

void split_t::update_allocation_pack1() {
	if (_pack1 == nullptr)
		return;
	rect b;
	if (_type == VERTICAL_SPLIT) {

	} else {
		b.x = allocation().x + _ctx->theme()->split.margin.left;
		b.y = allocation().y + floor(allocation().h * _ratio + 0.5) + _ctx->theme()->split.width + _ctx->theme()->split.margin.top;
		b.w = allocation().w - _ctx->theme()->split.margin.left - _ctx->theme()->split.margin.right;
		b.h = allocation().h - floor(allocation().h * _ratio + 0.5) - _ctx->theme()->split.width - _ctx->theme()->split.margin.top - _ctx->theme()->split.margin.bottom;
	}
	_pack1->set_allocation(b);
}

void split_t::replace(shared_ptr<page_component_t> src, shared_ptr<page_component_t> by) {
	if (_pack0 == src) {
		printf("replace %p by %p\n", src.get(), by.get());
		set_pack0(by);
		src->clear_parent();
	} else if (_pack1 == src) {
		printf("replace %p by %p\n", src.get(), by.get());
		set_pack1(by);
		src->clear_parent();
	} else {
		throw std::runtime_error("split: bad child replacement!");
	}

	update_allocation();
}

void split_t::set_split(double split) {
	if(split < 0.05)
		split = 0.05;
	if(split > 0.95)
		split = 0.95;
	_ratio = split;
	update_allocation();
}

void split_t::update_allocation() {
	compute_split_location(_ratio, _split_bar_area.x, _split_bar_area.y);
	compute_split_size(_ratio, _split_bar_area.w, _split_bar_area.h);

	if (_type == VERTICAL_SPLIT) {

		int w = allocation().w - 2 * _ctx->theme()->split.margin.left - 2 * _ctx->theme()->split.margin.right - _ctx->theme()->split.width;
		int w0 = floor(w * _ratio + 0.5);
		int w1 = w - w0;

		bpack0.x = allocation().x + _ctx->theme()->split.margin.left;
		bpack0.y = allocation().y + _ctx->theme()->split.margin.top;
		bpack0.w = w0;
		bpack0.h = allocation().h - _ctx->theme()->split.margin.top - _ctx->theme()->split.margin.bottom;

		bpack1.x = allocation().x + _ctx->theme()->split.margin.left + w0 + _ctx->theme()->split.margin.right + _ctx->theme()->split.width + _ctx->theme()->split.margin.left;
		bpack1.y = allocation().y + _ctx->theme()->split.margin.top;
		bpack1.w = w1;
		bpack1.h = allocation().h - _ctx->theme()->split.margin.top - _ctx->theme()->split.margin.bottom;


	} else {

		int h = allocation().h - 2 * _ctx->theme()->split.margin.top - 2 * _ctx->theme()->split.margin.bottom - _ctx->theme()->split.width;
		int h0 = floor(h * _ratio + 0.5);
		int h1 = h - h0;

		bpack0.x = allocation().x + _ctx->theme()->split.margin.left;
		bpack0.y = allocation().y + _ctx->theme()->split.margin.top;
		bpack0.w = allocation().w - _ctx->theme()->split.margin.left - _ctx->theme()->split.margin.right;
		bpack0.h = h0;

		bpack1.x = allocation().x + _ctx->theme()->split.margin.left;
		bpack1.y = allocation().y + _ctx->theme()->split.margin.top + h0 + _ctx->theme()->split.margin.bottom + _ctx->theme()->split.width + _ctx->theme()->split.margin.top;
		bpack1.w = allocation().w - _ctx->theme()->split.margin.left - _ctx->theme()->split.margin.right;
		bpack1.h = h1;

	}

	if(_pack0 != nullptr)
		_pack0->set_allocation(bpack0);
	if(_pack1 != nullptr)
		_pack1->set_allocation(bpack1);

}

void split_t::set_pack0(shared_ptr<page_component_t> x) {
	_children.remove(_pack0);
	_pack0 = x;
	if (_pack0 != nullptr) {
		_pack0->set_parent(shared_from_this());
		_children.push_back(_pack0);
		update_allocation();
	}
}

void split_t::set_pack1(shared_ptr<page_component_t> x) {
	_children.remove(_pack1);
	_pack1 = x;
	if (_pack1 != nullptr) {
		_pack1->set_parent(shared_from_this());
		_children.push_back(_pack1);
		update_allocation();
	}
}

/* compute the slider area */
void split_t::compute_split_location(double split, int & x,
		int & y) const {
	rect const & alloc = allocation();
	if (_type == VERTICAL_SPLIT) {
		int w = alloc.w - 2 * _ctx->theme()->split.margin.left
				- 2 * _ctx->theme()->split.margin.right - _ctx->theme()->split.width;
		int w0 = floor(w * split + 0.5);
		x = alloc.x + _ctx->theme()->split.margin.left + w0
				+ _ctx->theme()->split.margin.right;
		y = alloc.y;
	} else {
		int h = alloc.h - 2 * _ctx->theme()->split.margin.top
				- 2 * _ctx->theme()->split.margin.bottom - _ctx->theme()->split.width;
		int h0 = floor(h * split + 0.5);
		x = alloc.x;
		y = alloc.y + _ctx->theme()->split.margin.top + h0
				+ _ctx->theme()->split.margin.bottom;
	}
}

/* compute the slider area */
void split_t::compute_split_size(double split, int & w, int & h) const {
	rect const & alloc = allocation();
	if (_type == VERTICAL_SPLIT) {
		w = _ctx->theme()->split.width;
		h = alloc.h;
	} else {
		w = alloc.w;
		h = _ctx->theme()->split.width;
	}
}

void split_t::render_legacy(cairo_t * cr) const {
	theme_split_t ts;
	ts.split = _ratio;
	ts.type = _type;
	ts.allocation = compute_split_bar_location();
	ts.root_x = get_window_position().x;
	ts.root_y = get_window_position().y;
	_ctx->theme()->render_split(cr, &ts);
}

void split_t::activate(shared_ptr<tree_t> t) {
	if(has_key(_children, t)) {
		if(_parent.lock() != nullptr) {
			_parent.lock()->activate(shared_from_this());
		}
		_children.remove(t);
		_children.push_back(t);
	} else if (t != nullptr) {
		throw exception_t("split_t::raise_child trying to raise a non child tree");
	}
}

void split_t::remove(shared_ptr<tree_t> t) {
	if (_pack0 == t) {
		set_pack0(nullptr);
	} else if (_pack1 == t) {
		set_pack1(nullptr);
	}
}

void split_t::update_layout(time64_t const time) {

}

rect split_t::compute_split_bar_location() const {

	rect ret;
	rect const & alloc = _allocation;

	if (_type == VERTICAL_SPLIT) {

		int w = alloc.w - 2 * _ctx->theme()->split.margin.left
				- 2 * _ctx->theme()->split.margin.right - _ctx->theme()->split.width;
		int w0 = floor(w * _ratio + 0.5);

		ret.x = alloc.x + _ctx->theme()->split.margin.left + w0
				+ _ctx->theme()->split.margin.right;
		ret.y = alloc.y;

	} else {

		int h = alloc.h - 2 * _ctx->theme()->split.margin.top
				- 2 * _ctx->theme()->split.margin.bottom - _ctx->theme()->split.width;
		int h0 = floor(h * _ratio + 0.5);

		ret.x = alloc.x;
		ret.y = alloc.y + _ctx->theme()->split.margin.top + h0
				+ _ctx->theme()->split.margin.bottom;
	}

	if (_type == VERTICAL_SPLIT) {
		ret.w = _ctx->theme()->split.width;
		ret.h = alloc.h;
	} else {
		ret.w = alloc.w;
		ret.h = _ctx->theme()->split.width;
	}

	return ret;

}


void split_t::children(vector<shared_ptr<tree_t>> & out) const {
	out.insert(out.end(), _children.begin(), _children.end());
}

bool split_t::button_press(xcb_button_press_event_t const * e) {
	if (e->event == get_parent_xid()
			and e->child == XCB_NONE
			and e->detail == XCB_BUTTON_INDEX_1
			and _split_bar_area.is_inside(e->event_x, e->event_y)) {
		_ctx->grab_start(new grab_split_t { _ctx, shared_from_this() });
		return true;
	} else {
		return false;
	}
}

shared_ptr<split_t> split_t::shared_from_this() {
	return dynamic_pointer_cast<split_t>(tree_t::shared_from_this());
}

auto split_t::get_node_name() const -> string {
	return _get_node_name<'S'>();
}

rect split_t::allocation() const {
	return _allocation;
}

auto split_t::get_visible_region() -> region {
	/** workspace do not render any thing **/
	return region{};
}


}
