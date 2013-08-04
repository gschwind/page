/*
 * simple_theme.cxx
 *
 *  Created on: 24 mars 2013
 *      Author: gschwind
 */


#include "simple_theme.hxx"
#include "box.hxx"
#include "color.hxx"
#include <string>
#include <algorithm>

using namespace std;

namespace page {

simple_theme_layout_t::simple_theme_layout_t() {

		notebook_margin.top = 24;
		notebook_margin.bottom = 4;
		notebook_margin.left = 4;
		notebook_margin.right = 4;

		split_margin.top = 0;
		split_margin.bottom = 0;
		split_margin.left = 0;
		split_margin.right = 0;

		floating_margin.top = 28;
		floating_margin.bottom = 8;
		floating_margin.left = 8;
		floating_margin.right = 8;

		split_width = 8;

}

simple_theme_layout_t::~simple_theme_layout_t() {

}

list<box_int_t> simple_theme_layout_t::compute_client_tab(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {
	list<box_int_t> result;

	if (number_of_client != 0) {
		double box_width = ((allocation.w - 17.0 * 5.0)
				/ (number_of_client + 1.0));
		double offset = allocation.x;

		box_t<int> b;

		for (int i = 0; i < number_of_client; ++i) {
			if (i == selected_client_index) {
				b = box_int_t(floor(offset), allocation.y,
						floor(offset + 2.0 * box_width) - floor(offset),
						notebook_margin.top - 4);
				result.push_back(b);
				offset += box_width * 2;
			} else {
				b = box_int_t(floor(offset), allocation.y,
						floor(offset + box_width) - floor(offset),
						notebook_margin.top - 4);
				result.push_back(b);
				offset += box_width;
			}
		}
	}
	return result;
}



box_int_t simple_theme_layout_t::compute_notebook_close_window_position(
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

box_int_t simple_theme_layout_t::compute_notebook_unbind_window_position(box_int_t const & allocation, int number_of_client, int selected_client_index) const {

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

box_int_t simple_theme_layout_t::compute_notebook_bookmark_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {
	return box_int_t(allocation.x + allocation.w - 4 * 17 - 2, allocation.y + 2,
			16, 16);
}

box_int_t simple_theme_layout_t::compute_notebook_vsplit_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {

	return box_int_t(allocation.x + allocation.w - 3 * 17 - 2, allocation.y + 2,
			16, 16);
}

box_int_t simple_theme_layout_t::compute_notebook_hsplit_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {

	return box_int_t(allocation.x + allocation.w - 2 * 17 - 2, allocation.y + 2,
			16, 16);

}

box_int_t simple_theme_layout_t::compute_notebook_close_position(
		box_int_t const & allocation, int number_of_client,
		int selected_client_index) const {

	return box_int_t(allocation.x + allocation.w - 1 * 17 - 2, allocation.y + 2,
			16, 16);
}

box_int_t simple_theme_layout_t::compute_floating_close_position(box_int_t const & _allocation) const {
	return box_int_t(_allocation.x + _allocation.w - 1 * 17 - 8, _allocation.y + 8, 16, 16);
}

box_int_t simple_theme_layout_t::compute_floating_bind_position(box_int_t const & _allocation) const {
	return box_int_t(_allocation.x + _allocation.w - 2 * 17 - 8, _allocation.y + 8, 16, 16);
}



/**********************************************************************************/


inline string get_value_string(GKeyFile * conf, string const & group, string const & key) {
	string ret;
	gchar * value = g_key_file_get_string(conf, group.c_str(), key.c_str(), 0);
	if(value != 0) {
		ret = value;
		g_free(value);
		return ret;
	} else {
		throw runtime_error("config key missing");
	}
}


simple_theme_t::simple_theme_t(config_handler_t & conf) {

	string conf_img_dir = conf.get_string("default", "theme_dir");
	string font = conf.get_string("default", "font_file");
	string font_bold = conf.get_string("default", "font_bold_file");


	grey0.set(conf.get_string("simple_theme", "grey0"));
	grey1.set(conf.get_string("simple_theme", "grey1"));
	grey2.set(conf.get_string("simple_theme", "grey2"));
	grey3.set(conf.get_string("simple_theme", "grey3"));
	grey5.set(conf.get_string("simple_theme", "grey5"));

	plum0.set(conf.get_string("simple_theme", "plum0"));
	plum1.set(conf.get_string("simple_theme", "plum1"));
	plum2.set(conf.get_string("simple_theme", "plum2"));

	c_selected1.set(conf.get_string("simple_theme", "selected1"));
	c_selected2.set(conf.get_string("simple_theme", "selected2"));

	c_selected_highlight1.set(conf.get_string("simple_theme", "selected_highlight1"));
	c_selected_highlight2.set(conf.get_string("simple_theme", "selected_highlight2"));

	c_normal1.set(conf.get_string("simple_theme", "normal1"));
	c_normal2.set(conf.get_string("simple_theme", "normal2"));

	c_tab_boder_highlight.set(conf.get_string("simple_theme", "tab_boder_highlight"));
	c_tab_boder_normal.set(conf.get_string("simple_theme", "tab_boder_normal"));

	c_tab_selected_background.set(conf.get_string("simple_theme", "tab_selected_background"));
	c_tab_normal_background.set(conf.get_string("simple_theme", "tab_normal_background"));


	layout = new simple_theme_layout_t();

	ft_is_loaded = false;

	vsplit_button_s = 0;
	hsplit_button_s = 0;
	close_button_s = 0;
	pop_button_s = 0;
	pops_button_s = 0;
	unbind_button_s = 0;
	bind_button_s = 0;

	if(conf.has_key("simple_theme", "background_png")) {
		background_s = cairo_image_surface_create_from_png(conf.get_string("simple_theme", "background_png").c_str());
	} else {
		background_s = 0;
	}

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

		this->font = cairo_ft_font_face_create_for_ft_face(face, FT_LOAD_DEFAULT);
		if (!this->font)
			throw std::runtime_error("unable to load default font");

		printf("Load: %s\n", font_bold.c_str());

		error = FT_New_Face(library, font_bold.c_str(), 0, &face_bold);

		if (error != FT_Err_Ok)
			throw std::runtime_error("unable to load default bold font");

		this->font_bold = cairo_ft_font_face_create_for_ft_face(face_bold, FT_LOAD_DEFAULT);
		if (!this->font_bold)
			throw std::runtime_error("unable to load default bold font");

		ft_is_loaded = true;
	}

}

simple_theme_t::~simple_theme_t() {

	delete layout;

	cairo_surface_destroy(hsplit_button_s);
	cairo_surface_destroy(vsplit_button_s);
	cairo_surface_destroy(close_button_s);
	cairo_surface_destroy(pop_button_s);
	cairo_surface_destroy(pops_button_s);
	cairo_surface_destroy(unbind_button_s);
	cairo_surface_destroy(bind_button_s);

	if(this->font != 0)
		cairo_font_face_destroy(this->font);

	if(this->font_bold != 0)
		cairo_font_face_destroy(this->font_bold);

	FT_Done_Face(face);
	FT_Done_Face(face_bold);
	FT_Done_FreeType(library);


}

void simple_theme_t::rounded_rectangle(cairo_t * cr, double x, double y,
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

void simple_theme_t::render_notebook(cairo_t * cr, notebook_t * n, managed_window_t * focussed,
		bool is_default) {

	bool has_focuced_client = false;
	color_t c_selected_bg = c_tab_normal_background;

	cairo_reset_clip(cr);
	cairo_identity_matrix(cr);
	cairo_set_line_width(cr, 1.0);
	cairo_new_path(cr);

	cairo_rectangle(cr, n->_allocation.x, n->_allocation.y, n->_allocation.w,
			n->_allocation.h);
	if (background_s != 0) {
		cairo_set_source_surface(cr, background_s, 0.0, 0.0);
	} else {
		cairo_set_source_rgb(cr, c_tab_selected_background.r,
				c_tab_selected_background.g, c_tab_selected_background.b);
	}
	cairo_fill(cr);

	{
		box_int_t b;
		b.y = n->_allocation.y;
		b.h = layout->notebook_margin.top - 4;

		b.x = n->_allocation.x;
		b.w = n->_allocation.x + n->_allocation.w;

		if(background_s != 0) {
			cairo_set_source_surface(cr, background_s, 0.0, 0.0);
		} else {
			cairo_set_source_rgb(cr, grey0.r, grey0.g, grey0.b);
		}
		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		//cairo_set_source_rgb(cr, grey1.r, grey1.g, grey1.b);
		cairo_fill(cr);

	}

	int number_of_client = n->_clients.size();
	int selected_index = -1;

	if(!n->_selected.empty()) {
		list<managed_window_t *>::iterator i = find(n->_clients.begin(), n->_clients.end(), n->_selected.front());
		selected_index = distance(n->_clients.begin(), i);
	}

	list<box_int_t> tabs = layout->compute_client_tab(n->_allocation, number_of_client, selected_index);

	cairo_path_t * selected_path = 0;

	/* check if notebook have the focussed window */
	for(list<managed_window_t *>::iterator c = n->_clients.begin(); c != n->_clients.end(); ++c) {
		if(*c == focussed) {
			has_focuced_client = true;
			c_selected_bg = c_tab_selected_background;
			break;
		}
	}


	list<managed_window_t *>::iterator c = n->_clients.begin();
	for(list<box_int_t>::iterator i = tabs.begin(); i != tabs.end(); ++i, ++c) {
		box_int_t b = *i;

		if (*c == n->_selected.front()) {

			if(background_s != 0) {
				cairo_set_source_surface(cr, background_s, 0.0, 0.0);
			} else {
				cairo_set_source_rgb(cr, grey0.r, grey0.g, grey0.b);
			}
			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_fill(cr);

			cairo_set_source_rgba(cr, c_selected_bg.r, c_selected_bg.g, c_selected_bg.b, 0.2);
			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_fill(cr);

			cairo_new_path(cr);
			cairo_move_to(cr, b.x + 0.5, b.y + b.h + 0.5);
			cairo_line_to(cr, b.x + 0.5, b.y + 0.5);
			cairo_line_to(cr, b.x + b.w - 0.5, b.y + 0.5);
			cairo_line_to(cr, b.x + b.w - 0.5, b.y + b.h + 0.5);
			cairo_line_to(cr, n->_allocation.x + n->_allocation.w - 1.5, b.y + b.h + 0.5);
			cairo_line_to(cr, n->_allocation.x + n->_allocation.w - 1.5, n->_allocation.y + n->_allocation.h - 1.5);
			cairo_line_to(cr, n->_allocation.x + 0.5, n->_allocation.y + n->_allocation.h - 1.5);
			cairo_line_to(cr, n->_allocation.x + 0.5, b.y + b.h + 0.5);
			cairo_line_to(cr, b.x + 0.5, b.y + b.h + 0.5);
			selected_path = cairo_copy_path(cr);

		} else {
			b.x += 1;
			b.w -= 2;
			b.y += 1;
			b.h -= 1;

			if(background_s != 0) {
				cairo_set_source_surface(cr, background_s, 0.0, 0.0);
			} else {
				cairo_set_source_rgb(cr, grey2.r, grey2.g, grey2.b);
			}
			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_fill(cr);

			cairo_new_path(cr);
			cairo_move_to(cr, b.x + 0.5, b.y + b.h + 0.5);
			cairo_line_to(cr, b.x + 0.5, b.y + 0.5);
			cairo_line_to(cr, b.x + b.w - 0.5, b.y + 0.5);
			cairo_line_to(cr, b.x + b.w - 0.5, b.y + b.h + 0.5);
			cairo_set_source_rgb(cr, plum2.r, plum2.g, plum2.b);
			cairo_stroke(cr);
		}

		box_int_t bicon = b;
		bicon.h = 16;
		bicon.w = 16;
		bicon.x += 3;
		bicon.y += 2;

		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		if ((*c)->get_icon() != 0) {
			if ((*c)->get_icon()->get_cairo_surface() != 0) {
				cairo_rectangle(cr, bicon.x, bicon.y, bicon.w, bicon.h);
				cairo_set_source_surface(cr,
						(*c)->get_icon()->get_cairo_surface(), bicon.x,
						bicon.y);
				if (*c == n->_selected.front()) {
					cairo_paint_with_alpha(cr, 1.0);
				} else {
					cairo_paint_with_alpha(cr, 1.0);
				}
			}
		}
		cairo_restore(cr);

		box_int_t btext = b;

		if (*c == n->_selected.front()) {
			btext.h -= 0;
			btext.w -= 3 * 16 + 12;
			btext.x += 3 + 16 + 2;
			btext.y += 2;
		} else {
			btext.h -= 0;
			btext.w -= 1 * 16 + 8;
			btext.x += 3 + 16 + 2;
			btext.y += 2;
		}


		cairo_save(cr);

		cairo_reset_clip(cr);
		cairo_new_path(cr);
		cairo_rectangle(cr, btext.x, btext.y, btext.w, btext.h);
		cairo_clip(cr);
		cairo_set_font_size(cr, 13);

		if (*c == n->_selected.front()) {

			/* draw selectected tittle */
			cairo_set_font_face(cr, font_bold);

			cairo_translate(cr, btext.x + 2, btext.y + btext.h - 7);

			cairo_new_path(cr);
			cairo_text_path(cr, (*c)->get_title().c_str());
			cairo_set_line_width(cr, 3.0);
			cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
			cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
			if(*c == focussed) {
				cairo_set_source_rgb(cr, c_selected_highlight2.r, c_selected_highlight2.g, c_selected_highlight2.b);
			} else {
				cairo_set_source_rgb(cr, c_selected2.r, c_selected2.g, c_selected2.b);
			}

			cairo_stroke_preserve(cr);

			if(*c == focussed) {
				cairo_set_source_rgb(cr, c_selected_highlight1.r, c_selected_highlight1.g, c_selected_highlight1.b);
			} else {
				cairo_set_source_rgb(cr, c_selected1.r, c_selected1.g, c_selected1.b);
			}
			cairo_fill(cr);

		} else {

			/* draw tittle */
			cairo_set_font_face(cr, font);
			cairo_set_source_rgb(cr, c_normal2.r, c_normal2.g, c_normal2.b);

			cairo_translate(cr, btext.x + 2, btext.y + btext.h - 7);

			cairo_new_path(cr);
			cairo_text_path(cr, (*c)->get_title().c_str());
			cairo_set_line_width(cr, 3.0);
			cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
			cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);

			cairo_stroke_preserve(cr);

			cairo_set_source_rgb(cr, c_normal1.r, c_normal1.g, c_normal1.b);
			cairo_fill(cr);
		}

		cairo_restore(cr);
	}

	cairo_set_line_width(cr, 1.0);

	{
		box_int_t b;
		b.y = n->_allocation.y;
		b.h = layout->notebook_margin.top - 4;

		if (tabs.empty()) {
			b.x = n->_allocation.x;
			b.w = n->_allocation.x + n->_allocation.w;
		} else {
			b.x = tabs.back().x + tabs.back().w;
			b.w = n->_allocation.x + n->_allocation.w - tabs.back().x
					- tabs.back().w;
		}

		if(background_s != 0) {
			cairo_set_source_surface(cr, background_s, 0.0, 0.0);
		} else {
			cairo_set_source_rgb(cr, grey0.r, grey0.g, grey0.b);
		}
		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		//cairo_set_source_rgb(cr, grey1.r, grey1.g, grey1.b);
		cairo_fill(cr);

	}

	if (!n->_selected.empty()) {

		/* draw close button */

		box_int_t b = layout->compute_notebook_close_window_position(n->_allocation,
				number_of_client, selected_index);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_surface(cr, close_button_s, b.x, b.y);
		cairo_paint(cr);

//		cairo_set_line_width(cr, 1.0);
//		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
//		cairo_set_source_rgb(cr, grey2.r, grey2.g, grey2.b);
//		cairo_stroke(cr);
	}

	if (!n->_selected.empty()) {

		/* draw the unbind button */

		box_int_t b = layout->compute_notebook_unbind_window_position(n->_allocation,
				number_of_client, selected_index);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_surface(cr, unbind_button_s, b.x, b.y);
		cairo_paint(cr);

//		cairo_set_line_width(cr, 1.0);
//		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
//		cairo_set_source_rgb(cr, grey2.r, grey2.g, grey2.b);
//		cairo_stroke(cr);

	}

	{
		box_int_t b = layout->compute_notebook_bookmark_position(n->_allocation,
				number_of_client, selected_index);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		if(is_default) {
			cairo_set_source_surface(cr, pops_button_s, b.x, b.y);
		} else {
			cairo_set_source_surface(cr, pop_button_s, b.x, b.y);
		}
		cairo_paint(cr);

//		cairo_set_line_width(cr, 1.0);
//		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
//		cairo_set_source_rgb(cr, grey5.r, grey5.g, grey5.b);
//		cairo_stroke(cr);

	}

	{
		box_int_t b = layout->compute_notebook_vsplit_position(n->_allocation,
				number_of_client, selected_index);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_surface(cr, vsplit_button_s, b.x, b.y);
		cairo_paint(cr);

//		cairo_set_line_width(cr, 1.0);
//		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
//		cairo_set_source_rgb(cr, grey5.r, grey5.g, grey5.b);
//		cairo_stroke(cr);

	}

	{
		box_int_t b = layout->compute_notebook_hsplit_position(n->_allocation,
				number_of_client, selected_index);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_surface(cr, hsplit_button_s, b.x, b.y);
		cairo_paint(cr);

//		cairo_set_line_width(cr, 1.0);
//		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
//		cairo_set_source_rgb(cr, grey5.r, grey5.g, grey5.b);
//		cairo_stroke(cr);

	}

	{
		box_int_t b = layout->compute_notebook_close_position(n->_allocation,
				number_of_client, selected_index);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_surface(cr, close_button_s, b.x, b.y);
		cairo_fill(cr);

//		cairo_set_line_width(cr, 1.0);
//		cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
//		cairo_set_source_rgb(cr, grey5.r, grey5.g, grey5.b);
//		cairo_stroke(cr);

	}

	if (!n->_selected.empty()) {

		box_int_t area = n->_allocation;
		area.y += layout->notebook_margin.top - 4;
		area.h -= layout->notebook_margin.top - 4;

		box_int_t bclient = n->_selected.front()->get_base_position();

		{
			/* left */

			box_int_t b(area.x, area.y, bclient.x - area.x, area.h);

			if(background_s != 0) {
				cairo_set_source_surface(cr, background_s, 0.0, 0.0);
			} else {
				cairo_set_source_rgb(cr, grey0.r, grey0.g, grey0.b);
			}
			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_fill(cr);

			cairo_set_source_rgba(cr, c_selected_bg.r, c_selected_bg.g, c_selected_bg.b, 0.2);
			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_fill(cr);

		}

		{
			/* right */

			box_int_t b(bclient.x + bclient.w, area.y,
					area.x + area.w - bclient.x - bclient.w, area.h);

			if(background_s != 0) {
				cairo_set_source_surface(cr, background_s, 0.0, 0.0);
			} else {
				cairo_set_source_rgb(cr, grey0.r, grey0.g, grey0.b);
			}
			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_fill(cr);

			cairo_set_source_rgba(cr, c_selected_bg.r, c_selected_bg.g, c_selected_bg.b, 0.2);
			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_fill(cr);

		}

		{
			/* top */

			box_int_t b(bclient.x, area.y, bclient.w, bclient.y - area.y);

			if(background_s != 0) {
				cairo_set_source_surface(cr, background_s, 0.0, 0.0);
			} else {
				cairo_set_source_rgb(cr, grey0.r, grey0.g, grey0.b);
			}
			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_fill(cr);

			cairo_set_source_rgba(cr, c_selected_bg.r, c_selected_bg.g, c_selected_bg.b, 0.2);
			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_fill(cr);

		}

		{
			/* bottom */

			box_int_t b(bclient.x, bclient.y + bclient.h, bclient.w,
					area.y + area.h - bclient.y - bclient.h);

			if(background_s != 0) {
				cairo_set_source_surface(cr, background_s, 0.0, 0.0);
			} else {
				cairo_set_source_rgb(cr, grey0.r, grey0.g, grey0.b);
			}
			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_fill(cr);

			cairo_set_source_rgba(cr, c_selected_bg.r, c_selected_bg.g, c_selected_bg.b, 0.2);
			cairo_rectangle(cr, b.x, b.y, b.w, b.h);
			cairo_fill(cr);

		}

		{
			/* draw box around app window */
			cairo_rectangle(cr, bclient.x - 0.5, bclient.y - 0.5, bclient.w + 1.0, bclient.h + 1.0);
			cairo_set_source_rgb(cr, grey1.r, grey1.g, grey1.b);
			cairo_stroke(cr);
		}

	} else {
		box_int_t area = n->_allocation;
		area.y += layout->notebook_margin.top - 4;
		area.h -= layout->notebook_margin.top - 4;

		{
			/* empty tab */
			if(background_s != 0) {
				cairo_set_source_surface(cr, background_s, 0.0, 0.0);
			} else {
				cairo_set_source_rgb(cr, grey0.r, grey0.g, grey0.b);
			}
			cairo_rectangle(cr, area.x, area.y, area.w, area.h);
			//cairo_set_source_rgb(cr, grey0.r, grey0.g, grey0.b);
			cairo_fill(cr);

		}
	}

	if(selected_path != 0) {
		cairo_new_path(cr);
		cairo_append_path(cr, selected_path);
		if (has_focuced_client) {
			cairo_set_source_rgb(cr, c_tab_boder_highlight.r, c_tab_boder_highlight.g, c_tab_boder_highlight.b);
		} else {
			cairo_set_source_rgb(cr, c_tab_boder_normal.r, c_tab_boder_normal.g, c_tab_boder_normal.b);
		}
		cairo_stroke(cr);
		cairo_path_destroy(selected_path);
	}

}

void simple_theme_t::render_split(cairo_t * cr, split_t * s) {

	cairo_reset_clip(cr);
	cairo_identity_matrix(cr);
	cairo_set_line_width(cr, 1.0);

	box_int_t area = s->get_split_bar_area();
	//cairo_set_source_rgb(cr, grey0.r, grey0.g, grey0.b);
	if(background_s != 0) {
		cairo_set_source_surface(cr, background_s, 0.0, 0.0);
	} else {
		cairo_set_source_rgb(cr, grey0.r, grey0.g, grey0.b);
	}
	cairo_rectangle(cr, area.x, area.y, area.w, area.h);
	cairo_fill(cr);

//	cairo_new_path(cr);
//
//	if(s->get_split_type() == VERTICAL_SPLIT) {
//		cairo_move_to(cr, area.x + layout->split_width / 2, area.y);
//		cairo_line_to(cr, area.x + layout->split_width / 2, area.y + area.h);
//	} else {
//		cairo_move_to(cr, area.x, area.y + layout->split_width / 2);
//		cairo_line_to(cr, area.x + area.w, area.y + layout->split_width / 2);
//	}
//
//	cairo_set_line_width(cr, layout->split_width / 2 - 2.0);
//	cairo_set_source_rgb(cr, grey2.r, grey2.g, grey2.b);
//
//	static const double dashed3[] = {1.0};
//	cairo_set_dash(cr, dashed3, 1, 0);
//
//	cairo_stroke(cr);

}



void simple_theme_t::render_floating(managed_window_t * mw, bool is_focussed) {

	cairo_t * cr = mw->get_cairo();
	box_int_t _allocation = mw->get_base_position();

	_allocation.x = 0;
	_allocation.y = 0;

	cairo_reset_clip(cr);
	cairo_identity_matrix(cr);
	cairo_set_line_width(cr, 1.0);

	cairo_rectangle(cr, _allocation.x, _allocation.y, _allocation.w, _allocation.h);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_fill(cr);

	{
		box_int_t b = _allocation;
		b.h = layout->floating_margin.top - 8;
		b.y += 8;

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, plum0.r, plum0.g, plum0.b);
		cairo_fill(cr);

		//cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		//cairo_set_source_rgb(cr, 0.5, 0.0, 0.0);
		//cairo_stroke(cr);

		box_int_t bicon = b;
		bicon.h = 16;
		bicon.w = 16;
		bicon.x += 8;
		bicon.y += 0;

		if (mw->get_icon() != 0) {
			if (mw->get_icon()->get_cairo_surface() != 0) {
				cairo_rectangle(cr, bicon.x, bicon.y, bicon.w, bicon.h);
				cairo_set_source_surface(cr,
						mw->get_icon()->get_cairo_surface(), bicon.x, bicon.y);
				cairo_paint(cr);
			}
		}

		box_int_t btext = b;
		btext.h -= 4;
		btext.w -= 3 * 16 - 8;
		btext.x += 8 + 16 + 2;
		btext.y += 0;


		cairo_reset_clip(cr);

		cairo_save(cr);
		cairo_new_path(cr);
		cairo_rectangle(cr, btext.x, btext.y, btext.w, btext.h);
		cairo_clip(cr);

		//cairo_move_to(cr, btext.x, btext.y + btext.h - 3);
		cairo_set_font_size(cr, 13);

		if (is_focussed) {

			/* draw selectected tittle */
			cairo_set_font_face(cr, font_bold);

			cairo_translate(cr, btext.x + 2, btext.y + btext.h - 4);

			cairo_new_path(cr);
			cairo_text_path(cr, mw->get_title().c_str());
			cairo_set_line_width(cr, 3.0);
			cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
			cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
			if(true) {
				cairo_set_source_rgb(cr, c_selected_highlight2.r, c_selected_highlight2.g, c_selected_highlight2.b);
			} else {
				cairo_set_source_rgb(cr, c_selected2.r, c_selected2.g, c_selected2.b);
			}

			cairo_stroke_preserve(cr);

			if(true) {
				cairo_set_source_rgb(cr, c_selected_highlight1.r, c_selected_highlight1.g, c_selected_highlight1.b);
			} else {
				cairo_set_source_rgb(cr, c_selected1.r, c_selected1.g, c_selected1.b);
			}
			cairo_fill(cr);

		} else {

			/* draw tittle */
			cairo_set_font_face(cr, font);
			cairo_set_source_rgb(cr, c_normal2.r, c_normal2.g, c_normal2.b);

			cairo_translate(cr, btext.x + 2, btext.y + btext.h - 4);

			cairo_new_path(cr);
			cairo_text_path(cr, mw->get_title().c_str());
			cairo_set_line_width(cr, 3.0);
			cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
			cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);

			cairo_stroke_preserve(cr);

			cairo_set_source_rgb(cr, c_normal1.r, c_normal1.g, c_normal1.b);
			cairo_fill(cr);
		}

		cairo_restore(cr);
		cairo_reset_clip(cr);

	}

	{
		box_int_t b = layout->compute_floating_close_position(_allocation);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, plum0.r, plum0.g, plum0.b);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_surface(cr, close_button_s, b.x, b.y);
		cairo_fill(cr);

		//cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		//cairo_set_source_rgb(cr, 0.5, 0.5, 0.0);
		//cairo_stroke(cr);
	}

	{
		box_int_t b = layout->compute_floating_bind_position(_allocation);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, plum0.r, plum0.g, plum0.b);
		cairo_fill(cr);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_surface(cr, bind_button_s, b.x, b.y);
		cairo_fill(cr);

		//cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		//cairo_set_source_rgb(cr, 0.5, 0.5, 0.0);
		//cairo_stroke(cr);

	}

	{
		/* left */

		box_int_t b(_allocation.x,
				0,
				layout->floating_margin.left,
				_allocation.h);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, plum0.r, plum0.g, plum0.b);
		cairo_fill(cr);

		//cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		//cairo_set_source_rgb(cr, 0.5, 0.25, 0.0);
		//cairo_stroke(cr);

	}

	{
		/* right */

		box_int_t b(
				_allocation.x + _allocation.w
						- layout->floating_margin.right,
				0,
				layout->floating_margin.left,
				_allocation.h);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, plum0.r, plum0.g, plum0.b);
		cairo_fill(cr);

		//cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		//cairo_set_source_rgb(cr, 0.5, 0.25, 0.0);
		//cairo_stroke(cr);

	}

	{
		/* top */

		box_int_t b(_allocation.x + layout->floating_margin.left,
				0,
				_allocation.w - layout->floating_margin.left
						- layout->floating_margin.right, 8);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, plum0.r, plum0.g, plum0.b);
		cairo_fill(cr);

		//cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		//cairo_set_source_rgb(cr, 0.5, 0.25, 0.0);
		//cairo_stroke(cr);

	}

	{
		/* bottom */

		box_int_t b(_allocation.x + layout->floating_margin.left,
				_allocation.y + _allocation.h - layout->floating_margin.bottom,
				_allocation.w - layout->floating_margin.left
						- layout->floating_margin.right, 8);

		cairo_rectangle(cr, b.x, b.y, b.w, b.h);
		cairo_set_source_rgb(cr, plum0.r, plum0.g, plum0.b);
		cairo_fill(cr);

		//cairo_rectangle(cr, b.x + 0.5, b.y + 0.5, b.w - 1.0, b.h - 1.0);
		//cairo_set_source_rgb(cr, 0.5, 0.25, 0.0);
		//cairo_stroke(cr);

	}

	{
		/* outer border */

		cairo_rectangle(cr, 0.5, 0.5, _allocation.w - 1.0, _allocation.h - 1.0);
		if (is_focussed) {
			cairo_set_source_rgb(cr, c_tab_boder_highlight.r, c_tab_boder_highlight.g, c_tab_boder_highlight.b);
		} else {
			cairo_set_source_rgb(cr, c_tab_boder_normal.r, c_tab_boder_normal.g, c_tab_boder_normal.b);
		}
		cairo_stroke(cr);

	}


}

cairo_font_face_t * simple_theme_t::get_default_font() {
	return font;
}

}


//page::theme_t * get_theme(config_handler_t * conf) {
//	return new page::simple_theme_t(*conf);
//}
