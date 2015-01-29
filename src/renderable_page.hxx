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

#include <memory>

#include "display.hxx"
#include "theme.hxx"

#include "region.hxx"
#include "renderable_surface.hxx"

namespace page {

/**
 * renderable_page is responsible to render page background (i.e. notebooks and background image)
 **/
class renderable_page_t {
	display_t * _cnx;
	theme_t * _theme;
	region _damaged;

	xcb_pixmap_t _pix;
	xcb_window_t _win;

	i_rect _position;

	bool _has_alpha;
	bool _is_durty;
	bool _is_visible;

	int _width;
	int _height;

	/** rendering tabs is time consumming, thus use back buffer **/
	cairo_surface_t * _back_surf;

	std::shared_ptr<renderable_surface_t> _renderable;

public:

	renderable_page_t(display_t * cnx, theme_t * theme, int width,
			int height) {
		_theme = theme;
		_is_durty = true;
		_is_visible = true;
		_has_alpha = false;

		_width = width;
		_height = height;

		_cnx = cnx;

		_win = xcb_generate_id(cnx->xcb());

		xcb_visualid_t visual = _cnx->root_visual()->visual_id;
		int depth = _cnx->root_depth();

		/** if visual is 32 bits, this values are mandatory **/
		xcb_colormap_t cmap = xcb_generate_id(_cnx->xcb());
		xcb_create_colormap(_cnx->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, _cnx->root(), visual);

		uint32_t value_mask = 0;
		uint32_t value[5];

		value_mask |= XCB_CW_BACK_PIXEL;
		value[0] = _cnx->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_BORDER_PIXEL;
		value[1] = _cnx->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_OVERRIDE_REDIRECT;
		value[2] = True;

		value_mask |= XCB_CW_EVENT_MASK;
		value[3] = XCB_EVENT_MASK_EXPOSURE;

		value_mask |= XCB_CW_COLORMAP;
		value[4] = cmap;

		_win = xcb_generate_id(_cnx->xcb());
		xcb_create_window(_cnx->xcb(), depth, _win, _cnx->root(), 0, 0, width, height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, visual, value_mask, value);
		_cnx->map(_win);

		_pix = xcb_generate_id(cnx->xcb());
		xcb_create_pixmap(cnx->xcb(), depth, _pix, _win, width, height);
		_back_surf = cairo_xcb_surface_create(_cnx->xcb(), _pix, _cnx->root_visual(), width, height);
		_position = i_rect{0, 0, width, height};
		_renderable = std::shared_ptr<renderable_surface_t>{new renderable_surface_t{_back_surf, _position}};


	}

	~renderable_page_t() {
		cout << "call " << __FUNCTION__ << endl;
		cairo_surface_destroy(_back_surf);
		xcb_free_pixmap(_cnx->xcb(), _pix);
	}

	bool repair_damaged(std::vector<tree_t *> tree) {

		if(not _is_durty)
			return false;

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
		_damaged += _position;

		return true;

	}

	std::shared_ptr<renderable_surface_t> prepare_render() {
		_renderable->clear_damaged();
		_renderable->add_damaged(_damaged);
		_damaged.clear();
		return _renderable;
	}

	/* mark renderable_page for redraw */
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

	xcb_window_t wid() {
		return _win;
	}

	void expose() {
		expose(_position);
	}

	void expose(region const & r) {
		cairo_surface_t * surf = cairo_xcb_surface_create(_cnx->xcb(), _win, _cnx->root_visual(), _width, _height);
		cairo_t * cr = cairo_create(surf);
		for(auto a: r) {
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_set_source_surface(cr, _back_surf, 0.0, 0.0);
			cairo_rectangle(cr, a.x, a.y, a.w, a.h);
			cairo_fill(cr);
		}
		cairo_destroy(cr);
		cairo_surface_destroy(surf);
	}

};

}


#endif /* RENDERABLE_PAGE_HXX_ */
