/*
 * viewport.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef VIEWPORT_HXX_
#define VIEWPORT_HXX_

#include <memory>
#include <vector>

#include "renderable.hxx"
#include "split.hxx"
#include "theme.hxx"
#include "page_context.hxx"
#include "page_component.hxx"
#include "notebook.hxx"

namespace page {

class viewport_t: public page_component_t {

	static uint32_t const DEFAULT_BUTTON_EVENT_MASK = XCB_EVENT_MASK_BUTTON_PRESS|XCB_EVENT_MASK_BUTTON_RELEASE|XCB_EVENT_MASK_BUTTON_MOTION|XCB_EVENT_MASK_POINTER_MOTION;

	page_context_t * _ctx;

	region _damaged;

	xcb_pixmap_t _pix;
	xcb_window_t _win;

	bool _is_durty;
	bool _exposed;

	/** rendering tabs is time consuming, thus use back buffer **/
	shared_ptr<pixmap_t> _back_surf;

	renderable_pixmap_t * _renderable;

	/** area without considering dock windows **/
	rect _raw_aera;

	/** area considering dock windows **/
	rect _effective_area;
	rect _page_area;

	shared_ptr<page_component_t> _subtree;

	viewport_t(viewport_t const & v);
	viewport_t & operator= (viewport_t const &);

public:

	viewport_t(page_context_t * ctx, rect const & area, bool keep_focus);

	~viewport_t() {
		std::cout << "call " << __FUNCTION__ << std::endl;
		destroy_renderable();
		xcb_destroy_window(_ctx->dpy()->xcb(), _win);
		_win = XCB_NONE;
	}

	virtual void replace(page_component_t * src, page_component_t * by);
	virtual void remove(shared_ptr<tree_t> const & src);

	notebook_t * get_nearest_notebook();

	virtual void set_allocation(rect const & area);

	void set_raw_area(rect const & area);
	void set_effective_area(rect const & area);

	bool is_visible() {
		return _is_visible;
	}

	void activate(weak_ptr<tree_t> t) {
		if(not t.expired()) {
			if(t.lock() != _subtree) {
				throw exception_t("invalid call of viewport_t::activate");
			}
		}

		queue_redraw();

		if(not _parent.expired()) {
			_parent.lock()->activate(shared_from_this());
		}

	}

	virtual std::string get_node_name() const {
		return _get_node_name<'V'>();
	}

	virtual void update_layout(time64_t const time) {
		if(not _is_visible)
			return;
		if(_renderable == nullptr)
			update_renderable();
		_renderable->clear_damaged();
		_renderable->add_damaged(_damaged);
		_damaged.clear();
	}

	rect allocation() const {
		return _effective_area;
	}

	rect const & page_area() const {
		return _page_area;
	}

	void render_legacy(cairo_t * cr, rect const & area) const { }

	rect const & raw_area() const;

	void children(std::vector<weak_ptr<tree_t>> & out) const {
		//out.push_back(_renderable);
		if(_subtree != nullptr) {
			out.push_back(_subtree);
		}
	}

	void hide() {
		_is_visible = false;
		_ctx->dpy()->unmap(_win);
		destroy_renderable();
	}

	void show() {
		_is_visible = true;
		_ctx->dpy()->map(_win);
		update_renderable();
	}

	rect const & raw_area() {
		return _raw_aera;
	}

	void destroy_renderable() {

		delete _renderable;
		_renderable = nullptr;

		if(_pix != XCB_NONE) {
			xcb_free_pixmap(_ctx->dpy()->xcb(), _pix);
			_pix = XCB_NONE;
		}

		if(_back_surf != nullptr) {
			_back_surf.reset();
		}

	}

	void update_renderable() {
		if(_ctx->cmp() != nullptr) {
			_back_surf = _ctx->cmp()->create_composite_pixmap(_page_area.w, _page_area.h);
			_renderable = new renderable_pixmap_t{_back_surf, _effective_area, _effective_area};
			_renderable->set_parent(this);
		}

		_ctx->dpy()->move_resize(_win, _effective_area);
	}

	void create_window() {
		_win = xcb_generate_id(_ctx->dpy()->xcb());

		xcb_visualid_t visual = _ctx->dpy()->root_visual()->visual_id;
		int depth = _ctx->dpy()->root_depth();

		/** if visual is 32 bits, this values are mandatory **/
		xcb_colormap_t cmap = xcb_generate_id(_ctx->dpy()->xcb());
		xcb_create_colormap(_ctx->dpy()->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, _ctx->dpy()->root(), visual);

		uint32_t value_mask = 0;
		uint32_t value[5];

		value_mask |= XCB_CW_BACK_PIXEL;
		value[0] = _ctx->dpy()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_BORDER_PIXEL;
		value[1] = _ctx->dpy()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_OVERRIDE_REDIRECT;
		value[2] = True;

		value_mask |= XCB_CW_EVENT_MASK;
		value[3] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_LEAVE_WINDOW;

		value_mask |= XCB_CW_COLORMAP;
		value[4] = cmap;

		_win = xcb_generate_id(_ctx->dpy()->xcb());
		xcb_create_window(_ctx->dpy()->xcb(), depth, _win, _ctx->dpy()->root(), _effective_area.x, _effective_area.y, _effective_area.w, _effective_area.h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, visual, value_mask, value);

		_ctx->dpy()->set_window_cursor(_win, _ctx->dpy()->xc_left_ptr);

		/**
		 * This grab will freeze input for all client, all mouse button, until
		 * we choose what to do with them with XAllowEvents. we can choose to keep
		 * grabbing events or release event and allow further processing by other clients.
		 **/
		xcb_grab_button(_ctx->dpy()->xcb(), false, _win,
				DEFAULT_BUTTON_EVENT_MASK,
				XCB_GRAB_MODE_SYNC,
				XCB_GRAB_MODE_ASYNC,
				XCB_NONE,
				XCB_NONE,
				XCB_BUTTON_INDEX_ANY,
				XCB_MOD_MASK_ANY);

		_pix = XCB_NONE;

	}

	void _redraw_back_buffer() {
		if(_back_surf == nullptr)
			return;

		if(not _is_durty)
			return;

		cairo_t * cr = cairo_create(_back_surf->get_cairo_surface());
		cairo_identity_matrix(cr);

		auto splits = filter_class_lock<split_t>(tree_t::get_all_children());
		for (auto x : splits) {
			x->render_legacy(cr);
		}

		auto notebooks = filter_class_lock<notebook_t>(tree_t::get_all_children());
		for (auto x : notebooks) {
			x->render_legacy(cr);
		}

		cairo_surface_flush(_back_surf->get_cairo_surface());
		warn(cairo_get_reference_count(cr) == 1);
		cairo_destroy(cr);

		_is_durty = false;
		_exposed = true;
		_damaged += _page_area;

		/* tell page to render */
		_ctx->add_global_damage(_effective_area);

	}

	void trigger_redraw() {
		/** redraw all child **/
		tree_t::trigger_redraw();
		_redraw_back_buffer();

		if(_exposed and _ctx->cmp() == nullptr) {
			_exposed = false;
			___expose();
		}

	}

	/* mark renderable_page for redraw */
	void queue_redraw() {
		_is_durty = true;
	}

	region const & xxx_get_damaged() {
		return _damaged;
	}

	xcb_window_t wid() {
		return _win;
	}

	xcb_window_t get_xid() {
		return _win;
	}

	void ___expose() {
		if(not _is_visible)
			return;

		cairo_surface_t * surf = cairo_xcb_surface_create(_ctx->dpy()->xcb(), _win, _ctx->dpy()->root_visual(), _effective_area.w, _effective_area.h);
		cairo_t * cr = cairo_create(surf);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_surface(cr, _back_surf->get_cairo_surface(), 0.0, 0.0);
		cairo_rectangle(cr, 0.0, 0.0, _effective_area.w, _effective_area.h);
		cairo_fill(cr);

		cairo_destroy(cr);
		cairo_surface_destroy(surf);
	}

	xcb_window_t get_window() const {
		return _win;
	}

	rect get_window_position() const {
		return _effective_area;
	}

	void expose(xcb_expose_event_t const * e) {
		if(e->window == _win) {
			_exposed = true;
		}
	}

};

}

#endif /* VIEWPORT_HXX_ */
