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

namespace page_next {

split_t::split_t(split_type_t type) {
	_split_type = type;
	_split = 0.5;
	_pack0 = 0;
	_pack1 = 0;
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
		b.w = _allocation.w * _split - 2;
		b.h = _allocation.h;
	} else {
		b.x = _allocation.x;
		b.y = _allocation.y;
		b.w = _allocation.w;
		b.h = _allocation.h * _split - 2;
	}
	_pack0->update_allocation(b);
}

void split_t::update_allocation_pack1() {
	if (!_pack1)
		return;
	box_t<int> b;
	if (_split_type == VERTICAL_SPLIT) {
		b.x = _allocation.x + _allocation.w * _split + 2;
		b.y = _allocation.y;
		b.w = _allocation.w - _allocation.w * _split - 5;
		b.h = _allocation.h;
	} else {
		b.x = _allocation.x;
		b.y = _allocation.y + _allocation.h * _split + 2;
		b.w = _allocation.w;
		b.h = _allocation.h - _allocation.h * _split - 2;
	}
	_pack1->update_allocation(b);
}

void split_t::render(cairo_t * cr) {
	cairo_save(cr);
	cairo_set_source_rgb(cr, 0xf5U / 255.0, 0x79U / 255.0, 0x00U / 255.0);
	if (_split_type == VERTICAL_SPLIT) {
		cairo_rectangle(cr, _allocation.x + (_allocation.w * _split) - 2,
				_allocation.y, 4, _allocation.h);
	} else {
		cairo_rectangle(cr, _allocation.x,
				_allocation.y + (_allocation.h * _split) - 2, _allocation.w, 4);
	}
	cairo_fill(cr);
	cairo_restore(cr);
	if (_pack0)
		_pack0->render(cr);
	if (_pack1)
		_pack1->render(cr);

}

bool split_t::process_button_press_event(XEvent const * e) {
	if (_allocation.is_inside(e->xbutton.x, e->xbutton.y)) {
		box_t<int> slide;
		if (_split_type == VERTICAL_SPLIT) {
			slide.y = _allocation.y;
			slide.h = _allocation.h;
			slide.x = _allocation.x + (_allocation.w * _split) - 2;
			slide.w = 5;
		} else {
			slide.y = _allocation.y + (_allocation.h * _split) - 2;
			slide.h = 5;
			slide.x = _allocation.x;
			slide.w = _allocation.w;
		}

		if (slide.is_inside(e->xbutton.x, e->xbutton.y)) {
			XEvent ev;
			cairo_t * cr;
			cursor = XCreateFontCursor(_dpy, XC_fleur);

			XWindowAttributes w_attribute;
			XSetWindowAttributes swa;
			swa.background_pixel = XBlackPixel(_dpy, 0);
			swa.border_pixel = XWhitePixel(_dpy, 0);
			swa.save_under = True;

			XGetWindowAttributes(_dpy, _w, &w_attribute);

			Window w;

			if (_split_type == VERTICAL_SPLIT) {
				w = XCreateWindow(_dpy, _w, _allocation.x + (int) (_split * _allocation.w)
						- 2, _allocation.y, 5, _allocation.h, 1,
						w_attribute.depth, InputOutput, w_attribute.visual,
						CWBackPixel | CWBorderPixel | CWSaveUnder, &swa);
			} else {
				w = XCreateWindow(_dpy, _w, _allocation.x,
						_allocation.y + (int) (_split * _allocation.h)
								- 2, _allocation.w, 5, 1,
						w_attribute.depth, InputOutput, w_attribute.visual,
						CWBackPixel | CWBorderPixel | CWSaveUnder, &swa);
			}

			XMapWindow(_dpy, w);

			if (XGrabPointer(_dpy, _w, False,
					(ButtonPressMask | ButtonReleaseMask | PointerMotionMask),
					GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime)
					!= GrabSuccess)
				return true;
			do {
				XMaskEvent(
						_dpy,
						(ButtonPressMask | ButtonReleaseMask
								| PointerMotionMask) | ExposureMask
								| SubstructureRedirectMask, &ev);
				switch (ev.type) {
				case ConfigureRequest:
				case Expose:
				case MapRequest:
					break;
				case MotionNotify:
					if (_split_type == VERTICAL_SPLIT) {
						_split = (ev.xmotion.x - _allocation.x)
								/ (double) (_allocation.w);
						if (_split > 0.95)
							_split = 0.95;
						if (_split < 0.05)
							_split = 0.05;
						XMoveResizeWindow(
								_dpy,
								w,
								_allocation.x + (int) (_split * _allocation.w)
										- 2, _allocation.y, 5, _allocation.h);
					} else {
						_split = (ev.xmotion.y - _allocation.y)
								/ (double) (_allocation.h);
						if (_split > 0.95)
							_split = 0.95;
						if (_split < 0.05)
							_split = 0.05;
						XMoveResizeWindow(
								_dpy,
								w,
								_allocation.x,
								_allocation.y + (int) (_split * _allocation.h)
										- 2, _allocation.w, 5);
					}

					if (_split > 0.95)
						_split = 0.95;
					if (_split < 0.05)
						_split = 0.05;
					//cr = get_cairo();
					//render(cr);
					//cairo_destroy(cr);
					break;
				}
			} while (ev.type != ButtonRelease);
			XUngrabPointer(_dpy, CurrentTime);
			XFreeCursor(_dpy, cursor);
			XDestroyWindow(_dpy, w);

			update_allocation(_allocation);
			cr = get_cairo();
			cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
			cairo_rectangle(cr, _allocation.x, _allocation.y, _allocation.w,
					_allocation.h);
			cairo_fill(cr);
			render(cr);
			cairo_destroy(cr);

		} else {
			if (_pack0)
				_pack0->process_button_press_event(e);
			if (_pack1)
				_pack1->process_button_press_event(e);
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

cairo_t * split_t::get_cairo() {
	cairo_surface_t * surf;
	XWindowAttributes wa;
	XGetWindowAttributes(_dpy, _w, &wa);
	surf = cairo_xlib_surface_create(_dpy, _w, wa.visual, wa.width, wa.height);
	cairo_t * cr = cairo_create(surf);
	return cr;
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

	tree_t * dst = (src == _pack0) ? _pack1 : _pack0;
	while (i != client->end()) {
		dst->add_notebook((*i));
		++i;
	}
	_parent->replace(this, dst);
	cairo_t * cr = get_cairo();
	_parent->render(cr);
	cairo_destroy(cr);
	delete src;
	/* self destruction ^^ */
	delete this;
}

void split_t::activate_client(client_t * c) {
	if(_pack0)
		_pack0->activate_client(c);
	if(_pack1)
		_pack1->activate_client(c);
}

std::list<client_t *> * split_t::get_clients() {
	return 0;
}

void split_t::remove_client(Window w) {
	if (_pack0)
		_pack0->remove_client(w);
	if (_pack1)
		_pack1->remove_client(w);
}

}
