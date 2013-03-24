/*
 * renderable_page.cxx
 *
 *  Created on: 27 déc. 2012
 *      Author: gschwind
 */



#include "renderable_page.hxx"

namespace page {

renderable_page_t::renderable_page_t(theme_t * render, cairo_surface_t * target,
		int width, int height, std::set<split_t *> & splits,
		std::set<notebook_t *> & notebooks, std::set<viewport_t *> & viewports) :
		render(render), splits(splits), notebooks(notebooks), viewports(
				viewports) {
	is_durty = true;
	default_pop = 0;

	surf = cairo_surface_create_similar(target, CAIRO_CONTENT_COLOR, width,
			height);
	cr = cairo_create(surf);

}

void renderable_page_t::mark_durty() {
	is_durty = true;
}

void renderable_page_t::render_if_needed(notebook_t * default_pop) {
	if (is_durty) {
		render_splits(splits);
		render_notebooks(notebooks);
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
			cairo_set_source_surface(cr, surf, 0.0, 0.0);
			cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
			cairo_fill(cr);
			cairo_restore(cr);
		}
		++i;
	}
}


region_t<int> renderable_page_t::get_area() {
	region_t<int> r;
	for(std::set<split_t *>::const_iterator i = splits.begin(); i != splits.end(); ++i) {
		r = r + (*i)->get_area();
	}
	for(std::set<notebook_t *>::const_iterator i = notebooks.begin(); i != notebooks.end(); ++i) {
		r = r + (*i)->get_area();
	}

	for(std::set<viewport_t *>::iterator i = viewports.begin(); i != viewports.end(); ++i) {
		if((*i)->fullscreen_client != 0) {
			r = r - (*i)->get_absolute_extend();
		}
	}

	return r;

}


void renderable_page_t::reconfigure(box_int_t const & area) {

}

bool renderable_page_t::is_visible() {
	return true;
}

void renderable_page_t::render_splits(std::set<split_t *> const & splits) {
	for (std::set<split_t *>::const_iterator i = splits.begin();
			i != splits.end(); ++i) {
		render->render_split(cr, *i);
	}
}

void renderable_page_t::render_notebooks(std::set<notebook_t *> const & notebooks) {
	for (std::set<notebook_t *>::const_iterator i = notebooks.begin();
			i != notebooks.end(); ++i) {
		render->render_notebook(cr, *i, *i == default_pop);
	}
}


}
