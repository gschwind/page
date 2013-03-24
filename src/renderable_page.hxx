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

namespace page {


class renderable_page_t : public renderable_t {
	bool is_durty;

public:

	theme_t * render;

	std::set<split_t *> & splits;
	std::set<notebook_t *> & notebooks;
	std::set<viewport_t *> & viewports;

	notebook_t * default_pop;

	cairo_surface_t * surf;
	cairo_t * cr;

	renderable_page_t(theme_t * render, cairo_surface_t * target, int width,
			int height, std::set<split_t *> & splits,
			std::set<notebook_t *> & notebooks,
			std::set<viewport_t *> & viewports);

	void mark_durty();
	void render_if_needed(notebook_t *);

	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual region_t<int> get_area();
	virtual void reconfigure(box_int_t const & area);
	virtual bool is_visible();

	void render_splits(std::set<split_t *> const & splits);
	void render_notebooks(std::set<notebook_t *> const & notebooks);

};

}


#endif /* RENDERABLE_PAGE_HXX_ */
