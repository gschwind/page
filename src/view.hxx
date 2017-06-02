/*
 * Copyright (2017) Benoit Gschwind
 *
 * view.hxx is part of page-compositor.
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

#ifndef SRC_VIEW_HXX_
#define SRC_VIEW_HXX_

#include <typeinfo>

#include "tree.hxx"
#include "region.hxx"
#include "page-types.hxx"

namespace page {

struct view_t : public tree_t {
	static uint32_t const DEFAULT_BUTTON_EVENT_MASK =  XCB_EVENT_MASK_BUTTON_PRESS|XCB_EVENT_MASK_BUTTON_MOTION|XCB_EVENT_MASK_BUTTON_RELEASE;

	client_managed_p _client;
	client_view_p _client_view;

	mutable region _opaque_region_cache;
	mutable region _visible_region_cache;
	mutable region _damage_cache;

	tree_p _popup;
	tree_p _transiant;

	view_t(tree_t * ref, client_managed_p client);
	virtual ~view_t();

	auto shared_from_this() -> view_p;

	void _update_visible_region();
	void _update_opaque_region();

	void focus(xcb_timestamp_t t);
	void add_transient(view_floating_p v);
	void add_popup(view_popup_p v);
	void move_all_window();

	void on_opaque_region_change(client_managed_t * c);
	void _on_configure_request(client_managed_t * c, xcb_configure_request_event_t const * e);

	auto get_popups() -> vector<view_p>;
	auto get_transients() -> vector<view_p>;

	void _grab_button_focused_unsafe();
	void _grab_button_unfocused_unsafe();
	void _ungrab_all_button_unsafe();

	void _on_focus_change(client_managed_t * c);

	/**
	 * view_t API
	 **/

	virtual auto create_surface() -> client_view_p;
	virtual void xxactivate(xcb_timestamp_t time);
	virtual void remove_this_view();
	virtual void acquire_client();
	virtual void release_client();
	virtual void set_focus_state(bool is_focused);

	/**
	 * tree_t API
	 **/

	virtual void hide() override;
	virtual void show() override;
	virtual auto get_node_name() const -> string;
	// virtual void remove(shared_ptr<tree_t> t);

	// virtual void children(vector<shared_ptr<tree_t>> & out) const;
	virtual void update_layout(time64_t const time) override;
	virtual void render(cairo_t * cr, region const & area) override;
	virtual void render_finished() override;
	virtual void reconfigure() override;
	virtual void on_workspace_enable() override;
	virtual void on_workspace_disable() override;

	virtual auto get_opaque_region() -> region override;
	virtual auto get_visible_region() -> region override;
	virtual auto get_damaged() -> region override;

	// virtual auto button_press(xcb_button_press_event_t const * ev)  -> button_action_e;
	// virtual auto button_release(xcb_button_release_event_t const * ev)  -> button_action_e;
	// virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	// virtual bool leave(xcb_leave_notify_event_t const * ev);
	// virtual bool enter(xcb_enter_notify_event_t const * ev);
	// virtual void expose(xcb_expose_event_t const * ev);

	virtual auto get_toplevel_xid() const -> xcb_window_t override;
	// virtual rect get_window_position() const;

	//virtual void queue_redraw();
	//virtual void trigger_redraw();


};

} /* namespace page */

#endif /* SRC_VIEW_HXX_ */
