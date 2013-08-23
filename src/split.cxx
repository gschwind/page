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

split_t::split_t(split_type_e type, theme_t const * theme) :
	split_base_t(),
	_theme(theme),
	_split_bar_area(),
	_split_type(type),
	_split(0.5),
	_pack0(0),
	_pack1(0)
{

}

split_t::~split_t() {

}

void split_t::set_allocation(box_t<int> const & allocation) {
	tree_t::set_allocation(allocation);
	update_allocation();
}

void split_t::update_allocation_pack0() {

	box_t<int> b;
	if (_split_type == VERTICAL_SPLIT) {

		box_int_t bpack0;
		box_int_t bpack1;

		int w = allocation().w - 2 * _theme->layout()->split_margin.left - 2 * _theme->layout()->split_margin.right - _theme->layout()->split_width;
		int w0 = floor(w * _split + 0.5);
		int w1 = w - w0;

		bpack0.x = allocation().x + _theme->layout()->split_margin.left;
		bpack0.y = allocation().y + _theme->layout()->split_margin.top;
		bpack0.w = w0;
		bpack0.h = allocation().h - _theme->layout()->split_margin.top - _theme->layout()->split_margin.bottom;

		bpack1.x = allocation().x + _theme->layout()->split_margin.left + w0 + _theme->layout()->split_margin.right + _theme->layout()->split_width + _theme->layout()->split_margin.left;
		bpack1.y = allocation().y + _theme->layout()->split_margin.top;
		bpack1.w = w1;
		bpack1.h = allocation().h - _theme->layout()->split_margin.top - _theme->layout()->split_margin.bottom;

		if(_pack0)
			_pack0->set_allocation(bpack0);
		if(_pack1)
			_pack1->set_allocation(bpack1);

	} else {
		b.x = allocation().x + _theme->layout()->split_margin.left;
		b.y = allocation().y + _theme->layout()->split_margin.top;
		b.w = allocation().w - _theme->layout()->split_margin.left - _theme->layout()->split_margin.right;
		b.h = floor(allocation().h * _split + 0.5) - _theme->layout()->split_width - _theme->layout()->split_margin.top - _theme->layout()->split_margin.bottom;
		_pack0->set_allocation(b);
	}

}

void split_t::update_allocation_pack1() {
	if (!_pack1)
		return;
	box_t<int> b;
	if (_split_type == VERTICAL_SPLIT) {

	} else {
		b.x = allocation().x + _theme->layout()->split_margin.left;
		b.y = allocation().y + floor(allocation().h * _split + 0.5) + _theme->layout()->split_width + _theme->layout()->split_margin.top;
		b.w = allocation().w - _theme->layout()->split_margin.left - _theme->layout()->split_margin.right;
		b.h = allocation().h - floor(allocation().h * _split + 0.5) - _theme->layout()->split_width - _theme->layout()->split_margin.top - _theme->layout()->split_margin.bottom;
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

box_int_t split_t::get_absolute_extend() {
	return allocation();
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
	compute_split_location(_split, _split_bar_area.x, _split_bar_area.y);
	compute_split_size(_split, _split_bar_area.w, _split_bar_area.h);

	if (_split_type == VERTICAL_SPLIT) {

		int w = allocation().w - 2 * _theme->layout()->split_margin.left - 2 * _theme->layout()->split_margin.right - _theme->layout()->split_width;
		int w0 = floor(w * _split + 0.5);
		int w1 = w - w0;

		bpack0.x = allocation().x + _theme->layout()->split_margin.left;
		bpack0.y = allocation().y + _theme->layout()->split_margin.top;
		bpack0.w = w0;
		bpack0.h = allocation().h - _theme->layout()->split_margin.top - _theme->layout()->split_margin.bottom;

		bpack1.x = allocation().x + _theme->layout()->split_margin.left + w0 + _theme->layout()->split_margin.right + _theme->layout()->split_width + _theme->layout()->split_margin.left;
		bpack1.y = allocation().y + _theme->layout()->split_margin.top;
		bpack1.w = w1;
		bpack1.h = allocation().h - _theme->layout()->split_margin.top - _theme->layout()->split_margin.bottom;


	} else {

		int h = allocation().h - 2 * _theme->layout()->split_margin.top - 2 * _theme->layout()->split_margin.bottom - _theme->layout()->split_width;
		int h0 = floor(h * _split + 0.5);
		int h1 = h - h0;

		bpack0.x = allocation().x + _theme->layout()->split_margin.left;
		bpack0.y = allocation().y + _theme->layout()->split_margin.top;
		bpack0.w = allocation().w - _theme->layout()->split_margin.left - _theme->layout()->split_margin.right;
		bpack0.h = h0;

		bpack1.x = allocation().x + _theme->layout()->split_margin.left;
		bpack1.y = allocation().y + _theme->layout()->split_margin.top + h0 + _theme->layout()->split_margin.bottom + _theme->layout()->split_width + _theme->layout()->split_margin.top;
		bpack1.w = allocation().w - _theme->layout()->split_margin.left - _theme->layout()->split_margin.right;
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

void split_t::set_theme(theme_t const * theme) {
	_theme = theme;
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
