/*
 * Copyright (2017) Benoit Gschwind
 *
 * view_fullscreen.cxx is part of page-compositor.
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

#include "view_fullscreen.hxx"

#include "client_managed.hxx"
#include "viewport.hxx"
#include "workspace.hxx"
#include "page.hxx"
#include "grab_handlers.hxx"

namespace page {

view_fullscreen_t::view_fullscreen_t(client_managed_p client, viewport_p viewport) :
		view_rebased_t{viewport.get(), client},
		revert_type{MANAGED_FLOATING},
		_viewport{viewport}
{
	connect(_client->on_configure_request, this, &view_fullscreen_t::_on_configure_request);

	//printf("create %s\n", __PRETTY_FUNCTION__);
	_client->set_managed_type(MANAGED_FULLSCREEN);
	_client->_client_proxy->net_wm_allowed_actions_set(std::list<atom_e>{
		_NET_WM_ACTION_MOVE,
		_NET_WM_ACTION_MINIMIZE,
		_NET_WM_ACTION_SHADE,
		_NET_WM_ACTION_STICK,
		_NET_WM_ACTION_FULLSCREEN,
		_NET_WM_ACTION_CHANGE_DESKTOP,
		_NET_WM_ACTION_CLOSE
	});

	_client->net_wm_state_add(_NET_WM_STATE_FULLSCREEN);

}

view_fullscreen_t::view_fullscreen_t(view_rebased_t * src, viewport_p viewport) :
	view_rebased_t{src},
	revert_type{MANAGED_FLOATING},
	_viewport{viewport}
{
	connect(_client->on_configure_request, this, &view_fullscreen_t::_on_configure_request);

	_client->set_managed_type(MANAGED_FULLSCREEN);

	_client->_client_proxy->net_wm_allowed_actions_set(std::list<atom_e>{
		_NET_WM_ACTION_MOVE,
		_NET_WM_ACTION_MINIMIZE,
		_NET_WM_ACTION_SHADE,
		_NET_WM_ACTION_STICK,
		_NET_WM_ACTION_FULLSCREEN,
		_NET_WM_ACTION_CHANGE_DESKTOP,
		_NET_WM_ACTION_CLOSE
	});

	_client->net_wm_state_add(_NET_WM_STATE_FULLSCREEN);

}

view_fullscreen_t::~view_fullscreen_t()
{

}

auto view_fullscreen_t::shared_from_this() -> view_fullscreen_p
{
	return static_pointer_cast<view_fullscreen_t>(tree_t::shared_from_this());
}

void view_fullscreen_t::_on_configure_request(client_managed_t * c, xcb_configure_request_event_t const * e)
{
	if (_root->is_enable())
		reconfigure();
}

void view_fullscreen_t::remove_this_view()
{
	view_t::remove_this_view();
	if (not _viewport.expired()) {
		auto viewport = _viewport.lock();
		viewport->show();
		_root->_ctx->schedule_repaint();
	}
}

void view_fullscreen_t::reconfigure()
{
	auto _ctx = _root->_ctx;
	auto _dpy = _root->_ctx->dpy();

	_damage_cache += get_visible_region();

	if(not _viewport.expired())
		_client->_absolute_position = _viewport.lock()->raw_area();

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

auto view_fullscreen_t::button_press(xcb_button_press_event_t const * e) -> button_action_e
{

	if (e->event != _base->id() and e->event != _client->_client_proxy->id()) {
		return BUTTON_ACTION_CONTINUE;
	}

	if (e->detail == (XCB_BUTTON_INDEX_1) and (e->state & XCB_MOD_MASK_1)) {
		/** start moving fullscreen window **/
		_root->_ctx->grab_start(
				make_shared<grab_fullscreen_client_t>(_root->_ctx,
						shared_from_this(), e->detail, e->root_x, e->root_y),
				e->time);
		return BUTTON_ACTION_HAS_ACTIVE_GRAB;
	} else {
		_root->set_focus(shared_from_this(), e->time);
		return BUTTON_ACTION_REPLAY;
	}

	return BUTTON_ACTION_CONTINUE;
}

} /* namespace page */
