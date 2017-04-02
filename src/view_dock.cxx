/*
 * Copyright (2017) Benoit Gschwind
 *
 * view_dock.cxx is part of page-compositor.
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

#include "view_dock.hxx"

#include "workspace.hxx"
#include "page.hxx"

namespace page {

view_dock_t::view_dock_t(tree_t * ref, client_managed_p client) :
	view_t{ref, client}
{
	//printf("create %s\n", __PRETTY_FUNCTION__);
	connect(_client->on_strut_change, this, &view_dock_t::on_update_struct_change);
}

view_dock_t::~view_dock_t()
{

}

auto view_dock_t::shared_from_this() -> view_dock_p
{
	return static_pointer_cast<view_dock_t>(tree_t::shared_from_this());
}

/*
 * Reconfigure docks.
 */
void view_dock_t::update_dock_position()
{

	/* Partial struct content definition */
	enum {
		PS_LEFT = 0,
		PS_RIGHT = 1,
		PS_TOP = 2,
		PS_BOTTOM = 3,
		PS_LEFT_START_Y = 4,
		PS_LEFT_END_Y = 5,
		PS_RIGHT_START_Y = 6,
		PS_RIGHT_END_Y = 7,
		PS_TOP_START_X = 8,
		PS_TOP_END_X = 9,
		PS_BOTTOM_START_X = 10,
		PS_BOTTOM_END_X = 11,
	};

	auto j = _client;
	auto _root_position = _root->_ctx->_root_position;

	int32_t ps[12] = { 0 };
	bool has_strut{false};

	if(j->get<p_net_wm_strut_partial>() != nullptr) {
		if(j->get<p_net_wm_strut_partial>()->size() == 12) {
			std::copy(j->get<p_net_wm_strut_partial>()->begin(), j->get<p_net_wm_strut_partial>()->end(), &ps[0]);
			has_strut = true;
		}
	}

	if (j->get<p_net_wm_strut>() != nullptr and not has_strut) {
		if(j->get<p_net_wm_strut>()->size() == 4) {

			/** if strut is found, fake strut_partial **/

			std::copy(j->get<p_net_wm_strut>()->begin(), j->get<p_net_wm_strut>()->end(), &ps[0]);

			if(ps[PS_TOP] > 0) {
				ps[PS_TOP_START_X] = _root_position.x;
				ps[PS_TOP_END_X] = _root_position.x + _root_position.w;
			}

			if(ps[PS_BOTTOM] > 0) {
				ps[PS_BOTTOM_START_X] = _root_position.x;
				ps[PS_BOTTOM_END_X] = _root_position.x + _root_position.w;
			}

			if(ps[PS_LEFT] > 0) {
				ps[PS_LEFT_START_Y] = _root_position.y;
				ps[PS_LEFT_END_Y] = _root_position.y + _root_position.h;
			}

			if(ps[PS_RIGHT] > 0) {
				ps[PS_RIGHT_START_Y] = _root_position.y;
				ps[PS_RIGHT_END_Y] = _root_position.y + _root_position.h;
			}

			has_strut = true;
		}
	}

	if (has_strut) {

		if (ps[PS_LEFT] > 0) {
			rect pos;
			pos.x = 0;
			pos.y = ps[PS_LEFT_START_Y];
			pos.w = ps[PS_LEFT];
			pos.h = ps[PS_LEFT_END_Y] - ps[PS_LEFT_START_Y] + 1;
			j->set_floating_wished_position(pos);
			return;
		}

		if (ps[PS_RIGHT] > 0) {
			rect pos;
			pos.x = _root_position.w - ps[PS_RIGHT];
			pos.y = ps[PS_RIGHT_START_Y];
			pos.w = ps[PS_RIGHT];
			pos.h = ps[PS_RIGHT_END_Y] - ps[PS_RIGHT_START_Y] + 1;
			j->set_floating_wished_position(pos);
			return;
		}

		if (ps[PS_TOP] > 0) {
			rect pos;
			pos.x = ps[PS_TOP_START_X];
			pos.y = 0;
			pos.w = ps[PS_TOP_END_X] - ps[PS_TOP_START_X] + 1;
			pos.h = ps[PS_TOP];
			j->set_floating_wished_position(pos);
			return;
		}

		if (ps[PS_BOTTOM] > 0) {
			rect pos;
			pos.x = ps[PS_BOTTOM_START_X];
			pos.y = _root_position.h - ps[PS_BOTTOM];
			pos.w = ps[PS_BOTTOM_END_X] - ps[PS_BOTTOM_START_X] + 1;
			pos.h = ps[PS_BOTTOM];
			j->set_floating_wished_position(pos);
			return;
		}
	}
}

void view_dock_t::on_update_struct_change(client_managed_t * c)
{
	reconfigure();
}

void view_dock_t::reconfigure()
{
	update_dock_position();
	_client->_absolute_position = _client->_floating_wished_position;
	view_t::reconfigure();
}

bool view_dock_t::button_press(xcb_button_press_event_t const * e) {

	if (e->event != _client->_client_proxy->id()) {
		return false;
	}

	_root->set_focus(shared_from_this(), e->time);
	return true;

}

} /* namespace page */