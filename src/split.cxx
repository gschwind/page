/*
 * split.cxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#include <stdio.h>
#include <cairo-xlib.h>
#include <X11/cursorfont.h>
#include "split.hxx"
#include "X11/extensions/Xrender.h"

namespace page_next {

split_t::split_t(main_t & page, split_type_e type) :
		page(page) {
	_split_type = type;
	_split = 0.5;
	_pack0 = 0;
	_pack1 = 0;
}

split_t::~split_t() {

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
	_pack0->update_allocation(b);
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
	_pack1->update_allocation(b);
}

void split_t::render() {
	cairo_save(page.main_window_cr);
	cairo_set_source_rgb(page.main_window_cr, 0xeeU / 255.0, 0xeeU / 255.0,
			0xecU / 255.0);
	if (_split_type == VERTICAL_SPLIT) {
		cairo_rectangle(page.main_window_cr,
				_allocation.x + _allocation.w * _split - GRIP_SIZE,
				_allocation.y, GRIP_SIZE * 2.0, _allocation.h);
	} else {
		cairo_rectangle(page.main_window_cr, _allocation.x,
				_allocation.y + (_allocation.h * _split) - GRIP_SIZE,
				_allocation.w, GRIP_SIZE * 2.0);
	}
	cairo_fill(page.main_window_cr);
	cairo_restore(page.main_window_cr);
	if (_pack0)
		_pack0->render();
	if (_pack1)
		_pack1->render();
}

bool split_t::process_button_press_event(XEvent const * e) {
	if (_allocation.is_inside(e->xbutton.x, e->xbutton.y)) {
		box_t<int> slide;
		if (_split_type == VERTICAL_SPLIT) {
			slide.y = _allocation.y;
			slide.h = _allocation.h;
			slide.x = _allocation.x + (_allocation.w * _split) - GRIP_SIZE;
			slide.w = GRIP_SIZE * 2;
		} else {
			slide.y = _allocation.y + (_allocation.h * _split) - GRIP_SIZE;
			slide.h = GRIP_SIZE * 2;
			slide.x = _allocation.x;
			slide.w = _allocation.w;
		}

		if (slide.is_inside(e->xbutton.x, e->xbutton.y)) {
			process_drag_and_drop();
		} else {
			/* the pack test is for sanity, but should never be null */
			if (_pack0) {
				/* avoid double close */
				if (!_pack0->process_button_press_event(e) && _pack1) {
					_pack1->process_button_press_event(e);
				}
			}
		}
		return true;
	}
	return false;
}

bool split_t::add_client(client_t *c) {
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
	std::list<client_t *> * client = src->get_clients();
	std::list<client_t *>::iterator i = client->begin();

	if (src != _pack0 && src != _pack1)
		return;

	tree_t * dst = (src == _pack0) ? _pack1 : _pack0;
	while (i != client->end()) {
		dst->add_client((*i));
		++i;
	}

	_parent->replace(this, dst);
	_parent->render();
	delete src;
	/* self destruction ^^ */
	delete this;
}

void split_t::activate_client(client_t * c) {
	if (_pack0)
		_pack0->activate_client(c);
	if (_pack1)
		_pack1->activate_client(c);
}

void split_t::iconify_client(client_t * c) {
	if (_pack0)
		_pack0->iconify_client(c);
	if (_pack1)
		_pack1->iconify_client(c);
}

std::list<client_t *> * split_t::get_clients() {
	return 0;
}

void split_t::remove_client(client_t * c) {
	if (_pack0)
		_pack0->remove_client(c);
	if (_pack1)
		_pack1->remove_client(c);
}

void split_t::process_drag_and_drop() {
	XEvent ev;
	cursor = XCreateFontCursor(page.cnx.dpy, XC_fleur);
	popup_split_t * p;

	box_int_t slider_area;

	compute_slider_area(slider_area);

	p = new popup_split_t(slider_area);
	page.popups.push_back(p);

	if (XGrabPointer(page.cnx.dpy, page.main_window, False,
			(ButtonPressMask | ButtonReleaseMask | PointerMotionMask),
			GrabModeAsync, GrabModeAsync, None, cursor,
			CurrentTime) != GrabSuccess)
		return;

	do {

		XIfEvent(page.cnx.dpy, &ev, drag_and_drop_filter, (char*) this);

		if (ev.type == page.cnx.damage_event + XDamageNotify) {
			page.process_damage_event(&ev);
		} else if (ev.type == MotionNotify) {
			if (_split_type == VERTICAL_SPLIT) {
				_split = (ev.xmotion.x - _allocation.x)
						/ (double) (_allocation.w);
			} else {
				_split = (ev.xmotion.y - _allocation.y)
						/ (double) (_allocation.h);
			}

			if (_split > 0.95)
				_split = 0.95;
			if (_split < 0.05)
				_split = 0.05;

			/* Render slider with quite complex render method to avoid flickering */
			box_int_t old_area = slider_area;
			compute_slider_area(slider_area);

			p->area = slider_area;

			page.repair_back_buffer(old_area);
			page.repair_back_buffer(slider_area);

			box_int_t full_area;
			full_area.x = min(old_area.x, slider_area.x);
			full_area.y = min(old_area.y, slider_area.y);
			full_area.w = max(old_area.x + old_area.w,
					slider_area.x + slider_area.w) - full_area.x;
			full_area.h = max(old_area.y + old_area.h,
					slider_area.y + slider_area.h) - full_area.y;
			page.repair_overlay(full_area);
		}
	} while (ev.type != ButtonRelease);
	page.popups.remove(p);
	p->hide(page.composite_overlay_cr, page.main_window_s);
	delete p;
	XUngrabPointer(page.cnx.dpy, CurrentTime);
	XFreeCursor(page.cnx.dpy, cursor);
	update_allocation(_allocation);
	render();
}

Bool split_t::drag_and_drop_filter(Display * dpy, XEvent * ev, char * arg) {
	split_t * ths = reinterpret_cast<split_t *>(arg);
	return (ev->type == ConfigureRequest) || (ev->type == Expose)
			|| (ev->type == MotionNotify) || (ev->type == ButtonRelease)
			|| (ev->type == ths->page.cnx.damage_event + XDamageNotify);
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

}
