/*
 * notebook.cxx
 *
 * copyright (2011) Benoit Gschwind
 *
 */

#include <stdio.h>
#include <cairo-ft.h>
#include <cairo-xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include "notebook.hxx"

namespace page {

std::list<notebook_t *> notebook_t::notebooks;
cairo_surface_t * notebook_t::hsplit_button_s = 0;
cairo_surface_t * notebook_t::vsplit_button_s = 0;
cairo_surface_t * notebook_t::close_button_s = 0;
cairo_surface_t * notebook_t::pop_button_s = 0;
cairo_surface_t * notebook_t::pops_button_s = 0;

bool notebook_t::ft_is_loaded = false;
FT_Library notebook_t::library = 0;
FT_Face notebook_t::face = 0;
cairo_font_face_t * notebook_t::font = 0;
FT_Face notebook_t::face_bold = 0;
cairo_font_face_t * notebook_t::font_bold = 0;

notebook_t::notebook_t(page_t & page) :
		page(page) {
	back_buffer_is_valid = false;
	back_buffer = 0;
	back_buffer_cr = 0;
	notebooks.push_back(this);

	if (hsplit_button_s == 0) {
		std::string filename = page.page_base_dir + "/hsplit_button.png";
		printf("Load: %s\n", filename.c_str());
		hsplit_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (hsplit_button_s == 0)
			throw std::runtime_error("file not found!");
	}

	if (vsplit_button_s == 0) {
		std::string filename = page.page_base_dir + "/vsplit_button.png";
		printf("Load: %s\n", filename.c_str());
		vsplit_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (vsplit_button_s == 0)
			throw std::runtime_error("file not found!");
	}

	if (close_button_s == 0) {
		std::string filename = page.page_base_dir + "/close_button.png";
		printf("Load: %s\n", filename.c_str());
		close_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (close_button_s == 0)
			throw std::runtime_error("file not found!");
	}

	if (pop_button_s == 0) {
		std::string filename = page.page_base_dir + "/pop_button.png";
		printf("Load: %s\n", filename.c_str());
		pop_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (pop_button_s == 0)
			throw std::runtime_error("file not found!");
	}

	if (pops_button_s == 0) {
		std::string filename = page.page_base_dir + "/pops_button.png";
		printf("Load: %s\n", filename.c_str());
		pops_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (pop_button_s == 0)
			throw std::runtime_error("file not found!");
	}

	if (!ft_is_loaded) {
		FT_Error error = FT_Init_FreeType(&library);
		if (error) {
			throw std::runtime_error("unable to init freetype");
		}

		error = FT_New_Face(library, page.font.c_str(), 0, &face);

		if (error != FT_Err_Ok)
			throw std::runtime_error("unable to load default font");

		font = cairo_ft_font_face_create_for_ft_face(face, 0);

		error = FT_New_Face(library, page.font_bold.c_str(), 0, &face_bold);

		if (error != FT_Err_Ok)
			throw std::runtime_error("unable to load default bold font");

		font_bold = cairo_ft_font_face_create_for_ft_face(face_bold, 0);

		ft_is_loaded = true;
	}

}

notebook_t::~notebook_t() {

	client_list_t::iterator i = _clients.begin();
	while(i != _clients.end()) {
		delete (*i);
		++i;
	}

	if (back_buffer_cr != 0)
		cairo_destroy(back_buffer_cr);
	if (back_buffer != 0)
		cairo_surface_destroy(back_buffer);
	notebooks.remove(this);
	if (page.default_window_pop == this)
		page.default_window_pop = 0;
}

void notebook_t::update_allocation(box_t<int> & allocation) {

	if (_allocation.w < allocation.w) {
		cairo_destroy(back_buffer_cr);
		cairo_surface_destroy(back_buffer);
		back_buffer = 0;
		back_buffer_cr = 0;
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

	button_pop.x = _allocation.x + _allocation.w - 17 * 4;
	button_pop.y = _allocation.y;
	button_pop.w = 17;
	button_pop.h = HEIGHT;

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

	client_area.x = _allocation.x + BORDER_SIZE;
	client_area.y = _allocation.y + BORDER_SIZE + HEIGHT;
	client_area.w = _allocation.w - 2 * BORDER_SIZE;
	client_area.h = _allocation.h - HEIGHT - 2 * BORDER_SIZE;

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

	if (_selected.empty()) {
		if (!_clients.empty()) {
			_selected.push_front(_clients.front());
		}
	}

	client_list_t::iterator i = _clients.begin();
	while (i != _clients.end()) {
		update_client_position(*i);
		++i;
	}

}

void notebook_t::render() {

	if (back_buffer == 0) {
		cairo_surface_t * target = cairo_get_target(page.back_buffer_cr);
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
			int length = (_allocation.w - 17 * 4) / (_clients.size() + 1);
			for (i = _clients.begin(); i != _clients.end(); ++i) {

				cairo_save(back_buffer_cr);
				{

					cairo_translate(back_buffer_cr, offset, 0.0);

					if (_selected.front() == (*i)) {

						cairo_set_line_width(back_buffer_cr, 1.0);
						cairo_set_font_face(back_buffer_cr, font_bold);
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
								length * 2 - 17.0, HEIGHT - 2.0);
						cairo_clip(back_buffer_cr);
						cairo_set_source_rgb(back_buffer_cr, 0.0, 0.0, 0.0);
						cairo_set_font_size(back_buffer_cr, 13.0);
						cairo_move_to(back_buffer_cr, 20.5, 15.5);
						cairo_show_text(back_buffer_cr,
								(*i)->get_title().c_str());

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

						cairo_set_antialias(back_buffer_cr,
								CAIRO_ANTIALIAS_NONE);
						cairo_set_source_surface(back_buffer_cr, close_button_s,
								0.5, 0.5);
						cairo_paint(back_buffer_cr);

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
						cairo_set_font_face(back_buffer_cr, font);
						cairo_set_font_size(back_buffer_cr, 13);

						/* draw window name */
						cairo_rectangle(back_buffer_cr, 2, 0, length - 4,
								HEIGHT - 1);
						cairo_clip(back_buffer_cr);
						cairo_set_source_rgb(back_buffer_cr, 0.0, 0.0, 0.0);
						cairo_set_font_size(back_buffer_cr, 13);
						cairo_move_to(back_buffer_cr, HEIGHT + 0.5, 15.5);
						cairo_show_text(back_buffer_cr,
								(*i)->get_title().c_str());

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

				/* draw icon background */
				cairo_rectangle(back_buffer_cr, _allocation.w - 17.0 * 4, 0,
						17.0 * 4, HEIGHT - 1);
				cairo_set_source_rgb(back_buffer_cr, 0xeeU / 255.0,
						0xeeU / 255.0, 0xecU / 255.0);
				cairo_fill(back_buffer_cr);

				cairo_translate(back_buffer_cr, _allocation.w - 16.5, 1.5);
				/* draw close */
				cairo_new_path(back_buffer_cr);
				cairo_set_source_surface(back_buffer_cr, close_button_s, 0.5,
						0.5);
				cairo_paint(back_buffer_cr);

				/* draw vertical split */
				cairo_translate(back_buffer_cr, -17.0, 0.0);
				cairo_set_source_surface(back_buffer_cr, vsplit_button_s, 0.5,
						0.5);
				cairo_paint(back_buffer_cr);

				/* draw horizontal split */
				cairo_translate(back_buffer_cr, -17.0, 0.0);
				cairo_set_source_surface(back_buffer_cr, hsplit_button_s, 0.5,
						0.5);
				cairo_paint(back_buffer_cr);

				/* draw pop button */
				cairo_translate(back_buffer_cr, -17.0, 0.0);
				if (page.default_window_pop == this) {
					cairo_set_source_surface(back_buffer_cr, pops_button_s, 0.5,
							0.5);
					cairo_paint(back_buffer_cr);
				} else {
					cairo_set_source_surface(back_buffer_cr, pop_button_s, 0.5,
							0.5);
					cairo_paint(back_buffer_cr);
				}

			}
			cairo_restore(back_buffer_cr);

		}
		cairo_restore(back_buffer_cr);

	}

	cairo_save(page.gui_cr);
	{

		cairo_set_antialias(page.gui_cr, CAIRO_ANTIALIAS_DEFAULT);

		/* draw border */
		if (_selected.empty()) {
			cairo_set_source_rgb(page.gui_cr, 0xeeU / 255.0, 0xeeU / 255.0,
					0xecU / 255.0);
			cairo_rectangle(page.gui_cr, _allocation.x, _allocation.y,
					_allocation.w, _allocation.h);
			cairo_fill(page.gui_cr);
		} else {
			client_t * c = _selected.front();
			cairo_set_source_rgb(page.gui_cr, 0xeeU / 255.0, 0xeeU / 255.0,
					0xecU / 255.0);

			box_int_t size = c->get_absolute_extend();
			/* left */
			cairo_rectangle(page.gui_cr, _allocation.x, _allocation.y + HEIGHT,
					size.x - _allocation.x, _allocation.h - HEIGHT);
			cairo_fill(page.gui_cr);
			/* right */
			cairo_rectangle(page.gui_cr, size.x + size.w,
					_allocation.y + HEIGHT,
					_allocation.x + _allocation.w - size.x - size.w,
					_allocation.h - HEIGHT);
			cairo_fill(page.gui_cr);

			/* top */
			cairo_rectangle(page.gui_cr, size.x, _allocation.y + HEIGHT, size.w,
					size.y - _allocation.y - HEIGHT);

			/* bottom */
			cairo_fill(page.back_buffer_cr);
			cairo_rectangle(page.gui_cr, size.x, size.y + size.h, size.w,
					_allocation.y + _allocation.h - size.y - size.h);
			cairo_fill(page.gui_cr);

			cairo_set_line_width(page.gui_cr, 1.0);
			cairo_set_source_rgb(page.gui_cr, 0x88U / 255.0, 0x8aU / 255.0,
					0x85U / 255.0);
			cairo_rectangle(page.gui_cr, size.x - 0.5, size.y - 0.5,
					size.w + 1.0, size.h + 1.0);
			cairo_stroke(page.gui_cr);

		}

		/* draw top line */

		cairo_translate(page.gui_cr, _allocation.x, _allocation.y);
		cairo_rectangle(page.gui_cr, 0.0, 0.0, _allocation.w, HEIGHT);
		cairo_set_source_surface(page.gui_cr, back_buffer, 0.0, 0.0);
		cairo_fill(page.gui_cr);

		cairo_set_source_rgb(page.gui_cr, 0x88U / 255.0, 0x8aU / 255.0,
				0x85U / 255.0);
		//cairo_set_source_rgb(page.gui_cr, 1.0, 0.0, 0.0);
		cairo_set_line_width(page.gui_cr, 1.0);
		cairo_new_path(page.gui_cr);
		cairo_move_to(page.gui_cr, 0.5, 0.5);
		cairo_line_to(page.gui_cr, _allocation.w - 0.5, 0.5);
		cairo_line_to(page.gui_cr, _allocation.w - 0.5, _allocation.h - 0.5);
		cairo_line_to(page.gui_cr, 0.5, _allocation.h - 0.5);
		cairo_line_to(page.gui_cr, 0.5, 0.5);
		cairo_stroke(page.gui_cr);

	}
	cairo_restore(page.gui_cr);

}

bool notebook_t::process_button_press_event(XEvent const * e) {
	if (_allocation.is_inside(e->xbutton.x, e->xbutton.y)) {
		if (!_clients.empty()) {
			int box_width = ((_allocation.w - 17 * 4) / (_clients.size() + 1));
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
					(*c)->delete_window(e->xbutton.time);
				} else {
					process_drag_and_drop(*c, e->xbutton.x, e->xbutton.y);
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
		} else if (button_pop.is_inside(e->xbutton.x, e->xbutton.y)) {
			page.default_window_pop = this;
		}

		return true;
	}

	return false;
}

bool notebook_t::add_client(window_t * x) {
	assert(client_map.find(x) == client_map.end());
	client_t * c = new client_t(*x);
	_clients.push_front(c);
	client_map[x] = c;
	back_buffer_is_valid = false;
	update_client_position(c);
	set_selected(c);
	c->withdraw_to_X();
	render();
	page.add_damage_area(_allocation);
	return true;
}

void notebook_t::split(split_type_e type) {
	page.add_damage_area(_allocation);
	split_t * split = new split_t(page, type);
	_parent->replace(this, split);
	split->replace(0, this);
	notebook_t * n = new notebook_t(page);
	split->replace(0, n);
	render();
}

void notebook_t::split_left(client_t * c) {
	page.add_damage_area(_allocation);
	split_t * split = new split_t(page, VERTICAL_SPLIT);
	_parent->replace(this, split);
	notebook_t * n = new notebook_t(page);
	split->replace(0, n);
	split->replace(0, this);
	n->add_client(c->get_window());
	render();
}

void notebook_t::split_right(client_t * c) {
	page.add_damage_area(_allocation);
	split_t * split = new split_t(page, VERTICAL_SPLIT);
	_parent->replace(this, split);
	notebook_t * n = new notebook_t(page);
	split->replace(0, this);
	split->replace(0, n);
	n->add_client(c->get_window());
	render();
}

void notebook_t::split_top(client_t * c) {
	page.add_damage_area(_allocation);
	split_t * split = new split_t(page, HORIZONTAL_SPLIT);
	_parent->replace(this, split);
	notebook_t * n = new notebook_t(page);
	split->replace(0, n);
	split->replace(0, this);
	n->add_client(c->get_window());
	render();
}

void notebook_t::split_bottom(client_t * c) {
	page.add_damage_area(_allocation);
	split_t * split = new split_t(page, HORIZONTAL_SPLIT);
	_parent->replace(this, split);
	split->replace(0, this);
	notebook_t * n = new notebook_t(page);
	split->replace(0, n);
	n->add_client(c->get_window());
	render();
}

void notebook_t::replace(tree_t * src, tree_t * by) {

}

void notebook_t::close(tree_t * src) {

}

void notebook_t::remove(tree_t * src) {

}

void notebook_t::activate_client(window_t * x) {
	client_map_t::iterator i;
	if ((i = client_map.find(x)) != client_map.end()) {
		client_t * c = i->second;
		set_selected(c);
		back_buffer_is_valid = false;
	}
}

window_list_t notebook_t::get_windows() {
	window_list_t list;
	client_list_t::iterator i = _clients.begin();
	while (i != _clients.end()) {
		list.push_back((*i)->get_window());
		++i;
	}
	return list;
}

void notebook_t::remove_client(window_t * x) {
	client_map_t::iterator i;
	if ((i = client_map.find(x)) != client_map.end()) {
		remove_client(i->second);
	}
}

void notebook_t::remove_client(client_t * c) {
	if (c == _selected.front())
		select_next();
	_selected.remove(c);
	_clients.remove(c);
	client_map.erase(&(c->w));
	delete c;
	render();
	page.add_damage_area(_allocation);
}

void notebook_t::select_next() {
	if (!_selected.empty()) {
		_selected.pop_front();
		if (!_selected.empty()) {
			client_t * c = _selected.front();
			update_client_position(c);
			c->map();
			c->focus();
			c->write_wm_state(NormalState);
			page.update_focus(c->get_window());
		}
	}
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

void notebook_t::set_selected(client_t * c) {
	assert(std::find(_clients.begin(), _clients.end(), c) != _clients.end());
	c->map();
	c->focus();
	c->write_wm_state(NormalState);
	page.update_focus(c->get_window());

	if (!_selected.empty()) {
		if (c != _selected.front()) {
			client_t * x = _selected.front();
			x->unmap();
			x->write_wm_state(IconicState);
		}
	}

	_selected.remove(c);
	_selected.push_front(c);

}

void notebook_t::update_popup_position(popup_notebook0_t * p, int x, int y,
		int w, int h, bool show_popup) {
	box_int_t old_area = p->get_absolute_extend();
	p->reconfigure(box_int_t(x, y, w, h));
	if (show_popup) {
		page.repair_back_buffer(old_area);
		page.repair_overlay(old_area);
		page.repair_back_buffer(p->area);
		page.repair_overlay(p->area);
	}
}

void notebook_t::process_drag_and_drop(client_t * c, int start_x, int start_y) {
	XEvent ev;
	notebook_t * ns = 0;
	select_e zone = SELECT_NONE;

	cursor = XCreateFontCursor(page.cnx.dpy, XC_fleur);

	popup_notebook0_t * p = new popup_notebook0_t(tab_area.x, tab_area.y,
			tab_area.w, tab_area.h);

	std::string name = c->get_name();
	popup_notebook1_t * p1 = new popup_notebook1_t(_allocation.x, _allocation.y,
			font, c->icon_surf, name);

	page.popups.push_back(p);
	page.popups.push_back(p1);

	bool popup_is_added = false;

	if (XGrabPointer(page.cnx.dpy, page.cnx.xroot, False,
			(ButtonPressMask | ButtonReleaseMask | PointerMotionMask),
			GrabModeAsync, GrabModeAsync, None, cursor,
			CurrentTime) != GrabSuccess
			)
		return;
	do {

		XIfEvent(page.cnx.dpy, &ev, drag_and_drop_filter, (char *) this);

		if (ev.type == page.cnx.damage_event + XDamageNotify) {
			page.process_event(reinterpret_cast<XDamageNotifyEvent&>(ev));
		} else if (ev.type == MotionNotify) {
			if (ev.xmotion.window == page.cnx.xroot) {

				if (ev.xmotion.x < start_x - 5 || ev.xmotion.x > start_x + 5
						|| ev.xmotion.y < start_y - 5
						|| ev.xmotion.y > start_y + 5
						|| !tab_area.is_inside(ev.xmotion.x, ev.xmotion.y)) {
					if (!popup_is_added) {
						page.popups.push_back(p);
						page.popups.push_back(p1);
						popup_is_added = true;
					}
				}

				if (popup_is_added) {
					box_int_t old_area = p1->get_absolute_extend();
					p1->reconfigure(
							box_int_t(ev.xmotion.x + 10, ev.xmotion.y, 0, 0));
					page.repair_back_buffer(old_area);
					page.repair_overlay(old_area);
					page.repair_back_buffer(p1->area);
					page.repair_overlay(p1->area);
				}

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
							update_popup_position(p, (*i)->tab_area.x,
									(*i)->tab_area.y, (*i)->tab_area.w,
									(*i)->tab_area.h, popup_is_added);
						}
					} else if ((*i)->right_area.is_inside(ev.xmotion.x_root,
							ev.xmotion.y_root)) {
						//printf("right\n");
						if (zone != SELECT_RIGHT || ns != (*i)) {
							zone = SELECT_RIGHT;
							ns = (*i);
							update_popup_position(p, (*i)->right_area.x,
									(*i)->right_area.y, (*i)->right_area.w,
									(*i)->right_area.h, popup_is_added);
						}
					} else if ((*i)->top_area.is_inside(ev.xmotion.x_root,
							ev.xmotion.y_root)) {
						//printf("top\n");
						if (zone != SELECT_TOP || ns != (*i)) {
							zone = SELECT_TOP;
							ns = (*i);
							update_popup_position(p, (*i)->top_area.x,
									(*i)->top_area.y, (*i)->top_area.w,
									(*i)->top_area.h, popup_is_added);
						}
					} else if ((*i)->bottom_area.is_inside(ev.xmotion.x_root,
							ev.xmotion.y_root)) {
						//printf("bottom\n");
						if (zone != SELECT_BOTTOM || ns != (*i)) {
							zone = SELECT_BOTTOM;
							ns = (*i);
							update_popup_position(p, (*i)->bottom_area.x,
									(*i)->bottom_area.y, (*i)->bottom_area.w,
									(*i)->bottom_area.h, popup_is_added);
						}
					} else if ((*i)->left_area.is_inside(ev.xmotion.x_root,
							ev.xmotion.y_root)) {
						//printf("left\n");
						if (zone != SELECT_LEFT || ns != (*i)) {
							zone = SELECT_LEFT;
							ns = (*i);
							update_popup_position(p, (*i)->left_area.x,
									(*i)->left_area.y, (*i)->left_area.w,
									(*i)->left_area.h, popup_is_added);
						}
					}
					++i;
				}
			}
		}
	} while (ev.type != ButtonRelease && ev.type != ButtonPress);
	page.popups.remove(p);
	page.popups.remove(p1);
	delete p;
	delete p1;
	/* ev is button release
	 * so set the hidden focus parameter
	 */
	page.cnx.last_know_time = ev.xbutton.time;

	XUngrabPointer(page.cnx.dpy, CurrentTime);
	XFreeCursor(page.cnx.dpy, cursor);

	if (zone == SELECT_TAB && ns != 0 && ns != this) {
		ns->add_client(c->get_window());
		remove_client(c);
	} else if (zone == SELECT_TOP && ns != 0) {
		ns->split_top(c);
		remove_client(c);
	} else if (zone == SELECT_LEFT && ns != 0) {
		ns->split_left(c);
		remove_client(c);
	} else if (zone == SELECT_BOTTOM && ns != 0) {
		ns->split_bottom(c);
		remove_client(c);
	} else if (zone == SELECT_RIGHT && ns != 0) {
		ns->split_right(c);
		remove_client(c);
	} else {
		set_selected(c);
	}

	if (_clients.empty() && _parent != 0) {
		/* self destruct */
		_parent->remove(this);
	} else {
		render();
		page.add_damage_area(_allocation);
		page.render_flush();
	}
}

Bool notebook_t::drag_and_drop_filter(Display * dpy, XEvent * ev, char * arg) {
	notebook_t * ths = reinterpret_cast<notebook_t *>(arg);
	return (ev->type == ConfigureRequest) || (ev->type == Expose)
			|| (ev->type == MotionNotify) || (ev->type == ButtonRelease)
			|| (ev->type == ButtonPress)
			|| (ev->type == ths->page.cnx.damage_event + XDamageNotify);
}

void notebook_t::update_client_position(client_t * c) {
	c->update_size(_allocation.w - 2 * BORDER_SIZE,
			_allocation.h - HEIGHT - 2 * BORDER_SIZE);

	box_int_t client_size;
	client_size.x = (client_area.w - c->width) / 2;
	client_size.y = (client_area.h - c->height) / 2;
	client_size.w = c->width;
	client_size.h = c->height;

	if (client_size.x < 0)
		client_size.x = 0;
	if (client_size.y < 0)
		client_size.y = 0;

	client_size.x += client_area.x;
	client_size.y += client_area.y;

	/* clip client size */
	client_size = client_area & client_size;

	if (page.fullscreen_client == 0) {
//		XMoveResizeWindow(c->cnx.dpy, c->xwin, offset_x, offset_y, c->width,
//				c->height);
//		printf("XMoveResizeWindow(%p, #%lu, %d, %d, %d, %d)\n", c->cnx.dpy,
//				c->xwin, offset_x, offset_y, c->width, c->height);

		/* ICCCM mandate that client must be prepared to accept any size */
//		XMoveResizeWindow(c->cnx.dpy, c->xwin, client_size.x, client_size.y,
//				client_size.w, client_size.h);
		c->reconfigure(client_size);

//		printf("XMoveResizeWindow(%p, #%lu, %d, %d, %d, %d)\n", c->cnx.dpy,
//				c->clipping_window, _allocation.x + BORDER_SIZE,
//				_allocation.y + HEIGHT, _allocation.w - 2 * BORDER_SIZE,
//				_allocation.h - HEIGHT - BORDER_SIZE);

	}
}

void notebook_t::iconify_client(window_t * x) {
	client_map_t::iterator i;
	if ((i = client_map.find(x)) != client_map.end()) {
		client_t * c = i->second;

		if (!_selected.empty()) {
			if (_selected.front() == c) {
				_selected.pop_front();
				if (!_selected.empty()) {
					set_selected(_selected.front());
				}
			}
		}

		back_buffer_is_valid = false;
	}

}

void notebook_t::delete_all() {

}

}
