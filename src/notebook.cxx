/*
 * notebook.cxx
 *
 * copyright (2011) Benoit Gschwind
 *
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

notebook_t::notebook_t(main_t & page) :
		page(page) {
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
	button_close.h = HEIGHT;

	button_vsplit.x = _allocation.x + _allocation.w - 17 * 2;
	button_vsplit.y = _allocation.y;
	button_vsplit.w = 17;
	button_vsplit.h = HEIGHT;

	button_hsplit.x = _allocation.x + _allocation.w - 17 * 3;
	button_hsplit.y = _allocation.y;
	button_hsplit.w = 17;
	button_hsplit.h = HEIGHT;

	tab_area.x = _allocation.x;
	tab_area.y = _allocation.y;
	tab_area.w = _allocation.w;
	tab_area.h = HEIGHT;

	top_area.x = _allocation.x;
	top_area.y = _allocation.y + HEIGHT;
	top_area.w = _allocation.w;
	top_area.h = (_allocation.h - HEIGHT) * 0.2;

	bottom_area.x = _allocation.x;
	bottom_area.y = _allocation.y + HEIGHT + (0.8 * (_allocation.h - HEIGHT));
	bottom_area.w = _allocation.w;
	bottom_area.h = (_allocation.h - HEIGHT) * 0.2;

	left_area.x = _allocation.x;
	left_area.y = _allocation.y + HEIGHT;
	left_area.w = _allocation.w * 0.2;
	left_area.h = (_allocation.h - HEIGHT);

	right_area.x = _allocation.x + _allocation.w * 0.8;
	right_area.y = _allocation.y + HEIGHT;
	right_area.w = _allocation.w * 0.2;
	right_area.h = (_allocation.h - HEIGHT);

	//printf("xx %dx%d+%d+%d\n", _allocation.w, _allocation.h, _allocation.x,
	//		_allocation.y);
	//printf("xx %dx%d+%d+%d\n", tab_area.w, tab_area.h, tab_area.x, tab_area.y);
	//printf("xx %dx%d+%d+%d\n", top_area.w, top_area.h, top_area.x, top_area.y);
	//printf("xx %dx%d+%d+%d\n", bottom_area.w, bottom_area.h, bottom_area.x,
	//		bottom_area.y);
	//printf("xx %dx%d+%d+%d\n", left_area.w, left_area.h, left_area.x,
	//		left_area.y);
	//printf("xx %dx%d+%d+%d\n", right_area.w, right_area.h, right_area.x,
	//		right_area.y);

	//update_client_mapping();

	if (_selected.size() > 0) {
		client_t * c = _selected.front();
		c->update_client_size(_allocation.w - 2 * BORDER_SIZE,
				_allocation.h - HEIGHT - 2 * BORDER_SIZE);
		printf("XResizeWindow(%p, #%lu, %d, %d)\n", c->cnx.dpy, c->xwin,
				c->width, c->height);

		int offset_x = (_allocation.w - 2 * BORDER_SIZE - c->width) / 2;
		int offset_y = (_allocation.h - HEIGHT - BORDER_SIZE - c->height) / 2;
		if (offset_x < 0)
			offset_x = 0;
		if (offset_y < 0)
			offset_y = 0;
		XMoveResizeWindow(c->cnx.dpy, c->xwin, offset_x, offset_y, c->width,
				c->height);
		XMoveResizeWindow(c->cnx.dpy, c->clipping_window,
				_allocation.x + BORDER_SIZE, _allocation.y + HEIGHT,
				_allocation.w - 2 * BORDER_SIZE,
				_allocation.h - HEIGHT - BORDER_SIZE);

	}
}

void notebook_t::render() {

	//update_client_mapping();

	if (back_buffer == 0) {
		cairo_surface_t * target = cairo_get_target(page.main_window_cr);
		back_buffer = cairo_surface_create_similar(target, CAIRO_CONTENT_COLOR,
				_allocation.w, HEIGHT);
		back_buffer_cr = cairo_create(back_buffer);
		printf("allocate %p \n", back_buffer);
	}

	if (!back_buffer_is_valid) {

		cairo_save(back_buffer_cr);
		{
			cairo_set_line_width(back_buffer_cr, 1.0);
			cairo_set_antialias(back_buffer_cr, CAIRO_ANTIALIAS_DEFAULT);

			/* create tabs back ground */
			cairo_rectangle(back_buffer_cr, 0.0, 0.0, _allocation.w, HEIGHT);
			cairo_pattern_t *pat;
			pat = cairo_pattern_create_linear(0.0, 0.0, 0.0, HEIGHT);
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
			cairo_move_to(back_buffer_cr, 0.0, HEIGHT - 0.5);
			cairo_line_to(back_buffer_cr, _allocation.w, HEIGHT - 0.5);

			cairo_stroke(back_buffer_cr);

			std::list<client_t *>::iterator i;
			int offset = 0;
			int length = (_allocation.w - 17 * 3) / (_clients.size() + 1);
			for (i = _clients.begin(); i != _clients.end(); ++i) {

				cairo_save(back_buffer_cr);
				{

					cairo_translate(back_buffer_cr, offset, 0.0);

					if (_selected.front() == (*i)) {

						cairo_set_line_width(back_buffer_cr, 1.0);
						cairo_select_font_face(back_buffer_cr, "Sans",
								CAIRO_FONT_SLANT_NORMAL,
								CAIRO_FONT_WEIGHT_BOLD);
						cairo_set_font_size(back_buffer_cr, 13);

						/* draw light background */
						cairo_rectangle(back_buffer_cr, 0.0, 3.0, length * 2,
								HEIGHT);
						cairo_set_source_rgb(back_buffer_cr, 0xeeU / 255.0,
								0xeeU / 255.0, 0xecU / 255.0);
						cairo_fill(back_buffer_cr);

						if ((*i)->icon_surf != 0) {
							cairo_save(back_buffer_cr);
							cairo_translate(back_buffer_cr, 3.0, 4.0);
							cairo_set_source_surface(back_buffer_cr,
									(*i)->icon_surf, 0.0, 0.0);
							cairo_paint(back_buffer_cr);
							cairo_restore(back_buffer_cr);
						}

						/* draw the name */
						cairo_rectangle(back_buffer_cr, 2.0, 0.0,
								length * 2 - 16.0, HEIGHT - 2.0);
						cairo_clip(back_buffer_cr);
						cairo_set_source_rgb(back_buffer_cr, 0.0, 0.0, 0.0);
						cairo_set_font_size(back_buffer_cr, 13.0);
						cairo_move_to(back_buffer_cr, 20.5, 15.5);
						cairo_show_text(back_buffer_cr, (*i)->name.c_str());

						/* draw blue lines */
						cairo_reset_clip(back_buffer_cr);

						cairo_new_path(back_buffer_cr);
						cairo_move_to(back_buffer_cr, 1.0, 1.5);
						cairo_line_to(back_buffer_cr, length * 2, 1.5);
						cairo_move_to(back_buffer_cr, 1.0, 2.5);
						cairo_line_to(back_buffer_cr, length * 2, 2.5);
						cairo_set_source_rgb(back_buffer_cr, 0x72U / 255.0,
								0x9fU / 255.0, 0xcfU / 255.0);
						cairo_stroke(back_buffer_cr);
						cairo_new_path(back_buffer_cr);
						cairo_move_to(back_buffer_cr, 0.0, 3.5);
						cairo_line_to(back_buffer_cr, length * 2, 3.5);
						cairo_set_source_rgb(back_buffer_cr, 0x34U / 255.0,
								0x64U / 255.0, 0xa4U / 255.0);
						cairo_stroke(back_buffer_cr);

						cairo_set_source_rgb(back_buffer_cr, 0x88U / 255.0,
								0x8aU / 255.0, 0x85U / 255.0);
						rounded_rectangle(back_buffer_cr, 0.5, 0.5, length * 2,
								HEIGHT - 1.0, 3.0);

						/* draw close icon */
						cairo_set_line_width(back_buffer_cr, 2.0);
						cairo_translate(back_buffer_cr, length * 2 - 16.5, 3.5);
						/* draw close */
						cairo_new_path(back_buffer_cr);
						cairo_move_to(back_buffer_cr, 4.0, 4.0);
						cairo_line_to(back_buffer_cr, 12.0, 12.0);
						cairo_move_to(back_buffer_cr, 12.0, 4.0);
						cairo_line_to(back_buffer_cr, 4.0, 12.0);
						cairo_set_source_rgb(back_buffer_cr, 0xCCU / 255.0,
								0x00U / 255.0, 0x00U / 255.0);
						cairo_stroke(back_buffer_cr);
						offset += length * 2;
					} else {

						if ((*i)->icon_surf != 0) {
							cairo_save(back_buffer_cr);
							cairo_translate(back_buffer_cr, 3.0, 4.0);
							cairo_set_source_surface(back_buffer_cr,
									(*i)->icon_surf, 0.0, 0.0);
							cairo_paint(back_buffer_cr);
							cairo_restore(back_buffer_cr);
						}

						cairo_set_line_width(back_buffer_cr, 1.0);
						cairo_select_font_face(back_buffer_cr, "Sans",
								CAIRO_FONT_SLANT_NORMAL,
								CAIRO_FONT_WEIGHT_NORMAL);
						cairo_set_font_size(back_buffer_cr, 13);

						/* draw window name */
						cairo_rectangle(back_buffer_cr, 2, 0, length - 4,
								HEIGHT - 1);
						cairo_clip(back_buffer_cr);
						cairo_set_source_rgb(back_buffer_cr, 0.0, 0.0, 0.0);
						cairo_set_font_size(back_buffer_cr, 13);
						cairo_move_to(back_buffer_cr, HEIGHT + 0.5, 15.5);
						cairo_show_text(back_buffer_cr, (*i)->name.c_str());

						/* draw border */
						cairo_reset_clip(back_buffer_cr);

						cairo_rectangle(back_buffer_cr, 0.0, 0.0, length + 1.0,
								HEIGHT);
						cairo_clip(back_buffer_cr);
						cairo_set_source_rgb(back_buffer_cr, 0x88U / 255.0,
								0x8aU / 255.0, 0x85U / 255.0);
						rounded_rectangle(back_buffer_cr, 0.5, 2.5, length,
								HEIGHT, 2.0);
						cairo_new_path(back_buffer_cr);
						cairo_move_to(back_buffer_cr, 0.0, HEIGHT);
						cairo_line_to(back_buffer_cr, length + 1.0, HEIGHT);
						cairo_stroke(back_buffer_cr);
						offset += length;
					}
				}
				cairo_restore(back_buffer_cr);

			}

			cairo_save(back_buffer_cr);
			{
				cairo_translate(back_buffer_cr, _allocation.w - 16.5, 1.5);
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

	cairo_save(page.main_window_cr);
	{

		cairo_set_antialias(page.main_window_cr, CAIRO_ANTIALIAS_DEFAULT);

		/* draw border */
		if (_clients.size() == 0) {
			cairo_set_source_rgb(page.main_window_cr, 0xeeU / 255.0,
					0xeeU / 255.0, 0xecU / 255.0);
			cairo_rectangle(page.main_window_cr, _allocation.x, _allocation.y,
					_allocation.w, _allocation.h);
			cairo_fill(page.main_window_cr);
		}

		/* draw top line */

		cairo_translate(page.main_window_cr, _allocation.x, _allocation.y);
		cairo_rectangle(page.main_window_cr, 0.0, 0.0, _allocation.w, HEIGHT);
		cairo_set_source_surface(page.main_window_cr, back_buffer, 0.0, 0.0);
		cairo_fill(page.main_window_cr);

		cairo_set_source_rgb(page.main_window_cr, 0x88U / 255.0, 0x8aU / 255.0,
				0x85U / 255.0);
		cairo_set_line_width(page.main_window_cr, 1.0);
		cairo_new_path(page.main_window_cr);
		cairo_move_to(page.main_window_cr, 0.5, 0.5);
		cairo_line_to(page.main_window_cr, _allocation.w - 0.5, 0.5);
		cairo_line_to(page.main_window_cr, _allocation.w - 0.5,
				_allocation.h - 0.5);
		cairo_line_to(page.main_window_cr, 0.5, _allocation.h - 0.5);
		cairo_line_to(page.main_window_cr, 0.5, 0.5);
		cairo_stroke(page.main_window_cr);

	}
	cairo_restore(page.main_window_cr);

}

bool notebook_t::process_button_press_event(XEvent const * e) {
	if (_allocation.is_inside(e->xbutton.x, e->xbutton.y)) {
		if (_clients.size() > 0) {
			int box_width = ((_allocation.w - 17 * 3) / (_clients.size() + 1));
			box_t<int> b(_allocation.x, _allocation.y, box_width, HEIGHT);
			std::list<client_t *>::iterator c = _clients.begin();
			while (c != _clients.end()) {
				if (*c == _selected.front()) {
					box_t<int> b1 = b;
					b1.w *= 2;
					if (b1.is_inside(e->xbutton.x, e->xbutton.y)) {
						break;
					}
					b.x += box_width * 2;
				} else {
					if (b.is_inside(e->xbutton.x, e->xbutton.y)) {
						break;
					}
					b.x += box_width;
				}

				++c;
			}
			if (c != _clients.end()) {
				if (_selected.front() == (*c)
						&& b.x + b.w * 2 - 16 < e->xbutton.x) {

					XEvent ev;
					ev.xclient.display = page.cnx.dpy;
					ev.xclient.type = ClientMessage;
					ev.xclient.format = 32;
					ev.xclient.message_type = (*c)->cnx.atoms.WM_PROTOCOLS;
					ev.xclient.window = (*c)->xwin;
					ev.xclient.data.l[0] = (*c)->cnx.atoms.WM_DELETE_WINDOW;
					ev.xclient.data.l[1] = e->xbutton.time;

					XSendEvent(page.cnx.dpy, (*c)->xwin, False, NoEventMask,
							&ev);

				} else {
					process_drag_and_drop(*c);
					return true;
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
			client_t * c = (*i);
			c->update_client_size(_allocation.w - 2 * BORDER_SIZE,
					_allocation.h - HEIGHT - 2 * BORDER_SIZE);
			printf("XResizeWindow(%p, #%lu, %d, %d)\n", c->cnx.dpy, c->xwin,
					c->width, c->height);

			int offset_x = (_allocation.w - 2 * BORDER_SIZE - c->width) / 2;
			int offset_y = (_allocation.h - HEIGHT - BORDER_SIZE - c->height)
					/ 2;
			if (offset_x < 0)
				offset_x = 0;
			if (offset_y < 0)
				offset_y = 0;

			if (!c->is_fullscreen()) {
				XMoveResizeWindow(c->cnx.dpy, c->xwin, offset_x, offset_y,
						c->width, c->height);
				XMoveResizeWindow(c->cnx.dpy, c->clipping_window,
						_allocation.x + BORDER_SIZE, _allocation.y + HEIGHT,
						_allocation.w - 2 * BORDER_SIZE,
						_allocation.h - HEIGHT - BORDER_SIZE);
			}
			c->map();

			long state = NormalState;
			XChangeProperty((*i)->cnx.dpy, (*i)->xwin, (*i)->cnx.atoms.WM_STATE,
					(*i)->cnx.atoms.CARDINAL, 32, PropModeReplace,
					reinterpret_cast<unsigned char *>(&state), 1);
		}
	}

	for (i = _clients.begin(); i != _clients.end(); ++i) {
		if ((*i) != _selected.front()) {
			(*i)->unmap();
			long state = IconicState;
			XChangeProperty((*i)->cnx.dpy, (*i)->xwin, (*i)->cnx.atoms.WM_STATE,
					(*i)->cnx.atoms.CARDINAL, 32, PropModeReplace,
					reinterpret_cast<unsigned char *>(&state), 1);
		}
	}
}

bool notebook_t::add_notebook(client_t *c) {
	printf("Add client #%lu\n", c->xwin);
	_clients.push_front(c);
	back_buffer_is_valid = false;
	set_selected(c);
	return true;
}

void notebook_t::split(split_type_e type) {
	split_t * split = new split_t(page, type);
	_parent->replace(this, split);
	split->replace(0, this);
	split->replace(0, new notebook_t(page));
	//update_client_mapping();
}

void notebook_t::split_left(client_t * c) {
	split_t * split = new split_t(page, VERTICAL_SPLIT);
	_parent->replace(this, split);
	notebook_t * n = new notebook_t(page);
	split->replace(0, n);
	split->replace(0, this);
	n->add_notebook(c);
}

void notebook_t::split_right(client_t * c) {
	split_t * split = new split_t(page, VERTICAL_SPLIT);
	_parent->replace(this, split);
	notebook_t * n = new notebook_t(page);
	split->replace(0, this);
	split->replace(0, n);
	n->add_notebook(c);
}

void notebook_t::split_top(client_t * c) {
	split_t * split = new split_t(page, HORIZONTAL_SPLIT);
	_parent->replace(this, split);
	notebook_t * n = new notebook_t(page);
	split->replace(0, n);
	split->replace(0, this);
	n->add_notebook(c);
}

void notebook_t::split_bottom(client_t * c) {
	split_t * split = new split_t(page, HORIZONTAL_SPLIT);
	_parent->replace(this, split);
	split->replace(0, this);
	notebook_t * n = new notebook_t(page);
	split->replace(0, n);
	n->add_notebook(c);
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
		(*i)->focus();
		page.focuced = (*i);
	}

	back_buffer_is_valid = false;

}

std::list<client_t *> * notebook_t::get_clients() {
	return &_clients;
}

void notebook_t::remove_client(client_t * c) {
	std::list<client_t *>::iterator i = _clients.begin();
	while (i != _clients.end()) {
		if ((*i) == c) {
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
	if (_selected.size() > 0) {
		/* quite tricky, select next, them remove */
		client_t * c = _selected.front();
		if (_selected.size() > 1) {
			client_list_t::iterator i = _selected.begin();
			++i;
			set_selected(*i);
		}
		_selected.remove(c);
		back_buffer_is_valid = false;
	}
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

void notebook_t::set_selected(client_t * c) {
	if (c != _selected.front()) {
		if (!c->is_fullscreen()) {
			c->update_client_size(_allocation.w - 2 * BORDER_SIZE,
					_allocation.h - HEIGHT - 2 * BORDER_SIZE);
			printf("XResizeWindow(%p, #%lu, %d, %d)\n", c->cnx.dpy, c->xwin,
					c->width, c->height);

			int offset_x = (_allocation.w - 2 * BORDER_SIZE - c->width) / 2;
			int offset_y = (_allocation.h - HEIGHT - BORDER_SIZE - c->height)
					/ 2;
			if (offset_x < 0)
				offset_x = 0;
			if (offset_y < 0)
				offset_y = 0;
			XMoveResizeWindow(c->cnx.dpy, c->xwin, offset_x, offset_y, c->width,
					c->height);
			XMoveResizeWindow(c->cnx.dpy, c->clipping_window,
					_allocation.x + BORDER_SIZE, _allocation.y + HEIGHT,
					_allocation.w - 2 * BORDER_SIZE,
					_allocation.h - HEIGHT - BORDER_SIZE);
			c->map();
			if (_selected.size() > 0) {
				client_t * c1 = _selected.front();
				c1->unmap();
				long state = IconicState;
				XChangeProperty(page.cnx.dpy, c1->xwin, page.cnx.atoms.WM_STATE,
						page.cnx.atoms.CARDINAL, 32, PropModeReplace,
						reinterpret_cast<unsigned char *>(&state), 1);
			}
		}
		_selected.remove(c);
		_selected.push_front(c);
	}
}

void notebook_t::process_drag_and_drop(client_t * c) {
	XEvent ev;
	cairo_t * cr;

	notebook_t * ns = 0;
	select_e zone = SELECT_NONE;

	cursor = XCreateFontCursor(page.cnx.dpy, XC_fleur);

	popup_notebook_t * p = new popup_notebook_t(tab_area.x, tab_area.y, tab_area.w,
			tab_area.h);

	page.popups.push_back(p);

	if (XGrabPointer(page.cnx.dpy, page.main_window, False,
			(ButtonPressMask | ButtonReleaseMask | PointerMotionMask),
			GrabModeAsync, GrabModeAsync, None, cursor,
			CurrentTime) != GrabSuccess
			)
		return;
	do {

		XIfEvent(page.cnx.dpy, &ev, drag_and_drop_filter, (char *) this);

		if (ev.type == page.damage_event + XDamageNotify) {
			page.process_damage_event(&ev);
		} else if (ev.type == MotionNotify) {
			if (ev.xmotion.window == page.main_window) {
				//printf("%d %d %d %d\n", ev.xmotion.x, ev.xmotion.y,
				//		ev.xmotion.x_root, ev.xmotion.y_root);
				std::list<notebook_t *>::iterator i = notebooks.begin();
				while (i != notebooks.end()) {
					if ((*i)->tab_area.is_inside(ev.xmotion.x_root,
							ev.xmotion.y_root)) {
						//printf("tab\n");
						if (zone != SELECT_TAB || ns != (*i)) {
							zone = SELECT_TAB;
							ns = (*i);
							p->update_area(page.composite_overlay_cr,
									page.main_window_s, (*i)->tab_area.x,
									(*i)->tab_area.y, (*i)->tab_area.w,
									(*i)->tab_area.h);
						}
					} else if ((*i)->right_area.is_inside(ev.xmotion.x_root,
							ev.xmotion.y_root)) {
						//printf("right\n");
						if (zone != SELECT_RIGHT || ns != (*i)) {
							zone = SELECT_RIGHT;
							ns = (*i);
							p->update_area(page.composite_overlay_cr,
									page.main_window_s, (*i)->right_area.x,
									(*i)->right_area.y, (*i)->right_area.w,
									(*i)->right_area.h);
						}
					} else if ((*i)->top_area.is_inside(ev.xmotion.x_root,
							ev.xmotion.y_root)) {
						//printf("top\n");
						if (zone != SELECT_TOP || ns != (*i)) {
							zone = SELECT_TOP;
							ns = (*i);
							p->update_area(page.composite_overlay_cr,
									page.main_window_s, (*i)->top_area.x,
									(*i)->top_area.y, (*i)->top_area.w,
									(*i)->top_area.h);
						}
					} else if ((*i)->bottom_area.is_inside(ev.xmotion.x_root,
							ev.xmotion.y_root)) {
						//printf("bottom\n");
						if (zone != SELECT_BOTTOM || ns != (*i)) {
							zone = SELECT_BOTTOM;
							ns = (*i);
							p->update_area(page.composite_overlay_cr,
									page.main_window_s, (*i)->bottom_area.x,
									(*i)->bottom_area.y, (*i)->bottom_area.w,
									(*i)->bottom_area.h);
						}
					} else if ((*i)->left_area.is_inside(ev.xmotion.x_root,
							ev.xmotion.y_root)) {
						//printf("left\n");
						if (zone != SELECT_LEFT || ns != (*i)) {
							zone = SELECT_LEFT;
							ns = (*i);
							p->update_area(page.composite_overlay_cr,
									page.main_window_s, (*i)->left_area.x,
									(*i)->left_area.y, (*i)->left_area.w,
									(*i)->left_area.h);
						}
					}
					++i;
				}
			}
		}
	} while (ev.type != ButtonRelease);
	page.popups.remove(p);
	delete p;
	render();
	/* ev is button release
	 * so set the hidden focus parameter
	 */
	c->cnx.last_know_time = ev.xbutton.time;

	XUngrabPointer(page.cnx.dpy, CurrentTime);
	XFreeCursor(page.cnx.dpy, cursor);

	if (zone == SELECT_TAB && ns != 0) {

		if (ns != this) {
			client_t * move = c;
			/* reselect a new window */
			select_next();
			_clients.remove(move);
			(ns)->add_notebook(move);
		}

	} else if (zone == SELECT_TOP && ns != 0) {
		select_next();
		_clients.remove(c);
		ns->split_top(c);
	} else if (zone == SELECT_LEFT && ns != 0) {
		select_next();
		_clients.remove(c);
		ns->split_left(c);
	} else if (zone == SELECT_BOTTOM && ns != 0) {
		select_next();
		_clients.remove(c);
		ns->split_bottom(c);
	} else if (zone == SELECT_RIGHT && ns != 0) {
		select_next();
		_clients.remove(c);
		ns->split_right(c);
	} else {
		set_selected(c);
		if (_selected.size() > 0) {
			client_t * c = _selected.front();
			//c->map();
			c->focus();
			page.focuced = c;
			XChangeProperty(c->cnx.dpy, c->cnx.xroot,
					c->cnx.atoms._NET_ACTIVE_WINDOW, c->cnx.atoms.WINDOW, 32,
					PropModeReplace,
					reinterpret_cast<unsigned char *>(&(c->xwin)), 1);

		}
	}

	if (_clients.size() == 0) {
		_parent->remove(this);
	} else {
		render();
	}
}

Bool notebook_t::drag_and_drop_filter(Display * dpy, XEvent * ev, char * arg) {
	notebook_t * ths = (notebook_t *) arg;
	return (ev->type == ConfigureRequest) || (ev->type == Expose)
			|| (ev->type == MotionNotify) || (ev->type == ButtonRelease)
			|| (ev->type == ths->page.damage_event + XDamageNotify);
}

}
