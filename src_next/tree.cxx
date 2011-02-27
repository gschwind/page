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

#define CTOF(r, g, b) (r/255.0),(g/255.0),(b/255.0)

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
	_allocation.w = -1;
	_allocation.h = -1;
}

tree_t::~tree_t() {
	_shared->note_link.remove(this);
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
	_allocation.w = -1;
	_allocation.h = -1;

}

void tree_t::split_update_allocation(box_t<int> & alloc) {
	_allocation = alloc;
	fprintf(stderr, "%p : %s return %d,%d,%d,%d\n", this, __PRETTY_FUNCTION__,
			_allocation.x, _allocation.y, _allocation.w, _allocation.h);
	if (_split_type == VERTICAL_SPLIT) {
		box_t<int> b;
		b.x = _allocation.x;
		b.y = _allocation.y;
		b.w = _allocation.w * _split - 2;
		b.h = _allocation.h;
		_pack0->update_allocation(b);
		b.x = _allocation.x + _allocation.w * _split + 2;
		b.y = _allocation.y;
		b.w = _allocation.w - _allocation.w * _split - 5;
		b.h = _allocation.h;
		_pack1->update_allocation(b);
	} else {
		box_t<int> b;
		b.x = _allocation.x;
		b.y = _allocation.y;
		b.w = _allocation.w;
		b.h = _allocation.h * _split - 2;
		_pack0->update_allocation(b);
		b.x = _allocation.x;
		b.y = _allocation.y + _allocation.h * _split + 2;
		b.w = _allocation.w;
		b.h = _allocation.h - _allocation.h * _split - 2;
		_pack1->update_allocation(b);
	}
}

void tree_t::split_render(cairo_t * cr) {
	cairo_rectangle(cr, _allocation.x, _allocation.y, _allocation.w,
			_allocation.h);
	cairo_clip(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	if (_split_type == VERTICAL_SPLIT) {
		cairo_rectangle(cr, _allocation.x + (_allocation.w * _split) - 2,
				_allocation.y, 4, _allocation.h);
	} else {
		cairo_rectangle(cr, _allocation.x, _allocation.y + (_allocation.h
				* _split) - 2, _allocation.w, 4);
	}
	cairo_fill(cr);
	_pack0->render(cr);
	_pack1->render(cr);

}

bool tree_t::split_process_button_press_event(XEvent const * e) {
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

			if (XGrabPointer(_shared->_dpy, _shared->_w, False,
					(ButtonPressMask | ButtonReleaseMask | PointerMotionMask),
					GrabModeAsync, GrabModeAsync, None, _shared->cursor,
					CurrentTime) != GrabSuccess)
				return true;
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
							_allocation.w, _allocation.h);
					cairo_fill(cr);
					this->render(cr);
					cairo_destroy(cr);
					break;
				case MotionNotify:
					if (_split_type == VERTICAL_SPLIT) {
						_split = (ev.xmotion.x - _allocation.x)
								/ (double) (_allocation.w);
					} else {
						_split = (ev.xmotion.y - _allocation.y)
								/ (double) (_allocation.h);
					}
					update_allocation(_allocation);
					cr = this->get_cairo();
					cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
					cairo_rectangle(cr, _allocation.x, _allocation.y,
							_allocation.w, _allocation.h);
					cairo_fill(cr);
					this->render(cr);
					cairo_destroy(cr);
					break;
				}
			} while (ev.type != ButtonRelease);
			XUngrabPointer(_shared->_dpy, CurrentTime);
		} else {
			if (!_pack0->process_button_press_event(e)) {
				_pack1->process_button_press_event(e);
			}
		}
		return true;
	}
	return false;
}

bool tree_t::split_is_selected(int x, int y) {
	fprintf(stderr, "%s return %s\n", __PRETTY_FUNCTION__, "false");
	return false;
}

void tree_t::notebook_update_allocation(box_t<int> & alloc) {
	_allocation = alloc;
	fprintf(stderr, "%p : %s return %d,%d,%d,%d\n", this, __PRETTY_FUNCTION__,
			_allocation.x, _allocation.y, _allocation.w, _allocation.h);
}

void tree_t::notebook_render(cairo_t * cr) {

	update_client_mapping();
	cairo_rectangle(cr, _allocation.x, _allocation.y, _allocation.w,
			_allocation.h);
	cairo_clip(cr);
	cairo_set_source_rgb(cr, CTOF(0x72, 0x9f, 0xcf));
	cairo_rectangle(cr, _allocation.x + 1, _allocation.y + 20, _allocation.w - 2, _allocation.h - 22);
	cairo_stroke(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 13);
	std::list<client_t *>::iterator i;
	int offset = _allocation.x;
	int length = (_allocation.w - 17 * 3) / _clients.size();
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
		cairo_show_text(cr, (*i)->name.c_str());
		cairo_restore(cr);
	}

	cairo_save(cr);
	cairo_translate(cr, _allocation.x + _allocation.w - 16.0, _allocation.y
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

bool tree_t::notebook_process_button_press_event(XEvent const * e) {
	if (_allocation.is_inside(e->xbutton.x, e->xbutton.y)) {
		if (e->xbutton.y < _allocation.y + 20 && e->xbutton.y > _allocation.y) {
			if (_clients.size() > 0) {
				int s = (e->xbutton.x - _allocation.x) / ((_allocation.w - 17
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
						return true;
					do {
						XMaskEvent(_shared->_dpy, (ButtonPressMask
								| ButtonReleaseMask | PointerMotionMask)
								| ExposureMask | SubstructureRedirectMask, &ev);
						switch (ev.type) {
						case ConfigureRequest:
						case Expose:
						case MapRequest:
							cr = get_cairo();
							cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
							cairo_rectangle(cr, _allocation.x, _allocation.y,
									_allocation.w, _allocation.h);
							cairo_fill(cr);
							render(cr);
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

					cr = get_cairo();
					render(cr);
					cairo_destroy(cr);
				}
			}

			if (_allocation.x + _allocation.w - 17 < e->xbutton.x
					&& _allocation.x + _allocation.w > e->xbutton.x) {
				if (_parent != 0) {
					_parent->mutate_to_notebook(this);
				}
			}

			if (_allocation.x + _allocation.w - 34 < e->xbutton.x
					&& _allocation.x + _allocation.w - 17 > e->xbutton.x) {
				mutate_to_split(VERTICAL_SPLIT);
			}

			if (_allocation.x + _allocation.w - 51 < e->xbutton.x
					&& _allocation.x + _allocation.w - 34 > e->xbutton.x) {
				mutate_to_split(HORIZONTAL_SPLIT);
			}

		}

		return true;
	}

	return false;
}

bool tree_t::notebook_is_selected(int x, int y) {
	fprintf(stderr, "%p : %s return %d,%d,%d,%d:%s\n", this,
			__PRETTY_FUNCTION__, _allocation.x, _allocation.y, _allocation.w,
			_allocation.h, _allocation.is_inside(x, y) ? "true" : "false");
	return (_allocation.is_inside(x, y));
}

void tree_t::mutate_to_split(split_type_t type) {
	if ((_pack0 == 0 && _pack1 == 0)) {
		_pack0 = new tree_t(_shared, this);
		_pack1 = new tree_t(_shared, this);
		_vtable = _shared->vtable_split;
		_split = 0.5;
		_split_type = type;
		_pack0->_clients.splice(_pack0->_clients.end(), _clients);
		update_allocation(_allocation);
	} else {
		fprintf(stderr, "invalid call of %s\n", __PRETTY_FUNCTION__);
		return;
	}
}

void tree_t::mutate_to_notebook(tree_t * pack) {
	if (_pack0 != 0 && _pack1 != 0) {
		tree_t * other = _pack0;
		if (pack == _pack0) {
			other = _pack1;
		}
		if (other->_pack0 != 0 && other->_pack1 != 0) { /* other is split */
			_vtable = _shared->vtable_split;
			_pack0 = other->_pack0;
			_pack1 = other->_pack1;
			_pack0->_parent = this;
			_pack1->_parent = this;
			_split = other->_split;
			_split_type = other->_split_type;
			update_allocation(_allocation);
			other->_pack0 = 0;
			other->_pack1 = 0;
			std::list<client_t *>::iterator i;
			for (i = pack->_clients.begin(); i != pack->_clients.end(); ++i) {
				_pack0->add_notebook((*i));
			}
			delete pack;
			delete other;
		} else { /* other is notebook */
			_vtable = _shared->vtable_notebook;
			_clients.splice(_clients.end(), _pack0->_clients);
			_clients.splice(_clients.end(), _pack1->_clients);
			delete _pack0;
			delete _pack1;
			_pack0 = 0;
			_pack1 = 0;
		}
	} else {
		fprintf(stderr, "invalid call of %s\n", __PRETTY_FUNCTION__);
		return;
	}
}

void tree_t::split_add_notebook(client_t *c) {
	_pack0->add_notebook(c);
}

void tree_t::notebook_add_notebook(client_t *c) {
	_clients.push_front(c);
	_selected = 0;
	update_client_mapping();
	cairo_t * cr = get_cairo();
	render(cr);
	cairo_destroy(cr);
}

void tree_t::update_allocation(box_t<int> & alloc) {
	(this->*_vtable._update_allocation)(alloc);
}
void tree_t::render(cairo_t * cr) {
	(this->*_vtable._render)(cr);
}

bool tree_t::process_button_press_event(XEvent const * e) {
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

void tree_t::update_client_mapping() {
	int k = 0;
	std::list<client_t *>::iterator i;
	for (i = _clients.begin(); i != _clients.end(); ++i) {
		if (k != _selected) {
			(*i)->unmap();
		} else {
			client_t * c = (*i);
			update_client_size(c, _allocation.w, _allocation.h);
			XResizeWindow(c->dpy, c->xwin, c->width, c->height);
			XMoveResizeWindow(c->dpy, c->clipping_window, _allocation.x, _allocation.y
					+ 20, _allocation.w, _allocation.h - 20);
			c->map();
		}
		++k;
	}
}

void tree_t::update_client_size(client_t * c, int w, int h) {
	if (c) {
		if (c->maxw != 0 && w > c->maxw) {
			w = c->maxw;
		}

		if (c->maxh != 0 && h > c->maxh) {
			h = c->maxh;
		}

		if (c->minw != 0 && w < c->minw) {
			w = c->minw;
		}

		if (c->minh != 0 && h < c->minh) {
			h = c->minh;
		}

		if (c->incw != 0) {
			w -= ((w - c->basew) % c->incw);
		}

		if (c->inch != 0) {
			h -= ((h - c->baseh) % c->inch);
		}

		/* TODO respect Aspect */
		c->height = h;
		c->width = w;

		printf("Update #%p window size %dx%d\n", (void *) c->xwin, c->width,
				c->height);

	}
}

}

