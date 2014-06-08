/*
 * renderable_page.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef RENDERABLE_PAGE_HXX_
#define RENDERABLE_PAGE_HXX_

#include "theme.hxx"
#include "window_overlay.hxx"

namespace page {

class renderable_page_t: public window_overlay_t {
	theme_t * _theme;
	region damaged;
public:

	renderable_page_t(display_t * cnx, theme_t * render, int width,
			int height) :
			window_overlay_t(cnx, 24, rectangle(0, 0, width, height)), _theme(
					render) {
		window_overlay_t::show();

	}

	~renderable_page_t() {
		cout << "call " << __FUNCTION__ << endl;
	}

	void repair_notebook_border(notebook_t * n) {
		region r = n->allocation();

		if (n->selected() == 0) {

			rectangle b;
			b.x = n->allocation().x + _theme->notebook_margin.left;
			b.y = n->allocation().y + _theme->notebook_margin.top;
			b.w = n->allocation().w - _theme->notebook_margin.left
					- _theme->notebook_margin.right;
			b.h = n->allocation().h - _theme->notebook_margin.top
					- _theme->notebook_margin.bottom;
			r -= b;
		} else {
			r -= n->selected()->base_position();
		}

		damaged += r;

	}


	void expose(rectangle area) {

		cairo_t * cr = cairo_create(_front_surf);

		rectangle clip = _position & area;
		if (!clip.is_null()) {
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
			cairo_set_source_surface(cr, _back_surf, 0, 0);
			cairo_fill(cr);
		}
		cairo_surface_flush(_front_surf);
		cairo_destroy(cr);

	}

	void repair_damaged(list<tree_t *> tree) {

		time_t cur;
		cur.get_time();

		cairo_t * cr = cairo_create(_back_surf);

		for (auto &i : damaged) {
			for (auto &j : tree) {
				split_t * rtree = dynamic_cast<split_t *>(j);
				if (rtree != nullptr) {
					rtree->render_legacy(cr, i);
				}
			}
		}

		for (auto &i : damaged) {
			for (auto &j : tree) {
				notebook_t * rtree = dynamic_cast<notebook_t *>(j);
				if (rtree != nullptr) {
					rtree->render_legacy(cr, i);
				}
			}
		}

		cairo_destroy(cr);

		cr = cairo_create(_front_surf);
		for (region::iterator i = damaged.begin(); i != damaged.end();
				++i) {
			rectangle clip = _position & *i;
			if (!clip.is_null()) {
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
				cairo_set_source_surface(cr, _back_surf, 0, 0);
				cairo_paint(cr);
			}
		}
		cairo_destroy(cr);

		damaged.clear();

	}

	void add_damaged(rectangle area) {
		damaged += area;
	}

};

}


#endif /* RENDERABLE_PAGE_HXX_ */
