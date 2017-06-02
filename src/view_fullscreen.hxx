/*
 * Copyright (2017) Benoit Gschwind
 *
 * view_fullscreen.hxx is part of page-compositor.
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

#ifndef SRC_VIEW_FULLSCREEN_HXX_
#define SRC_VIEW_FULLSCREEN_HXX_

#include "view_rebased.hxx"
#include "client_managed.hxx"

namespace page {

struct view_fullscreen_t : public view_rebased_t {

	viewport_w _viewport;
	managed_window_type_e revert_type;
	notebook_w revert_notebook;

	view_fullscreen_t(client_managed_p client, viewport_p viewport);
	view_fullscreen_t(view_rebased_t * src, viewport_p viewport);
	virtual ~view_fullscreen_t();

	auto shared_from_this() -> view_fullscreen_p;

	void _on_configure_request(client_managed_t * c, xcb_configure_request_event_t const * e);

	/**
	 * view_t API
	 **/

	using view_rebased_t::create_surface;
	using view_t::xxactivate;
	virtual void remove_this_view() override;
	using view_rebased_t::acquire_client;
	using view_rebased_t::release_client;
	using view_rebased_t::set_focus_state;

	/**
	 * tree_t virtual API
	 **/

	using view_t::hide;
	using view_t::show;
	//virtual auto get_node_name() const -> string;
	//virtual void remove(shared_ptr<tree_t> t);

	using view_rebased_t::update_layout;
	using view_rebased_t::render;
	virtual void reconfigure() override;
	using view_rebased_t::on_workspace_enable;
	using view_rebased_t::on_workspace_disable;

	using view_t::get_opaque_region;
	using view_t::get_visible_region;
	using view_t::get_damaged;

	virtual auto button_press(xcb_button_press_event_t const * ev) -> button_action_e override;
	//virtual bool button_release(xcb_button_release_event_t const * ev);
	//virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	//virtual bool leave(xcb_leave_notify_event_t const * ev);
	//virtual bool enter(xcb_enter_notify_event_t const * ev);
	//virtual void expose(xcb_expose_event_t const * ev);
	//virtual void trigger_redraw();

	using view_rebased_t::get_toplevel_xid;
	//virtual rect get_window_position() const;
	//virtual void queue_redraw();

};

} /* namespace page */

#endif /* SRC_VIEW_FULLSCREEN_HXX_ */
