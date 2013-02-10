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

split_t::split_t(split_type_e type) :
	tree_t(),
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
	_allocation = allocation;
	update_allocation();
}

void split_t::update_allocation_pack0() {
	if (!_pack0)
		return;
	box_t<int> b;
	if (_split_type == VERTICAL_SPLIT) {
		b.x = _allocation.x;
		b.y = _allocation.y;
		b.w = _allocation.w * _split - GRIP_SIZE;
		b.h = _allocation.h;
	} else {
		b.x = _allocation.x;
		b.y = _allocation.y;
		b.w = _allocation.w;
		b.h = _allocation.h * _split - GRIP_SIZE;
	}
	_pack0->set_allocation(b);
}

void split_t::update_allocation_pack1() {
	if (!_pack1)
		return;
	box_t<int> b;
	if (_split_type == VERTICAL_SPLIT) {
		b.x = _allocation.x + _allocation.w * _split + GRIP_SIZE;
		b.y = _allocation.y;
		b.w = _allocation.w - _allocation.w * _split - GRIP_SIZE;
		b.h = _allocation.h;
	} else {
		b.x = _allocation.x;
		b.y = _allocation.y + _allocation.h * _split + GRIP_SIZE;
		b.w = _allocation.w;
		b.h = _allocation.h - _allocation.h * _split - GRIP_SIZE;
	}
	_pack1->set_allocation(b);
}

void split_t::replace(tree_t * src, tree_t * by) {
	if (_pack0 == src) {
		printf("replace %p by %p\n", src, by);
		_pack0 = by;
		_pack0->set_parent(this);
		update_allocation_pack0();
	} else if (_pack1 == src) {
		printf("replace %p by %p\n", src, by);
		_pack1 = by;
		_pack1->set_parent(this);
		update_allocation_pack1();
	}
}

window_set_t split_t::get_windows() {
	window_set_t list;
	if (_pack0) {
		window_set_t pack0_list = _pack0->get_windows();
		list.insert(pack0_list.begin(), pack0_list.end());
	}
	if (_pack1) {
		window_set_t pack1_list = _pack0->get_windows();
		list.insert(pack1_list.begin(), pack1_list.end());
	}
	return list;
}

/* compute the slider area */
void split_t::compute_split_bar_area(box_int_t & area, double split) const {

	if (_split_type == VERTICAL_SPLIT) {
		area.x = _allocation.x + (int) floor(split * _allocation.w) - GRIP_SIZE;
		area.y = _allocation.y;
		area.w = 2 * GRIP_SIZE;
		area.h = _allocation.h;
	} else {
		area.x = _allocation.x;
		area.y = _allocation.y + (int) floor(split * _allocation.h) - GRIP_SIZE;
		area.w = _allocation.w;
		area.h = 2 * GRIP_SIZE;
	}
}

box_int_t split_t::get_absolute_extend() {
	return _allocation;
}

region_t<int> split_t::get_area() {
	region_t<int> area;
	if (_pack0)
		area = area + _pack0->get_area();
	if (_pack1)
		area = area + _pack1->get_area();
	area = area + _split_bar_area;
	return area;
}

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
	update_allocation_pack0();
	update_allocation_pack1();
}

}
