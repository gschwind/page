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

	_surf = 0;
	_cr = 0;

	_w = width;
	_h = height;
	_target = target;

	rebuild_cairo();

}

renderable_page_t::~renderable_page_t() {
	if(_cr != 0)
		cairo_destroy(_cr);
	if(_surf != 0)
		cairo_surface_destroy(_surf);
}


void renderable_page_t::rebuild_cairo() {
	if (_cr != 0)
		cairo_destroy(_cr);
	if (_surf != 0)
		cairo_surface_destroy(_surf);

	_surf = cairo_surface_create_similar(_target, CAIRO_CONTENT_COLOR, _w, _h);
	_cr = cairo_create(_surf);

}

void renderable_page_t::mark_durty() {
	is_durty = true;
}

void renderable_page_t::render_if_needed(notebook_t * default_pop) {
	if (is_durty) {
		cairo_reset_clip(_cr);
		cairo_identity_matrix(_cr);
		cairo_set_operator(_cr, CAIRO_OPERATOR_OVER);
		render_splits(splits);
		render_notebooks(notebooks);
		printf("CAIRO = %s\n", cairo_status_to_string(cairo_status(_cr)));
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
		render->render_split(_cr, *i);
	}
}

void renderable_page_t::render_notebooks(std::set<notebook_t *> const & notebooks) {
	for (std::set<notebook_t *>::const_iterator i = notebooks.begin();
			i != notebooks.end(); ++i) {
		render->render_notebook(_cr, *i, *i == default_pop);
	}
}


}
