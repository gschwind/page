/*
 * Copyright (2017) Benoit Gschwind
 *
 * view_notebook.cxx is part of page-compositor.
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

#include "view_notebook.hxx"

#include "view.hxx"
#include "client_managed.hxx"
#include "workspace.hxx"
#include "page.hxx"
#include "grab_handlers.hxx"

namespace page {

view_notebook_t::view_notebook_t(tree_t * ref, client_managed_p client) :
	view_rebased_t{ref, client}
{

	connect(_client->on_configure_request, this, &view_notebook_t::_on_configure_request);

	_client->set_managed_type(MANAGED_NOTEBOOK);
	_client->_client_proxy->net_wm_allowed_actions_set(std::list<atom_e>{
		_NET_WM_ACTION_MOVE,
		_NET_WM_ACTION_MINIMIZE,
		_NET_WM_ACTION_SHADE,
		_NET_WM_ACTION_STICK,
		_NET_WM_ACTION_FULLSCREEN,
		_NET_WM_ACTION_CHANGE_DESKTOP,
		_NET_WM_ACTION_CLOSE
	});
}

view_notebook_t::view_notebook_t(view_rebased_t * src) :
	view_rebased_t{src}
{
	connect(_client->on_configure_request, this, &view_notebook_t::_on_configure_request);


	_client->set_managed_type(MANAGED_NOTEBOOK);
	_client->_client_proxy->net_wm_allowed_actions_set(std::list<atom_e>{
		_NET_WM_ACTION_MOVE,
		_NET_WM_ACTION_MINIMIZE,
		_NET_WM_ACTION_SHADE,
		_NET_WM_ACTION_STICK,
		_NET_WM_ACTION_FULLSCREEN,
		_NET_WM_ACTION_CHANGE_DESKTOP,
		_NET_WM_ACTION_CLOSE
	});
}

view_notebook_t::~view_notebook_t()
{

}

auto view_notebook_t::shared_from_this() -> view_notebook_p
{
	return static_pointer_cast<view_notebook_t>(tree_t::shared_from_this());
}

bool view_notebook_t::is_iconic() const
{
	return not _is_visible;
}

bool view_notebook_t::has_focus() const
{
	return _client->_has_focus;
}

auto view_notebook_t::title() const -> string const &
{
	return _client->title();
}

auto view_notebook_t::icon() const -> shared_ptr<icon16>
{
	return _client->icon();
}

void view_notebook_t::delete_window(xcb_timestamp_t t) {
	printf("request close for '%s'\n", title().c_str());
	_client->delete_window(t);
}

auto view_notebook_t::parent_notebook() -> notebook_p
{
	assert((_parent != nullptr) and (_parent->parent() != nullptr));
	return dynamic_pointer_cast<notebook_t>(_parent->parent()->shared_from_this());
}

void view_notebook_t::_on_configure_request(client_managed_t * c, xcb_configure_request_event_t const * e)
{
	if(_root->is_enable())
		reconfigure();
}

void view_notebook_t::xxactivate(xcb_timestamp_t time)
{
	auto nbk = parent_notebook();
	nbk->activate(shared_from_this(), time);
}

void view_notebook_t::remove_this_view()
{
	auto nbk = parent_notebook();
	nbk->remove_view_notebook(shared_from_this());
}

void view_notebook_t::set_focus_state(bool is_focused)
{
	view_rebased_t::set_focus_state(is_focused);
	parent_notebook()->_client_focus_change(_client.get());
}

void view_notebook_t::reconfigure()
{
	auto _ctx = _root->_ctx;
	auto _dpy = _root->_ctx->dpy();

	_damage_cache += get_visible_region();

	_base_position.x = _client->_absolute_position.x;
	_base_position.y = _client->_absolute_position.y;
	_base_position.w = _client->_absolute_position.w;
	_base_position.h = _client->_absolute_position.h;

	_orig_position.x = 0;
	_orig_position.y = 0;
	_orig_position.w = _client->_absolute_position.w;
	_orig_position.h = _client->_absolute_position.h;

	_reconfigure_windows();

	_update_opaque_region();
	_update_visible_region();
	_damage_cache += get_visible_region();
}

auto view_notebook_t::button_press(xcb_button_press_event_t const * e) -> button_action_e {

	if (e->event != _base->id() and e->event != _client->_client_proxy->id()) {
		return BUTTON_ACTION_CONTINUE;
	}

	if (e->detail == (XCB_BUTTON_INDEX_3) and (e->state & (XCB_MOD_MASK_1))) {
		_root->_ctx->grab_start(make_shared<grab_bind_view_notebook_t>(_root->_ctx, shared_from_this(), e->detail, rect{e->root_x-10, e->root_y-10, 20, 20}), e->time);
		return BUTTON_ACTION_HAS_ACTIVE_GRAB;
	} else {
		_root->set_focus(shared_from_this(), e->time);
		return BUTTON_ACTION_REPLAY;
	}

	return BUTTON_ACTION_CONTINUE;
}

} /* namespace page */
