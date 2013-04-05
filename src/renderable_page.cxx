/*
 * renderable_page.cxx
 *
 *  Created on: 27 déc. 2012
 *      Author: gschwind
 */



#include "renderable_page.hxx"

namespace page {

renderable_page_t::renderable_page_t(theme_t * render, cairo_surface_t * target,
		int width, int height, std::list<viewport_t *> & viewports) :
		render(render), viewports(
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

		list<split_t *> splits;
		list<notebook_t *> notebooks;

		for(list<viewport_t *>::iterator i = viewports.begin(); i != viewports.end(); ++i) {
			(*i)->get_splits(splits);
			(*i)->get_notebooks(notebooks);
		}

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

	for (std::list<viewport_t *>::iterator v = viewports.begin();
			v != viewports.end(); ++v) {
		if ((*v)->fullscreen_client == 0)
			r = r + (*v)->get_area();
	}

	return r;

}


void renderable_page_t::reconfigure(box_int_t const & area) {
	_w = area.w;
	_h = area.h;
	cairo_xlib_surface_set_size(_target, area.w, area.h);
	rebuild_cairo();
}

bool renderable_page_t::is_visible() {
	return true;
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
		render->render_notebook(_cr, *i, *i == default_pop);
	}
}


}
