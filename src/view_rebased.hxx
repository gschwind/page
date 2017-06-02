/*
 * Copyright (2017) Benoit Gschwind
 *
 * view_rebased.hxx is part of page-compositor.
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

#ifndef SRC_VIEW_REBASED_HXX_
#define SRC_VIEW_REBASED_HXX_

#include "view.hxx"

namespace page {

struct view_rebased_t : public view_t {
	static long const MANAGED_BASE_WINDOW_EVENT_MASK =
			  XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
			| XCB_EVENT_MASK_STRUCTURE_NOTIFY
			| XCB_EVENT_MASK_BUTTON_PRESS
			| XCB_EVENT_MASK_BUTTON_RELEASE
			| XCB_EVENT_MASK_ENTER_WINDOW
			| XCB_EVENT_MASK_LEAVE_WINDOW;

	rect _base_position;
	rect _orig_position;

	struct _base_frame_t {
		page_t * _ctx;
		client_proxy_p _window;
		xcb_colormap_t _colormap;
		xcb_visualid_t _visual;
		uint8_t        _depth;

		auto id() const -> xcb_window_t;

		~_base_frame_t();
		_base_frame_t(page_t * ctx, xcb_visualid_t visual, uint8_t depth);

	};

	std::unique_ptr<_base_frame_t> _base;

public:
	view_rebased_t(tree_t * ref, client_managed_p client);
	view_rebased_t(view_rebased_t * src);
	virtual ~view_rebased_t();

	auto shared_from_this() -> view_rebased_p;
	void _create_base_windows();
	void _reconfigure_windows();
	void _update_visible_region();
	void _ungrab_button_unsafe();
	void _grab_button_unsafe();
	void _ungrab_all_button_unsafe();
	void _on_focus_change(client_managed_t * c);
	/**
	 * view_t API
	 **/

	virtual auto create_surface() -> client_view_p override;
	using view_t::xxactivate;
	using view_t::remove_this_view;
	virtual void acquire_client() override;
	virtual void release_client() override;
	virtual void set_focus_state(bool is_focused) override;

	/**
	 * tree_t virtual API
	 **/

	using view_t::hide;
	using view_t::show;
	using view_t::get_node_name;
	using view_t::remove;

	virtual void update_layout(time64_t const time) override;
	virtual void render(cairo_t * cr, region const & area) override;
	virtual void reconfigure() = 0;
	virtual void on_workspace_enable() override;
	virtual void on_workspace_disable() override;

	using view_t::get_opaque_region;
	using view_t::get_visible_region;
	using view_t::get_damaged;

	//virtual auto button_press(xcb_button_press_event_t const * ev) -> button_action_e override;
	//virtual bool button_release(xcb_button_release_event_t const * ev);
	//virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	//virtual bool leave(xcb_leave_notify_event_t const * ev);
	//virtual bool enter(xcb_enter_notify_event_t const * ev);
	//virtual void expose(xcb_expose_event_t const * ev);
	//virtual void trigger_redraw();

	virtual auto get_toplevel_xid() const -> xcb_window_t override;
	//virtual rect get_window_position() const;
	//virtual void queue_redraw();

};

} /* namespace page */

#endif /* SRC_VIEW_REBASED_HXX_ */
