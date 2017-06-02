/*
 * Copyright (2017) Benoit Gschwind
 *
 * view_floating.hxx is part of page-compositor.
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

#ifndef SRC_VIEW_FLOATING_HXX_
#define SRC_VIEW_FLOATING_HXX_

#include "page-types.hxx"
#include "view_rebased.hxx"

namespace page {

struct view_floating_t : public view_rebased_t {
	static long const MANAGED_DECO_WINDOW_EVENT_MASK = XCB_EVENT_MASK_EXPOSURE;
	static uint32_t const DEFAULT_BUTTON_EVENT_MASK =  XCB_EVENT_MASK_BUTTON_PRESS|XCB_EVENT_MASK_BUTTON_MOTION|XCB_EVENT_MASK_BUTTON_RELEASE;

	xcb_window_t _deco;

	// the output surface (i.e. surface where we write things)
	cairo_surface_t * _surf;

	// border surface of floating window
	shared_ptr<pixmap_t> _top_buffer;
	shared_ptr<pixmap_t> _bottom_buffer;
	shared_ptr<pixmap_t> _left_buffer;
	shared_ptr<pixmap_t> _right_buffer;

	xcb_window_t _input_top;
	xcb_window_t _input_left;
	xcb_window_t _input_right;
	xcb_window_t _input_bottom;
	xcb_window_t _input_top_left;
	xcb_window_t _input_top_right;
	xcb_window_t _input_bottom_left;
	xcb_window_t _input_bottom_right;
	xcb_window_t _input_center;

	rect _area_top;
	rect _area_left;
	rect _area_right;
	rect _area_bottom;
	rect _area_top_left;
	rect _area_top_right;
	rect _area_bottom_left;
	rect _area_bottom_right;
	rect _area_center;

	struct floating_area_t {
		rect close_button;
		rect bind_button;
		rect title_button;
		rect grip_top;
		rect grip_bottom;
		rect grip_left;
		rect grip_right;
		rect grip_top_left;
		rect grip_top_right;
		rect grip_bottom_left;
		rect grip_bottom_right;
	} _floating_area;

	bool _is_resized;
	bool _is_exposed;
	bool _has_change;

	view_floating_t(tree_t * ref, client_managed_p client);
	view_floating_t(view_rebased_t * src);
	virtual ~view_floating_t();

	auto shared_from_this() -> view_floating_p;

	void _init();
	void _paint_exposed();
	void _create_back_buffer();
	void _destroy_back_buffer();
	void _update_floating_areas();
	void _compute_floating_areas();
	rect _compute_floating_close_position(rect const & allocation) const;
	rect _compute_floating_bind_position(rect const & allocation) const;
	void _update_backbuffers();
	void _reconfigure_deco_windows();
	void _reconfigure_input_windows();
	void _create_inputs_windows();
	void _create_deco_windows();
	void _map_windows_unsafe();
	void _unmap_windows_unsafe();
	void _grab_button_unsafe();
	void _ungrab_all_button_unsafe();
	void _select_inputs_unsafe();
	void _unselect_inputs_unsafe();
	void _on_client_title_change(client_managed_t * c);

	void _on_opaque_region_change(client_managed_t * c);
	void _on_focus_change(client_managed_t * c);
	void _on_configure_request(client_managed_t * c, xcb_configure_request_event_t const * e);

	/**
	 * view_t API
	 **/

	using view_rebased_t::create_surface;
	using view_t::xxactivate;
	virtual void remove_this_view() override;
	using view_rebased_t::acquire_client;
	using view_rebased_t::release_client;
	void set_focus_state(bool is_focused) override;

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

	virtual auto button_press(xcb_button_press_event_t const * ev)  -> button_action_e override;
	//virtual bool button_release(xcb_button_release_event_t const * ev);
	//virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	//virtual bool leave(xcb_leave_notify_event_t const * ev);
	//virtual bool enter(xcb_enter_notify_event_t const * ev);
	virtual void expose(xcb_expose_event_t const * ev) override;
	virtual void trigger_redraw() override;

	using view_rebased_t::get_toplevel_xid;
	//virtual rect get_window_position() const;
	virtual void queue_redraw() override;
};

} /* namespace page */

#endif /* SRC_VIEW_FLOATING_HXX_ */
