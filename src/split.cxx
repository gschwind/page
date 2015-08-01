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

split_t::split_t(page_context_t * ctx, split_type_e type) :
		_ctx{ctx},
		_split_bar_area(),
		_type(type),
		_ratio{0.5},
		_pack0(nullptr),
		_pack1(nullptr),
		_parent(nullptr),
		_is_hidden(false)
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

void split_t::replace(page_component_t * src, page_component_t * by) {
	if (_pack0 == src) {
		printf("replace %p by %p\n", src, by);
		set_pack0(by);
		src->set_parent(nullptr);
	} else if (_pack1 == src) {
		printf("replace %p by %p\n", src, by);
		set_pack1(by);
		src->set_parent(nullptr);
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

void split_t::set_pack0(page_component_t * x) {
	_children.remove(_pack0);
	_pack0 = x;
	if (_pack0 != nullptr) {
		_pack0->set_parent(this);
		_children.push_back(_pack0);
		update_allocation();
	}
}

void split_t::set_pack1(page_component_t * x) {
	_children.remove(_pack1);
	_pack1 = x;
	if (_pack1 != nullptr) {
		_pack1->set_parent(this);
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

void split_t::activate(tree_t * t) {
	if(has_key(_children, t)) {
		if(_parent != nullptr) {
			_parent->activate(this);
		}
		_children.remove(t);
		_children.push_back(t);
	} else if (t != nullptr) {
		throw exception_t("split_t::raise_child trying to raise a non child tree");
	}
}

void split_t::remove(tree_t * t) {
	if (_pack0 == t) {
		set_pack0(nullptr);
	} else if (_pack1 == t) {
		set_pack1(nullptr);
	}
}

void split_t::prepare_render(std::vector<std::shared_ptr<renderable_t>> & out, page::time_t const & time) {
	if(_is_hidden)
		return;
	for(auto i: _children) {
		i->prepare_render(out, time);
	}
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
//
//void split_t::get_all_children(std::vector<tree_t *> & out) const {
//	for(auto x: _children) {
//		out.push_back(x);
//		x->get_all_children(out);
//	}
//}

void split_t::children(std::vector<tree_t *> & out) const {
	out.insert(out.end(), _children.begin(), _children.end());
}

void split_t::hide() {
	_is_hidden = true;
	for(auto i: tree_t::children()) {
		i->hide();
	}
}

void split_t::show() {
	_is_hidden = false;
	for(auto i: tree_t::children()) {
		i->show();
	}
}

void split_t::get_visible_children(std::vector<tree_t *> & out) {
	if (not _is_hidden) {
		out.push_back(this);
		for (auto i : tree_t::children()) {
			i->get_visible_children(out);
		}
	}
}

void split_t::set_parent(tree_t * t) {
	if(t == nullptr) {
		_parent = nullptr;
		return;
	}

	auto xt = dynamic_cast<page_component_t*>(t);
	if(xt == nullptr) {
		throw exception_t("page_component_t must have a page_component_t as parent");
	} else {
		_parent = xt;
	}
}

bool split_t::button_press(xcb_button_press_event_t const * e) {
	if (e->event == get_window()
			and e->child == XCB_NONE
			and e->detail == XCB_BUTTON_INDEX_1
			and _split_bar_area.is_inside(e->event_x, e->event_y)) {
		_ctx->grab_start(new grab_split_t { _ctx, this });
		return true;
	} else {
		return false;
	}
}

}
