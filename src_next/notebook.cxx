/*
 * notebook.cxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#include <stdio.h>
#include <cairo-xlib.h>
#include <X11/cursorfont.h>
#include "notebook.hxx"

#define CTOF(r, g, b) (r/255.0),(g/255.0),(b/255.0)

namespace page_next {

std::list<notebook_t *> notebook_t::notebooks;

notebook_t::notebook_t(int group) :
	group(group) {

	close_img = cairo_image_surface_create_from_png(
			"/home/gschwind/page/data/close.png");
	hsplit_img = cairo_image_surface_create_from_png(
			"/home/gschwind/page/data/hsplit.png");
	vsplit_img = cairo_image_surface_create_from_png(
			"/home/gschwind/page/data/vsplit.png");

	notebooks.push_back(this);

}

void notebook_t::update_allocation(box_t<int> & allocation) {
	_allocation = allocation;

	button_close.x = _allocation.x + _allocation.w - 17;
	button_close.y = 0;
	button_close.w = 17;
	button_close.h = 20;

	button_vsplit.x = _allocation.x + _allocation.w - 17 * 2;
	button_vsplit.y = 0;
	button_vsplit.w = 17;
	button_vsplit.h = 20;

	button_hsplit.x = _allocation.x + _allocation.w - 17 * 3;
	button_hsplit.y = 0;
	button_hsplit.w = 17;
	button_hsplit.h = 20;
}

void notebook_t::render(cairo_t * cr) {
	update_client_mapping();
	cairo_rectangle(cr, _allocation.x, _allocation.y, _allocation.w,
			_allocation.h);
	cairo_clip(cr);
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgb(cr, CTOF(0x72, 0x9f, 0xcf));
	cairo_rectangle(cr, _allocation.x + 1, _allocation.y + 20, _allocation.w
			- 2, _allocation.h - 22);
	cairo_stroke(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, 13);
	std::list<client_t *>::iterator i;
	int offset = _allocation.x;
	int length = (_allocation.w - 17 * 3) / _clients.size();
	int s = 0;
	for (i = _clients.begin(); i != _clients.end(); ++i) {
		cairo_save(cr);
		cairo_translate(cr, offset, _allocation.y);
		cairo_rectangle(cr, 0, 0, length - 1, 19);
		cairo_clip(cr);
		offset += length;
		if (_selected == i) {
			cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
		} else {
			cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
		}
		cairo_paint(cr);
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr, 13);
		cairo_move_to(cr, 2, 13);
		cairo_show_text(cr, (*i)->name.c_str());
		cairo_restore(cr);
	}

	cairo_save(cr);
	cairo_translate(cr, _allocation.x + _allocation.w - 16.0, _allocation.y
			+ 1.0);
	cairo_set_source_surface(cr, close_img, 0.0, 0.0);
	cairo_paint(cr);
	cairo_translate(cr, -17.0, 0.0);
	cairo_set_source_surface(cr, vsplit_img, 0.0, 0.0);
	cairo_paint(cr);
	cairo_translate(cr, -17.0, 0.0);
	cairo_set_source_surface(cr, hsplit_img, 0.0, 0.0);
	cairo_paint(cr);
	cairo_restore(cr);
	cairo_reset_clip(cr);

}

bool notebook_t::process_button_press_event(XEvent const * e) {
	if (_allocation.is_inside(e->xbutton.x, e->xbutton.y)) {
		if (_clients.size() > 0) {
			int box_width = ((_allocation.w - 17 * 3) / _clients.size());
			box_t<int> b(_allocation.x, _allocation.y, box_width, 20);
			std::list<client_t *>::iterator c = _clients.begin();
			while (c != _clients.end()) {
				if (b.is_inside(e->xbutton.x, e->xbutton.y)) {
					break;
				}
				b.x += box_width;
				++c;
			}

			if (c != _clients.end()) {
				_selected = c;
				XEvent ev;
				cairo_t * cr;

				cursor = XCreateFontCursor(_dpy, XC_fleur);

				if (XGrabPointer(_dpy, _w, False, (ButtonPressMask
						| ButtonReleaseMask | PointerMotionMask),
						GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime)
						!= GrabSuccess)
					return true;
				do {
					XMaskEvent(_dpy, (ButtonPressMask | ButtonReleaseMask
							| PointerMotionMask) | ExposureMask
							| SubstructureRedirectMask, &ev);
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
				XUngrabPointer(_dpy, CurrentTime);
				XFreeCursor(_dpy, cursor);

				std::list<notebook_t *>::iterator dst;
				for (dst = notebooks.begin(); dst != notebooks.end(); ++dst) {
					if ((*dst)->_allocation.is_inside(ev.xbutton.x,
							ev.xbutton.y) && this->group == (*dst)->group) {
						break;
					}
				}

				if (dst != notebooks.end() && (*dst) != this) {
					client_t * move = *(c);
					/* reselect a new window */
					_selected = c;
					++_selected;
					if(_selected == _clients.end())
						_selected = _clients.begin();
					_clients.remove(move);
					(*dst)->add_notebook(move);
				}

				cr = get_cairo();
				render(cr);
				cairo_destroy(cr);
			}

		}

		if (button_close.is_inside(e->xbutton.x, e->xbutton.y)) {
			if (_parent != 0) {
				_parent->remove(this);
			}
		} else if (button_vsplit.is_inside(e->xbutton.x, e->xbutton.y)) {
			split(VERTICAL_SPLIT);
		} else if (button_hsplit.is_inside(e->xbutton.x, e->xbutton.y)) {
			split(HORIZONTAL_SPLIT);
		}

		return true;
	}

	return false;
}

void notebook_t::update_client_mapping() {
	std::list<client_t *>::iterator i;
	for (i = _clients.begin(); i != _clients.end(); ++i) {
		if (i != _selected) {
			(*i)->unmap();
		} else {
			client_t * c = (*i);
			c->update_client_size(_allocation.w, _allocation.h);
			XResizeWindow(c->dpy, c->xwin, c->width, c->height);
			XMoveResizeWindow(c->dpy, c->clipping_window, _allocation.x + 2,
					_allocation.y + 2 + 20, _allocation.w - 4, _allocation.h
							- 20 - 4);
			c->map();
		}
	}
}

void notebook_t::add_notebook(client_t *c) {
	_clients.push_front(c);
	_selected = _clients.begin();
}

cairo_t * notebook_t::get_cairo() {
	cairo_surface_t * surf;
	XWindowAttributes wa;
	XGetWindowAttributes(_dpy, _w, &wa);
	surf = cairo_xlib_surface_create(_dpy, _w, wa.visual, wa.width, wa.height);
	cairo_t * cr = cairo_create(surf);
	return cr;
}

void notebook_t::split(split_type_t type) {
	split_t * split = new split_t(type);
	_parent->replace(this, split);
	split->replace(0, this);
	split->replace(0, new notebook_t());
	update_client_mapping();
}

void notebook_t::replace(tree_t * src, tree_t * by) {

}

void notebook_t::close(tree_t * src) {

}

void notebook_t::remove(tree_t * src) {

}

std::list<client_t *> * notebook_t::get_clients() {
	return &_clients;
}

void notebook_t::remove_client(Window w) {
	std::list<client_t *>::iterator i = _clients.begin();
	while(i != _clients.end()) {
		if((*i)->xwin == w) {
			_clients.remove((*i));
			break;
		}
		++i;
	}
}

}
