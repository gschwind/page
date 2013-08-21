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
	theme_t * render;
public:

	notebook_t * default_pop;
	managed_window_t * focused;

	renderable_page_t(xconnection_t * cnx, theme_t * render, int width,
			int height) :
			window_overlay_t(cnx, 24, box_int_t(0, 0, width, height)), render(
					render) {
		window_overlay_t::show();

		default_pop = 0;
		focused = 0;

	}

	~renderable_page_t() {

	}

	void render_splits(cairo_t * _cr, std::list<split_t *> const & splits) {
		for (std::list<split_t *>::const_iterator i = splits.begin();
				i != splits.end(); ++i) {
			render->render_split(_cr, *i);
		}
	}

	void render_notebooks(cairo_t * _cr,  std::list<notebook_t *> const & notebooks) {
		for (std::list<notebook_t *>::const_iterator i = notebooks.begin();
				i != notebooks.end(); ++i) {
			render->render_notebook(_cr, *i, focused, *i == default_pop);
		}
	}

	void set_focuced_client(managed_window_t * mw) {
		focused = mw;
	}

	void set_default_pop(notebook_t * nk) {
		default_pop = nk;
	}


	void repair_back_buffer(list<viewport_t *> & viewports) {

		if(!_is_durty)
			return;
		_is_durty = false;

		cairo_t * cr = cairo_create(_back_surf);

		list<split_t *> splits;
		list<notebook_t *> notebooks;

		for (list<viewport_t *>::iterator i = viewports.begin();
				i != viewports.end(); ++i) {
			(*i)->get_splits(splits);
			(*i)->get_notebooks(notebooks);
		}

		/* off screen render */
		cairo_reset_clip (cr);
		cairo_identity_matrix(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		render_splits(cr, splits);
		render_notebooks(cr, notebooks);

		cairo_destroy(cr);

	}

	void expose(list<viewport_t *> viewports, box_int_t area) {

		if(!_is_visible)
			return;

		cairo_xlib_surface_set_size(_front_surf, _position.w, _position.h);

		repair_back_buffer(viewports);

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

	void render_if_needed(list<viewport_t *> viewports) {
		if(_is_durty) {
			expose(viewports, _position);
		}
	}

};

}


#endif /* RENDERABLE_PAGE_HXX_ */
