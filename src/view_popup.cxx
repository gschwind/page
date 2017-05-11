/*
 * Copyright (2017) Benoit Gschwind
 *
 * view_popup.cxx is part of page-compositor.
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

#include <view_popup.hxx>

#include "workspace.hxx"
#include "page.hxx"

namespace page {

view_popup_t::view_popup_t(tree_t * ref, client_managed_p client) :
	view_t{ref, client}
{
	_is_visible = true;

	connect(_client->on_configure_notify, this, &view_popup_t::_on_configure_notify);
	_client->set_managed_type(MANAGED_POPUP);
	_client->_absolute_position = _client->_client_proxy->position();
	if(_client_view == nullptr)
		_client_view = create_surface();
}

view_popup_t::~view_popup_t()
{
	// TODO Auto-generated destructor stub
}

void view_popup_t::_on_configure_notify(client_managed_t * c)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	reconfigure();
}

void view_popup_t::remove_this_view()
{
	view_t::remove_this_view();
	_root->_ctx->add_global_damage(_client->_absolute_position);
}

void view_popup_t::set_focus_state(bool is_focused)
{
	_client->_has_focus = true;
}

void view_popup_t::reconfigure()
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	auto _ctx = _root->_ctx;
	auto _dpy = _root->_ctx->dpy();

	_damage_cache += get_visible_region();

	_client->_absolute_position = _client->_client_proxy->position();

//	printf("configure to %d,%d,%d,%d\n",
//			_client->_absolute_position.x,
//			_client->_absolute_position.y,
//			_client->_absolute_position.w,
//			_client->_absolute_position.h);

	if(_is_visible) {
		//_client->map_unsafe();
		if(_client_view == nullptr)
			_client_view = create_surface();
	} else {
		//_client->unmap_unsafe();
		_client_view = nullptr;
		_root->_ctx->add_global_damage(get_visible_region());
	}

	_update_opaque_region();
	_update_visible_region();
	_damage_cache += get_visible_region();

}

} /* namespace page */
