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
#include "popup_split.hxx"

namespace page {

split_t::split_t(page_base_t & page, split_type_e type) :
		page(page), even_handler(*this) {
	_split_type = type;
	_split = 0.5;
	_pack0 = 0;
	_pack1 = 0;

	cursor = None;

	page.get_xconnection().add_event_handler(&even_handler);

}

split_t::~split_t() {
	page.get_xconnection().remove_event_handler(&even_handler);
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
	_pack0->reconfigure(b);
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
	_pack1->reconfigure(b);
}

void split_t::repair1(cairo_t * cr, box_int_t const & area) {
	box_int_t clip = _allocation & area;
	if (clip.h > 0 && clip.w > 0) {
		cairo_save(cr);
		cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
		cairo_clip(cr);
		cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0, 0xecU / 255.0);
		if (_split_type == VERTICAL_SPLIT) {
			cairo_rectangle(cr, _allocation.x + _allocation.w * _split - GRIP_SIZE, _allocation.y, GRIP_SIZE * 2.0, _allocation.h);
		} else {
			cairo_rectangle(cr, _allocation.x, _allocation.y + (_allocation.h * _split) - GRIP_SIZE, _allocation.w, GRIP_SIZE * 2.0);
		}
		cairo_fill(cr);
		cairo_restore(cr);
		if (_pack0)
			_pack0->repair1(cr, area);
		if (_pack1)
			_pack1->repair1(cr, area);
	}
}

void split_t::xevent_handler_t::process_event(XEvent const & e) {
	if (e.type == ButtonPress) {
		if (split._allocation.is_inside(e.xbutton.x, e.xbutton.y)) {
			box_t<int> slide;
			if (split._split_type == VERTICAL_SPLIT) {
				slide.y = split._allocation.y;
				slide.h = split._allocation.h;
				slide.x = split._allocation.x + (split._allocation.w * split._split) - split.GRIP_SIZE;
				slide.w = GRIP_SIZE * 2;
			} else {
				slide.y = split._allocation.y + (split._allocation.h * split._split) - split.GRIP_SIZE;
				slide.h = split.GRIP_SIZE * 2;
				slide.x = split._allocation.x;
				slide.w = split._allocation.w;
			}

			if (slide.is_inside(e.xbutton.x, e.xbutton.y)) {
				split.process_drag_and_drop();
			} else {
				/* the pack test is for sanity, but should never be null */
				if (split._pack0) {
					/* avoid double close */
//				if (!_pack0->process_button_press_event(e) && _pack1) {
//					_pack1->process_button_press_event(e);
//				}
				}
			}
			return;
		}
		return;
	}
}

bool split_t::add_client(window_t * c) {
	if (_pack0) {
		if (!_pack0->add_client(c)) {
			if (_pack1)
				return _pack1->add_client(c);
			else
				return false;
		} else
			return true;
	} else {
		if (_pack1)
			return _pack1->add_client(c);
		else
			return false;
	}

}

box_int_t split_t::get_new_client_size() {
	if (_pack0)
		return _pack0->get_new_client_size();
	if (_pack1)
		return _pack1->get_new_client_size();
	return box_int_t(0, 0, 0, 0);
}

void split_t::replace(tree_t * src, tree_t * by) {
	if (_pack0 == src) {
		printf("replace %p by %p\n", src, by);
		_pack0 = by;
		_pack0->reparent(this);
		update_allocation_pack0();
	} else if (_pack1 == src) {
		printf("replace %p by %p\n", src, by);
		_pack1 = by;
		_pack1->reparent(this);
		update_allocation_pack1();
	}
}

void split_t::close(tree_t * src) {
	_parent->replace(this, src);
}

void split_t::remove(tree_t * src) {
	assert(src == _pack0 || src == _pack1);

	page.get_render_context().add_damage_area(_allocation);

	tree_t * dst = (src == _pack0) ? _pack1 : _pack0;

	window_list_t windows = src->get_windows();
	window_list_t::iterator i = windows.begin();
	while (i != windows.end()) {
		dst->add_client((*i));
		++i;
	}

	_parent->replace(this, dst);
	//_parent->render();
	delete src;
	/* self destruction ^^ */
	delete this;
}

void split_t::activate_client(window_t * c) {
	if (_pack0)
		_pack0->activate_client(c);
	if (_pack1)
		_pack1->activate_client(c);
}

void split_t::iconify_client(window_t * c) {
	if (_pack0)
		_pack0->iconify_client(c);
	if (_pack1)
		_pack1->iconify_client(c);
}

window_list_t split_t::get_windows() {
	window_list_t list;
	if (_pack0) {
		window_list_t pack0_list = _pack0->get_windows();
		list.insert(list.end(), pack0_list.begin(), pack0_list.end());
	}
	if (_pack1) {
		window_list_t pack1_list = _pack0->get_windows();
		list.insert(list.end(), pack1_list.begin(), pack1_list.end());
	}
	return list;
}

void split_t::remove_client(window_t * c) {
	if (_pack0)
		_pack0->remove_client(c);
	if (_pack1)
		_pack1->remove_client(c);
}

void split_t::process_drag_and_drop() {
	xconnection_t & cnx = page.get_xconnection();
	XEvent ev;
	cursor = XCreateFontCursor(cnx.dpy, XC_fleur);
	popup_split_t * p;

	box_int_t slider_area;

	compute_slider_area(slider_area);

	p = new popup_split_t(slider_area);
	p->z = 1000;
	page.get_render_context().overlay_add(p);

	if (XGrabPointer(cnx.dpy, cnx.xroot, False, (ButtonPressMask | ButtonReleaseMask | PointerMotionMask),
	GrabModeAsync, GrabModeAsync, None, cursor,
	CurrentTime) != GrabSuccess)
		return;

	do {

		XIfEvent(cnx.dpy, &ev, drag_and_drop_filter, (char*) this);

		if (ev.type == cnx.damage_event + XDamageNotify) {
			page.process_event(reinterpret_cast<XDamageNotifyEvent&>(ev));
		} else if (ev.type == MotionNotify) {
			if (_split_type == VERTICAL_SPLIT) {
				_split = (ev.xmotion.x - _allocation.x) / (double) (_allocation.w);
			} else {
				_split = (ev.xmotion.y - _allocation.y) / (double) (_allocation.h);
			}

			if (_split > 0.95)
				_split = 0.95;
			if (_split < 0.05)
				_split = 0.05;

			/* Render slider with quite complex render method to avoid flickering */
			box_int_t old_area = slider_area;
			compute_slider_area(slider_area);

			p->area = slider_area;

			page.get_render_context().add_damage_overlay_area(old_area);
			page.get_render_context().add_damage_overlay_area(slider_area);
		}

		//page.get_render_context().render_flush();
	} while (ev.type != ButtonRelease);
	page.get_render_context().overlay_remove(p);
	delete p;
	XUngrabPointer(cnx.dpy, CurrentTime);
	XFreeCursor(cnx.dpy, cursor);
	update_allocation(_allocation);
	page.get_render_context().add_damage_area(_allocation);
}

Bool split_t::drag_and_drop_filter(Display * dpy, XEvent * ev, char * arg) {
	split_t * ths = reinterpret_cast<split_t *>(arg);
	return (ev->type == ConfigureRequest) || (ev->type == Expose) || (ev->type == MotionNotify) || (ev->type == ButtonRelease)
			|| (ev->type == ths->page.get_xconnection().damage_event + XDamageNotify);
}

void split_t::delete_all() {
	if (_pack0) {
		_pack0->delete_all();
		delete _pack0;
	}

	if (_pack1) {
		_pack1->delete_all();
		delete _pack1;
	}
}


void split_t::unmap_all() {
	if (_pack0) {
		_pack0->unmap_all();
	}

	if (_pack1) {
		_pack1->unmap_all();
	}
}

void split_t::map_all() {
	if (_pack0) {
		_pack0->map_all();
	}

	if (_pack1) {
		_pack1->map_all();
	}
}

/* compute the slider area */
void split_t::compute_slider_area(box_int_t & area) {
	if (_split_type == VERTICAL_SPLIT) {
		area.x = _allocation.x + (int) (_split * _allocation.w) - GRIP_SIZE;
		area.y = _allocation.y;
		area.w = 2 * GRIP_SIZE;
		area.h = _allocation.h;

	} else {
		area.x = _allocation.x;
		area.y = _allocation.y + (int) (_split * _allocation.h) - GRIP_SIZE;
		area.w = _allocation.w;
		area.h = 2 * GRIP_SIZE;
	}
}

void split_t::reconfigure(box_t<int> const & allocation) {
	_allocation = allocation;
	update_allocation_pack0();
	update_allocation_pack1();
}

box_int_t split_t::get_absolute_extend() {
	return _allocation;
}

}
