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

namespace page {

split_t::split_t(split_type_e type, theme_t const * theme, tree_t * p0,
		tree_t * p1) :
		split_base_t(), _theme(theme), _split_bar_area(), _split_type(type), _split(
				0.5), _pack0(nullptr), _pack1(nullptr) {

	set_pack0(p0);
	set_pack1(p1);

}

split_t::~split_t() {

}

void split_t::set_allocation(i_rect const & allocation) {
	tree_t::set_allocation(allocation);
	update_allocation();
}

void split_t::update_allocation_pack0() {

	i_rect b;
	if (_split_type == VERTICAL_SPLIT) {

		i_rect bpack0;
		i_rect bpack1;

		int w = allocation().w - 2 * _theme->split_margin.left - 2 * _theme->split_margin.right - _theme->split_width;
		int w0 = floor(w * _split + 0.5);
		int w1 = w - w0;

		bpack0.x = allocation().x + _theme->split_margin.left;
		bpack0.y = allocation().y + _theme->split_margin.top;
		bpack0.w = w0;
		bpack0.h = allocation().h - _theme->split_margin.top - _theme->split_margin.bottom;

		bpack1.x = allocation().x + _theme->split_margin.left + w0 + _theme->split_margin.right + _theme->split_width + _theme->split_margin.left;
		bpack1.y = allocation().y + _theme->split_margin.top;
		bpack1.w = w1;
		bpack1.h = allocation().h - _theme->split_margin.top - _theme->split_margin.bottom;

		if(_pack0)
			_pack0->set_allocation(bpack0);
		if(_pack1)
			_pack1->set_allocation(bpack1);

	} else {
		b.x = allocation().x + _theme->split_margin.left;
		b.y = allocation().y + _theme->split_margin.top;
		b.w = allocation().w - _theme->split_margin.left - _theme->split_margin.right;
		b.h = floor(allocation().h * _split + 0.5) - _theme->split_width - _theme->split_margin.top - _theme->split_margin.bottom;
		_pack0->set_allocation(b);
	}

}

void split_t::update_allocation_pack1() {
	if (_pack1 == nullptr)
		return;
	i_rect b;
	if (_split_type == VERTICAL_SPLIT) {

	} else {
		b.x = allocation().x + _theme->split_margin.left;
		b.y = allocation().y + floor(allocation().h * _split + 0.5) + _theme->split_width + _theme->split_margin.top;
		b.w = allocation().w - _theme->split_margin.left - _theme->split_margin.right;
		b.h = allocation().h - floor(allocation().h * _split + 0.5) - _theme->split_width - _theme->split_margin.top - _theme->split_margin.bottom;
	}
	_pack1->set_allocation(b);
}

void split_t::replace(tree_t * src, tree_t * by) {
	if (_pack0 == src) {
		printf("replace %p by %p\n", src, by);
		set_pack0(by);
	} else if (_pack1 == src) {
		printf("replace %p by %p\n", src, by);
		set_pack1(by);
	} else {
		throw std::runtime_error("split: bad child replacement!");
	}

	update_allocation();
}

i_rect split_t::get_absolute_extend() {
	return allocation();
}

void split_t::set_split(double split) {
	if(split < 0.05)
		split = 0.05;
	if(split > 0.95)
		split = 0.95;
	_split = split;
	update_allocation();
}

i_rect const & split_t::get_split_bar_area() const {
	return _split_bar_area;
}

tree_t * split_t::get_pack0() {
	return _pack0;
}
tree_t * split_t::get_pack1() {
	return _pack1;
}

split_type_e split_t::get_split_type() {
	return _split_type;
}

void split_t::update_allocation() {
	compute_split_location(_split, _split_bar_area.x, _split_bar_area.y);
	compute_split_size(_split, _split_bar_area.w, _split_bar_area.h);

	if (_split_type == VERTICAL_SPLIT) {

		int w = allocation().w - 2 * _theme->split_margin.left - 2 * _theme->split_margin.right - _theme->split_width;
		int w0 = floor(w * _split + 0.5);
		int w1 = w - w0;

		bpack0.x = allocation().x + _theme->split_margin.left;
		bpack0.y = allocation().y + _theme->split_margin.top;
		bpack0.w = w0;
		bpack0.h = allocation().h - _theme->split_margin.top - _theme->split_margin.bottom;

		bpack1.x = allocation().x + _theme->split_margin.left + w0 + _theme->split_margin.right + _theme->split_width + _theme->split_margin.left;
		bpack1.y = allocation().y + _theme->split_margin.top;
		bpack1.w = w1;
		bpack1.h = allocation().h - _theme->split_margin.top - _theme->split_margin.bottom;


	} else {

		int h = allocation().h - 2 * _theme->split_margin.top - 2 * _theme->split_margin.bottom - _theme->split_width;
		int h0 = floor(h * _split + 0.5);
		int h1 = h - h0;

		bpack0.x = allocation().x + _theme->split_margin.left;
		bpack0.y = allocation().y + _theme->split_margin.top;
		bpack0.w = allocation().w - _theme->split_margin.left - _theme->split_margin.right;
		bpack0.h = h0;

		bpack1.x = allocation().x + _theme->split_margin.left;
		bpack1.y = allocation().y + _theme->split_margin.top + h0 + _theme->split_margin.bottom + _theme->split_width + _theme->split_margin.top;
		bpack1.w = allocation().w - _theme->split_margin.left - _theme->split_margin.right;
		bpack1.h = h1;

	}

	if(_pack0 != nullptr)
		_pack0->set_allocation(bpack0);
	if(_pack1 != nullptr)
		_pack1->set_allocation(bpack1);

}

double split_t::get_split_ratio() {
	return _split;
}

void split_t::set_theme(theme_t const * theme) {
	_theme = theme;
}

void split_t::set_pack0(tree_t * x) {
	_children.remove(_pack0);
	_pack0 = x;
	if (_pack0 != nullptr) {
		_pack0->set_parent(this);
		_children.push_back(_pack0);
		update_allocation();
	}
}

void split_t::set_pack1(tree_t * x) {
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
	i_rect const & alloc = allocation();
	if (_split_type == VERTICAL_SPLIT) {
		int w = alloc.w - 2 * _theme->split_margin.left
				- 2 * _theme->split_margin.right - _theme->split_width;
		int w0 = floor(w * split + 0.5);
		x = alloc.x + _theme->split_margin.left + w0
				+ _theme->split_margin.right;
		y = alloc.y;
	} else {
		int h = alloc.h - 2 * _theme->split_margin.top
				- 2 * _theme->split_margin.bottom - _theme->split_width;
		int h0 = floor(h * split + 0.5);
		x = alloc.x;
		y = alloc.y + _theme->split_margin.top + h0
				+ _theme->split_margin.bottom;
	}
}

/* compute the slider area */
void split_t::compute_split_size(double split, int & w, int & h) const {
	i_rect const & alloc = allocation();
	if (_split_type == VERTICAL_SPLIT) {
		w = _theme->split_width;
		h = alloc.h;
	} else {
		w = alloc.w;
		h = _theme->split_width;
	}
}

double split_t::split() const {
	return _split;
}

split_type_e split_t::type() const {
	return _split_type;
}

void split_t::render_legacy(cairo_t * cr, i_rect const & area) const {
	_theme->render_split(cr, this, area);
}

list<tree_t *> split_t::childs() const {
	list<tree_t *> ret(_children.begin(), _children.end());
	return ret;
}

void split_t::raise_child(tree_t * t) {

	if(has_key(_children, t)) {
		_children.remove(t);
		_children.push_back(t);
	}

	if(_parent != nullptr) {
		_parent->raise_child(this);
	}

}

void split_t::remove(tree_t * t) {
	if (_pack0 == t) {
		set_pack0(nullptr);
	} else if (_pack1 == t) {
		set_pack1(nullptr);
	}
}

void split_t::prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time) {
	for(auto i: _children) {
		i->prepare_render(out, time);
	}
}


}
