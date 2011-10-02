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

	back_buffer_is_valid = false;
	back_buffer = 0;
	back_buffer_cr = 0;
	notebooks.push_back(this);

}

notebook_t::~notebook_t() {
	cairo_destroy(back_buffer_cr);
	cairo_surface_destroy(back_buffer);
	notebooks.remove(this);
}

void notebook_t::update_allocation(box_t<int> & allocation) {

	if (_allocation.w < allocation.w) {
		cairo_destroy(back_buffer_cr);
		cairo_surface_destroy(back_buffer);
		back_buffer = 0;
	}

	back_buffer_is_valid = false;

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

	if (back_buffer == 0) {
		cairo_surface_t * target = cairo_get_target(cr);
		back_buffer = cairo_surface_create_similar(target, CAIRO_CONTENT_COLOR,
				_allocation.w, 20);
		back_buffer_cr = cairo_create(back_buffer);
		printf("allocate %p \n", back_buffer);
	}

	if (!back_buffer_is_valid) {

		cairo_save(back_buffer_cr);
		{
			cairo_set_line_width(back_buffer_cr, 1.0);
			cairo_set_antialias(back_buffer_cr, CAIRO_ANTIALIAS_DEFAULT);

			/* create tabs back ground */
			cairo_rectangle(back_buffer_cr, 0.0, 0.0, _allocation.w, 20.0);
			cairo_pattern_t *pat;
			pat = cairo_pattern_create_linear(0.0, 0.0, 0.0, 20.0);
			cairo_pattern_add_color_stop_rgba(pat, 0, 0xeeU / 255.0,
					0xeeU / 255.0, 0xecU / 255.0, 1);
			cairo_pattern_add_color_stop_rgba(pat, 1, 0xbaU / 255.0,
					0xbdU / 255.0, 0xd6U / 255.0, 1);
			cairo_set_source(back_buffer_cr, pat);
			cairo_fill(back_buffer_cr);
			cairo_pattern_destroy(pat);

			/* draw top line */
			cairo_set_source_rgb(back_buffer_cr, 0x88U / 255.0, 0x8aU / 255.0,
					0x85U / 255.0);
			cairo_new_path(back_buffer_cr);
			cairo_move_to(back_buffer_cr, 0.0, 19.5);
			cairo_line_to(back_buffer_cr, _allocation.w, 19.5);

			cairo_stroke(back_buffer_cr);

			std::list<client_t *>::iterator i;
			int offset = 0;
			int length = (_allocation.w - 17 * 3) / _clients.size();
			for (i = _clients.begin(); i != _clients.end(); ++i) {

				cairo_save(back_buffer_cr);
				{
					cairo_translate(back_buffer_cr, offset, 0.0);

					if (_selected.front() == (*i)) {

						cairo_set_line_width(back_buffer_cr, 1.0);
						cairo_select_font_face(back_buffer_cr, "Sans",
								CAIRO_FONT_SLANT_NORMAL,
								CAIRO_FONT_WEIGHT_NORMAL);
						cairo_set_font_size(back_buffer_cr, 13);

						/* draw light background */
						cairo_rectangle(back_buffer_cr, 0.0, 3.0, length, 20.0);
						cairo_set_source_rgb(back_buffer_cr, 0xeeU / 255.0,
								0xeeU / 255.0, 0xecU / 255.0);
						cairo_fill(back_buffer_cr);

						/* draw the name */
						cairo_rectangle(back_buffer_cr, 2.0, 0.0, length - 16.0,
								19.0);
						cairo_clip(back_buffer_cr);
						cairo_set_source_rgb(back_buffer_cr, 0.0, 0.0, 0.0);
						cairo_set_font_size(back_buffer_cr, 13.0);
						cairo_move_to(back_buffer_cr, 3.5, 15.5);
						cairo_show_text(back_buffer_cr, (*i)->name.c_str());

						/* draw blue lines */
						cairo_reset_clip(back_buffer_cr);

						cairo_new_path(back_buffer_cr);
						cairo_move_to(back_buffer_cr, 1.0, 1.5);
						cairo_line_to(back_buffer_cr, length, 1.5);
						cairo_move_to(back_buffer_cr, 1.0, 2.5);
						cairo_line_to(back_buffer_cr, length, 2.5);
						cairo_set_source_rgb(back_buffer_cr, 0x72U / 255.0,
								0x9fU / 255.0, 0xcfU / 255.0);
						cairo_stroke(back_buffer_cr);
						cairo_new_path(back_buffer_cr);
						cairo_move_to(back_buffer_cr, 0.0, 3.5);
						cairo_line_to(back_buffer_cr, length, 3.5);
						cairo_set_source_rgb(back_buffer_cr, 0x34U / 255.0,
								0x64U / 255.0, 0xa4U / 255.0);
						cairo_stroke(back_buffer_cr);

						cairo_set_source_rgb(back_buffer_cr, 0x88U / 255.0,
								0x8aU / 255.0, 0x85U / 255.0);
						rounded_rectangle(back_buffer_cr, 0.5, 0.5, length,
								19.0, 3.0);

						/* draw close icon */
						cairo_set_line_width(back_buffer_cr, 2.0);
						cairo_translate(back_buffer_cr, length - 16.0, 2.0);
						/* draw close */
						cairo_new_path(back_buffer_cr);
						cairo_move_to(back_buffer_cr, 4.0, 4.0);
						cairo_line_to(back_buffer_cr, 12.0, 12.0);
						cairo_move_to(back_buffer_cr, 12.0, 4.0);
						cairo_line_to(back_buffer_cr, 4.0, 12.0);
						cairo_set_source_rgb(back_buffer_cr, 0xCCU / 255.0,
								0x00U / 255.0, 0x00U / 255.0);
						cairo_stroke(back_buffer_cr);

					} else {

						cairo_set_line_width(back_buffer_cr, 1.0);
						cairo_select_font_face(back_buffer_cr, "Sans",
								CAIRO_FONT_SLANT_NORMAL,
								CAIRO_FONT_WEIGHT_NORMAL);
						cairo_set_font_size(back_buffer_cr, 13);

						/* draw window name */
						cairo_rectangle(back_buffer_cr, 2, 0, length - 4, 19);
						cairo_clip(back_buffer_cr);
						cairo_set_source_rgb(back_buffer_cr, 0.0, 0.0, 0.0);
						cairo_set_font_size(back_buffer_cr, 13);
						cairo_move_to(back_buffer_cr, 3.0, 15.0);
						cairo_show_text(back_buffer_cr, (*i)->name.c_str());

						/* draw border */
						cairo_reset_clip(back_buffer_cr);

						cairo_rectangle(back_buffer_cr, 0.0, 0.0, length + 1.0,
								20.0);
						cairo_clip(back_buffer_cr);
						cairo_set_source_rgb(back_buffer_cr, 0x88U / 255.0,
								0x8aU / 255.0, 0x85U / 255.0);
						rounded_rectangle(back_buffer_cr, 0.5, 2.5, length,
								20.0, 2.0);
						cairo_new_path(back_buffer_cr);
						cairo_move_to(back_buffer_cr, 0.0, 20.0);
						cairo_line_to(back_buffer_cr, length + 1.0, 20.0);
						cairo_stroke(back_buffer_cr);
					}
				}
				cairo_restore(back_buffer_cr);
				offset += length;
			}

			cairo_save(back_buffer_cr);
			{
				cairo_translate(back_buffer_cr, _allocation.w - 16.0, 1.0);
				/* draw close */
				cairo_new_path(back_buffer_cr);
				cairo_move_to(back_buffer_cr, 4.0, 4.0);
				cairo_line_to(back_buffer_cr, 12.0, 12.0);
				cairo_move_to(back_buffer_cr, 12.0, 4.0);
				cairo_line_to(back_buffer_cr, 4.0, 12.0);
				cairo_set_source_rgb(back_buffer_cr, 0xCCU / 255.0,
						0x00U / 255.0, 0x00U / 255.0);
				cairo_stroke(back_buffer_cr);

				/* draw vertical split */
				cairo_translate(back_buffer_cr, -17.0, 0.0);
				cairo_move_to(back_buffer_cr, 8.0, 2.0);
				cairo_line_to(back_buffer_cr, 8.0, 14.0);
				cairo_move_to(back_buffer_cr, 9.0, 2.0);
				cairo_line_to(back_buffer_cr, 9.0, 14.0);
				cairo_set_source_rgb(back_buffer_cr, 0.0, 0.0, 0.0);
				cairo_stroke(back_buffer_cr);

				/* draw horizontal split */
				cairo_translate(back_buffer_cr, -17.0, 0.0);
				cairo_move_to(back_buffer_cr, 2.0, 8.0);
				cairo_line_to(back_buffer_cr, 14.0, 8.0);
				cairo_move_to(back_buffer_cr, 2.0, 8.0);
				cairo_line_to(back_buffer_cr, 14.0, 8.0);
				cairo_set_source_rgb(back_buffer_cr, 0.0, 0.0, 0.0);
				cairo_stroke(back_buffer_cr);
			}
			cairo_restore(back_buffer_cr);

		}
		cairo_restore(back_buffer_cr);

	}

	cairo_save(cr);
	{

		cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

		/* draw border */
		//cairo_set_line_width(cr, 1.0);
		//cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0, 0xecU / 255.0);
		//cairo_rectangle(cr, _allocation.x + 1, _allocation.y + 21,
		//		_allocation.w - 2, _allocation.h - 22);
		//if (_selected.size() == 0) {
		//	cairo_stroke(cr);
		//} else {
		//	cairo_fill(cr);
		//}
		/* draw top line */
		cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0, 0x85U / 255.0);
		cairo_set_line_width(cr, 1.0);
		cairo_new_path(cr);
		cairo_move_to(cr, _allocation.x + _allocation.w - 0.5,
				_allocation.y + 19.5);
		cairo_line_to(cr, _allocation.x + _allocation.w - 0.5,
				_allocation.y + _allocation.h - 0.5);
		cairo_line_to(cr, _allocation.x + 0.5,
				_allocation.y + _allocation.h - 0.5);
		cairo_line_to(cr, _allocation.x + 0.5, _allocation.y + 19.5);
		cairo_stroke(cr);

		cairo_translate(cr, _allocation.x, _allocation.y);
		cairo_rectangle(cr, 0.0, 0.0, _allocation.w, 20.0);
		cairo_set_source_surface(cr, back_buffer, 0.0, 0.0);
		cairo_fill(cr);

	}
	cairo_restore(cr);

	//cairo_save(cr);
	//cairo_set_line_width(cr, 1.0);
	//cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
	//cairo_rectangle(cr, _allocation.x + 0.5, _allocation.y + 0.5, _allocation.w - 1.0, _allocation.h - 1.0);
	//cairo_stroke(cr);
	//cairo_restore(cr);

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
				if (_selected.front() == (*c)
						&& b.x + b.w - 16 < e->xbutton.x) {

					XEvent ev;
					ev.xclient.display = _dpy;
					ev.xclient.type = ClientMessage;
					ev.xclient.format = 32;
					ev.xclient.message_type = (*c)->atoms->WM_PROTOCOLS;
					ev.xclient.window = (*c)->xwin;
					ev.xclient.data.l[0] = (*c)->atoms->WM_DELETE_WINDOW;
					ev.xclient.data.l[1] = e->xbutton.time;

					XSendEvent(_dpy, (*c)->xwin, False, NoEventMask, &ev);

				} else {

					set_selected((*c));
					XEvent ev;
					cairo_t * cr;

					cursor = XCreateFontCursor(_dpy, XC_fleur);

					if (XGrabPointer(
							_dpy,
							_w,
							False,
							(ButtonPressMask | ButtonReleaseMask
									| PointerMotionMask), GrabModeAsync,
							GrabModeAsync, None, cursor,
							CurrentTime) != GrabSuccess
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
					for (dst = notebooks.begin(); dst != notebooks.end();
							++dst) {
						if ((*dst)->_allocation.is_inside(ev.xbutton.x,
								ev.xbutton.y) && this->group == (*dst)->group) {
							break;
						}
					}

					if (dst != notebooks.end() && (*dst) != this) {
						client_t * move = *(c);
						/* reselect a new window */
						set_selected((*c));
						select_next();
						_clients.remove(move);
						(*dst)->add_notebook(move);
					}

					cr = get_cairo();
					render(cr);
					cairo_destroy(cr);

					if ((_selected.front())->try_lock_client()) {
						client_t * c = _selected.front();

						XRaiseWindow(c->dpy, c->xwin);
						XSetInputFocus(c->dpy, c->xwin, RevertToNone,
								CurrentTime);

						XChangeProperty(c->dpy, XDefaultRootWindow(c->dpy),
								c->atoms->_NET_ACTIVE_WINDOW, c->atoms->WINDOW,
								32, PropModeReplace,
								reinterpret_cast<unsigned char *>(&(c->xwin)),
								1);

						(_selected.front())->unlock_client();
					}

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

	/* map before unmap */

	for (i = _clients.begin(); i != _clients.end(); ++i) {
		if ((*i) == _selected.front()) {
			if (!((*i)->try_lock_client()))
				continue;
			client_t * c = (*i);
			c->update_client_size(_allocation.w - 2, _allocation.h - 21);
			printf("XResizeWindow(%p, %lu, %d, %d)\n", c->dpy, c->xwin,
					c->width, c->height);

			int offset_x = (_allocation.w - 2 - c->width) / 2;
			int offset_y = (_allocation.h - 21 - c->height) / 2;
			if (offset_x < 0)
				offset_x = 0;
			if (offset_y < 0)
				offset_y = 0;

			XMoveResizeWindow(c->dpy, c->xwin, offset_x, offset_y, c->width,
					c->height);
			XMoveResizeWindow(c->dpy, c->clipping_window, _allocation.x + 1,
					_allocation.y + 20, _allocation.w - 2, _allocation.h - 21);
			c->map();

			long state = NormalState;
			XChangeProperty((*i)->dpy, (*i)->xwin, (*i)->atoms->WM_STATE,
					(*i)->atoms->CARDINAL, 32, PropModeReplace,
					reinterpret_cast<unsigned char *>(&state), 1);
			(*i)->unlock_client();
		}
	}

	for (i = _clients.begin(); i != _clients.end(); ++i) {
		if ((*i) != _selected.front()) {
			if (!((*i)->try_lock_client()))
				continue;
			(*i)->unmap();
			long state = IconicState;
			XChangeProperty((*i)->dpy, (*i)->xwin, (*i)->atoms->WM_STATE,
					(*i)->atoms->CARDINAL, 32, PropModeReplace,
					reinterpret_cast<unsigned char *>(&state), 1);
			(*i)->unlock_client();
		}

	}
}

bool notebook_t::add_notebook(client_t *c) {
	printf("Add client %lu\n", c->xwin);
	_clients.push_front(c);
	_selected.push_front(c);
	back_buffer_is_valid = false;
	update_client_mapping();
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
		if ((*i) == c) {
			has_client = true;
			break;
		}
		++i;
	}

	if (has_client) {
		set_selected(*i);
	}

	back_buffer_is_valid = false;

}

std::list<client_t *> * notebook_t::get_clients() {
	return &_clients;
}

void notebook_t::remove_client(Window w) {
	std::list<client_t *>::iterator i = _clients.begin();
	while (i != _clients.end()) {
		if ((*i)->xwin == w) {
			if ((*i) == _selected.front())
				select_next();
			_clients.remove((*i));
			break;
		}
		++i;
	}

	back_buffer_is_valid = false;
}

void notebook_t::select_next() {
	_selected.remove(_selected.front());
	back_buffer_is_valid = false;
}

void notebook_t::rounded_rectangle(cairo_t * cr, double x, double y, double w,
		double h, double r) {
	cairo_save(cr);

	cairo_new_path(cr);
	cairo_move_to(cr, x, y + h);
	cairo_line_to(cr, x, y + r);
	cairo_arc(cr, x + r, y + r, r, 2.0 * (M_PI_2), 3.0 * (M_PI_2));
	cairo_move_to(cr, x + r, y);
	cairo_line_to(cr, x + w - r, y);
	cairo_arc(cr, x + w - r, y + r, r, 3.0 * (M_PI_2), 4.0 * (M_PI_2));
	cairo_move_to(cr, x + w, y + h);
	cairo_line_to(cr, x + w, y + r);
	cairo_stroke(cr);

	cairo_restore(cr);
}

}
