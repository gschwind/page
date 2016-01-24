/*
 * popup_split.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef POPUP_SPLIT_HXX_
#define POPUP_SPLIT_HXX_

#include "split.hxx"

namespace page {

struct popup_split_t : public tree_t {
	static int const border_width = 6;

	tree_t * _parent;

	page_context_t * _ctx;
	double _current_split;

	weak_ptr<split_t> _s_base;

	rect _position;
	bool _exposed;
	bool _damaged;

	xcb_window_t _wid;

public:
	void show() {
		_is_visible = true;
		_damaged = true;
	}

	void hide() {
		_is_visible = false;
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
		if(_damaged) {
			return region{_position};
		} else {
			return region{};
		}
	}


	popup_split_t(page_context_t * ctx, shared_ptr<split_t> split) :
		_ctx{ctx},
		_s_base{split},
		_current_split{split->ratio()},
		_position{split->to_root_position(split->allocation())},
		_exposed{true}
	{

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

		update_layout();

		_ctx->dpy()->map(_wid);

	}

	~popup_split_t() {
		xcb_destroy_window(_ctx->dpy()->xcb(), _wid);
	}

	void update_layout() {

		rect rect0;
		rect rect1;

		_s_base.lock()->compute_children_allocation(_current_split, rect0, rect1);

		xcb_rectangle_t rects[8];

		rects[0].x = rect0.x;
		rects[0].y = rect0.y;
		rects[0].width = rect0.w;
		rects[0].height = border_width;

		rects[1].x = rect0.x;
		rects[1].y = rect0.y;
		rects[1].width = border_width;
		rects[1].height = rect0.h;

		rects[2].x = rect0.x;
		rects[2].y = rect0.y+rect0.h-border_width;
		rects[2].width = rect0.w;
		rects[2].height = border_width;

		rects[3].x = rect0.x+rect0.w-border_width;
		rects[3].y = rect0.y;
		rects[3].width = border_width;
		rects[3].height = rect0.h;

		rects[4].x = rect1.x;
		rects[4].y = rect1.y;
		rects[4].width = rect1.w;
		rects[4].height = border_width;

		rects[5].x = rect1.x;
		rects[5].y = rect1.y;
		rects[5].width = border_width;
		rects[5].height = rect1.h;

		rects[6].x = rect1.x;
		rects[6].y = rect1.y+rect1.h-border_width;
		rects[6].width = rect1.w;
		rects[6].height = border_width;

		rects[7].x = rect1.x+rect1.w-border_width;
		rects[7].y = rect1.y;
		rects[7].width = border_width;
		rects[7].height = rect1.h;

		/** making clip and bounding region matching make window without border **/
		xcb_shape_rectangles(_ctx->dpy()->xcb(), XCB_SHAPE_SO_SET, XCB_SHAPE_SK_BOUNDING, 0, _wid, 0, 0, 8, rects);
		xcb_shape_rectangles(_ctx->dpy()->xcb(), XCB_SHAPE_SO_SET, XCB_SHAPE_SK_CLIP, 0, _wid, 0, 0, 8, rects);

		_ctx->dpy()->move_resize(_wid, _position);

		_damaged = true;

	}

	void set_position(double pos) {
		_current_split = pos;
		update_layout();
	}

	virtual void render(cairo_t * cr, region const & area) {
		if(_s_base.expired())
			return;

		auto s = _s_base.lock();

		theme_split_t ts;
		ts.split = s->ratio();
		ts.type = s->type();
		ts.allocation = s->to_root_position(s->allocation());

		region r = area & get_visible_region();
		for (auto const & a : area.rects()) {
			cairo_save(cr);
			cairo_clip(cr, a);
			_ctx->theme()->render_popup_split(cr, &ts, _current_split);
			cairo_restore(cr);
		}

	}

	void _paint_exposed() {
		rect rect0;
		rect rect1;

		_s_base.lock()->compute_children_allocation(_current_split, rect0, rect1);

		cairo_surface_t * surf = cairo_xcb_surface_create(_ctx->dpy()->xcb(), _wid, _ctx->dpy()->root_visual(), _position.w, _position.h);
		cairo_t * cr = cairo_create(surf);

		cairo_set_source_rgb(cr, 0.0, 0.4, 0.0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(cr, 1.0);

		cairo_rectangle(cr, rect0.x+1.0, rect0.y+1.0, rect0.w-2.0, border_width-2);
		cairo_fill(cr);

		cairo_rectangle(cr, rect0.x+1.0, rect0.y+1.0, border_width-2.0, rect0.h-2.0);
		cairo_fill(cr);

		cairo_rectangle(cr, rect0.x+1.0, rect0.y+rect0.h-border_width+1.0, rect0.w-2.0, border_width-2.0);
		cairo_fill(cr);

		cairo_rectangle(cr, rect0.w-border_width+1.0, rect0.y+1.0, border_width-2.0, rect0.h-2.0);
		cairo_fill(cr);

		cairo_rectangle(cr, rect1.x+1.0, rect1.y+1.0, rect1.w-2.0, border_width-2.0);
		cairo_fill(cr);

		cairo_rectangle(cr, rect1.x+1.0, rect1.y+1.0, border_width-2.0, rect1.h-2.0);
		cairo_fill(cr);

		cairo_rectangle(cr, rect1.x+1.0, rect1.y+rect1.h-border_width+1.0, rect1.w-2.0, border_width-2.0);
		cairo_fill(cr);

		cairo_rectangle(cr, rect1.x+rect1.w-border_width+1.0, rect1.y+1.0, border_width-2.0, rect1.h-2.0);
		cairo_fill(cr);

		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);

		cairo_rectangle(cr, rect0.x+0.5, rect0.y+0.5, rect0.w-1.0, rect0.h-1.0);
		cairo_stroke(cr);

		cairo_rectangle(cr, rect0.x+border_width-0.5, rect0.y+border_width-0.5, rect0.w-2*border_width+1.0, rect0.h-2*border_width+1.0);
		cairo_stroke(cr);

		cairo_rectangle(cr, rect1.x+0.5, rect1.y+0.5, rect1.w-1.0, rect1.h-1.0);
		cairo_stroke(cr);

		cairo_rectangle(cr, rect1.x+border_width-0.5, rect1.y+border_width-0.5, rect1.w-2*border_width+1.0, rect1.h-2*border_width+1.0);
		cairo_stroke(cr);

		cairo_destroy(cr);
		cairo_surface_destroy(surf);
	}

	xcb_window_t get_xid() const {
		return _wid;
	}

	xcb_window_t get_parent_xid() const {
		return _wid;
	}

	void expose(xcb_expose_event_t const * ev) {
		if(ev->window == _wid)
			_exposed = true;
	}

	void trigger_redraw() {
		if(_exposed) {
			_exposed = false;
			_paint_exposed();
		}
	}

	void render_finished() {
		_damaged = false;
	}

};

}

#endif /* POPUP_SPLIT_HXX_ */
