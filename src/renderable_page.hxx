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

class renderable_page_t {
	display_t * _cnx;
	theme_t * _theme;
	region _damaged;

	Pixmap _pix;

	i_rect _position;

	bool _has_alpha;
	bool _is_durty;
	bool _is_visible;

	/** rendering tabs is time consumming, thus use back buffer **/
	cairo_surface_t * _back_surf;

	ptr<renderable_surface_t> _renderable;

public:

	renderable_page_t(display_t * cnx, theme_t * theme, int width,
			int height) {
		_theme = theme;
		_is_durty = true;
		_is_visible = true;
		_has_alpha = false;

		_cnx = cnx;
		XWindowAttributes wa;
		XGetWindowAttributes(cnx->dpy(), cnx->root(), &wa);
		_pix = XCreatePixmap(cnx->dpy(), cnx->root(), width, height, wa.depth);
		_back_surf = cairo_xlib_surface_create(cnx->dpy(), _pix, wa.visual, wa.width, wa.height);
		_position = i_rect{wa.x, wa.y, wa.width, wa.height};
		_renderable = ptr<renderable_surface_t>{new renderable_surface_t{_back_surf, _position}};
	}

	~renderable_page_t() {
		cout << "call " << __FUNCTION__ << endl;
		cairo_surface_destroy(_back_surf);
		XFreePixmap(_cnx->dpy(), _pix);
	}

	void repair_damaged(list<tree_t *> tree) {

		if(_damaged.empty() and not _is_durty)
			return;

		cairo_t * cr = cairo_create(_back_surf);

		cairo_rectangle(cr, _position.x, _position.y, _position.w, _position.h);
		cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
		cairo_fill(cr);

		i_rect area = _position;
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
		_damaged.clear();

	}

	void add_damaged(i_rect area) {
		_damaged += area;
	}

	ptr<renderable_surface_t> prepare_render() {
		_renderable->clear_damaged();
		_renderable->add_damaged(_damaged);
		return _renderable;
	}

	void mark_durty() {
		_is_durty = true;
	}

	void move_resize(i_rect const & area) {
		_position = area;
	}

	void move(int x, int y) {
		_position.x = x;
		_position.y = y;
	}

	void show() {
		_is_visible = true;
	}

	void hide() {
		_is_visible = false;
	}

	bool is_visible() {
		return _is_visible;
	}

	i_rect const & position() {
		return _position;
	}

	region const & get_damaged() {
		return _damaged;
	}

};

}


#endif /* RENDERABLE_PAGE_HXX_ */
