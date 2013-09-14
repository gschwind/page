/*
 * renderable_page.hxx
 *
 *  Created on: 27 déc. 2012
 *      Author: gschwind
 */

#ifndef RENDERABLE_PAGE_HXX_
#define RENDERABLE_PAGE_HXX_

#include "theme.hxx"
#include "window_overlay.hxx"

namespace page {

class renderable_page_t: public window_overlay_t {
	theme_t * _theme;
	region_t<int> damaged;
public:

	renderable_page_t(xconnection_t * cnx, theme_t * render, int width,
			int height) :
			window_overlay_t(cnx, 24, box_int_t(0, 0, width, height)), _theme(
					render) {
		window_overlay_t::show();

	}

	~renderable_page_t() {

	}

	void repair_notebook_border(notebook_t * n) {
		region_t<int> r = n->allocation();

		if (n->selected() == 0) {

			box_int_t b;
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


	void expose(box_int_t area) {

		cairo_t * cr = cairo_create(_front_surf);

		box_int_t clip = _position & area;
		if (!clip.is_null()) {
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
			cairo_set_source_surface(cr, _back_surf, 0, 0);
			cairo_paint(cr);
		}

		cairo_destroy(cr);

	}

	void repair_damaged(vector<tree_t *> tree) {

		cairo_t * cr = cairo_create(_back_surf);

		for (region_t<int>::iterator i = damaged.begin(); i != damaged.end();
				++i) {
			for (vector<tree_t *>::iterator j = tree.begin(); j != tree.end();
					++j) {
				tree_renderable_t * rtree =
						dynamic_cast<tree_renderable_t *>(*j);
				page_assert(rtree != 0);
				rtree->render(cr, *i);
			}

		}

		cairo_destroy(cr);

		cr = cairo_create(_front_surf);
		for (region_t<int>::iterator i = damaged.begin(); i != damaged.end();
				++i) {
			box_int_t clip = _position & *i;
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

	void add_damaged(box_t<int> area) {
		damaged += area;
	}

};

}


#endif /* RENDERABLE_PAGE_HXX_ */
