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

class renderable_page_t : public window_overlay_t {

	bool is_durty;

	map<RRCrtc, viewport_t *> & viewports;

	theme_t * render;

public:

	Window const & wid;

	notebook_t * default_pop;
	managed_window_t * focused;


	renderable_page_t(xconnection_t * cnx, theme_t * render, int width, int height,
			map<RRCrtc, viewport_t *> & viewports);

	~renderable_page_t();

	void mark_durty();
	void render_if_needed(notebook_t *);

	region_t<int> get_area();
	bool is_visible();
	bool has_alpha();

	void render_splits(std::list<split_t *> const & splits);
	void render_notebooks(std::list<notebook_t *> const & notebooks);

	void set_focuced_client(managed_window_t * mw);
	void set_default_pop(notebook_t * nk);


};

}


#endif /* RENDERABLE_PAGE_HXX_ */
