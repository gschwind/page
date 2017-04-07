/*
 * Copyright (2017) Benoit Gschwind
 *
 * popup_notebook0.cxx is part of page-compositor.
 *
 * page-compositor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * page-compositor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with page-compositor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "popup_notebook0.hxx"

#include "page.hxx"
#include "workspace.hxx"

namespace page {

popup_notebook0_t::popup_notebook0_t(tree_t * ref) :
	tree_t{ref->_root},
	_position{-1, -1, 1, 1} ,
	_ctx{_root->_ctx}
{
	_is_visible = false;
	_exposed = false;

	_create_window();

}

void popup_notebook0_t::_create_window() {
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
	_root->_ctx->_page_windows.insert(_wid);
}

void popup_notebook0_t::move_resize(rect const & area) {

	if(area == _position)
		return;

	if(area.w == _position.w and area.h == _position.h) {
		move(area.x, area.y);
		return;
	}

	_damaged += get_visible_region();
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
	_damaged += get_visible_region();

	rect inner(_position.x+10, _position.y+10, _position.w-20, _position.h-20);
	_visible_cache = _position;

	if (inner.w > 0 and inner.h > 0)
		_visible_cache -= inner;

	_ctx->schedule_repaint();

}

void popup_notebook0_t::move(int x, int y) {
	_damaged += get_visible_region();
	_visible_cache.translate(x - _position.x, y - _position.y);
	_position.x = x;
	_position.y = y;
	_damaged += get_visible_region();
}

rect const & popup_notebook0_t::position() {
	return _position;
}

/**
 * Derived class must return opaque region for this object,
 * If unknown it's safe to leave this empty.
 **/
region popup_notebook0_t::get_opaque_region() {
	return region{};
}

/**
 * Derived class must return visible region,
 * If unknow the whole screen can be returned, but draw will be called each time.
 **/
region popup_notebook0_t::get_visible_region() {
	return _visible_cache;
}

/**
 * return currently damaged area (absolute)
 **/
region popup_notebook0_t::get_damaged()  {
	return _damaged;
}

popup_notebook0_t::~popup_notebook0_t() {
	xcb_destroy_window(_ctx->dpy()->xcb(), _wid);
	_root->_ctx->_page_windows.erase(_wid);
}

void popup_notebook0_t::show() {
	_is_visible = true;
	_damaged = _position;
	_ctx->dpy()->map(_wid);
}

void popup_notebook0_t::hide() {
	_is_visible = false;
	_ctx->dpy()->unmap(_wid);
}

void popup_notebook0_t::render(cairo_t * cr, region const & area) {
	for (auto &a : area.rects()) {
		cairo_save(cr);
		cairo_clip(cr, a);
		cairo_translate(cr, _position.x, _position.y);
		_ctx->theme()->render_popup_notebook0(cr, (icon64*)nullptr, _position.w, _position.h, string{"none"});
		cairo_restore(cr);
	}
}

void popup_notebook0_t::_paint_exposed() {

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

xcb_window_t popup_notebook0_t::get_toplevel_xid() const {
	return _wid;
}

void popup_notebook0_t::expose(xcb_expose_event_t const * ev) {
	if(ev->window == _wid)
		_exposed = true;
}

void popup_notebook0_t::trigger_redraw() {
	if(_exposed) {
		_exposed = false;
		_paint_exposed();
	}
}

void popup_notebook0_t::render_finished() {
	_damaged.clear();
}

}


