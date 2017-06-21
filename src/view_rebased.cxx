/*
 * Copyright (2017) Benoit Gschwind
 *
 * view_rebased.cxx is part of page-compositor.
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

#include "view_rebased.hxx"

#include "page.hxx"
#include "workspace.hxx"

namespace page {

auto view_rebased_t::_base_frame_t::id() const -> xcb_window_t { return _window->id(); }

view_rebased_t::_base_frame_t::~_base_frame_t()
{
	xcb_destroy_window(_ctx->_dpy->xcb(), _window->id());
	xcb_free_colormap(_ctx->_dpy->xcb(), _colormap);
	_ctx->_page_windows.erase(_window->id());
}


view_rebased_t::_base_frame_t::_base_frame_t(page_t * ctx, xcb_visualid_t visual, uint8_t depth) :
	_ctx{ctx}
{

	auto _dpy = _ctx->_dpy;
	/**
	 * Create the base window, window that will content managed window
	 **/

	xcb_visualid_t root_visual = _dpy->root_visual()->visual_id;
	int root_depth = _dpy->find_visual_depth(_dpy->root_visual()->visual_id);

	/**
	 * If window visual is 32 bit (have alpha channel, and root do not
	 * have alpha channel, use the window visual, otherwise always prefer
	 * root visual.
	 **/
	if (depth == 32 and root_depth != 32) {
		_visual = visual;
		_depth = depth;
	} else {
		_visual = _dpy->default_visual_rgba()->visual_id;
		_depth = 32;
	}

	/** if visual is 32 bits, this values are mandatory **/
	_colormap = xcb_generate_id(_dpy->xcb());
	xcb_create_colormap(_dpy->xcb(), XCB_COLORMAP_ALLOC_NONE, _colormap, _dpy->root(), _visual);

	uint32_t value_mask = 0;
	uint32_t value[4];

	value_mask |= XCB_CW_BACK_PIXEL;
	value[0] = _dpy->xcb_screen()->black_pixel;

	value_mask |= XCB_CW_BORDER_PIXEL;
	value[1] = _dpy->xcb_screen()->black_pixel;

	value_mask |= XCB_CW_OVERRIDE_REDIRECT;
	value[2] = True;

	value_mask |= XCB_CW_COLORMAP;
	value[3] = _colormap;

	xcb_window_t base = xcb_generate_id(_dpy->xcb());
	_ctx->_page_windows.insert(base);
	xcb_create_window(_dpy->xcb(), _depth, base, _dpy->root(), -10, -10,
			1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _visual, value_mask,
			value);
	_window = _dpy->ensure_client_proxy(base);
}


view_rebased_t::view_rebased_t(tree_t * ref, client_managed_p client) :
	view_t{ref, client}
{
	_client->_client_proxy->set_border_width(0);
	_base = std::unique_ptr<_base_frame_t>{new _base_frame_t(_root->_ctx, _client->_client_proxy->visualid(), _client->_client_proxy->visual_depth())};
	_base->_window->select_input(MANAGED_BASE_WINDOW_EVENT_MASK);
	_grab_button_unsafe();
	xcb_flush(_root->_ctx->_dpy->xcb());
}

view_rebased_t::view_rebased_t(view_rebased_t * src) :
	view_t{src->_root, src->_client},
	_base{std::move(src->_base)}, // Take the ownership
	_base_position{src->_base_position},
	_orig_position{src->_orig_position}
{

}

view_rebased_t::~view_rebased_t()
{
	release_client();
}

auto view_rebased_t::shared_from_this() -> view_rebased_p
{
	return static_pointer_cast<view_rebased_t>(tree_t::shared_from_this());
}

void view_rebased_t::_reconfigure_windows()
{
	auto _ctx = _root->_ctx;
	auto _dpy = _ctx->_dpy;

	if (not _root->is_enable()) {
		_client_view = nullptr;
		if (_client->current_owner_view() == static_cast<view_t*>(this))
			_base->_window->unmap();
		return;
	}

	if (_is_visible) {
		_client->_client_proxy->set_wm_state(NormalState);
		if(_client->_has_focus)
			_client->net_wm_state_add(_NET_WM_STATE_FOCUSED);
		else
			_client->net_wm_state_remove(_NET_WM_STATE_FOCUSED);
		if(_client_view == nullptr)
			_client_view = create_surface();
		_base->_window->move_resize(_base_position);
		_client->_client_proxy->xmap();
		_base->_window->xmap();
		_client->_client_proxy->move_resize(_orig_position);
		_client->fake_configure_unsafe(_client->_absolute_position);
	} else {
		_client->_client_proxy->set_wm_state(IconicState);
		_client->net_wm_state_remove(_NET_WM_STATE_FOCUSED);
		rect hidden_position{
			_ctx->left_most_border() - 1 - _base_position.w,
			_ctx->top_most_border(),
			_base_position.w,
			_base_position.h };
		/* if iconic move outside visible area */
		_base->_window->move_resize(hidden_position);
		_client_view = nullptr;
		_root->_ctx->add_global_damage(get_visible_region());
		_client->_client_proxy->move_resize(_orig_position);
		_client->fake_configure_unsafe(_client->_absolute_position);
	}

}

void view_rebased_t::_update_visible_region() {
	/** update visible cache **/
	_visible_region_cache = region{_base_position};
}

void view_rebased_t::_ungrab_button_unsafe()
{
	auto _dpy = _root->_ctx->_dpy;

	/** First ungrab all **/
	_ungrab_all_button_unsafe();

//	/** grab alt-button1 move **/
//	_base->_window->grab_button(true, DEFAULT_BUTTON_EVENT_MASK,
//			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
//			XCB_NONE, XCB_BUTTON_INDEX_1, XCB_MOD_MASK_1/*ALT*/);
//
//	/** grab alt-button3 resize **/
//	_base->_window->grab_button(true, DEFAULT_BUTTON_EVENT_MASK,
//			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
//			XCB_NONE, XCB_BUTTON_INDEX_3, XCB_MOD_MASK_1/*ALT*/);

	_base->_window->grab_button(true, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_1, XCB_MOD_MASK_ANY);

	_base->_window->grab_button(true, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_2, XCB_MOD_MASK_ANY);

	_base->_window->grab_button(true, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_3, XCB_MOD_MASK_ANY);

}

void view_rebased_t::_grab_button_unsafe() {
	auto _dpy = _root->_ctx->_dpy;

	/** First ungrab all **/
	_ungrab_all_button_unsafe();

	_base->_window->grab_button(true, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_ANY, XCB_MOD_MASK_ANY);

}

void view_rebased_t::_ungrab_all_button_unsafe() {
	auto _dpy = _root->_ctx->_dpy;
	_base->_window->ungrab_button(XCB_BUTTON_INDEX_ANY, XCB_MOD_MASK_ANY);
	_client->_client_proxy->ungrab_button(XCB_BUTTON_INDEX_ANY, XCB_MOD_MASK_ANY);
}

void view_rebased_t::_on_focus_change(client_managed_t * c)
{
	if (_client->_has_focus) {
		_client->net_wm_state_add(_NET_WM_STATE_FOCUSED);
		_ungrab_button_unsafe();
	} else {
		_client->net_wm_state_remove(_NET_WM_STATE_FOCUSED);
		_grab_button_unsafe();
	}
}

auto view_rebased_t::create_surface() -> client_view_p
{
	return _client->create_surface(_base->id());
}

void view_rebased_t::acquire_client()
{
	if(_client->current_owner_view() == static_cast<view_t*>(this))
		return;
	auto _dpy = _root->_ctx->dpy();
	_client->acquire(this);
	_dpy->reparentwindow(_client->_client_proxy->id(), _base->id(),
			_orig_position.x, _orig_position.y);
}

void view_rebased_t::release_client()
{
	_client_view = nullptr;

	if (_client->current_owner_view() != static_cast<view_t*>(this))
		return;

	auto _ctx = _root->_ctx;
	auto _dpy = _ctx->dpy();
	_client->release(this);
	_dpy->reparentwindow(_client->_client_proxy->id(),_dpy->root(),
			_ctx->left_most_border() - 1 - _orig_position.w,
			_ctx->top_most_border());
}

void view_rebased_t::set_focus_state(bool is_focused)
{
	view_t::set_focus_state(is_focused);
	if (_client->_has_focus) {
		_ungrab_button_unsafe();
	} else {
		_grab_button_unsafe();
	}
}

void view_rebased_t::update_layout(time64_t const time)
{
	if (not _is_visible)
		return;

	/** update damage_cache **/
	region dmg = _client_view->get_damaged();
	dmg.translate(_base_position.x, _base_position.y);
	_damage_cache += dmg;
	_client_view->clear_damaged();
}

void view_rebased_t::render(cairo_t * cr, region const & area)
{
	auto pix = _client_view->get_pixmap();
	if(pix == nullptr)
		return;

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(cr, pix->get_cairo_surface(),
			_base_position.x, _base_position.y);
	region r = get_visible_region() & area;
	for (auto &i : r.rects()) {
		cairo_clip(cr, i);
		cairo_mask_surface(cr, pix->get_cairo_surface(),
				_base_position.x, _base_position.y);
	}
	cairo_restore(cr);
}

void view_rebased_t::on_workspace_enable()
{
	auto _ctx = _root->_ctx;
	auto _dpy = _root->_ctx->dpy();
	acquire_client();
	_base->_window->xmap();
	reconfigure();
	_grab_button_unsafe();
}

void view_rebased_t::on_workspace_disable()
{
	auto _ctx = _root->_ctx;
	auto _dpy = _root->_ctx->dpy();
	release_client();
	if (_client->current_owner_view() == static_cast<view_t*>(this))
		_base->_window->unmap();
}

auto view_rebased_t::get_toplevel_xid() const -> xcb_window_t
{
	return _base->id();
}

} /* namespace page */
