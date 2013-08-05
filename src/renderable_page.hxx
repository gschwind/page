/*
 * renderable_page.hxx
 *
 *  Created on: 27 déc. 2012
 *      Author: gschwind
 */

#ifndef RENDERABLE_PAGE_HXX_
#define RENDERABLE_PAGE_HXX_

#include "default_theme.hxx"
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

	bool is_visible() {
		return true;
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
		XWindowAttributes const * wa = _cnx->get_window_attributes(_wid);
		assert(wa != 0);

		cairo_t * cr = cairo_create(_back_surf);

		list<split_t *> splits;
		list<notebook_t *> notebooks;

		for (list<viewport_t *>::iterator i = viewports.begin();
				i != viewports.end(); ++i) {
			if(*i == 0)
				continue;
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

	void expose (list<viewport_t *> viewports) {
		XWindowAttributes const * wa = _cnx->get_window_attributes(_wid);
		assert(wa != 0);

		if(_back_surf == 0) {
			create_back_buffer();
			repair_back_buffer(viewports);
		}

		cairo_surface_t * front_surf = cairo_xlib_surface_create(_cnx->dpy, _wid, wa->visual, wa->width, wa->height);
		cairo_t * cr = cairo_create(front_surf);

		box_int_t clip = box_int_t(wa->x, wa->y, wa->width, wa->height);
		if (!clip.is_null()) {
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
			cairo_set_source_surface(cr, _back_surf, 0, 0);
			cairo_paint(cr);
		}

		cairo_destroy(cr);
		cairo_surface_destroy(front_surf);

	}



};

}


#endif /* RENDERABLE_PAGE_HXX_ */
