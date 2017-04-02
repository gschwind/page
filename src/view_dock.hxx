/*
 * Copyright (2017) Benoit Gschwind
 *
 * view_dock.hxx is part of page-compositor.
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

#ifndef SRC_VIEW_DOCK_HXX_
#define SRC_VIEW_DOCK_HXX_

#include "page-types.hxx"
#include "view.hxx"

namespace page {

struct view_dock_t : public view_t {
	view_dock_t(tree_t * ref, client_managed_p client);
	virtual ~view_dock_t();

	auto shared_from_this() -> view_dock_p;
	void update_dock_position();

	void on_update_struct_change(client_managed_t * c);

	/**
	 * view_t API
	 **/
	using view_t::create_surface;
	using view_t::xxactivate;
	using view_t::detach;
	using view_t::acquire_client;
	using view_t::release_client;

	/**
	 * tree_t virtual API
	 **/

	using view_t::hide;
	using view_t::show;
	//virtual auto get_node_name() const -> string;
	//virtual void remove(shared_ptr<tree_t> t);

	//virtual void update_layout(time64_t const time);
	//virtual void render(cairo_t * cr, region const & area);
	virtual void reconfigure() override;
	using view_t::on_workspace_enable;
	using view_t::on_workspace_disable;

	//virtual auto get_opaque_region() -> region;
	//virtual auto get_visible_region() -> region;
	//virtual auto get_damaged() -> region;

	virtual bool button_press(xcb_button_press_event_t const * ev) override;
	//virtual bool button_release(xcb_button_release_event_t const * ev);
	//virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	//virtual bool leave(xcb_leave_notify_event_t const * ev);
	//virtual bool enter(xcb_enter_notify_event_t const * ev);
	//virtual void expose(xcb_expose_event_t const * ev);
	//virtual void trigger_redraw();

	//virtual auto get_toplevel_xid() const -> xcb_window_t;
	//virtual rect get_window_position() const;
	//virtual void queue_redraw();

};

} /* namespace page */

#endif /* SRC_VIEW_DOCK_HXX_ */