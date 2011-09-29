/*
 * notebook.cxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#include <stdio.h>
#include <cairo-xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cmath>
#include "notebook.hxx"

namespace page_next {

std::list<notebook_t *> notebook_t::notebooks;

notebook_t::notebook_t(int group) :
		group(group) {

	// close_img = cairo_image_surface_create_from_png(
	//    "/home/gschwind/page/data/close.png");
	// hsplit_img = cairo_image_surface_create_from_png(
	//     "/home/gschwind/page/data/hsplit.png");
	// vsplit_img = cairo_image_surface_create_from_png(
	//     "/home/gschwind/page/data/vsplit.png");

	notebooks.push_back(this);

}

void notebook_t::update_allocation(box_t<int> & allocation) {
	_allocation = allocation;

	button_close.x = _allocation.x + _allocation.w - 17;
	button_close.y = _allocation.y;
	button_close.w = 17;
	button_close.h = 20;

	button_vsplit.x = _allocation.x + _allocation.w - 17 * 2;
	button_vsplit.y = _allocation.y;
	button_vsplit.w = 17;
	button_vsplit.h = 20;

	button_hsplit.x = _allocation.x + _allocation.w - 17 * 3;
	button_hsplit.y = _allocation.y;
	button_hsplit.w = 17;
	button_hsplit.h = 20;
}

void notebook_t::render(cairo_t * cr) {
	update_client_mapping();
	cairo_save(cr);
	{
		cairo_rectangle(cr, _allocation.x, _allocation.y, _allocation.w, 19);
		cairo_pattern_t *pat;
		pat = cairo_pattern_create_linear(0.0, 0.0, 0.0, 19.0);
		cairo_pattern_add_color_stop_rgba(pat, 0, 0xeeU / 255.0, 0xeeU / 255.0,
				0xecU / 255.0, 1);
		cairo_pattern_add_color_stop_rgba(pat, 1, 0xbaU / 255.0, 0xbdU / 255.0,
				0xd6U / 255.0, 1);
		cairo_set_source(cr, pat);
		cairo_fill(cr);
		cairo_pattern_destroy(pat);

		cairo_set_line_width(cr, 1.0);
		cairo_set_source_rgb(cr, 0x72U / 255.0, 0x9fU / 255.0, 0xcfU / 255.0);
		cairo_rectangle(cr, _allocation.x + 1, _allocation.y + 20,
				_allocation.w - 2, _allocation.h - 22);
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
			{
				cairo_translate(cr, offset, _allocation.y);
				if (_selected == i) {
					cairo_rectangle(cr, 0, 3.0, length, 19);
					cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0,
							0xecU / 255.0);
					cairo_fill(cr);
				}
				offset += length;
				cairo_save(cr);
				{
					cairo_rectangle(cr, 2, 0, length - 4, 19);
					cairo_clip(cr);
					cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
					cairo_set_font_size(cr, 13);
					cairo_move_to(cr, 3.0, 15.0);
					cairo_show_text(cr, (*i)->name.c_str());
				}
				cairo_restore(cr);

				cairo_set_line_width(cr, 1.0);
				if (_selected == i) {
					cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
					cairo_new_path(cr);
					cairo_rectangle(cr, 1.0, 1.0, length, 3.0);
					cairo_set_source_rgb(cr, 0x72U / 255.0, 0x9fU / 255.0,
							0xcfU / 255.0);
					cairo_fill(cr);
					cairo_new_path(cr);
					cairo_move_to(cr, 2.0, 4.0);
					cairo_line_to(cr, length, 4.0);
					cairo_set_source_rgb(cr, 0x34U / 255.0, 0x64U / 255.0,
							0xa4U / 255.0);
					cairo_stroke(cr);
					cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0,
							0x85U / 255.0);
					rounded_rectangle(cr, 1.0, 1.0, length, 19.0, 3.0);
				} else {
					cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0,
							0x85U / 255.0);
					rounded_rectangle(cr, 1.0, 3.0, length, 17.0, 2.0);
					cairo_set_source_rgb(cr, 0xd3U / 255.0, 0xd7U / 255.0,
							0xcfU / 255.0);
					cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
					cairo_new_path(cr);
					cairo_move_to(cr, 0.0, 19.0);
					cairo_line_to(cr, length + 1.0, 20.0);
					cairo_stroke(cr);
				}
				cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
			}
			cairo_restore(cr);
		}
	}
	cairo_restore(cr);
	cairo_save(cr);
	{
		cairo_translate(cr, _allocation.x + _allocation.w - 16.0,
				_allocation.y + 1.0);
		/* draw close */
		cairo_new_path(cr);
		cairo_move_to(cr, 4.0, 4.0);
		cairo_line_to(cr, 12.0, 12.0);
		cairo_move_to(cr, 12.0, 4.0);
		cairo_line_to(cr, 4.0, 12.0);
		cairo_set_source_rgb(cr, 0xCCU / 255.0, 0x00U / 255.0, 0x00U / 255.0);
		cairo_stroke(cr);

		/* draw vertical split */
		cairo_translate(cr, -17.0, 0.0);
		cairo_move_to(cr, 8.0, 2.0);
		cairo_line_to(cr, 8.0, 14.0);
		cairo_move_to(cr, 9.0, 2.0);
		cairo_line_to(cr, 9.0, 14.0);
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_stroke(cr);

		/* draw horizontal split */
		cairo_translate(cr, -17.0, 0.0);
		cairo_move_to(cr, 2.0, 8.0);
		cairo_line_to(cr, 14.0, 8.0);
		cairo_move_to(cr, 2.0, 8.0);
		cairo_line_to(cr, 14.0, 8.0);
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_stroke(cr);
	}
	cairo_restore(cr);
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

				if (XGrabPointer(
						_dpy,
						_w,
						False,
						(ButtonPressMask | ButtonReleaseMask
								| PointerMotionMask),
						GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime)
						!= GrabSuccess
					)
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
					select_next();
					_clients.remove(move);
					(*dst)->add_notebook(move);
				}

				cr = get_cairo();
				render(cr);
				cairo_destroy(cr);

				if (((*_selected)->try_lock_client())) {
					client_t * c = (*_selected);

					XRaiseWindow(c->dpy, c->xwin);
					XSetInputFocus(c->dpy, c->xwin, RevertToNone, CurrentTime);

					XChangeProperty(c->dpy, XDefaultRootWindow(c->dpy),
							c->atoms->_NET_ACTIVE_WINDOW, c->atoms->WINDOW, 32,
							PropModeReplace,
							reinterpret_cast<unsigned char *>(&(c->xwin)), 1);

					(*_selected)->unlock_client();
				}

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
		if (!((*i)->try_lock_client()))
			continue;
		if (i != _selected) {
			(*i)->unmap();
			long state = IconicState;
			XChangeProperty((*i)->dpy, (*i)->xwin, (*i)->atoms->WM_STATE, (*i)->atoms->CARDINAL, 32,
					PropModeReplace,
					reinterpret_cast<unsigned char *>(&state), 1);

		} else {
			client_t * c = (*i);
			c->update_client_size(_allocation.w - 4, _allocation.h - 24);
			printf("XResizeWindow(%p, %lu, %d, %d)\n", c->dpy, c->xwin,
					c->width, c->height);
			XMoveResizeWindow(c->dpy, c->xwin, 0, 0, c->width, c->height);
			XMoveResizeWindow(c->dpy, c->clipping_window, _allocation.x + 2,
					_allocation.y + 2 + 20, _allocation.w - 4,
					_allocation.h - 20 - 4);
			c->map();

			long state = NormalState;
			XChangeProperty((*i)->dpy, (*i)->xwin, (*i)->atoms->WM_STATE, (*i)->atoms->CARDINAL, 32,
					PropModeReplace,
					reinterpret_cast<unsigned char *>(&state), 1);
		}
		(*i)->unlock_client();
	}
}

bool notebook_t::add_notebook(client_t *c) {
	printf("Add client %lu\n", c->xwin);
	_clients.push_front(c);
	_selected = _clients.begin();
	return true;
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

void notebook_t::activate_client(client_t * c) {
	bool has_client = false;
	std::list<client_t *>::iterator i = _clients.begin();
	while (i != _clients.end()) {
		if((*i) == c) {
			has_client = true;
			break;
		}
		++i;
	}

	if(has_client) {
		_selected = i;
	}

}

std::list<client_t *> * notebook_t::get_clients() {
	return &_clients;
}

void notebook_t::remove_client(Window w) {
	std::list<client_t *>::iterator i = _clients.begin();
	while (i != _clients.end()) {
		if ((*i)->xwin == w) {
			if (i == _selected)
				select_next();
			_clients.remove((*i));
			break;
		}
		++i;
	}
}

void notebook_t::select_next() {
	++_selected;
	if (_selected == _clients.end())
		_selected = _clients.begin();
}

void notebook_t::rounded_rectangle(cairo_t * cr, double x, double y, double w,
		double h, double r) {
	cairo_save(cr);
	{
		cairo_new_path(cr);
		cairo_move_to(cr, x, y + h);
		cairo_line_to(cr, x, y + r);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_stroke(cr);
		cairo_new_path(cr);
		cairo_arc(cr, x + r, y + r, r, 2.0 * (M_PI_2), 3.0 * (M_PI_2));
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
		cairo_stroke(cr);
		cairo_new_path(cr);
		cairo_move_to(cr, x + r, y);
		cairo_line_to(cr, x + w - r, y);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_stroke(cr);
		cairo_new_path(cr);
		cairo_arc(cr, x + w - r, y + r, r, 3.0 * (M_PI_2), 4.0 * (M_PI_2));
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
		cairo_stroke(cr);
		cairo_new_path(cr);
		cairo_move_to(cr, x + w, y + h);
		cairo_line_to(cr, x + w, y + r);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
		cairo_stroke(cr);
	}
	cairo_restore(cr);
}

}
