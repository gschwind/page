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
	cairo_surface_t * _surf;
	cairo_t * _cr;

	std::list<viewport_t *> & viewports;

	theme_t * render;

	unsigned int _w, _h;

	cairo_surface_t * _target;

public:

	notebook_t * default_pop;
	managed_window_t * focuced;


	renderable_page_t(theme_t * render, cairo_surface_t * target, int width, int height,
			std::list<viewport_t *> & viewports);

	~renderable_page_t();

	void mark_durty();
	void render_if_needed(notebook_t *);

	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual region_t<int> get_area();
	virtual void reconfigure(box_int_t const & area);
	virtual bool is_visible();
	virtual bool has_alpha();

	void render_splits(std::list<split_t *> const & splits);
	void render_notebooks(std::list<notebook_t *> const & notebooks);

	void rebuild_cairo();


	void set_focuced_client(managed_window_t * mw);
	void set_default_pop(notebook_t * nk);


};

}


#endif /* RENDERABLE_PAGE_HXX_ */
