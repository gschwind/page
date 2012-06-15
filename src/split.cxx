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
			XEvent ev;
			//cairo_t * cr;
			cursor = XCreateFontCursor(page.cnx.dpy, XC_fleur);
			XWindowAttributes w_attribute;
			XSetWindowAttributes swa;
			swa.background_pixel = XBlackPixel(page.cnx.dpy, 0);
			swa.border_pixel = XWhitePixel(page.cnx.dpy, 0);
			swa.save_under = True;

			XGetWindowAttributes(page.cnx.dpy, page.main_window, &w_attribute);

			if (XGrabPointer(page.cnx.dpy, page.main_window, False,
					(ButtonPressMask | ButtonReleaseMask | PointerMotionMask),
					GrabModeAsync, GrabModeAsync, None, cursor,
					CurrentTime) != GrabSuccess)
				return true;

			int save_render = 0;
			int expose_count = 0;
			do {
				XMaskEvent(page.cnx.dpy,
						(ButtonPressMask | ButtonReleaseMask | PointerMotionMask)
								| ExposureMask | SubstructureRedirectMask, &ev);
				switch (ev.type) {
				case ConfigureRequest:
				case Expose:
					expose_count += 1;
					break;
				case MapRequest:
					save_render = (save_render + 1) % 10;
					break;
				case MotionNotify:

					if (_split_type == VERTICAL_SPLIT) {

						cairo_set_source_surface(page.composite_overlay_cr,
								page.main_window_s, 0, 0);
						cairo_rectangle(page.composite_overlay_cr,
								_allocation.x + (int) (_split * _allocation.w)
										- GRIP_SIZE, _allocation.y,
								2 * GRIP_SIZE, _allocation.h);
						cairo_fill(page.composite_overlay_cr);

						_split = (ev.xmotion.x - _allocation.x)
								/ (double) (_allocation.w);
						if (_split > 0.95)
							_split = 0.95;
						if (_split < 0.05)
							_split = 0.05;

						cairo_set_source_rgba(page.composite_overlay_cr, 0.0,
								0.0, 0.0, 0.5);
						cairo_rectangle(page.composite_overlay_cr,
								_allocation.x + (int) (_split * _allocation.w)
										- GRIP_SIZE, _allocation.y,
								2 * GRIP_SIZE, _allocation.h);
						cairo_fill(page.composite_overlay_cr);
					} else {

						cairo_set_source_surface(page.composite_overlay_cr,
								page.main_window_s, 0, 0);
						cairo_rectangle(page.composite_overlay_cr,
								_allocation.x,
								_allocation.y + (int) (_split * _allocation.h)
										- GRIP_SIZE, _allocation.w,
								2 * GRIP_SIZE);
						cairo_fill(page.composite_overlay_cr);

						_split = (ev.xmotion.y - _allocation.y)
								/ (double) (_allocation.h);
						if (_split > 0.95)
							_split = 0.95;
						if (_split < 0.05)
							_split = 0.05;

						cairo_set_source_rgba(page.composite_overlay_cr, 0.0,
								0.0, 0.0, 0.5);
						cairo_rectangle(page.composite_overlay_cr,
								_allocation.x,
								_allocation.y + (int) (_split * _allocation.h)
										- GRIP_SIZE, _allocation.w,
								2 * GRIP_SIZE);
						cairo_fill(page.composite_overlay_cr);
					}

					if (_split > 0.95)
						_split = 0.95;
					if (_split < 0.05)
						_split = 0.05;

					break;
				}
			} while (ev.type != ButtonRelease);
			XUngrabPointer(page.cnx.dpy, CurrentTime);
			XFreeCursor(page.cnx.dpy, cursor);
			update_allocation(_allocation);
			render();
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

bool split_t::add_notebook(client_t *c) {
	if (_pack0) {
		if (!_pack0->add_notebook(c)) {
			if (_pack1)
				return _pack1->add_notebook(c);
			else
				return false;
		} else
			return true;
	} else {
		if (_pack1)
			return _pack1->add_notebook(c);
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
		dst->add_notebook((*i));
		++i;
	}

	_parent->replace(this, dst);
	//cairo_t * cr = get_cairo();
	_parent->render();
	//cairo_destroy(cr);
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

std::list<client_t *> * split_t::get_clients() {
	return 0;
}

void split_t::remove_client(client_t * c) {
	if (_pack0)
		_pack0->remove_client(c);
	if (_pack1)
		_pack1->remove_client(c);
}

}
