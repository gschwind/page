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

view_rebased_t::view_rebased_t(tree_t * ref, client_managed_p client) :
	view_t{ref, client},
	_base{XCB_WINDOW_NONE},
	_colormap{XCB_NONE}
{
	_create_base_windows();
	_root->_ctx->_dpy->select_input(_base, MANAGED_BASE_WINDOW_EVENT_MASK);
}

view_rebased_t::~view_rebased_t()
{
	release_client();
	xcb_destroy_window(_root->_ctx->_dpy->xcb(), _base);
	xcb_free_colormap(_root->_ctx->_dpy->xcb(), _colormap);
	_root->_ctx->_page_windows.erase(_base);
}

auto view_rebased_t::shared_from_this() -> view_rebased_p
{
	return static_pointer_cast<view_rebased_t>(tree_t::shared_from_this());
}

void view_rebased_t::_create_base_windows()
{
	auto _dpy = _root->_ctx->_dpy;

	auto _orig_visual = _client->_client_proxy->wa().visual;
	auto _orig_depth = _client->_client_proxy->geometry().depth;

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
	if (_orig_depth == 32 and root_depth != 32) {
		_deco_visual = _orig_visual;
		_deco_depth = _orig_depth;
	} else {
		_deco_visual = _dpy->default_visual_rgba()->visual_id;
		_deco_depth = 32;
	}

	/** if visual is 32 bits, this values are mandatory **/
	_colormap = xcb_generate_id(_dpy->xcb());
	xcb_create_colormap(_dpy->xcb(), XCB_COLORMAP_ALLOC_NONE, _colormap, _dpy->root(), _deco_visual);

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

	_base = xcb_generate_id(_dpy->xcb());
	_root->_ctx->_page_windows.insert(_base);
	xcb_create_window(_dpy->xcb(), _deco_depth, _base, _dpy->root(), -10, -10,
			1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _deco_visual, value_mask,
			value);
}

void view_rebased_t::_reconfigure_windows()
{
	auto _ctx = _root->_ctx;
	auto _dpy = _ctx->_dpy;

	if (not _root->is_enable()) {
		_dpy->unmap(_base);
		return;
	}

	_client->_client_proxy->move_resize(_orig_position);

	if (_is_visible) {
		_dpy->move_resize(_base, _base_position);
		_client->_client_proxy->xmap();
		_dpy->map(_base);
		_client->fake_configure_unsafe(_client->_absolute_position);
		_client->_client_proxy->set_wm_state(NormalState);
		if(_client->_has_focus)
			_client->net_wm_state_add(_NET_WM_STATE_FOCUSED);
		else
			_client->net_wm_state_remove(_NET_WM_STATE_FOCUSED);
		if(_client_view == nullptr)
			_client_view = _root->_ctx->create_view(_base);
	} else {
		_client->_client_proxy->set_wm_state(IconicState);
		_client->net_wm_state_remove(_NET_WM_STATE_FOCUSED);
		rect hidden_position{
			_ctx->left_most_border() - 1 - _base_position.w,
			_ctx->top_most_border(),
			_base_position.w,
			_base_position.h };
		/* if iconic move outside visible area */
		_dpy->move_resize(_base, hidden_position);
		_client_view = nullptr;
		_root->_ctx->add_global_damage(get_visible_region());
	}

}

void view_rebased_t::_update_visible_region() {
	/** update visible cache **/
	_visible_region_cache = region{_base_position};
}

auto view_rebased_t::create_surface() -> client_view_p
{
	return _root->_ctx->create_view(_base);
}

void view_rebased_t::acquire_client()
{
	if(_client->current_owner_view() == static_cast<view_t*>(this))
		return;
	auto _dpy = _root->_ctx->dpy();
	_client->acquire(this);
	_dpy->reparentwindow(_client->_client_proxy->id(), _base,
			_orig_position.x, _orig_position.y);
}

void view_rebased_t::release_client()
{
	if (_client->current_owner_view() != static_cast<view_t*>(this))
		return;

	auto _ctx = _root->_ctx;
	auto _dpy = _ctx->dpy();
	_client->release(this);
	_dpy->reparentwindow(_client->_client_proxy->id(),_dpy->root(),
			_ctx->left_most_border() - 1 - _orig_position.w,
			_ctx->top_most_border());
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
	_dpy->map(_base);
	reconfigure();
	_grab_button_unfocused_unsafe();
}

void view_rebased_t::on_workspace_disable()
{
	auto _ctx = _root->_ctx;
	auto _dpy = _root->_ctx->dpy();
	release_client();
	_dpy->unmap(_base);
}

auto view_rebased_t::get_toplevel_xid() const -> xcb_window_t
{
	return _base;
}

} /* namespace page */
