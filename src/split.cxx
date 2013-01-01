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

#include "split.hxx"

namespace page {

split_t::split_t(split_type_e type) {
	_split_type = type;
	_split = 0.5;
	_pack0 = 0;
	_pack1 = 0;

}

split_t::~split_t() {

}

notebook_t * split_t::get_nearest_notebook() {
	notebook_t * n = 0;
	if(_pack0 != 0)
		n = _pack0->get_nearest_notebook();
	if(n != 0)
		return n;
	if(_pack1 != 0)
		n = _pack1->get_nearest_notebook();
	if(n != 0)
		return n;
	return 0;
}

void split_t::update_allocation(box_t<int> &allocation) {
	_allocation = allocation;
	update_allocation_pack0();
	update_allocation_pack1();
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

//bool split_t::add_client(window_t * c) {
//	if (_pack0) {
//		if (!_pack0->add_client(c)) {
//			if (_pack1)
//				return _pack1->add_client(c);
//			else
//				return false;
//		} else
//			return true;
//	} else {
//		if (_pack1)
//			return _pack1->add_client(c);
//		else
//			return false;
//	}
//
//}

//box_int_t split_t::get_new_client_size() {
//	if (_pack0)
//		return _pack0->get_new_client_size();
//	if (_pack1)
//		return _pack1->get_new_client_size();
//	return box_int_t(0, 0, 0, 0);
//}

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

void split_t::close(tree_t * src) {
	_parent->replace(this, src);
}

//void split_t::activate_client(window_t * c) {
//	if (_pack0)
//		_pack0->activate_client(c);
//	if (_pack1)
//		_pack1->activate_client(c);
//}
//
//void split_t::iconify_client(window_t * c) {
//	if (_pack0)
//		_pack0->iconify_client(c);
//	if (_pack1)
//		_pack1->iconify_client(c);
//}


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

//void split_t::remove_client(window_t * c) {
//	if (_pack0)
//		_pack0->remove_client(c);
//	if (_pack1)
//		_pack1->remove_client(c);
//}
//
//void split_t::delete_all() {
//	if (_pack0) {
//		_pack0->delete_all();
//		delete _pack0;
//	}
//
//	if (_pack1) {
//		_pack1->delete_all();
//		delete _pack1;
//	}
//}
//
//
//void split_t::unmap_all() {
//	if (_pack0) {
//		_pack0->unmap_all();
//	}
//
//	if (_pack1) {
//		_pack1->unmap_all();
//	}
//}
//
//void split_t::map_all() {
//	if (_pack0) {
//		_pack0->map_all();
//	}
//
//	if (_pack1) {
//		_pack1->map_all();
//	}
//}

/* compute the slider area */
void split_t::compute_slider_area(box_int_t & area, double split) {
	if (_split_type == VERTICAL_SPLIT) {
		area.x = _allocation.x + (int) (split * _allocation.w) - GRIP_SIZE;
		area.y = _allocation.y;
		area.w = 2 * GRIP_SIZE;
		area.h = _allocation.h;
	} else {
		area.x = _allocation.x;
		area.y = _allocation.y + (int) (split * _allocation.h) - GRIP_SIZE;
		area.w = _allocation.w;
		area.h = 2 * GRIP_SIZE;
	}
}

void split_t::set_allocation(box_t<int> const & allocation) {
	_allocation = allocation;
	update_allocation_pack0();
	update_allocation_pack1();
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

	box_int_t x;
	compute_slider_area(x, _split);
	area = area + x;
	return area;
}

bool split_t::is_inside(int x, int y) {
	box_int_t area;
	compute_slider_area(area, _split);
	return area.is_inside(x, y);
}

}
