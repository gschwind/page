/*
 * render_tree.cxx
 *
 *  Created on: 26 déc. 2012
 *      Author: gschwind
 */

#include "default_theme.hxx"
#include "box.hxx"
#include <string>

namespace page {

default_theme_layout_t::default_theme_layout_t() {

		notebook_margin.top = 28;
		notebook_margin.bottom = 2;
		notebook_margin.left = 2;
		notebook_margin.right = 2;

		split_margin.top = 0;
		split_margin.bottom = 0;
		split_margin.left = 0;
		split_margin.right = 0;

		floating_margin.top = 28;
		floating_margin.bottom = 4;
		floating_margin.left = 4;
		floating_margin.right = 4;

		split_width = 3;

}

list<box_int_t> default_theme_layout_t::compute_client_tab(box_int_t const & allocation,
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
						ceil(2.0 * box_width), notebook_margin.top);
				result.push_back(b);
				offset += box_width * 2;
			} else {
				b = box_int_t(floor(offset), allocation.y, ceil(box_width),
						notebook_margin.top);
				result.push_back(b);
				offset += box_width;
			}
		}
	}
	return result;
}



box_int_t default_theme_layout_t::compute_notebook_close_window_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {

	if (number_of_client != 0) {
		double box_width = ((allocation.w - 17.0 * 5.0)
				/ (number_of_client + 1.0));
		box_t<int> b;

		b.x = allocation.x + box_width * (selected_client_index + 2) - 16 - 3;
		b.y = allocation.y + 2;
		b.w = 16;
		b.h = 16;

		return b;

	} else {
		return box_int_t(-1, -1, 0, 0);
	}
}

box_int_t default_theme_layout_t::compute_notebook_unbind_window_position(box_int_t const & allocation, int number_of_client, int selected_client_index) const {

	if (number_of_client != 0) {
		double box_width = ((allocation.w - 17.0 * 5.0)
				/ (number_of_client + 1.0));
		box_t<int> b;

		b.x = allocation.x + box_width * (selected_client_index + 2) - 32 - 3;
		b.y = allocation.y + 2;
		b.w = 16;
		b.h = 16;

		return b;

	} else {
		return box_int_t(-1, -1, 0, 0);
	}

}

box_int_t default_theme_layout_t::compute_notebook_bookmark_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {
	return box_int_t(allocation.x + allocation.w - 4 * 16 - 1, allocation.y + 2,
			16, 16);
}

box_int_t default_theme_layout_t::compute_notebook_vsplit_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {

	return box_int_t(allocation.x + allocation.w - 3 * 16 - 1, allocation.y + 2,
			16, 16);
}

box_int_t default_theme_layout_t::compute_notebook_hsplit_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {

	return box_int_t(allocation.x + allocation.w - 2 * 16 - 1, allocation.y + 2,
			16, 16);

}

box_int_t default_theme_layout_t::compute_notebook_close_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {

	return box_int_t(allocation.x + allocation.w - 1 * 16 - 1, allocation.y + 2,
			16, 16);
}

box_int_t default_theme_layout_t::compute_floating_close_position(box_int_t const & _allocation) const {
	return box_int_t(_allocation.x + _allocation.w - 16 - 2, _allocation.y, 16, 16);
}

box_int_t default_theme_layout_t::compute_floating_bind_position(box_int_t const & _allocation) const {
	return box_int_t(_allocation.x + _allocation.w - 16 - 2, _allocation.y, 16, 16);
}



/**********************************************************************************/

default_theme_t::default_theme_t(std::string conf_img_dir, std::string font,
		std::string font_bold) {

	layout = new default_theme_layout_t();

	ft_is_loaded = false;

	vsplit_button_s = 0;
	hsplit_button_s = 0;
	close_button_s = 0;
	pop_button_s = 0;
	pops_button_s = 0;
	unbind_button_s = 0;
	bind_button_s = 0;

	pop_notebook = 0;

	/* open icons */
	if (hsplit_button_s == 0) {
		std::string filename = conf_img_dir + "/hsplit_button.png";
		printf("Load: %s\n", filename.c_str());
		hsplit_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (hsplit_button_s == 0)
			throw std::runtime_error("file not found!");
	}

	if (vsplit_button_s == 0) {
		std::string filename = conf_img_dir + "/vsplit_button.png";
		printf("Load: %s\n", filename.c_str());
		vsplit_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (vsplit_button_s == 0)
			throw std::runtime_error("file not found!");
	}

	if (close_button_s == 0) {
		std::string filename = conf_img_dir + "/window-close.png";
		printf("Load: %s\n", filename.c_str());
		close_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (close_button_s == 0)
			throw std::runtime_error("file not found!");
	}

	if (pop_button_s == 0) {
		std::string filename = conf_img_dir + "/pop.png";
		printf("Load: %s\n", filename.c_str());
		pop_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (pop_button_s == 0)
			throw std::runtime_error("file not found!");
	}

	if (pops_button_s == 0) {
		std::string filename = conf_img_dir + "/pop_selected.png";
		printf("Load: %s\n", filename.c_str());
		pops_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (pop_button_s == 0)
			throw std::runtime_error("file not found!");
	}

	if (unbind_button_s == 0) {
		std::string filename = conf_img_dir + "/media-eject.png";
		printf("Load: %s\n", filename.c_str());
		unbind_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (unbind_button_s == 0)
			throw std::runtime_error("file not found!");
	}

	if (bind_button_s == 0) {
		std::string filename = conf_img_dir + "/view-restore.png";
		printf("Load: %s\n", filename.c_str());
		bind_button_s = cairo_image_surface_create_from_png(filename.c_str());
		if (bind_button_s == 0)
			throw std::runtime_error("file not found!");
	}

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

		printf("Load: %s\n", font_bold.c_str());

		error = FT_New_Face(library, font_bold.c_str(), 0, &face_bold);

		if (error != FT_Err_Ok)
			throw std::runtime_error("unable to load default bold font");

		this->font_bold = cairo_ft_font_face_create_for_ft_face(face_bold, 0);
		if (!this->font_bold)
			throw std::runtime_error("unable to load default bold font");

		ft_is_loaded = true;
	}
}

void default_theme_t::rounded_rectangle(cairo_t * cr, double x, double y,
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

void default_theme_t::draw_unselected_tab(cairo_t * cr, managed_window_t * nw,
		box_int_t location) {
	cairo_save(cr);
	cairo_translate(cr, location.x, location.y);

	/* draw window icon */
	if (nw->get_icon() != 0) {
		cairo_save(cr);
		cairo_translate(cr, 3.0, 4.0);
		cairo_set_source_surface(cr, nw->get_icon()->get_cairo_surface(), 0.0,
				0.0);
		cairo_paint(cr);
		cairo_restore(cr);
	}

	cairo_set_line_width(cr, 1.0);
	cairo_set_font_face(cr, font);
	cairo_set_font_size(cr, 13);

	/* draw window name */
	cairo_rectangle(cr, 2, 0, location.w - 4, layout->floating_margin.top - 1);
	cairo_clip(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_font_size(cr, 13);
	cairo_move_to(cr, layout->floating_margin.top + 0.5, 15.5);
	cairo_show_text(cr, nw->get_title().c_str());

	/* draw border */
	cairo_reset_clip(cr);

	cairo_rectangle(cr, 0.0, 0.0, location.w + 1.0,
			layout->floating_margin.top);
	cairo_clip(cr);
	cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0, 0x85U / 255.0);
	rounded_rectangle(cr, 0.5, 2.5, location.w, layout->floating_margin.top,
			2.0);
	cairo_new_path(cr);
	cairo_move_to(cr, 0.0, layout->floating_margin.top);
	cairo_line_to(cr, location.w + 1.0, layout->floating_margin.top);
	cairo_stroke(cr);

//		if ((*i)->get_orig()->demands_atteniion()) {
//			cairo_new_path(cr);
//			cairo_move_to(cr, 0.0, 3.5);
//			cairo_line_to(cr, length * 2, 3.5);
//			cairo_set_source_rgb(cr, 0xFFU / 255.0,
//					0x00U / 255.0, 0x00U / 255.0);
//			cairo_stroke(cr);
//		}

	cairo_restore(cr);
}

void default_theme_t::draw_selected_tab(cairo_t * cr, managed_window_t * nw,
		box_int_t location) {
	cairo_save(cr);
	cairo_translate(cr, location.x, location.y);

	cairo_set_line_width(cr, 1.0);
	cairo_set_font_face(cr, font_bold);
	cairo_set_font_size(cr, 13);

	/* draw light background */
	cairo_rectangle(cr, 0.0, 3.0, location.w, layout->floating_margin.top);
	cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0, 0xecU / 255.0);
	cairo_fill(cr);

	if (nw->get_icon() != 0) {
		cairo_save(cr);
		cairo_translate(cr, 3.0, 4.0);
		cairo_set_source_surface(cr, nw->get_icon()->get_cairo_surface(), 0.0,
				0.0);
		cairo_paint(cr);
		cairo_restore(cr);
	}

	/* draw the name */
	cairo_rectangle(cr, 2.0, 0.0, location.w - 17.0,
			layout->floating_margin.top - 2.0);
	cairo_clip(cr);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_set_font_size(cr, 13.0);
	cairo_move_to(cr, 20.5, 15.5);

	cairo_show_text(cr, nw->get_title().c_str());

	/* draw blue lines */
	cairo_reset_clip(cr);

	cairo_new_path(cr);
	cairo_move_to(cr, 1.0, 1.5);
	cairo_line_to(cr, location.w, 1.5);
	cairo_move_to(cr, 1.0, 2.5);
	cairo_line_to(cr, location.w, 2.5);
	cairo_set_source_rgb(cr, 0x72U / 255.0, 0x9fU / 255.0, 0xcfU / 255.0);
	cairo_stroke(cr);
	cairo_new_path(cr);
	cairo_move_to(cr, 0.0, 3.5);
	cairo_line_to(cr, location.w, 3.5);
	cairo_set_source_rgb(cr, 0x34U / 255.0, 0x64U / 255.0, 0xa4U / 255.0);
	cairo_stroke(cr);

	cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0, 0x85U / 255.0);
	rounded_rectangle(cr, 0.5, 0.5, location.w,
			layout->floating_margin.top - 1.0, 3.0);

	/* draw close icon */
	cairo_set_line_width(cr, 2.0);
	cairo_translate(cr, location.w - 16.5, 3.5);
	/* draw close */
	cairo_new_path(cr);
	cairo_move_to(cr, 4.0, 4.0);
	cairo_line_to(cr, 12.0, 12.0);
	cairo_move_to(cr, 12.0, 4.0);
	cairo_line_to(cr, 4.0, 12.0);
	cairo_set_source_rgb(cr, 0xCCU / 255.0, 0x00U / 255.0, 0x00U / 255.0);
	cairo_stroke(cr);

	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
	cairo_set_source_surface(cr, close_button_s, 0.5, 0.5);
	cairo_paint(cr);

	/* draw undock button */
	cairo_translate(cr, -16.0, 0.0);
	cairo_rectangle(cr, 0.0, 0.0, 16.0, 16.0);
	cairo_set_source_rgb(cr, 0xCCU / 255.0, 0x00U / 255.0, 0x00U / 255.0);
	cairo_stroke(cr);

	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
	cairo_set_source_surface(cr, unbind_button_s, 0.5, 0.5);
	cairo_paint(cr);

	cairo_restore(cr);

}

void default_theme_t::render_notebook(cairo_t * cr, notebook_t * n,
		bool is_default) {

	cairo_save(cr);
	{

		//std::cout << "xxx: " << n->_allocation.to_string() << std::endl;

		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

		cairo_translate(cr, n->_allocation.x, n->_allocation.y);
		cairo_set_line_width(cr, 1.0);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

		/* create tabs back ground */
		cairo_rectangle(cr, 0.0, 0.0, n->_allocation.w,
				layout->notebook_margin.top - 2);
		cairo_pattern_t *pat;
		pat = cairo_pattern_create_linear(0.0, 0.0, 0.0,
				layout->notebook_margin.top);
		cairo_pattern_add_color_stop_rgba(pat, 0, 0xeeU / 255.0, 0xeeU / 255.0,
				0xecU / 255.0, 1);
		cairo_pattern_add_color_stop_rgba(pat, 1, 0xbaU / 255.0, 0xbdU / 255.0,
				0xd6U / 255.0, 1);
		cairo_set_source(cr, pat);
		cairo_fill(cr);
		cairo_pattern_destroy(pat);

		/* draw top line */
		cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0, 0x85U / 255.0);
		cairo_new_path(cr);
		cairo_move_to(cr, 0.0, layout->notebook_margin.top - 2 - 0.5);
		cairo_line_to(cr, n->_allocation.w,
				layout->notebook_margin.top - 2 - 0.5);

		cairo_stroke(cr);

		cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0, 0xecU / 255.0);
		cairo_rectangle(cr, 0.0, layout->notebook_margin.top - 2,
				n->_allocation.w,
				n->_allocation.h - layout->notebook_margin.top - 2);
		cairo_fill(cr);

		list<managed_window_t *>::iterator i;
		double offset = 0;
		double length = (n->_allocation.w - 17.0 * 5.0)
				/ (n->_clients.size() + 1.0);
		for (i = n->_clients.begin(); i != n->_clients.end(); ++i) {
			if (n->_selected.front() == (*i)) {
				draw_selected_tab(cr, *i,
						box_int_t(floor(offset), 0, length * 2 - 2,
								layout->notebook_margin.top - 2));
				offset += length * 2;
			} else {
				draw_unselected_tab(cr, *i,
						box_int_t(floor(offset), 0, length - 2,
								layout->notebook_margin.top - 2));
				offset += length;
			}
		}

		cairo_save(cr);
		{

			/* draw icon background */
			cairo_rectangle(cr, n->_allocation.w - 17.0 * 4, 0, 17.0 * 4,
					layout->notebook_margin.top - 2 - 1);
			cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0,
					0xecU / 255.0);
			cairo_fill(cr);

			cairo_translate(cr, n->_allocation.w - 16.5, 1.5);
			/* draw close */
			cairo_new_path(cr);
			cairo_set_source_surface(cr, close_button_s, 0.5, 0.5);
			cairo_paint(cr);

			/* draw vertical split */
			cairo_translate(cr, -17.0, 0.0);
			cairo_set_source_surface(cr, vsplit_button_s, 0.5, 0.5);
			cairo_paint(cr);

			/* draw horizontal split */
			cairo_translate(cr, -17.0, 0.0);
			cairo_set_source_surface(cr, hsplit_button_s, 0.5, 0.5);
			cairo_paint(cr);

			/* draw pop button */
			cairo_translate(cr, -17.0, 0.0);
			if (is_default) {
				cairo_set_source_surface(cr, pops_button_s, 0.5, 0.5);
				cairo_paint(cr);
			} else {
				cairo_set_source_surface(cr, pop_button_s, 0.5, 0.5);
				cairo_paint(cr);
			}

		}
		cairo_restore(cr);

	}
	cairo_restore(cr);

	cairo_save(cr);
	{

		cairo_translate(cr, n->_allocation.x, n->_allocation.y);

		cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

		/* draw border */
		if (n->_selected.empty()) {
			cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0,
					0xecU / 255.0);
			cairo_rectangle(cr, n->_allocation.x, n->_allocation.y,
					n->_allocation.w, n->_allocation.h);
			cairo_fill(cr);
		} else {
			managed_window_t * c = n->_selected.front();
			cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0,
					0xecU / 255.0);

			box_int_t size = c->get_base_position();
			/* left */
			cairo_rectangle(cr, n->_allocation.x,
					n->_allocation.y + layout->notebook_margin.top - 2,
					size.x - n->_allocation.x,
					n->_allocation.h - layout->notebook_margin.top - 2);
			cairo_fill(cr);
			/* right */
			cairo_rectangle(cr, size.x + size.w,
					n->_allocation.y + layout->notebook_margin.top - 2,
					n->_allocation.x + n->_allocation.w - size.x - size.w,
					n->_allocation.h - layout->notebook_margin.top - 2);
			cairo_fill(cr);

			/* top */
			cairo_rectangle(cr, size.x,
					n->_allocation.y + layout->notebook_margin.top - 2, size.w,
					size.y - n->_allocation.y - layout->notebook_margin.top
							- 2);

			/* bottom */
			cairo_fill(cr);
			cairo_rectangle(cr, size.x, size.y + size.h, size.w,
					n->_allocation.y + n->_allocation.h - size.y - size.h);
			cairo_fill(cr);

			cairo_set_line_width(cr, 1.0);
			cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0,
					0x85U / 255.0);
			cairo_rectangle(cr, size.x - 0.5, size.y - 0.5, size.w + 1.0,
					size.h + 1.0);
			cairo_stroke(cr);

		}

		/* draw top line */

//			cairo_translate(cr, n->_allocation.x, n->_allocation.y);
//			cairo_rectangle(cr, 0.0, 0.0, n->_allocation.w, HEIGHT);
//			cairo_set_source_surface(cr, s, 0.0, 0.0);
//			cairo_fill(cr);
		cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0, 0x85U / 255.0);
		//cairo_set_source_rgb(page.gui_cr, 1.0, 0.0, 0.0);
		cairo_set_line_width(cr, 1.0);
		cairo_new_path(cr);
		cairo_move_to(cr, 0.5, 0.5);
		cairo_line_to(cr, n->_allocation.w - 0.5, 0.5);
		cairo_line_to(cr, n->_allocation.w - 0.5, n->_allocation.h - 0.5);
		cairo_line_to(cr, 0.5, n->_allocation.h - 0.5);
		cairo_line_to(cr, 0.5, 0.5);
		cairo_stroke(cr);

	}
	cairo_restore(cr);

	if (false) {
		/* debug zone */
		cairo_identity_matrix(cr);
		cairo_reset_clip(cr);

		cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
		cairo_rectangle(cr, n->_allocation.x, n->_allocation.y,
				n->_allocation.w, n->_allocation.h);
		cairo_stroke(cr);

		cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
		cairo_rectangle(cr, n->tab_area.x, n->tab_area.y, n->tab_area.w,
				n->tab_area.h);
		cairo_stroke(cr);

		cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
		cairo_set_line_width(cr, 1.0);
		cairo_set_font_face(cr, font_bold);
		cairo_set_font_size(cr, 16);
		std::stringstream s;
		s << n->_allocation.to_string();
		cairo_move_to(cr, n->_allocation.x, n->_allocation.y + 20.0);
		cairo_show_text(cr, s.str().c_str());
	}
}

void default_theme_t::render_split(cairo_t * cr, split_t * s) {
	cairo_save(cr);
	cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0, 0xecU / 255.0);
	//cairo_set_source_rgb(cr, 0xeeU / 255.0, 0x00U / 255.0, 0x00U / 255.0);
	box_int_t area = s->get_split_bar_area();
	cairo_rectangle(cr, area.x, area.y, area.w, area.h);
	cairo_fill(cr);
	cairo_restore(cr);
}

void default_theme_t::render_notebook(cairo_t * cr, notebook_t * n) {
	bool is_default = false;

	cairo_save(cr);
	{

		//std::cout << "xxx: " << n->_allocation.to_string() << std::endl;

		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

		cairo_translate(cr, n->_allocation.x, n->_allocation.y);
		cairo_set_line_width(cr, 1.0);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

		/* create tabs back ground */
		cairo_rectangle(cr, 0.0, 0.0, n->_allocation.w,
				layout->floating_margin.top);
		cairo_pattern_t *pat;
		pat = cairo_pattern_create_linear(0.0, 0.0, 0.0,
				layout->floating_margin.top);
		cairo_pattern_add_color_stop_rgba(pat, 0, 0xeeU / 255.0, 0xeeU / 255.0,
				0xecU / 255.0, 1);
		cairo_pattern_add_color_stop_rgba(pat, 1, 0xbaU / 255.0, 0xbdU / 255.0,
				0xd6U / 255.0, 1);
		cairo_set_source(cr, pat);
		cairo_fill(cr);
		cairo_pattern_destroy(pat);

		/* draw top line */
		cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0, 0x85U / 255.0);
		cairo_new_path(cr);
		cairo_move_to(cr, 0.0, layout->floating_margin.top - 0.5);
		cairo_line_to(cr, n->_allocation.w, layout->floating_margin.top - 0.5);

		cairo_stroke(cr);

		cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0, 0xecU / 255.0);
		cairo_rectangle(cr, 0.0, layout->floating_margin.top, n->_allocation.w,
				n->_allocation.h - layout->floating_margin.top);
		cairo_fill(cr);

		list<managed_window_t *>::iterator i;
		double offset = 0;
		double length = (n->_allocation.w - 17.0 * 5.0)
				/ (n->_clients.size() + 1.0);
		for (i = n->_clients.begin(); i != n->_clients.end(); ++i) {
			if (n->_selected.front() == (*i)) {
				draw_selected_tab(cr, *i,
						box_int_t(floor(offset), 0, length * 2 - 2,
								layout->floating_margin.top));
				offset += length * 2;
			} else {
				draw_unselected_tab(cr, *i,
						box_int_t(floor(offset), 0, length - 2,
								layout->floating_margin.top));
				offset += length;
			}
		}

		cairo_save(cr);
		{

			/* draw icon background */
			cairo_rectangle(cr, n->_allocation.w - 17.0 * 4, 0, 17.0 * 4,
					layout->floating_margin.top - 1);
			cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0,
					0xecU / 255.0);
			cairo_fill(cr);

			cairo_translate(cr, n->_allocation.w - 16.5, 1.5);
			/* draw close */
			cairo_new_path(cr);
			cairo_set_source_surface(cr, close_button_s, 0.5, 0.5);
			cairo_paint(cr);

			/* draw vertical split */
			cairo_translate(cr, -17.0, 0.0);
			cairo_set_source_surface(cr, vsplit_button_s, 0.5, 0.5);
			cairo_paint(cr);

			/* draw horizontal split */
			cairo_translate(cr, -17.0, 0.0);
			cairo_set_source_surface(cr, hsplit_button_s, 0.5, 0.5);
			cairo_paint(cr);

			/* draw pop button */
			cairo_translate(cr, -17.0, 0.0);
			if (is_default) {
				cairo_set_source_surface(cr, pops_button_s, 0.5, 0.5);
				cairo_paint(cr);
			} else {
				cairo_set_source_surface(cr, pop_button_s, 0.5, 0.5);
				cairo_paint(cr);
			}

		}
		cairo_restore(cr);

	}
	cairo_restore(cr);

	cairo_save(cr);
	{

		cairo_translate(cr, n->_allocation.x, n->_allocation.y);

		cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

		/* draw border */
		if (n->_selected.empty()) {
			cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0,
					0xecU / 255.0);
			cairo_rectangle(cr, n->_allocation.x, n->_allocation.y,
					n->_allocation.w, n->_allocation.h);
			cairo_fill(cr);
		} else {
			managed_window_t * c = n->_selected.front();
			cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0,
					0xecU / 255.0);

			box_int_t size = c->get_base_position();
			/* left */
			cairo_rectangle(cr, n->_allocation.x,
					n->_allocation.y + layout->floating_margin.top,
					size.x - n->_allocation.x,
					n->_allocation.h - layout->floating_margin.top);
			cairo_fill(cr);
			/* right */
			cairo_rectangle(cr, size.x + size.w,
					n->_allocation.y + layout->floating_margin.top,
					n->_allocation.x + n->_allocation.w - size.x - size.w,
					n->_allocation.h - layout->floating_margin.top);
			cairo_fill(cr);

			/* top */
			cairo_rectangle(cr, size.x,
					n->_allocation.y + layout->floating_margin.top, size.w,
					size.y - n->_allocation.y - layout->floating_margin.top);

			/* bottom */
			cairo_fill(cr);
			cairo_rectangle(cr, size.x, size.y + size.h, size.w,
					n->_allocation.y + n->_allocation.h - size.y - size.h);
			cairo_fill(cr);

			cairo_set_line_width(cr, 1.0);
			cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0,
					0x85U / 255.0);
			cairo_rectangle(cr, size.x - 0.5, size.y - 0.5, size.w + 1.0,
					size.h + 1.0);
			cairo_stroke(cr);

		}

		/* draw top line */

		//			cairo_translate(cr, n->_allocation.x, n->_allocation.y);
		//			cairo_rectangle(cr, 0.0, 0.0, n->_allocation.w, HEIGHT);
		//			cairo_set_source_surface(cr, s, 0.0, 0.0);
		//			cairo_fill(cr);
		cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0, 0x85U / 255.0);
		//cairo_set_source_rgb(page.gui_cr, 1.0, 0.0, 0.0);
		cairo_set_line_width(cr, 1.0);
		cairo_new_path(cr);
		cairo_move_to(cr, 0.5, 0.5);
		cairo_line_to(cr, n->_allocation.w - 0.5, 0.5);
		cairo_line_to(cr, n->_allocation.w - 0.5, n->_allocation.h - 0.5);
		cairo_line_to(cr, 0.5, n->_allocation.h - 0.5);
		cairo_line_to(cr, 0.5, 0.5);
		cairo_stroke(cr);

	}
	cairo_restore(cr);

	if (false) {
		/* debug zone */
		cairo_identity_matrix(cr);
		cairo_reset_clip(cr);

		cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
		cairo_rectangle(cr, n->_allocation.x, n->_allocation.y,
				n->_allocation.w, n->_allocation.h);
		cairo_stroke(cr);

		cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
		cairo_rectangle(cr, n->tab_area.x, n->tab_area.y, n->tab_area.w,
				n->tab_area.h);
		cairo_stroke(cr);

		cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
		cairo_set_line_width(cr, 1.0);
		cairo_set_font_face(cr, font_bold);
		cairo_set_font_size(cr, 16);
		std::stringstream s;
		s << n->_allocation.to_string();
		cairo_move_to(cr, n->_allocation.x, n->_allocation.y + 20.0);
		cairo_show_text(cr, s.str().c_str());
	}
}

void default_theme_t::render_floating(managed_window_t * mw) {

	cairo_t * cr = mw->get_cairo();
	box_int_t _allocation = mw->get_base_position();

	_allocation.x = 0;
	_allocation.y = 0;

	cairo_save(cr);
	{
		//cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

		//std::cout << "xxx: " << n->_allocation.to_string() << std::endl;
		cairo_translate(cr, _allocation.x, _allocation.y);
		cairo_set_line_width(cr, 1.0);
		cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

		/* draw top line */
		cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0, 0x85U / 255.0);
		cairo_new_path(cr);
		cairo_move_to(cr, 0.0, layout->floating_margin.top - 0.5);
		cairo_line_to(cr, _allocation.w, layout->floating_margin.top - 0.5);

		cairo_stroke(cr);

		cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0, 0xecU / 255.0);
		cairo_rectangle(cr, 0.0, layout->floating_margin.top, _allocation.w,
				_allocation.h - layout->floating_margin.top);
		cairo_fill(cr);

		double length = _allocation.w;

		cairo_save(cr);
		{

			cairo_translate(cr, 0.0, 0.0);

			cairo_set_line_width(cr, 1.0);
			cairo_set_font_face(cr, font_bold);
			cairo_set_font_size(cr, 13);

			/* draw light background */
			cairo_rectangle(cr, 0.0, 3.0, length,
					layout->floating_margin.top - 2);
			cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0,
					0xecU / 255.0);
			cairo_fill(cr);

//				if (n->get_icon_surface((*i)) != 0) {
//					cairo_save(cr);
//					cairo_translate(cr, 3.0, 4.0);
//					cairo_set_source_surface(cr, n->get_icon_surface((*i)), 0.0,
//							0.0);
//					cairo_paint(cr);
//					cairo_restore(cr);
//				}

			/* draw the name */
			cairo_rectangle(cr, 2.0, 0.0, length - 17.0,
					layout->floating_margin.top - 2.0);
			cairo_clip(cr);
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_set_font_size(cr, 13.0);
			cairo_move_to(cr, 20.5, 15.5);

			cairo_show_text(cr, mw->get_title().c_str());

			/* draw blue lines */
			cairo_reset_clip(cr);

			cairo_new_path(cr);
			cairo_move_to(cr, 0.0, 1.5);
			cairo_line_to(cr, length, 1.5);
			cairo_move_to(cr, 0.0, 2.5);
			cairo_line_to(cr, length, 2.5);
			cairo_set_source_rgb(cr, 0x72U / 255.0, 0x9fU / 255.0,
					0xcfU / 255.0);
			cairo_stroke(cr);
			cairo_new_path(cr);
			cairo_move_to(cr, 0.0, 3.5);
			cairo_line_to(cr, length, 3.5);
			cairo_set_source_rgb(cr, 0x34U / 255.0, 0x64U / 255.0,
					0xa4U / 255.0);
			cairo_stroke(cr);

			cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0,
					0x85U / 255.0);

			/* draw close icon */
			cairo_set_line_width(cr, 2.0);
			cairo_translate(cr, length - 16.5, 3.5);

			/* draw close (an ugly vectorial fallback) */
			cairo_new_path(cr);
			cairo_move_to(cr, 4.0, 4.0);
			cairo_line_to(cr, 12.0, 12.0);
			cairo_move_to(cr, 12.0, 4.0);
			cairo_line_to(cr, 4.0, 12.0);
			cairo_set_source_rgb(cr, 0xCCU / 255.0, 0x00U / 255.0,
					0x00U / 255.0);
			cairo_stroke(cr);

			/* draw nice close */
			cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
			cairo_set_source_surface(cr, close_button_s, 0.5, 0.5);
			cairo_paint(cr);

			/* draw dock button */
			cairo_translate(cr, -17.0, 0.0);
			cairo_rectangle(cr, 0.0, 0.0, 16.0, 16.0);
			cairo_set_source_rgb(cr, 0xCCU / 255.0, 0x00U / 255.0,
					0x00U / 255.0);
			cairo_stroke(cr);

			cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
			cairo_set_source_surface(cr, bind_button_s, 0.5, 0.5);
			cairo_paint(cr);

		}
		cairo_restore(cr);

	}
	cairo_restore(cr);

	cairo_save(cr);
	{

		cairo_translate(cr, _allocation.x, _allocation.y);

		cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

		cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0, 0xecU / 255.0);

		box_int_t size(_allocation);
		margin_t margin = layout->floating_margin;
		size.x += margin.left;
		size.y += margin.top;
		size.w -= (margin.left + margin.right);
		size.h -= (margin.top + margin.bottom);

		/* left */
		cairo_rectangle(cr, _allocation.x,
				_allocation.y + layout->floating_margin.top,
				size.x - _allocation.x,
				_allocation.h - layout->floating_margin.top);
		cairo_fill(cr);
		/* right */
		cairo_rectangle(cr, size.x + size.w,
				_allocation.y + layout->floating_margin.top,
				_allocation.x + _allocation.w - size.x - size.w,
				_allocation.h - layout->floating_margin.top);
		cairo_fill(cr);

		/* top */
		cairo_rectangle(cr, size.x, _allocation.y + layout->floating_margin.top,
				size.w, size.y - _allocation.y - layout->floating_margin.top);

		/* bottom */
		cairo_fill(cr);
		cairo_rectangle(cr, size.x, size.y + size.h, size.w,
				_allocation.y + _allocation.h - size.y - size.h);
		cairo_fill(cr);

		cairo_set_line_width(cr, 1.0);
		cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0, 0x85U / 255.0);
		cairo_rectangle(cr, size.x - 0.5, size.y - 0.5, size.w + 1.0,
				size.h + 1.0);
		cairo_stroke(cr);

//			cairo_translate(cr, n->_allocation.x, n->_allocation.y);
//			cairo_rectangle(cr, 0.0, 0.0, n->_allocation.w, HEIGHT);
//			cairo_set_source_surface(cr, s, 0.0, 0.0);
//			cairo_fill(cr);
		cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0, 0x85U / 255.0);
		//cairo_set_source_rgb(page.gui_cr, 1.0, 0.0, 0.0);
		cairo_set_line_width(cr, 1.0);
		cairo_new_path(cr);
		cairo_move_to(cr, 0.5, 0.5);
		cairo_line_to(cr, _allocation.w - 0.5, 0.5);
		cairo_line_to(cr, _allocation.w - 0.5, _allocation.h - 0.5);
		cairo_line_to(cr, 0.5, _allocation.h - 0.5);
		cairo_line_to(cr, 0.5, 0.5);
		cairo_stroke(cr);

		cairo_rectangle(cr, _allocation.w - 20.0, _allocation.h - 20.0,
				_allocation.w, _allocation.h);
		cairo_fill(cr);

	}
	cairo_restore(cr);

}

cairo_font_face_t * default_theme_t::get_default_font() {
	return font;
}

}

