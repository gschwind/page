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
		viewport{viewport}
{
	//printf("create %s\n", __PRETTY_FUNCTION__);
	_client->net_wm_state_add(_NET_WM_STATE_FULLSCREEN);
	_client->set_managed_type(MANAGED_FULLSCREEN);
}

view_fullscreen_t::~view_fullscreen_t()
{

}

auto view_fullscreen_t::shared_from_this() -> view_fullscreen_p
{
	return static_pointer_cast<view_fullscreen_t>(tree_t::shared_from_this());
}

void view_fullscreen_t::reconfigure()
{
	auto _ctx = _root->_ctx;
	auto _dpy = _root->_ctx->dpy();

	_damage_cache += get_visible_region();

	if(not viewport.expired())
		_client->_absolute_position = viewport.lock()->raw_area();

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

bool view_fullscreen_t::button_press(xcb_button_press_event_t const * e) {

	if (e->event != _base and e->event != _client->_client_proxy->id()) {
		return false;
	}

	if (e->detail == (XCB_BUTTON_INDEX_1) and (e->state & XCB_MOD_MASK_1)) {
		/** start moving fullscreen window **/
		_root->_ctx->grab_start(make_shared<grab_fullscreen_client_t>(_root->_ctx, shared_from_this(), e->detail, e->root_x, e->root_y), e->time);
		return true;
	} else {
		_root->set_focus(shared_from_this(), e->time);
		return true;
	}
	return true;
}

} /* namespace page */