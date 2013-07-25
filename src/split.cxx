/*
 * split.cxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#include <cassert>
#include <cstdio>
#include <cairo-xlib.h>
#include <X11/cursorfont.h>
#include <cmath>

#include "split.hxx"

namespace page {

split_t::split_t(split_type_e type, theme_layout_t const * theme) :
	tree_t(),
	_split_bar_area(),
	_split_type(type),
	_split(0.5),
	_pack0(0),
	_pack1(0)
{
	set_theme(theme);
}

split_t::~split_t() {

}

void split_t::set_allocation(box_t<int> const & allocation) {
	_allocation = allocation;
	update_allocation();
}

void split_t::update_allocation_pack0() {

	box_t<int> b;
	if (_split_type == VERTICAL_SPLIT) {

		box_int_t bpack0;
		box_int_t bpack1;

		int w = _allocation.w - 2 * theme->split_margin.left - 2 * theme->split_margin.right - theme->split_width;
		int w0 = floor(w * _split + 0.5);
		int w1 = w - w0;

		bpack0.x = _allocation.x + theme->split_margin.left;
		bpack0.y = _allocation.y + theme->split_margin.top;
		bpack0.w = w0;
		bpack0.h = _allocation.h - theme->split_margin.top - theme->split_margin.bottom;

		bpack1.x = _allocation.x + theme->split_margin.left + w0 + theme->split_margin.right + theme->split_width + theme->split_margin.left;
		bpack1.y = _allocation.y + theme->split_margin.top;
		bpack1.w = w1;
		bpack1.h = _allocation.h - theme->split_margin.top - theme->split_margin.bottom;

		if(_pack0)
			_pack0->set_allocation(bpack0);
		if(_pack1)
			_pack1->set_allocation(bpack1);

	} else {
		b.x = _allocation.x + theme->split_margin.left;
		b.y = _allocation.y + theme->split_margin.top;
		b.w = _allocation.w - theme->split_margin.left - theme->split_margin.right;
		b.h = floor(_allocation.h * _split + 0.5) - theme->split_width - theme->split_margin.top - theme->split_margin.bottom;
		_pack0->set_allocation(b);
	}

}

void split_t::update_allocation_pack1() {
	if (!_pack1)
		return;
	box_t<int> b;
	if (_split_type == VERTICAL_SPLIT) {

	} else {
		b.x = _allocation.x + theme->split_margin.left;
		b.y = _allocation.y + floor(_allocation.h * _split + 0.5) + theme->split_width + theme->split_margin.top;
		b.w = _allocation.w - theme->split_margin.left - theme->split_margin.right;
		b.h = _allocation.h - floor(_allocation.h * _split + 0.5) - theme->split_width - theme->split_margin.top - theme->split_margin.bottom;
	}
	_pack1->set_allocation(b);
}

void split_t::replace(tree_t * src, tree_t * by) {
	if (_pack0 == src) {
		//printf("replace %p by %p\n", src, by);
		set_pack0(by);
	} else if (_pack1 == src) {
		//printf("replace %p by %p\n", src, by);
		set_pack1(by);
	}

	update_allocation();
}

//tab_window_set_t split_t::get_windows() {
//	tab_window_set_t list;
//	if (_pack0) {
//		tab_window_set_t pack0_list = _pack0->get_windows();
//		list.insert(pack0_list.begin(), pack0_list.end());
//	}
//	if (_pack1) {
//		tab_window_set_t pack1_list = _pack0->get_windows();
//		list.insert(pack1_list.begin(), pack1_list.end());
//	}
//	return list;
//}

/* compute the slider area */
void split_t::compute_split_bar_area(box_int_t & area, double split) const {

	if (_split_type == VERTICAL_SPLIT) {

		int w = _allocation.w - 2 * theme->split_margin.left - 2 * theme->split_margin.right - theme->split_width;
		int w0 = floor(w * split + 0.5);
		int w1 = w - w0;

		area.x = _allocation.x + theme->split_margin.left + w0 + theme->split_margin.right;
		area.y = _allocation.y;
		area.w = theme->split_width;
		area.h = _allocation.h;
	} else {

		int h = _allocation.h - 2 * theme->split_margin.top - 2 * theme->split_margin.bottom - theme->split_width;
		int h0 = floor(h * split + 0.5);
		int h1 = h - h0;

		area.x = _allocation.x;
		area.y = _allocation.y + theme->split_margin.top + h0 + theme->split_margin.bottom;
		area.w = _allocation.w;
		area.h = theme->split_width;
	}
}

box_int_t split_t::get_absolute_extend() {
	return _allocation;
}

//region_t<int> split_t::get_area() {
//	region_t<int> area;
//	if (_pack0)
//		area = area + _pack0->get_area();
//	if (_pack1)
//		area = area + _pack1->get_area();
//	area = area + _split_bar_area;
//	return area;
//}

void split_t::set_split(double split) {
	if(split < 0.05)
		split = 0.05;
	if(split > 0.95)
		split = 0.95;
	_split = split;
	update_allocation();
}

box_int_t const & split_t::get_split_bar_area() const {
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
	compute_split_bar_area(_split_bar_area, _split);

	if (_split_type == VERTICAL_SPLIT) {

		int w = _allocation.w - 2 * theme->split_margin.left - 2 * theme->split_margin.right - theme->split_width;
		int w0 = floor(w * _split + 0.5);
		int w1 = w - w0;

		bpack0.x = _allocation.x + theme->split_margin.left;
		bpack0.y = _allocation.y + theme->split_margin.top;
		bpack0.w = w0;
		bpack0.h = _allocation.h - theme->split_margin.top - theme->split_margin.bottom;

		bpack1.x = _allocation.x + theme->split_margin.left + w0 + theme->split_margin.right + theme->split_width + theme->split_margin.left;
		bpack1.y = _allocation.y + theme->split_margin.top;
		bpack1.w = w1;
		bpack1.h = _allocation.h - theme->split_margin.top - theme->split_margin.bottom;


	} else {

		int h = _allocation.h - 2 * theme->split_margin.top - 2 * theme->split_margin.bottom - theme->split_width;
		int h0 = floor(h * _split + 0.5);
		int h1 = h - h0;

		bpack0.x = _allocation.x + theme->split_margin.left;
		bpack0.y = _allocation.y + theme->split_margin.top;
		bpack0.w = _allocation.w - theme->split_margin.left - theme->split_margin.right;
		bpack0.h = h0;

		bpack1.x = _allocation.x + theme->split_margin.left;
		bpack1.y = _allocation.y + theme->split_margin.top + h0 + theme->split_margin.bottom + theme->split_width + theme->split_margin.top;
		bpack1.w = _allocation.w - theme->split_margin.left - theme->split_margin.right;
		bpack1.h = h1;

	}

	if(_pack0 != 0)
		_pack0->set_allocation(bpack0);
	if(_pack1 != 0)
		_pack1->set_allocation(bpack1);

}

double split_t::get_split_ratio() {
	return _split;
}

void split_t::set_theme(theme_layout_t const * theme) {
	this->theme = theme;
}

void split_t::get_childs(list<tree_t *> & lst) {

	if(_pack0 != 0) {
		_pack0->get_childs(lst);
		lst.push_back(_pack0);
	}

	if(_pack1 != 0) {
		_pack1->get_childs(lst);
		lst.push_back(_pack1);
	}

}

void split_t::set_pack0(tree_t * x) {
	_pack0 = x;
	x->set_parent(this);
	update_allocation();
}

void split_t::set_pack1(tree_t * x) {
	_pack1 = x;
	x->set_parent(this);
	update_allocation();
}

}
