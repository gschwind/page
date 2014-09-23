/*
 * renderable_page.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef RENDERABLE_PAGE_HXX_
#define RENDERABLE_PAGE_HXX_

#include "theme.hxx"
#include "window_overlay.hxx"
#include "renderable_surface.hxx"

namespace page {

class renderable_page_t: public window_overlay_t {
	display_t * _cnx;
	theme_t * _theme;
	region damaged;

	Pixmap pix;

	/** rendering tabs is time consumming, thus use back buffer **/
	cairo_surface_t * _back_surf;

public:

	renderable_page_t(display_t * cnx, theme_t * theme, int width,
			int height) :
			window_overlay_t(rectangle(0, 0, width, height)), _theme(
					theme) {
		window_overlay_t::show();

		_cnx = cnx;
		XWindowAttributes wa;
		XGetWindowAttributes(cnx->dpy(), cnx->root(), &wa);
		pix = XCreatePixmap(cnx->dpy(), cnx->root(), width, height, 24);
		_back_surf = cairo_xlib_surface_create(cnx->dpy(), pix, wa.visual, wa.width, wa.height);


	}

	~renderable_page_t() {
		cout << "call " << __FUNCTION__ << endl;
		cairo_surface_destroy(_back_surf);
		XFreePixmap(_cnx->dpy(), pix);
	}

	void repair_damaged(list<tree_t *> tree) {

		if(damaged.empty() and not _is_durty)
			return;

		cairo_t * cr = cairo_create(_back_surf);

		cairo_rectangle(cr, _position.x, _position.y, _position.w, _position.h);
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
		cairo_fill(cr);

		rectangle area = _position;
		area.x = 0;
		area.y = 0;

		for (auto &j : tree) {
			split_t * rtree = dynamic_cast<split_t *>(j);
			if (rtree != nullptr) {
				rtree->render_legacy(cr, area);
			}
		}

		for (auto &j : tree) {
			notebook_t * rtree = dynamic_cast<notebook_t *>(j);
			if (rtree != nullptr) {
				rtree->render_legacy(cr, area);
			}
		}

		warn(cairo_get_reference_count(cr) == 1);
		cairo_destroy(cr);

		_is_durty = false;
		damaged.clear();

	}

	void add_damaged(rectangle area) {
		damaged += area;
	}

	void render(cairo_t * cr, time_t time) {
		rectangle clip = _position;
		if (!clip.is_null()) {
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
			cairo_set_source_surface(cr, _back_surf, 0, 0);
			cairo_fill(cr);
		}
	}

	renderable_t * prepare_render() {
		renderable_surface_t * surf = new renderable_surface_t(_back_surf, _position);
		return surf;
	}

};

}


#endif /* RENDERABLE_PAGE_HXX_ */
