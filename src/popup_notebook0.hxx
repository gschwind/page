/*
 * popup_notebook0.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef POPUP_NOTEBOOK0_HXX_
#define POPUP_NOTEBOOK0_HXX_

#include "theme.hxx"
#include "renderable.hxx"
#include "icon_handler.hxx"

namespace page {

using namespace std;

struct popup_notebook0_t : public tree_t {
	static int const border_width = 6;

	page_context_t * _ctx;
	shared_ptr<icon64> icon;
	string title;
	bool _show;

protected:
	rect _position;

	bool _has_alpha;
	bool _is_durty;
	bool _is_visible;

	xcb_window_t _wid;

public:
	popup_notebook0_t(page_context_t * ctx) :
			_position{-1, -1, 1, 1} , _ctx{ctx} {
		icon = nullptr;
		_show = false;
		icon = nullptr;
		_has_alpha = true;
		_is_durty = true;
		_is_visible = false;

		/** if visual is 32 bits, this values are mandatory **/
		xcb_colormap_t cmap = xcb_generate_id(_ctx->dpy()->xcb());
		xcb_create_colormap(_ctx->dpy()->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, _ctx->dpy()->root(), _ctx->dpy()->root_visual()->visual_id);

		uint32_t value_mask = 0;
		uint32_t value[5];

		value_mask |= XCB_CW_BACK_PIXEL;
		value[0] = _ctx->dpy()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_BORDER_PIXEL;
		value[1] = _ctx->dpy()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_OVERRIDE_REDIRECT;
		value[2] = True;

		value_mask |= XCB_CW_EVENT_MASK;
		value[3] = XCB_EVENT_MASK_EXPOSURE;

		value_mask |= XCB_CW_COLORMAP;
		value[4] = cmap;

		_wid = xcb_generate_id(_ctx->dpy()->xcb());
		xcb_create_window(_ctx->dpy()->xcb(), _ctx->dpy()->root_depth(), _wid, _ctx->dpy()->root(), _position.x, _position.y, _position.w, _position.h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _ctx->dpy()->root_visual()->visual_id, value_mask, value);

		_ctx->dpy()->map(_wid);

	}


	void mark_durty() {
		_is_durty = true;
	}

	void move_resize(rect const & area) {
		_ctx->add_global_damage(get_visible_region());
		_position = area;

		xcb_rectangle_t rects[4];

		rects[0].x = 0;
		rects[0].y = 0;
		rects[0].width = _position.w;
		rects[0].height = border_width;

		rects[1].x = 0;
		rects[1].y = 0;
		rects[1].width = border_width;
		rects[1].height = _position.h;

		rects[2].x = 0;
		rects[2].y = _position.h-border_width;
		rects[2].width = _position.w;
		rects[2].height = border_width;

		rects[3].x = _position.w-border_width;
		rects[3].y = 0;
		rects[3].width = border_width;
		rects[3].height = _position.h;

		/** making clip and bounding region matching make window without border **/
		xcb_shape_rectangles(_ctx->dpy()->xcb(), XCB_SHAPE_SO_SET, XCB_SHAPE_SK_BOUNDING, 0, _wid, 0, 0, 4, rects);
		xcb_shape_rectangles(_ctx->dpy()->xcb(), XCB_SHAPE_SO_SET, XCB_SHAPE_SK_CLIP, 0, _wid, 0, 0, 4, rects);

		_ctx->dpy()->move_resize(_wid, area);
		expose();
		_ctx->add_global_damage(get_visible_region());
	}

	void move(int x, int y) {
		_position.x = x;
		_position.y = y;
	}

	rect const & position() {
		return _position;
	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return region{};
	}

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() {
		return region{_position};
	}

	/**
	 * return currently damaged area (absolute)
	 **/
	virtual region get_damaged()  {
		if(_is_durty) {
			return region{_position};
		} else {
			return region{};
		}
	}

	~popup_notebook0_t() {
		xcb_destroy_window(_ctx->dpy()->xcb(), _wid);
	}

	void update_window(shared_ptr<client_managed_t> c) {
		icon = make_shared<icon64>(c);
		this->title = c->title();
	}

	void show() {
		_is_visible = true;
		_show = true;
	}

	void hide() {
		_is_visible = false;
		_show = false;
	}

	bool is_visible() {
		return _show;
	}

	virtual void render(cairo_t * cr, region const & area) {

		if(not _is_visible)
			return;

		for (auto &a : area) {
			cairo_save(cr);
			cairo_clip(cr, a);
			cairo_translate(cr, _position.x, _position.y);
			_ctx->theme()->render_popup_notebook0(cr, icon.get(), _position.w, _position.h, title);
			cairo_restore(cr);
		}
	}

	void expose() {

		cairo_surface_t * surf = cairo_xcb_surface_create(_ctx->dpy()->xcb(), _wid, _ctx->dpy()->root_visual(), _position.w, _position.h);
		cairo_t * cr = cairo_create(surf);

		cairo_set_source_rgb(cr, 0.0, 0.4, 0.0);

		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(cr, 1.0);
		cairo_rectangle(cr, 0, 0, _position.w, border_width);
		cairo_fill(cr);

		cairo_rectangle(cr, 0, 0, border_width, _position.h);
		cairo_fill(cr);

		cairo_rectangle(cr, 0, _position.h-border_width, _position.w, border_width);
		cairo_fill(cr);

		cairo_rectangle(cr, _position.w-border_width, 0, border_width, _position.h);
		cairo_fill(cr);

		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		cairo_rectangle(cr, 0.5, 0.5, _position.w-1.0, _position.h-1.0);
		cairo_stroke(cr);

		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		cairo_rectangle(cr, border_width-0.5, border_width-0.5, _position.w-2*border_width+1.0, _position.h-2*border_width+1.0);
		cairo_stroke(cr);

		cairo_destroy(cr);
		cairo_surface_destroy(surf);

	}

	xcb_window_t id() const {
		return _wid;
	}

};

}


#endif /* POPUP_NOTEBOOK0_HXX_ */
