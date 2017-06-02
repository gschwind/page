/*
 * Copyright (2017) Benoit Gschwind
 *
 * view_notebook.hxx is part of page-compositor.
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

#ifndef SRC_VIEW_NOTEBOOK_HXX_
#define SRC_VIEW_NOTEBOOK_HXX_

#include "page-types.hxx"

#include "view_rebased.hxx"
#include "icon_handler.hxx"

namespace page {

struct view_notebook_t : public view_rebased_t {

	view_notebook_t(tree_t * ref, client_managed_p client);
	view_notebook_t(view_rebased_t * src);
	virtual ~view_notebook_t();

	void _create_base_windows();
	void _reconfigure_base_windows();

	auto shared_from_this() -> view_notebook_p;
	bool is_iconic() const;
	bool has_focus() const;
	auto title() const -> string const &;
	auto icon() const -> shared_ptr<icon16>;
	void delete_window(xcb_timestamp_t t);

	auto parent_notebook() -> notebook_p;

	void _on_configure_request(client_managed_t * c, xcb_configure_request_event_t const * e);

	/**
	 * view_t virtual API
	 **/

	using view_rebased_t::create_surface;
	virtual void xxactivate(xcb_timestamp_t time) override;
	virtual void remove_this_view() override;
	using view_rebased_t::acquire_client;
	using view_rebased_t::release_client;
	virtual void set_focus_state(bool is_focused) override;


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

#endif /* SRC_VIEW_NOTEBOOK_HXX_ */
