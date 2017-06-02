/*
 * Copyright (2017) Benoit Gschwind
 *
 * view_floating.cxx is part of page-compositor.
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

#include "view_floating.hxx"
#include "workspace.hxx"
#include "page.hxx"
#include "grab_handlers.hxx"

namespace page {

view_floating_t::view_floating_t(tree_t * ref, client_managed_p client) :
		view_rebased_t{ref, client},
		_deco{XCB_WINDOW_NONE},
		_surf{nullptr},
		_top_buffer{nullptr},
		_bottom_buffer{nullptr},
		_left_buffer{nullptr},
		_right_buffer{nullptr},
		_is_resized{true},
		_is_exposed{true},
		_has_change{true}
{
	_init();
}

view_floating_t::view_floating_t(view_rebased_t * src) :
	view_rebased_t{src},
	_deco{XCB_WINDOW_NONE},
	_surf{nullptr},
	_top_buffer{nullptr},
	_bottom_buffer{nullptr},
	_left_buffer{nullptr},
	_right_buffer{nullptr},
	_is_resized{true},
	_is_exposed{true},
	_has_change{true}
{
	_init();
}

view_floating_t::~view_floating_t()
{
	auto _dpy = _root->_ctx->_dpy;
	auto _xcb = _dpy->xcb();

	if (_surf != nullptr) {
		warn(cairo_surface_get_reference_count(_surf) == 1);
		cairo_surface_destroy(_surf);
		_surf = nullptr;
	}

	_destroy_back_buffer();

	xcb_destroy_window(_xcb, _input_top);
	xcb_destroy_window(_xcb, _input_left);
	xcb_destroy_window(_xcb, _input_right);
	xcb_destroy_window(_xcb, _input_bottom);
	xcb_destroy_window(_xcb, _input_top_left);
	xcb_destroy_window(_xcb, _input_top_right);
	xcb_destroy_window(_xcb, _input_bottom_left);
	xcb_destroy_window(_xcb, _input_bottom_right);
	xcb_destroy_window(_xcb, _input_center);

	xcb_destroy_window(_xcb, _deco);

	_root->_ctx->_page_windows.erase(_input_top);
	_root->_ctx->_page_windows.erase(_input_left);
	_root->_ctx->_page_windows.erase(_input_right);
	_root->_ctx->_page_windows.erase(_input_bottom);
	_root->_ctx->_page_windows.erase(_input_top_left);
	_root->_ctx->_page_windows.erase(_input_top_right);
	_root->_ctx->_page_windows.erase(_input_bottom_left);
	_root->_ctx->_page_windows.erase(_input_bottom_right);
	_root->_ctx->_page_windows.erase(_input_center);

	_root->_ctx->_page_windows.erase(_deco);

}

auto view_floating_t::shared_from_this() -> view_floating_p
{
	return static_pointer_cast<view_floating_t>(tree_t::shared_from_this());
}

void view_floating_t::_init()
{
	//printf("create %s\n", __PRETTY_FUNCTION__);
	_client->set_managed_type(MANAGED_FLOATING);
	_client->_client_proxy->net_wm_allowed_actions_set(std::list<atom_e>{
		_NET_WM_ACTION_MOVE,
		_NET_WM_ACTION_RESIZE,
		_NET_WM_ACTION_SHADE,
		_NET_WM_ACTION_STICK,
		_NET_WM_ACTION_FULLSCREEN,
		_NET_WM_ACTION_CHANGE_DESKTOP,
		_NET_WM_ACTION_CLOSE
	});

	connect(_client->on_opaque_region_change, this, &view_floating_t::_on_opaque_region_change);
	connect(_client->on_title_change, this, &view_floating_t::_on_client_title_change);
	connect(_client->on_configure_request, this, &view_floating_t::_on_configure_request);

	auto _ctx = _root->_ctx;

	_client->_floating_wished_position = rect(
			_client->_client_proxy->geometry().x,
			_client->_client_proxy->geometry().y,
			_client->_client_proxy->geometry().width,
			_client->_client_proxy->geometry().height);

	// if x == 0 then place window at center of the screen
	if (_client->_floating_wished_position.x == 0) {
		_client->_floating_wished_position.x =
				(_root->primary_viewport()->raw_area().w
						- _client->_floating_wished_position.w) / 2;
	}

	// if y == 0 then place window at center of the screen
	if (_client->_floating_wished_position.y == 0) {
		_client->_floating_wished_position.y =
				(_root->primary_viewport()->raw_area().h
						- _client->_floating_wished_position.h) / 2;
	}

	_update_floating_areas();
	_create_deco_windows();
	_create_inputs_windows();

	auto _dpy = _root->_ctx->_dpy;
	auto _xcb = _root->_ctx->_dpy->xcb();
	_surf = cairo_xcb_surface_create(_dpy->xcb(), _deco,
			_dpy->find_visual(_base->_visual),
			_client->_floating_wished_position.w,
			_client->_floating_wished_position.h);

	_grab_button_unsafe();
	xcb_flush(_root->_ctx->_dpy->xcb());

}

void view_floating_t::_paint_exposed() {
	if (not _client->prefer_window_border())
		return;

	auto _ctx = _root->_ctx;

	cairo_xcb_surface_set_size(_surf, _base_position.w, _base_position.h);

	cairo_t * _cr = cairo_create(_surf);

	/** top **/
	if (_top_buffer != nullptr) {
		cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(_cr, 0, 0, _base_position.w,
				_ctx->theme()->floating.margin.top+_ctx->theme()->floating.title_height);
		cairo_set_source_surface(_cr, _top_buffer->get_cairo_surface(), 0, 0);
		cairo_fill(_cr);
	}

	/** bottom **/
	if (_bottom_buffer != nullptr) {
		cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(_cr, 0,
				_base_position.h - _ctx->theme()->floating.margin.bottom,
				_base_position.w, _ctx->theme()->floating.margin.bottom);
		cairo_set_source_surface(_cr, _bottom_buffer->get_cairo_surface(), 0,
				_base_position.h - _ctx->theme()->floating.margin.bottom);
		cairo_fill(_cr);
	}

	/** left **/
	if (_left_buffer != nullptr) {
		cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(_cr, 0.0, _ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height,
				_ctx->theme()->floating.margin.left,
				_base_position.h - _ctx->theme()->floating.margin.top
						- _ctx->theme()->floating.margin.bottom - _ctx->theme()->floating.title_height);
		cairo_set_source_surface(_cr, _left_buffer->get_cairo_surface(), 0.0,
				_ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height);
		cairo_fill(_cr);
	}

	/** right **/
	if (_right_buffer != nullptr) {
		cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(_cr,
				_base_position.w - _ctx->theme()->floating.margin.right,
				_ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height, _ctx->theme()->floating.margin.right,
				_base_position.h - _ctx->theme()->floating.margin.top
						- _ctx->theme()->floating.margin.bottom - _ctx->theme()->floating.title_height);
		cairo_set_source_surface(_cr, _right_buffer->get_cairo_surface(),
				_base_position.w - _ctx->theme()->floating.margin.right,
				_ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height);
		cairo_fill(_cr);
	}

	cairo_surface_flush(_surf);

	warn(cairo_get_reference_count(_cr) == 1);
	cairo_destroy(_cr);

}

void view_floating_t::_create_back_buffer() {

	if (not _client->prefer_window_border()) {
		_destroy_back_buffer();
		return;
	}

	auto _ctx = _root->_ctx;

	{
		int w = _base_position.w;
		int h = _ctx->theme()->floating.margin.top
				+ _ctx->theme()->floating.title_height;
		if(w > 0 and h > 0) {
			_top_buffer =
					make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGBA, w, h);
		}
	}

	{
		int w = _base_position.w;
		int h = _ctx->theme()->floating.margin.bottom;
		if(w > 0 and h > 0) {
			_bottom_buffer =
					make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGBA, w, h);
		}
	}

	{
		int w = _ctx->theme()->floating.margin.left;
		int h = _base_position.h - _ctx->theme()->floating.margin.top
				- _ctx->theme()->floating.margin.bottom;
		if(w > 0 and h > 0) {
			_left_buffer =
					make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGBA, w, h);
		}
	}

	{
		int w = _ctx->theme()->floating.margin.right;
		int h = _base_position.h - _ctx->theme()->floating.margin.top
				- _ctx->theme()->floating.margin.bottom;
		if(w > 0 and h > 0) {
			_right_buffer =
					make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGBA, w, h);
		}
	}
}

void view_floating_t::_destroy_back_buffer()
{
	_top_buffer = nullptr;
	_bottom_buffer = nullptr;
	_left_buffer = nullptr;
	_right_buffer = nullptr;
}

void view_floating_t::_update_floating_areas() {
	auto _ctx = _root->_ctx;

	theme_managed_window_t tm;
	tm.position = _base_position;
	tm.title = _client->_title;

	margin_t ajusted = _ctx->theme()->floating.margin;

	ajusted.top += 15;
	ajusted.bottom += 15;
	ajusted.left += 15;
	ajusted.right += 15;

	if(_base_position.w < (ajusted.left + ajusted.right)) {
		ajusted.left = ajusted.right = _base_position.w/2;
	}

	if(_base_position.h < (ajusted.top + ajusted.bottom)) {
		ajusted.top = ajusted.bottom = _base_position.h/2;
	}

	int x0 = ajusted.left;
	int x1 = _base_position.w - ajusted.right;

	int y0 = ajusted.bottom;
	int y1 = _base_position.h - ajusted.bottom;

	int w0 = _base_position.w - ajusted.left - ajusted.right;
	int h0 = _base_position.h - ajusted.bottom - ajusted.bottom;

	_area_top = rect(x0, 0, w0, ajusted.bottom);
	_area_bottom = rect(x0, y1, w0, ajusted.bottom);
	_area_left = rect(0, y0, ajusted.left, h0);
	_area_right = rect(x1, y0, ajusted.right, h0);

	_area_top_left = rect(0, 0, ajusted.left, ajusted.bottom);
	_area_top_right = rect(x1, 0, ajusted.right, ajusted.bottom);
	_area_bottom_left = rect(0, y1, ajusted.left, ajusted.bottom);
	_area_bottom_right = rect(x1, y1, ajusted.right, ajusted.bottom);

	auto & m = _ctx->theme()->floating.margin;
	_area_center = rect(m.left, m.top, _base_position.w - m.left - m.right,
			_base_position.h - m.bottom - m.bottom);

	_compute_floating_areas();

}

void view_floating_t::_compute_floating_areas() {
	auto _ctx = _root->_ctx;

	rect position{0, 0, _base_position.w, _base_position.h};

	_floating_area.close_button = _compute_floating_close_position(position);
	_floating_area.bind_button = _compute_floating_bind_position(position);

	int x0 = _ctx->theme()->floating.margin.left;
	int x1 = position.w - _ctx->theme()->floating.margin.right;

	int y0 = _ctx->theme()->floating.margin.bottom;
	int y1 = position.h - _ctx->theme()->floating.margin.bottom;

	int w0 = position.w - _ctx->theme()->floating.margin.left
			- _ctx->theme()->floating.margin.right;
	int h0 = position.h - _ctx->theme()->floating.margin.bottom
			- _ctx->theme()->floating.margin.bottom;

	_floating_area.title_button = rect(x0, y0, w0, _ctx->theme()->floating.title_height);

	_floating_area.grip_top = _area_top;
	_floating_area.grip_bottom = _area_bottom;
	_floating_area.grip_left = _area_left;
	_floating_area.grip_right = _area_right;
	_floating_area.grip_top_left = _area_top_left;
	_floating_area.grip_top_right = _area_top_right;
	_floating_area.grip_bottom_left = _area_bottom_left;
	_floating_area.grip_bottom_right = _area_bottom_right;

}

rect view_floating_t::_compute_floating_close_position(rect const & allocation) const {
	auto _ctx = _root->_ctx;

	rect position;
	position.x = allocation.w - _ctx->theme()->floating.close_width
			- _ctx->theme()->floating.margin.right;
	position.y = 0.0;
	position.w = _ctx->theme()->floating.close_width;
	position.h = _ctx->theme()->floating.title_height;

	return position;
}

rect view_floating_t::_compute_floating_bind_position(rect const & allocation) const {
	auto _ctx = _root->_ctx;

	rect position;
	position.x = allocation.w - _ctx->theme()->floating.bind_width
			- _ctx->theme()->floating.close_width
			- _ctx->theme()->floating.margin.right;
	position.y = 0.0;
	position.w = _ctx->theme()->floating.bind_width;
	position.h = _ctx->theme()->floating.title_height;

	return position;
}

void view_floating_t::_update_backbuffers() {
	if(not _client->prefer_window_border())
		return;

	theme_managed_window_t fw;

	if (_bottom_buffer != nullptr) {
		fw.cairo_bottom = cairo_create(_bottom_buffer->get_cairo_surface());
	} else {
		fw.cairo_bottom = nullptr;
	}

	if (_top_buffer != nullptr) {
		fw.cairo_top = cairo_create(_top_buffer->get_cairo_surface());
	} else {
		fw.cairo_top = nullptr;
	}

	if (_right_buffer != nullptr) {
		fw.cairo_right = cairo_create(_right_buffer->get_cairo_surface());
	} else {
		fw.cairo_right = nullptr;
	}

	if (_left_buffer != nullptr) {
		fw.cairo_left = cairo_create(_left_buffer->get_cairo_surface());
	} else {
		fw.cairo_left = nullptr;
	}

	fw.focuced = _client->_has_focus;
	fw.position = _base_position;
	fw.icon = _client->icon();
	fw.title = _client->title();
	fw.demand_attention = _client->_demands_attention;

	_root->_ctx->theme()->render_floating(&fw);
}

void view_floating_t::_reconfigure_deco_windows()
{
	auto _ctx = _root->_ctx;
	auto _dpy = _ctx->_dpy;

	if (not _root->is_enable()) {
		return;
	}

	_dpy->move_resize(_deco, rect { 0, 0, _base_position.w, _base_position.h });

	if (_is_visible) {
		_map_windows_unsafe();
		_create_back_buffer();
	} else {
		_destroy_back_buffer();
	}
}

void view_floating_t::_reconfigure_input_windows()
{
	auto _dpy = _root->_ctx->_dpy;
	_dpy->move_resize(_input_top, _area_top);
	_dpy->move_resize(_input_bottom, _area_bottom);
	_dpy->move_resize(_input_right, _area_right);
	_dpy->move_resize(_input_left, _area_left);

	_dpy->move_resize(_input_top_left, _area_top_left);
	_dpy->move_resize(_input_top_right, _area_top_right);
	_dpy->move_resize(_input_bottom_left, _area_bottom_left);
	_dpy->move_resize(_input_bottom_right, _area_bottom_right);

	_dpy->move_resize(_input_center, _area_center);
}

void view_floating_t::_create_inputs_windows()
{
	auto _dpy = _root->_ctx->_dpy;
	uint32_t cursor;
	rect r{0, 0, 1, 1};

	cursor = _dpy->xc_top_side;
	_input_top = _dpy->create_input_only_window(_deco, r, XCB_CW_CURSOR, &cursor);
	cursor = _dpy->xc_bottom_side;
	_input_bottom = _dpy->create_input_only_window(_deco, r, XCB_CW_CURSOR, &cursor);
	cursor = _dpy->xc_left_side;
	_input_left = _dpy->create_input_only_window(_deco, r, XCB_CW_CURSOR, &cursor);
	cursor = _dpy->xc_right_side;
	_input_right = _dpy->create_input_only_window(_deco, r, XCB_CW_CURSOR, &cursor);

	cursor = _dpy->xc_top_left_corner;
	_input_top_left = _dpy->create_input_only_window(_deco, r, XCB_CW_CURSOR, &cursor);
	cursor = _dpy->xc_top_right_corner;
	_input_top_right = _dpy->create_input_only_window(_deco, r, XCB_CW_CURSOR, &cursor);
	cursor = _dpy->xc_bottom_left_corner;
	_input_bottom_left = _dpy->create_input_only_window(_deco, r, XCB_CW_CURSOR, &cursor);
	cursor = _dpy->xc_bottom_righ_corner;
	_input_bottom_right = _dpy->create_input_only_window(_deco, r, XCB_CW_CURSOR, &cursor);
	cursor = _dpy->xc_left_ptr;
	_input_center = _dpy->create_input_only_window(_deco, r, XCB_CW_CURSOR, &cursor);

	_root->_ctx->_page_windows.insert(_input_top);
	_root->_ctx->_page_windows.insert(_input_left);
	_root->_ctx->_page_windows.insert(_input_right);
	_root->_ctx->_page_windows.insert(_input_bottom);
	_root->_ctx->_page_windows.insert(_input_top_left);
	_root->_ctx->_page_windows.insert(_input_top_right);
	_root->_ctx->_page_windows.insert(_input_bottom_left);
	_root->_ctx->_page_windows.insert(_input_bottom_right);
	_root->_ctx->_page_windows.insert(_input_center);

}

void view_floating_t::_create_deco_windows()
{
	auto _dpy = _root->_ctx->_dpy;

	uint32_t value_mask = 0;
	uint32_t value[4];

	value_mask |= XCB_CW_BACK_PIXEL;
	value[0] = _dpy->xcb_screen()->black_pixel;

	value_mask |= XCB_CW_BORDER_PIXEL;
	value[1] = _dpy->xcb_screen()->black_pixel;

	value_mask |= XCB_CW_OVERRIDE_REDIRECT;
	value[2] = True;

	value_mask |= XCB_CW_COLORMAP;
	value[3] = _base->_colormap;

	_deco = xcb_generate_id(_dpy->xcb());
	xcb_create_window(_dpy->xcb(), _base->_depth, _deco, _base->id(), 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _base->_visual, value_mask, value);
	_root->_ctx->_page_windows.insert(_deco);

}

void view_floating_t::_map_windows_unsafe()
{
	auto _dpy = _root->_ctx->_dpy;

	_dpy->map(_input_top);
	_dpy->map(_input_left);
	_dpy->map(_input_right);
	_dpy->map(_input_bottom);

	_dpy->map(_input_top_left);
	_dpy->map(_input_top_right);
	_dpy->map(_input_bottom_left);
	_dpy->map(_input_bottom_right);

	_dpy->map(_input_center);

	_dpy->map(_deco);
	_base->_window->xmap();
}

void view_floating_t::_unmap_windows_unsafe()
{
	auto _dpy = _root->_ctx->_dpy;

	_dpy->unmap(_input_top);
	_dpy->unmap(_input_left);
	_dpy->unmap(_input_right);
	_dpy->unmap(_input_bottom);

	_dpy->unmap(_input_top_left);
	_dpy->unmap(_input_top_right);
	_dpy->unmap(_input_bottom_left);
	_dpy->unmap(_input_bottom_right);

	_dpy->unmap(_input_center);

	_base->_window->unmap();
	_dpy->unmap(_deco);
}

/**
 * set usual passive button grab for a not focused client.
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void view_floating_t::_grab_button_unsafe() {
	auto _dpy = _root->_ctx->_dpy;

	view_rebased_t::_grab_button_unsafe();

	/** for decoration, grab all **/
	xcb_grab_button(_dpy->xcb(), true, _deco, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_1, XCB_MOD_MASK_ANY);
	xcb_grab_button(_dpy->xcb(), true, _deco, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_2, XCB_MOD_MASK_ANY);
	xcb_grab_button(_dpy->xcb(), true, _deco, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_3, XCB_MOD_MASK_ANY);

}

/**
 * Remove all passive grab on windows
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void view_floating_t::_ungrab_all_button_unsafe() {
	auto _dpy = _root->_ctx->_dpy;
	xcb_ungrab_button(_dpy->xcb(), XCB_BUTTON_INDEX_ANY, _base->id(), XCB_MOD_MASK_ANY);
	xcb_ungrab_button(_dpy->xcb(), XCB_BUTTON_INDEX_ANY, _deco, XCB_MOD_MASK_ANY);
	_client->_client_proxy->ungrab_button(XCB_BUTTON_INDEX_ANY, XCB_MOD_MASK_ANY);
}

/**
 * select usual input events
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void view_floating_t::_select_inputs_unsafe() {
	auto _dpy = _root->_ctx->_dpy;
	_base->_window->select_input(MANAGED_BASE_WINDOW_EVENT_MASK);
	_dpy->select_input(_deco, MANAGED_DECO_WINDOW_EVENT_MASK);
}

/**
 * Remove all selected input event
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void view_floating_t::_unselect_inputs_unsafe() {
	auto _dpy = _root->_ctx->_dpy;
	_base->_window->select_input(XCB_EVENT_MASK_NO_EVENT);
	_dpy->select_input(_deco, XCB_EVENT_MASK_NO_EVENT);
	_client->_client_proxy->select_input(XCB_EVENT_MASK_NO_EVENT);
	_client->_client_proxy->select_input_shape(false);
}

void view_floating_t::_on_client_title_change(client_managed_t * c)
{
	_has_change = true;
}

void view_floating_t::_on_opaque_region_change(client_managed_t * c)
{
	_update_opaque_region();
}

void view_floating_t::_on_focus_change(client_managed_t * c)
{
	_has_change = true;
	region r = _base_position;
	r -= _client->_absolute_position;
	_damage_cache += r;
	_root->_ctx->schedule_repaint();

	view_rebased_t::_on_focus_change(c);
}

void view_floating_t::_on_configure_request(client_managed_t * c, xcb_configure_request_event_t const * e)
{
	if (_root->is_enable())
		reconfigure();
}

void view_floating_t::remove_this_view()
{
	view_t::remove_this_view();
	_root->_ctx->add_global_damage(_base_position);
}

void view_floating_t::set_focus_state(bool is_focused)
{
	view_rebased_t::set_focus_state(is_focused);

	_has_change = true;
	region r = _base_position;
	r -= _client->_absolute_position;
	_damage_cache += r;
	_root->_ctx->schedule_repaint();
}

void view_floating_t::reconfigure() {
	//printf("call %s\n", __PRETTY_FUNCTION__);

	auto _ctx = _root->_ctx;
	auto _dpy = _root->_ctx->dpy();

	_damage_cache += get_visible_region();

	_client->_absolute_position = _client->_floating_wished_position;

	if (_client->prefer_window_border()) {
		_base_position.x = _client->_absolute_position.x
				- _ctx->theme()->floating.margin.left;
		_base_position.y = _client->_absolute_position.y - _ctx->theme()->floating.margin.top - _ctx->theme()->floating.title_height;
		_base_position.w = _client->_absolute_position.w + _ctx->theme()->floating.margin.left
				+ _ctx->theme()->floating.margin.right;
		_base_position.h = _client->_absolute_position.h + _ctx->theme()->floating.margin.top
				+ _ctx->theme()->floating.margin.bottom + _ctx->theme()->floating.title_height;

		_orig_position.x = _ctx->theme()->floating.margin.left;
		_orig_position.y = _ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height;
		_orig_position.w = _client->_absolute_position.w;
		_orig_position.h = _client->_absolute_position.h;

	} else {
		_base_position.x = _client->_absolute_position.x;
		_base_position.y = _client->_absolute_position.y;
		_base_position.w = _client->_absolute_position.w;
		_base_position.h = _client->_absolute_position.h;

		_orig_position.x = 0;
		_orig_position.y = 0;
		_orig_position.w = _client->_absolute_position.w;
		_orig_position.h = _client->_absolute_position.h;
	}

	_is_resized = true;
	_destroy_back_buffer();
	_update_floating_areas();

	_reconfigure_windows();
	_reconfigure_deco_windows();
	_reconfigure_input_windows();

	_update_opaque_region();
	_update_visible_region();
	_damage_cache += get_visible_region();
}

auto view_floating_t::button_press(xcb_button_press_event_t const * e) -> button_action_e
{
	if (not (e->event == _deco or e->event == _base->id())) {
		return BUTTON_ACTION_CONTINUE;
	}

	auto _ctx = _root->_ctx;

	if (e->detail == XCB_BUTTON_INDEX_1 and (e->state & XCB_MOD_MASK_1)) {
		_ctx->grab_start(make_shared<grab_floating_move_t>(_ctx, dynamic_pointer_cast<view_floating_t>(shared_from_this()), e->detail, e->root_x, e->root_y), e->time);
		return BUTTON_ACTION_HAS_ACTIVE_GRAB;
	} else if (e->detail == XCB_BUTTON_INDEX_3
			and (e->state & XCB_MOD_MASK_1)) {
		_ctx->grab_start(make_shared<grab_floating_resize_t>(_ctx, dynamic_pointer_cast<view_floating_t>(shared_from_this()), e->detail, e->root_x, e->root_y, RESIZE_BOTTOM_RIGHT), e->time);
		return BUTTON_ACTION_HAS_ACTIVE_GRAB;
	} else if (e->detail == XCB_BUTTON_INDEX_1
			and e->child != _client->_client_proxy->id()
			and e->event == _deco) {

		if (_floating_area.close_button.is_inside(e->event_x, e->event_y)) {
			_client->delete_window(e->time);
			return BUTTON_ACTION_GRAB_ASYNC;
		} else if (_floating_area.bind_button.is_inside(e->event_x, e->event_y)) {
			rect absolute_position = _floating_area.bind_button;
			absolute_position.x += _base_position.x;
			absolute_position.y += _base_position.y;
			_ctx->grab_start(make_shared<grab_bind_view_floating_t>(_ctx, shared_from_this(), e->detail, absolute_position), e->time);
		} else if (_floating_area.title_button.is_inside(e->event_x, e->event_y)) {
			_ctx->grab_start(make_shared<grab_floating_move_t>(_ctx, dynamic_pointer_cast<view_floating_t>(shared_from_this()), e->detail, e->root_x, e->root_y), e->time);
		} else {
			if (_floating_area.grip_top.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(make_shared<grab_floating_resize_t>(_ctx, dynamic_pointer_cast<view_floating_t>(shared_from_this()), e->detail, e->root_x, e->root_y, RESIZE_TOP), e->time);
			} else if (_floating_area.grip_bottom.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(make_shared<grab_floating_resize_t>(_ctx, dynamic_pointer_cast<view_floating_t>(shared_from_this()), e->detail, e->root_x, e->root_y, RESIZE_BOTTOM), e->time);
			} else if (_floating_area.grip_left.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(make_shared<grab_floating_resize_t>(_ctx, dynamic_pointer_cast<view_floating_t>(shared_from_this()), e->detail, e->root_x, e->root_y, RESIZE_LEFT), e->time);
			} else if (_floating_area.grip_right.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(make_shared<grab_floating_resize_t>(_ctx, dynamic_pointer_cast<view_floating_t>(shared_from_this()), e->detail, e->root_x, e->root_y, RESIZE_RIGHT), e->time);
			} else if (_floating_area.grip_top_left.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(make_shared<grab_floating_resize_t>(_ctx, dynamic_pointer_cast<view_floating_t>(shared_from_this()), e->detail, e->root_x, e->root_y, RESIZE_TOP_LEFT), e->time);
			} else if (_floating_area.grip_top_right.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(make_shared<grab_floating_resize_t>(_ctx, dynamic_pointer_cast<view_floating_t>(shared_from_this()), e->detail, e->root_x, e->root_y, RESIZE_TOP_RIGHT), e->time);
			} else if (_floating_area.grip_bottom_left.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(make_shared<grab_floating_resize_t>(_ctx, dynamic_pointer_cast<view_floating_t>(shared_from_this()), e->detail, e->root_x, e->root_y, RESIZE_BOTTOM_LEFT), e->time);
			} else if (_floating_area.grip_bottom_right.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(make_shared<grab_floating_resize_t>(_ctx, dynamic_pointer_cast<view_floating_t>(shared_from_this()), e->detail, e->root_x, e->root_y, RESIZE_BOTTOM_RIGHT), e->time);
			} else {
				_ctx->grab_start(make_shared<grab_floating_move_t>(_ctx, dynamic_pointer_cast<view_floating_t>(shared_from_this()), e->detail, e->root_x, e->root_y), e->time);
			}
		}

		return BUTTON_ACTION_HAS_ACTIVE_GRAB;

	} else {
		_root->set_focus(dynamic_pointer_cast<view_t>(shared_from_this()), e->time);
		return BUTTON_ACTION_REPLAY;
	}

	return BUTTON_ACTION_CONTINUE;
}

void view_floating_t::expose(xcb_expose_event_t const * ev) {
	_is_exposed = true;
}

void view_floating_t::trigger_redraw()
{

	if (_is_resized) {
		_is_resized = false;
		_has_change = true;
		_create_back_buffer();
	}

	if (_has_change) {
		_has_change = false;
		_is_exposed = true;
		_update_backbuffers();
	}

	if (_is_exposed) {
		_is_exposed = false;
		_paint_exposed();
	}

}

void view_floating_t::queue_redraw() {
	_is_resized = true;
	tree_t::queue_redraw();
}

} /* namespace page */
