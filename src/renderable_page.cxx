/*
 * renderable_page.cxx
 *
 *  Created on: 27 déc. 2012
 *      Author: gschwind
 */



#include "renderable_page.hxx"

namespace page {

renderable_page_t::renderable_page_t(xconnection_t * cnx, theme_t * render, cairo_surface_t * target,
		int width, int height, std::list<viewport_t *> & viewports) : window_overlay_t(cnx, 24, box_int_t(0, 0, width, height)), wid(_wid),
		render(render), viewports(
				viewports) {
	window_overlay_t::show();

	is_durty = true;
	default_pop = 0;
	focused = 0;

	mark_durty();

}

renderable_page_t::~renderable_page_t() {

}

void renderable_page_t::mark_durty() {
	is_durty = true;
}

void renderable_page_t::render_if_needed(notebook_t * default_pop) {
	if (is_durty) {

		list<split_t *> splits;
		list<notebook_t *> notebooks;

		for(list<viewport_t *>::iterator i = viewports.begin(); i != viewports.end(); ++i) {
			(*i)->get_splits(splits);
			(*i)->get_notebooks(notebooks);
		}

		/* off screen render */
		cairo_reset_clip(_cr);
		cairo_identity_matrix(_cr);
		cairo_set_operator(_cr, CAIRO_OPERATOR_OVER);
		render_splits(splits);
		render_notebooks(notebooks);
		printf("CAIRO = %s\n", cairo_status_to_string(cairo_status(_cr)));

		cairo_surface_flush(_surf);

		/* blit result to visible output */

		repair(_area);

		is_durty = false;
	}
}

void renderable_page_t::repair1(cairo_t * cr, box_int_t const & area) {
	region_t<int> r = get_area();
	//std::cout << r.to_string() << std::endl;
	region_t<int>::box_list_t::const_iterator i = r.begin();
	while(i != r.end()) {
		box_int_t clip = *i & area;
		if (!clip.is_null()) {
			//std::cout << "yyy: " << clip.to_string() << std::endl;
			cairo_save(cr);
			cairo_reset_clip(cr);
			cairo_identity_matrix(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_set_source_surface(cr, _surf, 0.0, 0.0);
			cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
			cairo_fill(cr);
			cairo_restore(cr);
		}
		++i;
	}
}



region_t<int> renderable_page_t::get_area() {
	region_t<int> r;

	for (std::list<viewport_t *>::iterator v = viewports.begin();
			v != viewports.end(); ++v) {
		if ((*v)->fullscreen_client == 0)
			r = r + (*v)->get_area();
	}

	return r;

}

bool renderable_page_t::is_visible() {
	return true;
}

bool renderable_page_t::has_alpha() {
	return false;
}

void renderable_page_t::render_splits(std::list<split_t *> const & splits) {
	for (std::list<split_t *>::const_iterator i = splits.begin();
			i != splits.end(); ++i) {
		render->render_split(_cr, *i);
	}
}

void renderable_page_t::render_notebooks(std::list<notebook_t *> const & notebooks) {
	for (std::list<notebook_t *>::const_iterator i = notebooks.begin();
			i != notebooks.end(); ++i) {
		render->render_notebook(_cr, *i, focused, *i == default_pop);
	}
}

void renderable_page_t::set_focuced_client(managed_window_t * mw) {
	focused = mw;
}

void renderable_page_t::set_default_pop(notebook_t * nk) {
	default_pop = nk;
}


}
