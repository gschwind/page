/*
 * Copyright (2017) Benoit Gschwind
 *
 * view.cxx is part of page-compositor.
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

#include "view.hxx"

#include "tree.hxx"
#include "client_proxy.hxx"
#include "client_managed.hxx"
#include "workspace.hxx"
#include "page.hxx"
#include "view_floating.hxx"
#include "view_popup.hxx"

namespace page {

view_t::view_t(tree_t * ref, client_managed_p client) :
	tree_t{ref->_root},
	_client{client}
{
	//printf("create %s\n", __PRETTY_FUNCTION__);

	connect(_client->on_opaque_region_change, this, &view_t::on_opaque_region_change);
	connect(_client->on_configure_request, this, &view_t::_on_configure_request);
	_stack_is_locked = true;

	_popup = make_shared<tree_t>(_root);
	_transiant = make_shared<tree_t>(_root);

	push_back(_popup);
	push_back(_transiant);

	_client->net_wm_state_remove(_NET_WM_STATE_FOCUSED);
	//_grab_button_unfocused_unsafe();

}

view_t::~view_t()
{
	release_client();
	_popup.reset();
	_transiant.reset();
}

auto view_t::shared_from_this() -> view_p
{
	return static_pointer_cast<view_t>(tree_t::shared_from_this());
}

void view_t::_update_visible_region() {
	/** update visible cache **/
	_visible_region_cache = region{_client->_absolute_position};
}

void view_t::_update_opaque_region() {
	/** update opaque region cache **/
	_opaque_region_cache.clear();

	auto opaque_region = _client->get<p_net_wm_opaque_region>();
	if (opaque_region != nullptr) {
		_opaque_region_cache = region{*(opaque_region)};
		_opaque_region_cache &= rect{0, 0, _client->_absolute_position.w, _client->_absolute_position.h};
	} else {
		if (_client->_client_proxy->geometry().depth != 32) {
			_opaque_region_cache = rect{0, 0, _client->_absolute_position.w, _client->_absolute_position.h};
		}
	}

	if(_client->shape() != nullptr) {
		_opaque_region_cache &= *_client->shape();
		_opaque_region_cache &= rect{0, 0, _client->_absolute_position.w, _client->_absolute_position.h};
	}

	_opaque_region_cache.translate(_client->_absolute_position.x, _client->_absolute_position.y);
}

void view_t::focus(xcb_timestamp_t t) {
	_client->icccm_focus_unsafe(t);
}

void view_t::add_transient(view_floating_p v)
{
	_transiant->push_back(v);
}

void view_t::add_popup(view_popup_p v)
{
	_popup->push_back(v);
}

void view_t::move_all_window()
{
	auto _ctx = _root->_ctx;
	auto _dpy = _root->_ctx->dpy();

	if (_root->is_enable()) {

		if(_is_visible) {
			_client->map_unsafe();
			if (_client->_has_focus) {
				_client->net_wm_state_add(_NET_WM_STATE_FOCUSED);
			} else {
				_client->net_wm_state_remove(_NET_WM_STATE_FOCUSED);
			}
			_client->_client_proxy->set_wm_state(NormalState);
			if(_client_view == nullptr)
				_client_view = create_surface();
			_client->_client_proxy->move_resize(_client->_absolute_position);
			_client->fake_configure_unsafe(_client->_absolute_position);
		} else {
			_client->net_wm_state_remove(_NET_WM_STATE_FOCUSED);
			_client->_client_proxy->set_wm_state(IconicState);
			rect hidden_position{ _ctx->left_most_border() - 1 -
				_client->_absolute_position.w,
					_ctx->top_most_border(), _client->_absolute_position.w,
					_client->_absolute_position.h };
			_client_view = nullptr;
			_root->_ctx->add_global_damage(get_visible_region());
			_client->_client_proxy->move_resize(hidden_position);
			_client->fake_configure_unsafe(_client->_absolute_position);
		}

	} else {
		_client_view = nullptr;
	}
}

void view_t::on_opaque_region_change(client_managed_t * c)
{
	_update_opaque_region();
}

void view_t::_on_configure_request(client_managed_t * c, xcb_configure_request_event_t const * e)
{
	_client->_absolute_position = _client->_floating_wished_position;
	if (_is_visible)
		reconfigure();
}


auto view_t::get_popups() -> vector<view_p>
{
	return filter_class<view_t>(_popup->children());
}

auto view_t::get_transients() -> vector<view_p>
{
	return filter_class<view_t>(_transiant->children());
}

/**
 * set usual passive button grab for a focused client.
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void view_t::_grab_button_focused_unsafe() {
	auto _dpy = _root->_ctx->_dpy;

	/** First ungrab all **/
	_ungrab_all_button_unsafe();
//	/** for decoration, grab all **/
//	_client->_client_proxy->grab_button(false, DEFAULT_BUTTON_EVENT_MASK,
//			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
//			XCB_NONE, XCB_BUTTON_INDEX_1, XCB_MOD_MASK_ANY);
//	_client->_client_proxy->grab_button(false, DEFAULT_BUTTON_EVENT_MASK,
//			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
//			XCB_NONE, XCB_BUTTON_INDEX_2, XCB_MOD_MASK_ANY);
//	_client->_client_proxy->grab_button(false, DEFAULT_BUTTON_EVENT_MASK,
//			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
//			XCB_NONE, XCB_BUTTON_INDEX_3, XCB_MOD_MASK_ANY);
}

/**
 * set usual passive button grab for a not focused client.
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void view_t::_grab_button_unfocused_unsafe() {
	auto _dpy = _root->_ctx->_dpy;

	/** First ungrab all **/
	_ungrab_all_button_unsafe();

	_client->_client_proxy->grab_button(false, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_1, XCB_MOD_MASK_ANY);

	_client->_client_proxy->grab_button(false, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_2, XCB_MOD_MASK_ANY);

	_client->_client_proxy->grab_button(false, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_3, XCB_MOD_MASK_ANY);

}

/**
 * Remove all passive grab on windows
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void view_t::_ungrab_all_button_unsafe() {
	_client->_client_proxy->ungrab_button(XCB_BUTTON_INDEX_ANY, XCB_MOD_MASK_ANY);
}

auto view_t::create_surface() -> client_view_p
{
	return _client->create_surface(get_toplevel_xid());
}

void view_t::_on_focus_change(client_managed_t * c)
{
	if (_is_visible) {
		if (_client->_has_focus) {
			//_grab_button_focused_unsafe();
		} else {
			//_grab_button_unfocused_unsafe();
		}
	}
}

void view_t::xxactivate(xcb_timestamp_t time)
{
	raise();
	_root->set_focus(shared_from_this(), time);
}

void view_t::remove_this_view()
{
	assert(_parent != nullptr);
	_parent->remove(shared_from_this());
}

void view_t::acquire_client()
{
	if (_client->current_owner_view() != static_cast<view_t*>(this))
		_client->acquire(this);
}

void view_t::release_client()
{
	if (_client->current_owner_view() == this)
		_client->release(this);
}

void view_t::set_focus_state(bool is_focused)
{
	_client->_has_focus = is_focused;
	if (_client->_has_focus) {
		_client->net_wm_state_add(_NET_WM_STATE_FOCUSED);
	} else {
		_client->net_wm_state_remove(_NET_WM_STATE_FOCUSED);
	}
}

void view_t::reconfigure()
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	auto _ctx = _root->_ctx;

	_damage_cache += get_visible_region();

	move_all_window();

	_update_opaque_region();
	_update_visible_region();
	_damage_cache += get_visible_region();

}

void view_t::on_workspace_enable()
{
	acquire_client();
	reconfigure();
}

void view_t::on_workspace_disable()
{
	release_client();
}

void view_t::hide()
{
	tree_t::hide();
	reconfigure();
}

void view_t::show()
{
	tree_t::show();
	reconfigure();
}

auto view_t::get_node_name() const -> string {
	string s = _get_node_name<'M'>();
	ostringstream oss;
	oss << s << " " << _client->_client_proxy->id() << " " << _client->title();

	oss << " " << _client->_client_proxy->geometry().width << "x" <<
			_client->_client_proxy->geometry().height << "+" <<
			_client->_client_proxy->geometry().x << "+" << _client->_client_proxy->geometry().y;

	return oss.str();
}

region view_t::get_visible_region() {
	return _visible_region_cache;
}

region view_t::get_opaque_region() {
	return _opaque_region_cache;
}

region view_t::get_damaged() {
	return _damage_cache;
}

auto view_t::get_toplevel_xid() const -> xcb_window_t
{
	return _client->_client_proxy->id();
}

void view_t::update_layout(time64_t const time)
{
	if (not _is_visible)
		return;

	/** update damage_cache **/
	region dmg = _client_view->get_damaged();
	dmg.translate(_client->_absolute_position.x, _client->_absolute_position.y);
	_damage_cache += dmg;
	_client_view->clear_damaged();

}

void view_t::render(cairo_t * cr, region const & area)
{
	auto pix = _client_view->get_pixmap();

	if (pix != nullptr) {
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(cr, pix->get_cairo_surface(),
				_client->_absolute_position.x,
				_client->_absolute_position.y);
		region r = get_visible_region() & area;
		for (auto &i : r.rects()) {
			cairo_clip(cr, i);
			cairo_mask_surface(cr, pix->get_cairo_surface(),
					_client->_absolute_position.x,
					_client->_absolute_position.y);
		}
		cairo_restore(cr);
	}
}

void view_t::render_finished()
{
	_damage_cache.clear();
}


} /* namespace page */
