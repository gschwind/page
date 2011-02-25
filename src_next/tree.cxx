/*
 * tree.cxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstdio>
#include "tree.hxx"

namespace page_next {

tree_t::tree_t(tree_t::shared_t * s, tree_t * parent) {
	_shared = s;
	_parent = parent;
	_shared->note_link.push_back(this);
	_pack0 = 0;
	_pack1 = 0;
	_split = 0.5;
	_vtable = _shared->vtable_notebook;
	_allocation.x = -1;
	_allocation.y = -1;
	_allocation.width = -1;
	_allocation.height = -1;
}

tree_t::tree_t(Display * dpy, Window w) {
	_shared = new shared_t;
	_shared->_dpy = dpy;
	_shared->_w = w;
	_parent = 0;

	_shared->close_img = cairo_image_surface_create_from_png(
			"/home/gschwind/page/data/close.png");
	_shared->hsplit_img = cairo_image_surface_create_from_png(
			"/home/gschwind/page/data/hsplit.png");
	_shared->vsplit_img = cairo_image_surface_create_from_png(
			"/home/gschwind/page/data/vsplit.png");

	_shared->cursor = XCreateFontCursor(_shared->_dpy, XC_fleur);

	_shared->note_link.push_back(this);
	_pack0 = 0;
	_pack1 = 0;
	_split = 0.5;

	_shared->vtable_notebook._update_allocation
			= &tree_t::notebook_update_allocation;
	_shared->vtable_notebook._render = &tree_t::notebook_render;
	_shared->vtable_notebook._process_button_press_event
			= &tree_t::notebook_process_button_press_event;
	_shared->vtable_notebook._add_notebook = &tree_t::notebook_add_notebook;
	_shared->vtable_notebook._is_selected = &tree_t::notebook_is_selected;

	_shared->vtable_split._update_allocation = &tree_t::split_update_allocation;
	_shared->vtable_split._render = &tree_t::split_render;
	_shared->vtable_split._process_button_press_event
			= &tree_t::split_process_button_press_event;
	_shared->vtable_split._add_notebook = &tree_t::split_add_notebook;
	_shared->vtable_split._is_selected = &tree_t::split_is_selected;

	_vtable = _shared->vtable_notebook;

	_allocation.x = -1;
	_allocation.y = -1;
	_allocation.width = -1;
	_allocation.height = -1;

}

void tree_t::split_update_allocation(box_t & alloc) {
	_allocation = alloc;
	fprintf(stderr, "%p : %s return %d,%d,%d,%d\n", this, __PRETTY_FUNCTION__,
			_allocation.x, _allocation.y, _allocation.width, _allocation.height);
	box_t b;
	b.x = _allocation.x;
	b.y = _allocation.y;
	b.width = _allocation.width * _split - 2;
	b.height = _allocation.height;
	_pack0->update_allocation(b);
	b.x = _allocation.x + _allocation.width * _split + 2;
	b.y = _allocation.y;
	b.width = _allocation.width - _allocation.width * _split - 5;
	b.height = _allocation.height;
	_pack1->update_allocation(b);
}

void tree_t::split_render(cairo_t * cr) {
	cairo_rectangle(cr, _allocation.x, _allocation.y, _allocation.width,
			_allocation.height);
	cairo_clip(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_rectangle(cr, _allocation.x + (_allocation.width * _split) - 2,
			_allocation.y, 4, _allocation.height);
	cairo_fill(cr);
	_pack0->render(cr);
	_pack1->render(cr);

}

void tree_t::split_process_button_press_event(XEvent const * e) {
	if (_allocation.is_inside(e->xbutton.x, e->xbutton.y)) {
		if (_allocation.x + (_allocation.width * _split) - 2 < e->xbutton.x
				&& _allocation.x + (_allocation.width * _split) + 2
						> e->xbutton.x) {
			XEvent ev;
			cairo_t * cr;

			if (XGrabPointer(_shared->_dpy, _shared->_w, False,
					(ButtonPressMask | ButtonReleaseMask | PointerMotionMask),
					GrabModeAsync, GrabModeAsync, None, _shared->cursor,
					CurrentTime) != GrabSuccess)
				return;
			do {
				XMaskEvent(_shared->_dpy, (ButtonPressMask | ButtonReleaseMask
						| PointerMotionMask) | ExposureMask
						| SubstructureRedirectMask, &ev);
				switch (ev.type) {
				case ConfigureRequest:
				case Expose:
				case MapRequest:
					cr = this->get_cairo();
					cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
					cairo_rectangle(cr, _allocation.x, _allocation.y,
							_allocation.width, _allocation.height);
					cairo_fill(cr);
					this->render(cr);
					cairo_destroy(cr);
					break;
				case MotionNotify:
					_split = (ev.xmotion.x - _allocation.x)
							/ (double) (_allocation.width);
					this->update_allocation(_allocation);
					cr = this->get_cairo();
					cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
					cairo_rectangle(cr, _allocation.x, _allocation.y,
							_allocation.width, _allocation.height);
					cairo_fill(cr);
					this->render(cr);
					cairo_destroy(cr);
					break;
				}
			} while (ev.type != ButtonRelease);
			XUngrabPointer(_shared->_dpy, CurrentTime);
		}
	}

	_pack0->process_button_press_event(e);
	_pack1->process_button_press_event(e);
}

bool tree_t::split_is_selected(int x, int y) {
	fprintf(stderr, "%s return %s\n", __PRETTY_FUNCTION__, "false");
	return false;
}

void tree_t::notebook_update_allocation(box_t & alloc) {
	_allocation = alloc;
	fprintf(stderr, "%p : %s return %d,%d,%d,%d\n", this, __PRETTY_FUNCTION__,
			_allocation.x, _allocation.y, _allocation.width, _allocation.height);
}

void tree_t::notebook_render(cairo_t * cr) {

	cairo_rectangle(cr, _allocation.x, _allocation.y, _allocation.width,
			_allocation.height);
	cairo_clip(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 13);
	std::list<client_t *>::iterator i;
	int offset = _allocation.x;
	int length = (_allocation.width - 17 * 3) / _clients.size();
	int s = 0;
	for (i = _clients.begin(); i != _clients.end(); ++i, ++s) {
		cairo_save(cr);
		cairo_translate(cr, offset, _allocation.y);
		cairo_rectangle(cr, 0, 0, length - 1, 19);
		cairo_clip(cr);
		offset += length;
		if (_selected == s) {
			cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
		} else {
			cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
		}
		cairo_paint(cr);
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(cr, 13);
		cairo_move_to(cr, 2, 13);
		cairo_show_text(cr, (*i)->get_name().c_str());
		cairo_restore(cr);
	}

	cairo_save(cr);
	cairo_translate(cr, _allocation.x + _allocation.width - 16.0, _allocation.y
			+ 1.0);
	cairo_set_source_surface(cr, _shared->close_img, 0.0, 0.0);
	cairo_paint(cr);
	cairo_translate(cr, -17.0, 0.0);
	cairo_set_source_surface(cr, _shared->vsplit_img, 0.0, 0.0);
	cairo_paint(cr);
	cairo_translate(cr, -17.0, 0.0);
	cairo_set_source_surface(cr, _shared->hsplit_img, 0.0, 0.0);
	cairo_paint(cr);
	cairo_restore(cr);
	cairo_reset_clip(cr);
}

void tree_t::notebook_process_button_press_event(XEvent const * e) {
	if (_allocation.is_inside(e->xbutton.x, e->xbutton.y)) {
		if ((e->xbutton.y - _allocation.y) < 20 && _clients.size() != 0) {
			int s = (e->xbutton.x - _allocation.x) / ((_allocation.width - 17
					* 3) / _clients.size());
			if (s >= 0 && s < _clients.size()) {
				_selected = s;
				printf("select %d\n", s);
				XEvent ev;
				cairo_t * cr;

				if (XGrabPointer(_shared->_dpy, _shared->_w, False,
						(ButtonPressMask | ButtonReleaseMask
								| PointerMotionMask), GrabModeAsync,
						GrabModeAsync, None, _shared->cursor, CurrentTime)
						!= GrabSuccess)
					return;
				do {
					XMaskEvent(_shared->_dpy, (ButtonPressMask
							| ButtonReleaseMask | PointerMotionMask)
							| ExposureMask | SubstructureRedirectMask, &ev);
					switch (ev.type) {
					case ConfigureRequest:
					case Expose:
					case MapRequest:
						cr = this->get_cairo();
						cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
						cairo_rectangle(cr, _allocation.x, _allocation.y,
								_allocation.width, _allocation.height);
						cairo_fill(cr);
						this->render(cr);
						cairo_destroy(cr);
						break;
					case MotionNotify:
						break;
					}
				} while (ev.type != ButtonRelease);
				XUngrabPointer(_shared->_dpy, CurrentTime);

				std::list<tree_t *>::iterator i;
				for (i = _shared->note_link.begin(); i
						!= _shared->note_link.end(); ++i) {
					if ((*i)->is_selected(ev.xbutton.x, ev.xbutton.y))
						break;
				}

				if (i != _shared->note_link.end() && (*i) != this) {
					std::list<client_t *>::iterator c = _clients.begin();
					std::advance(c, s);
					client_t * move = *(c);
					_clients.remove(move);
					(*i)->add_notebook(move);
				}

				cr = this->get_cairo();
				this->render(cr);
				cairo_destroy(cr);

			}

			if (_allocation.x + _allocation.width - 17 < e->xbutton.x
					&& _allocation.x + _allocation.width > e->xbutton.x) {
				_parent->mutate_to_notebook();
				cairo_t * cr = this->get_cairo();
				this->render(cr);
				cairo_destroy(cr);
			}

			if (_allocation.x + _allocation.width - 34 < e->xbutton.x
					&& _allocation.x + _allocation.width - 17 > e->xbutton.x) {
				/* TODO: vsplit */
			}

			if (_allocation.x + _allocation.width - 51 < e->xbutton.x
					&& _allocation.x + _allocation.width - 34 > e->xbutton.x) {
				/* TODO: hsplit */
			}

		}
	}
}

bool tree_t::notebook_is_selected(int x, int y) {
	fprintf(stderr, "%p : %s return %d,%d,%d,%d:%s\n", this,
			__PRETTY_FUNCTION__, _allocation.x, _allocation.y,
			_allocation.width, _allocation.height,
			_allocation.is_inside(x, y) ? "true" : "false");
	return (_allocation.is_inside(x, y));
}

void tree_t::mutate_to_split() {
	if ((_pack0 == 0 && _pack1 == 0)) {
		_pack0 = new tree_t(_shared, this);
		_pack1 = new tree_t(_shared, this);
		_vtable = _shared->vtable_split;
		_split = 0.5;
		_pack0->_clients.splice(_pack0->_clients.end(), _clients);
		box_t b;
		b.x = _allocation.x;
		b.y = _allocation.y;
		b.width = _allocation.width * _split - 2;
		b.height = _allocation.height;
		_pack0->update_allocation(b);
		b.x = _allocation.x + _allocation.width * _split + 2;
		b.y = _allocation.y;
		b.width = _allocation.width - _allocation.width * _split - 5;
		b.height = _allocation.height;
		_pack1->update_allocation(b);
	} else {
		fprintf(stderr, "invalid call of %s\n", __PRETTY_FUNCTION__);
		return;
	}
}

void tree_t::mutate_to_notebook() {
	if (_pack0 != 0 && _pack1 != 0) {
		_vtable = _shared->vtable_notebook;
		_clients.splice(_clients.end(), _pack0->_clients);
		_clients.splice(_clients.end(), _pack1->_clients);
		delete _pack0;
		delete _pack1;
		_pack0 = 0;
		_pack1 = 0;

	} else {
		fprintf(stderr, "invalid call of %s\n", __PRETTY_FUNCTION__);
		return;
	}
}

void tree_t::split_add_notebook(client_t *c) {
	_pack0->add_notebook(c);
}

void tree_t::notebook_add_notebook(client_t *c) {
	_clients.push_back(c);
}

void tree_t::update_allocation(box_t & alloc) {
	(this->*_vtable._update_allocation)(alloc);
}
void tree_t::render(cairo_t * cr) {
	(this->*_vtable._render)(cr);
}
void tree_t::process_button_press_event(XEvent const * e) {
	(this->*_vtable._process_button_press_event)(e);
}

bool tree_t::is_selected(int x, int y) {
	(this->*_vtable._is_selected)(x, y);
}

void tree_t::add_notebook(client_t * c) {
	(this->*_vtable._add_notebook)(c);
}

cairo_t * tree_t::get_cairo() {
	cairo_surface_t * surf;
	XWindowAttributes wa;
	XGetWindowAttributes(_shared->_dpy, _shared->_w, &wa);
	surf = cairo_xlib_surface_create(_shared->_dpy, _shared->_w, wa.visual,
			wa.width, wa.height);
	cairo_t * cr = cairo_create(surf);
	return cr;
}

}

