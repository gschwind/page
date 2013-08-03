/*
 * minimal_theme.cxx
 *
 *  Created on: 24 mars 2013
 *      Author: gschwind
 */

#include "minimal_theme.hxx"
#include "box.hxx"
#include "color.hxx"
#include <string>
#include <algorithm>

using namespace std;

namespace page {

minimal_theme_layout_t::minimal_theme_layout_t() {

		notebook_margin.top = 24;
		notebook_margin.bottom = 4;
		notebook_margin.left = 4;
		notebook_margin.right = 4;

		split_margin.top = 0;
		split_margin.bottom = 0;
		split_margin.left = 0;
		split_margin.right = 0;

		floating_margin.top = 24;
		floating_margin.bottom = 4;
		floating_margin.left = 4;
		floating_margin.right = 4;

		split_width = 8;

}

list<box_int_t> minimal_theme_layout_t::compute_client_tab(box_int_t const & allocation,
		int number_of_client, int selected_client_index) const {
	list<box_int_t> result;

	if (number_of_client != 0) {
		double box_width = ((allocation.w - 17.0 * 5.0)
				/ (number_of_client + 1.0));
		double offset = allocation.x;

		box_t<int> b;

		for (int i = 0; i < number_of_client; ++i) {
			if (i == selected_client_index) {
				b = box_int_t(floor(offset), allocation.y,
						floor(offset + 2.0 * box_width) - floor(offset), notebook_margin.top - 4);
				result.push_back(b);
				offset += box_width * 2;
			} else {
				b = box_int_t(floor(offset), allocation.y, floor(offset + box_width) - floor(offset),
						notebook_margin.top - 4);
				result.push_back(b);
				offset += box_width;
			}
		}
	}
	return result;
}



box_int_t minimal_theme_layout_t::compute_notebook_close_window_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {

	if (number_of_client != 0) {
		double box_width = ((allocation.w - 17.0 * 5.0)
				/ (number_of_client + 1.0));
		box_t<int> b;

		b.x = allocation.x + box_width * (selected_client_index + 2) - 1 * 17 - 3;
		b.y = allocation.y + 2;
		b.w = 16;
		b.h = 16;

		return b;

	} else {
		return box_int_t(-1, -1, 0, 0);
	}
}

box_int_t minimal_theme_layout_t::compute_notebook_unbind_window_position(box_int_t const & allocation, int number_of_client, int selected_client_index) const {

	if (number_of_client != 0) {
		double box_width = ((allocation.w - 17.0 * 5.0)
				/ (number_of_client + 1.0));
		box_t<int> b;

		b.x = allocation.x + box_width * (selected_client_index + 2) - 2 * 17 - 3;
		b.y = allocation.y + 2;
		b.w = 16;
		b.h = 16;

		return b;

	} else {
		return box_int_t(-1, -1, 0, 0);
	}

}

box_int_t minimal_theme_layout_t::compute_notebook_bookmark_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {
	return box_int_t(allocation.x + allocation.w - 4 * 17 - 2, allocation.y + 2,
			16, 16);
}

box_int_t minimal_theme_layout_t::compute_notebook_vsplit_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {

	return box_int_t(allocation.x + allocation.w - 3 * 17 - 2, allocation.y + 2,
			16, 16);
}

box_int_t minimal_theme_layout_t::compute_notebook_hsplit_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {

	return box_int_t(allocation.x + allocation.w - 2 * 17 - 2, allocation.y + 2,
			16, 16);

}

box_int_t minimal_theme_layout_t::compute_notebook_close_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {

	return box_int_t(allocation.x + allocation.w - 1 * 17 - 2, allocation.y + 2,
			16, 16);
}

box_int_t minimal_theme_layout_t::compute_floating_close_position(box_int_t const & _allocation) const {
	return box_int_t(_allocation.x + _allocation.w - 1 * 17 - 2, _allocation.y + 2, 16, 16);
}

box_int_t minimal_theme_layout_t::compute_floating_bind_position(box_int_t const & _allocation) const {
	return box_int_t(_allocation.x + _allocation.w - 2 * 17 - 2, _allocation.y + 2, 16, 16);
}



/**********************************************************************************/

minimal_theme_t::minimal_theme_t(std::string conf_img_dir, std::string font,
		std::string font_bold) {

	layout = new minimal_theme_layout_t();

	ft_is_loaded = false;

	/* load fonts */
	if (!ft_is_loaded) {
		FT_Error error = FT_Init_FreeType(&library);
		if (error) {
			throw std::runtime_error("unable to init freetype");
		}

		printf("Load: %s\n", font.c_str());

		error = FT_New_Face(library, font.c_str(), 0, &face);

		if (error != FT_Err_Ok)
			throw std::runtime_error("unable to load default font");

		this->font = cairo_ft_font_face_create_for_ft_face(face, 0);
		if (!this->font)
			throw std::runtime_error("unable to load default font");

		ft_is_loaded = true;
	}
}

void minimal_theme_t::rounded_rectangle(cairo_t * cr, double x, double y,
		double w, double h, double r) {
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

void minimal_theme_t::render_notebook(cairo_t * cr, notebook_t * n,
		bool is_default) {

	cairo_identity_matrix(cr);
	cairo_reset_clip(cr);
	cairo_set_line_width(cr, 1.0);

	cairo_rectangle(cr, n->_allocation.x, n->_allocation.y, n->_allocation.w, n->_allocation.h);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_fill(cr);

	int number_of_client = n->_clients.size();
	int selected_index = -1;

	if(!n->_selected.empty()) {
		list<managed_window_t *>::iterator i = find(n->_clients.begin(), n->_clients.end(), n->_selected.front());
		selected_index = distance(n->_clients.begin(), i);
	}

	list<box_int_t> tabs = layout->compute_client_tab(n->_allocation, number_of_client, selected_index);

	list<managed_window_t *>::iterator c = n->_clients.begin();
	for(list<box_int_t>::iterator i = tabs.begin(); i != tabs.end(); ++i, ++c) {
		box_int_t & b = *i;

		if (*c == n->_selected.front()) {

			color_t c0(0xeeeeecU);
			color_t c1(0xd3d7cfU);

			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_set_source_rgb(cr, c0.r, c0.g, c0.b);
			cairo_fill(cr);

			cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
			cairo_set_source_rgb(cr, c1.r, c1.g, c1.b);
			cairo_stroke(cr);

		} else {
			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
			cairo_fill(cr);

			cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
			cairo_set_source_rgb(cr, 0.5, 0.0, 0.0);
			cairo_stroke(cr);
		}

		box_int_t bicon = b;
		bicon.h = 16;
		bicon.w = 16;
		bicon.x += 2;
		bicon.y += 2;

		if ((*c)->get_icon() != 0) {
			cairo_rectangle(cr, bicon.x, bicon.y, bicon.w, bicon.h);
			cairo_set_source_surface(cr, (*c)->get_icon()->get_cairo_surface(),
					bicon.x, bicon.y);
			cairo_paint(cr);
		}

		box_int_t btext = b;
		btext.h -= 4;
		btext.w -= 3 * 16 - 8;
		btext.x += 2 + 16 + 2;
		btext.y += 2;

		cairo_rectangle(cr, btext.x, btext.y, btext.w, btext.h);
		cairo_clip(cr);

		cairo_move_to(cr, btext.x, btext.y + btext.h - 3);
		cairo_set_font_size(cr, 13);
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_show_text(cr, (*c)->get_title().c_str());

		cairo_reset_clip(cr);

	}

	{
		box_int_t b = layout->compute_notebook_close_window_position(n->_allocation,
				number_of_client, selected_index);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.5, 0.5, 0.0);
		cairo_stroke(cr);
	}

	{
		box_int_t b = layout->compute_notebook_unbind_window_position(n->_allocation,
				number_of_client, selected_index);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.5, 0.5, 0.0);
		cairo_stroke(cr);

	}

	{
		box_int_t b = layout->compute_notebook_bookmark_position(n->_allocation,
				number_of_client, selected_index);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.0, 0.5, 0.0);
		cairo_stroke(cr);

	}

	{
		box_int_t b = layout->compute_notebook_vsplit_position(n->_allocation,
				number_of_client, selected_index);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.0, 0.5, 0.0);
		cairo_stroke(cr);

	}

	{
		box_int_t b = layout->compute_notebook_hsplit_position(n->_allocation,
				number_of_client, selected_index);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.0, 0.5, 0.0);
		cairo_stroke(cr);

	}

	{
		box_int_t b = layout->compute_notebook_close_position(n->_allocation,
				number_of_client, selected_index);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.0, 0.5, 0.0);
		cairo_stroke(cr);

	}

	if (!n->_selected.empty()) {

		box_int_t area = n->_allocation;
		area.y += layout->notebook_margin.top - 4;
		area.h -= layout->notebook_margin.top - 4;

		box_int_t bclient = n->_selected.front()->get_base_position();

		{
			/* left */

			box_int_t b(
					area.x,
					area.y,
					bclient.x - area.x,
					area.h);

			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_set_source_rgb(cr, 1.0, 0.5, 0.0);
			cairo_fill(cr);

			cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
			cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
			cairo_stroke(cr);

		}

		{
			/* right */

			box_int_t b(
					bclient.x + bclient.w,
					area.y,
					area.x + area.w - bclient.x - bclient.w,
					area.h);

			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_set_source_rgb(cr, 1.0, 0.5, 0.0);
			cairo_fill(cr);

			cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
			cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
			cairo_stroke(cr);

		}

		{
			/* top */

			box_int_t b(
					bclient.x,
					area.y,
					bclient.w,
					bclient.y - area.y);

			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_set_source_rgb(cr, 1.0, 0.5, 0.0);
			cairo_fill(cr);

			cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
			cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
			cairo_stroke(cr);

		}

		{
			/* bottom */

			box_int_t b(
					bclient.x,
					bclient.y + bclient.h,
					bclient.w,
					area.y + area.h - bclient.y - bclient.h);

			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_set_source_rgb(cr, 1.0, 0.5, 0.0);
			cairo_fill(cr);

			cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
			cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
			cairo_stroke(cr);

		}

	}

}

void minimal_theme_t::render_split(cairo_t * cr, split_t * s) {
	box_int_t area = s->get_split_bar_area();
	cairo_reset_clip(cr);
	cairo_save(cr);
	cairo_set_source_rgb(cr, 1.0, 0.0, 1.0);
	cairo_rectangle(cr, area.x, area.y, area.w, area.h);
	cairo_fill(cr);

	cairo_set_source_rgb(cr, 0.5, 0.0, 0.5);
	cairo_rectangle(cr, area.x + 0.5, area.y + 0.5, area.w - 1.0, area.h - 1.0);
	cairo_stroke(cr);

	cairo_restore(cr);
}



void minimal_theme_t::render_floating(managed_window_t * mw) {

	cairo_t * cr = mw->get_cairo();
	box_int_t _allocation = mw->get_base_position();

	_allocation.x = 0;
	_allocation.y = 0;

	cairo_set_line_width(cr, 1.0);

	cairo_rectangle(cr, _allocation.x, _allocation.y, _allocation.w, _allocation.h);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_fill(cr);

	{
		box_int_t b = _allocation;
		b.h = layout->floating_margin.top - 4;

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.5, 0.0, 0.0);
		cairo_stroke(cr);

		box_int_t bicon = b;
		bicon.h = 16;
		bicon.w = 16;
		bicon.x += 2;
		bicon.y += 2;

		if (mw->get_icon() != 0) {
			cairo_rectangle(cr, bicon.x, bicon.y, bicon.w, bicon.h);
			cairo_set_source_surface(cr, mw->get_icon()->get_cairo_surface(),
					bicon.x, bicon.y);
			cairo_paint(cr);
		}

		box_int_t btext = b;
		btext.h -= 4;
		btext.w -= 3 * 16 - 8;
		btext.x += 2 + 16 + 2;
		btext.y += 2;

		cairo_rectangle(cr, btext.x, btext.y, btext.w, btext.h);
		cairo_clip(cr);

		cairo_move_to(cr, btext.x, btext.y + btext.h - 3);
		cairo_set_font_size(cr, 13);
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_show_text(cr, mw->get_title().c_str());

		cairo_reset_clip(cr);

	}

	{
		box_int_t b = layout->compute_floating_close_position(_allocation);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.5, 0.5, 0.0);
		cairo_stroke(cr);
	}

	{
		box_int_t b = layout->compute_floating_bind_position(_allocation);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.5, 0.5, 0.0);
		cairo_stroke(cr);

	}

	{
		/* left */

		box_int_t b(_allocation.x,
				_allocation.y + layout->floating_margin.top - 4,
				layout->floating_margin.left,
				_allocation.h - layout->floating_margin.top + 4);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 1.0, 0.5, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.5, 0.25, 0.0);
		cairo_stroke(cr);

	}

	{
		/* right */

		box_int_t b(
				_allocation.x + _allocation.w
						- layout->floating_margin.right,
				_allocation.y + layout->floating_margin.top - 4,
				layout->floating_margin.left,
				_allocation.h - layout->floating_margin.top + 4);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 1.0, 0.5, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.5, 0.25, 0.0);
		cairo_stroke(cr);

	}

	{
		/* top */

		box_int_t b(_allocation.x + layout->floating_margin.left,
				_allocation.y + layout->floating_margin.top - 4,
				_allocation.w - layout->floating_margin.left
						- layout->floating_margin.right, 4);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 1.0, 0.5, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.5, 0.25, 0.0);
		cairo_stroke(cr);

	}

	{
		/* bottom */

		box_int_t b(_allocation.x + layout->floating_margin.left,
				_allocation.y + _allocation.h - layout->floating_margin.bottom,
				_allocation.w - layout->floating_margin.left
						- layout->floating_margin.right, 4);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, 1.0, 0.5, 0.0);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		cairo_set_source_rgb(cr, 0.5, 0.25, 0.0);
		cairo_stroke(cr);

	}


}

cairo_font_face_t * minimal_theme_t::get_default_font() {
	return font;
}

}

