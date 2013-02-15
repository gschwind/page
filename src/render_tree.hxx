/*
 * render_tree.hxx
 *
 *  Created on: 26 déc. 2012
 *      Author: gschwind
 */

#ifndef RENDER_TREE_HXX_
#define RENDER_TREE_HXX_

#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <ft2build.h>
#include <freetype/freetype.h>
#include FT_FREETYPE_H

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo-ft.h>

#include "box.hxx"
#include "split.hxx"
#include "notebook.hxx"

namespace page {

class render_tree_t {
public:

	static int const BORDER_SIZE = 4;
	static int const HEIGHT = 24;
	static int const GRIP_SIZE = 3;

	cairo_surface_t * s;
	cairo_t * cr;

	bool ft_is_loaded;
	FT_Library library; /* handle to library */
	FT_Face face; /* handle to face object */
	cairo_font_face_t * font;
	FT_Face face_bold; /* handle to face object */
	cairo_font_face_t * font_bold;

	cairo_surface_t * vsplit_button_s;
	cairo_surface_t * hsplit_button_s;
	cairo_surface_t * close_button_s;
	cairo_surface_t * pop_button_s;
	cairo_surface_t * pops_button_s;

	int width;
	int height;

	render_tree_t(cairo_surface_t * target, std::string conf_img_dir,
			std::string font, std::string font_bold, int width, int heigth) :
			width(width), height(heigth) {
		s = cairo_surface_create_similar(target, CAIRO_CONTENT_COLOR, width,
				heigth);
		cr = cairo_create(s);

		/* open icons */
		if (hsplit_button_s == 0 || true) {
			std::string filename = conf_img_dir + "/hsplit_button.png";
			printf("Load: %s\n", filename.c_str());
			hsplit_button_s = cairo_image_surface_create_from_png(
					filename.c_str());
			if (hsplit_button_s == 0)
				throw std::runtime_error("file not found!");
		}

		if (vsplit_button_s == 0|| true) {
			std::string filename = conf_img_dir + "/vsplit_button.png";
			printf("Load: %s\n", filename.c_str());
			vsplit_button_s = cairo_image_surface_create_from_png(
					filename.c_str());
			if (vsplit_button_s == 0)
				throw std::runtime_error("file not found!");
		}

		if (close_button_s == 0|| true) {
			std::string filename = conf_img_dir + "/close_button.png";
			printf("Load: %s\n", filename.c_str());
			close_button_s = cairo_image_surface_create_from_png(
					filename.c_str());
			if (close_button_s == 0)
				throw std::runtime_error("file not found!");
		}

		if (pop_button_s == 0|| true) {
			std::string filename = conf_img_dir + "/pop_button.png";
			printf("Load: %s\n", filename.c_str());
			pop_button_s = cairo_image_surface_create_from_png(
					filename.c_str());
			if (pop_button_s == 0)
				throw std::runtime_error("file not found!");
		}

		if (pops_button_s == 0|| true) {
			std::string filename = conf_img_dir + "/pops_button.png";
			printf("Load: %s\n", filename.c_str());
			pops_button_s = cairo_image_surface_create_from_png(
					filename.c_str());
			if (pop_button_s == 0)
				throw std::runtime_error("file not found!");
		}

		/* load fonts */
		if (!ft_is_loaded) {
			FT_Error error = FT_Init_FreeType(&library);
			if (error) {
				throw std::runtime_error("unable to init freetype");
			}

			error = FT_New_Face(library, font.c_str(), 0, &face);

			if (error != FT_Err_Ok)
				throw std::runtime_error("unable to load default font");

			this->font = cairo_ft_font_face_create_for_ft_face(face, 0);

			error = FT_New_Face(library, font_bold.c_str(), 0, &face_bold);

			if (error != FT_Err_Ok)
				throw std::runtime_error("unable to load default bold font");

			this->font_bold = cairo_ft_font_face_create_for_ft_face(face_bold,
					0);

			ft_is_loaded = true;
		}
	}

	static void rounded_rectangle(cairo_t * cr, double x, double y, double w,
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

	void render_notebook(notebook_t * n) {

		cairo_save(cr);
		{

			//std::cout << "xxx: " << n->_allocation.to_string() << std::endl;

			cairo_translate(cr, n->_allocation.x, n->_allocation.y);
			cairo_set_line_width(cr, 1.0);
			cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

			/* create tabs back ground */
			cairo_rectangle(cr, 0.0, 0.0, n->_allocation.w, HEIGHT);
			cairo_pattern_t *pat;
			pat = cairo_pattern_create_linear(0.0, 0.0, 0.0, HEIGHT);
			cairo_pattern_add_color_stop_rgba(pat, 0, 0xeeU / 255.0,
					0xeeU / 255.0, 0xecU / 255.0, 1);
			cairo_pattern_add_color_stop_rgba(pat, 1, 0xbaU / 255.0,
					0xbdU / 255.0, 0xd6U / 255.0, 1);
			cairo_set_source(cr, pat);
			cairo_fill(cr);
			cairo_pattern_destroy(pat);

			/* draw top line */
			cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0,
					0x85U / 255.0);
			cairo_new_path(cr);
			cairo_move_to(cr, 0.0, HEIGHT - 0.5);
			cairo_line_to(cr, n->_allocation.w, HEIGHT - 0.5);

			cairo_stroke(cr);

			cairo_set_source_rgb(cr, 0xeeU / 255.0,
					0xeeU / 255.0, 0xecU / 255.0);
			cairo_rectangle(cr, 0.0, HEIGHT, n->_allocation.w,
					n->_allocation.h - HEIGHT);
			cairo_fill(cr);

			std::list<window_t *>::iterator i;
			double offset = 0;
			double length = (n->_allocation.w - 17.0 * 5.0) / (n->_clients.size() + 1.0);
			for (i = n->_clients.begin(); i != n->_clients.end(); ++i) {

				cairo_save(cr);
				{

					cairo_translate(cr, floor(offset), 0.0);

					if (n->_selected.front() == (*i)) {

						cairo_set_line_width(cr, 1.0);
						cairo_set_font_face(cr, font_bold);
						cairo_set_font_size(cr, 13);

						/* draw light background */
						cairo_rectangle(cr, 0.0, 3.0, length * 2, HEIGHT);
						cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0,
								0xecU / 255.0);
						cairo_fill(cr);

						if ((*i)->icon_surf != 0) {
							cairo_save(cr);
							cairo_translate(cr, 3.0, 4.0);
							cairo_set_source_surface(cr, (*i)->icon_surf, 0.0,
									0.0);
							cairo_paint(cr);
							cairo_restore(cr);
						}

						/* draw the name */
						cairo_rectangle(cr, 2.0, 0.0, length * 2 - 17.0,
								HEIGHT - 2.0);
						cairo_clip(cr);
						cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
						cairo_set_font_size(cr, 13.0);
						cairo_move_to(cr, 20.5, 15.5);

						cairo_show_text(cr, (*i)->get_title().c_str());

						/* draw blue lines */
						cairo_reset_clip(cr);

						cairo_new_path(cr);
						cairo_move_to(cr, 1.0, 1.5);
						cairo_line_to(cr, length * 2, 1.5);
						cairo_move_to(cr, 1.0, 2.5);
						cairo_line_to(cr, length * 2, 2.5);
						cairo_set_source_rgb(cr, 0x72U / 255.0, 0x9fU / 255.0,
								0xcfU / 255.0);
						cairo_stroke(cr);
						cairo_new_path(cr);
						cairo_move_to(cr, 0.0, 3.5);
						cairo_line_to(cr, length * 2, 3.5);
						cairo_set_source_rgb(cr, 0x34U / 255.0, 0x64U / 255.0,
								0xa4U / 255.0);
						cairo_stroke(cr);

						cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0,
								0x85U / 255.0);
						rounded_rectangle(cr, 0.5, 0.5, length * 2,
								HEIGHT - 1.0, 3.0);

						/* draw close icon */
						cairo_set_line_width(cr, 2.0);
						cairo_translate(cr, length * 2 - 16.5, 3.5);
						/* draw close */
						cairo_new_path(cr);
						cairo_move_to(cr, 4.0, 4.0);
						cairo_line_to(cr, 12.0, 12.0);
						cairo_move_to(cr, 12.0, 4.0);
						cairo_line_to(cr, 4.0, 12.0);
						cairo_set_source_rgb(cr, 0xCCU / 255.0, 0x00U / 255.0,
								0x00U / 255.0);
						cairo_stroke(cr);

						cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
						cairo_set_source_surface(cr, close_button_s, 0.5, 0.5);
						cairo_paint(cr);

						offset += length * 2;
					} else {

						if ((*i)->icon_surf != 0) {
							cairo_save(cr);
							cairo_translate(cr, 3.0, 4.0);
							cairo_set_source_surface(cr, (*i)->icon_surf, 0.0,
									0.0);
							cairo_paint(cr);
							cairo_restore(cr);
						}

						cairo_set_line_width(cr, 1.0);
						cairo_set_font_face(cr, font);
						cairo_set_font_size(cr, 13);

						/* draw window name */
						cairo_rectangle(cr, 2, 0, length - 4, HEIGHT - 1);
						cairo_clip(cr);
						cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
						cairo_set_font_size(cr, 13);
						cairo_move_to(cr, HEIGHT + 0.5, 15.5);
						cairo_show_text(cr, (*i)->get_title().c_str());

						/* draw border */
						cairo_reset_clip(cr);

						cairo_rectangle(cr, 0.0, 0.0, length + 1.0, HEIGHT);
						cairo_clip(cr);
						cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0,
								0x85U / 255.0);
						rounded_rectangle(cr, 0.5, 2.5, length, HEIGHT, 2.0);
						cairo_new_path(cr);
						cairo_move_to(cr, 0.0, HEIGHT);
						cairo_line_to(cr, length + 1.0, HEIGHT);
						cairo_stroke(cr);

						if ((*i)->demands_atteniion()) {
							cairo_new_path(cr);
							cairo_move_to(cr, 0.0, 3.5);
							cairo_line_to(cr, length * 2, 3.5);
							cairo_set_source_rgb(cr, 0xFFU / 255.0,
									0x00U / 255.0, 0x00U / 255.0);
							cairo_stroke(cr);
						}

						offset += length;
					}
				}
				cairo_restore(cr);

			}

			cairo_save(cr);
			{

				/* draw icon background */
				cairo_rectangle(cr, n->_allocation.w - 17.0 * 4, 0, 17.0 * 4,
						HEIGHT - 1);
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
				if (/*page.default_window_pop == this*/false) {
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
				window_t * c = n->_selected.front();
				cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0,
						0xecU / 255.0);

				box_int_t size = c->get_absolute_extend();
				/* left */
				cairo_rectangle(cr, n->_allocation.x, n->_allocation.y + HEIGHT,
						size.x - n->_allocation.x, n->_allocation.h - HEIGHT);
				cairo_fill(cr);
				/* right */
				cairo_rectangle(cr, size.x + size.w, n->_allocation.y + HEIGHT,
						n->_allocation.x + n->_allocation.w - size.x - size.w,
						n->_allocation.h - HEIGHT);
				cairo_fill(cr);

				/* top */
				cairo_rectangle(cr, size.x, n->_allocation.y + HEIGHT, size.w,
						size.y - n->_allocation.y - HEIGHT);

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
			cairo_set_source_rgb(cr, 0x88U / 255.0, 0x8aU / 255.0,
					0x85U / 255.0);
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

	/* this for call for each, is it usefull ? */
	void render_split(split_t * s) {
		cairo_save(cr);
		//cairo_set_source_rgb(cr, 0xeeU / 255.0, 0xeeU / 255.0, 0xecU / 255.0);
		cairo_set_source_rgb(cr, 0xeeU / 255.0, 0x00U / 255.0, 0x00U / 255.0);
		box_int_t area = s->get_split_bar_area();
		cairo_rectangle(cr, area.x, area.y, area.w, area.h);
		cairo_fill(cr);
		cairo_restore(cr);
	}

	void render_splits(std::set<split_t *> const & splits) {
		for (std::set<split_t *>::const_iterator i = splits.begin();
				i != splits.end(); ++i) {
			render_split(*i);
		}
	}

	void render_notebooks(std::set<notebook_t *> const & notebooks) {
		for (std::set<notebook_t *>::const_iterator i = notebooks.begin();
				i != notebooks.end(); ++i) {
			render_notebook(*i);
		}
	}

};

}

#endif /* RENDER_TREE_HXX_ */
