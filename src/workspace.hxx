/*
 * workspace.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef DESKTOP_HXX_
#define DESKTOP_HXX_

#include <map>
#include <vector>

#include "utils.hxx"
#include "viewport.hxx"
#include "client_managed.hxx"
#include "page-types.hxx"
#include "renderable_pixmap.hxx"

namespace page {

using namespace std;

enum workspace_switch_direction_e {
	WORKSPACE_SWITCH_LEFT,
	WORKSPACE_SWITCH_RIGHT
};

struct workspace_t: public tree_t {
	page_t * _ctx;

private:

	//rect _allocation;
	rect _workarea;

	unsigned const _id;
	string _name;

	/* list of viewports in creation order, to make a sane reconfiguration */
	vector<viewport_p> _viewport_outputs;

	tree_p _viewport_layer;
	tree_p _floating_layer;
	tree_p _dock_layer;
	tree_p _fullscreen_layer;
	tree_p _tooltips_layer;
	tree_p _notification_layer;
	tree_p _overlays_layer;

	tree_p _unknown_layer;

	viewport_w _primary_viewport;
	notebook_w _default_pop;

	static time64_t const _switch_duration;

	time64_t _switch_start_time;
	shared_ptr<pixmap_t> _switch_screenshot;
	shared_ptr<renderable_pixmap_t> _switch_renderable;

	workspace_switch_direction_e _switch_direction;

	list<view_w> _client_focus_history;

	bool _is_enable;

	void _fix_view_floating_position();

public:
	view_w _net_active_window;

	xcb_atom_t A(atom_e atom);

	workspace_t(page_t * ctx, unsigned id);
	workspace_t(workspace_t const & v) = delete;
	workspace_t & operator= (workspace_t const &) = delete;

	virtual ~workspace_t();

	auto shared_from_this() -> workspace_p;

	auto get_any_viewport() const -> shared_ptr<viewport_t>;
	void set_default_pop(notebook_p n);
	int  id();
	auto primary_viewport() const -> shared_ptr<viewport_t>;
	void start_switch(workspace_switch_direction_e direction);
	void set_workarea(rect const & r);
	auto workarea() -> rect const &;
	auto get_viewports() const -> vector<shared_ptr<viewport_t>> ;
	auto ensure_default_notebook() -> notebook_p;
	auto get_viewport_map() const -> vector<shared_ptr<viewport_t>>;
	void set_primary_viewport(shared_ptr<viewport_t> v);
	void update_viewports_layout(vector<rect> const & layout);
	void remove_viewport(viewport_p v);
	void attach(shared_ptr<client_managed_t> c) __attribute__((deprecated));

	void enable(xcb_timestamp_t time);
	void disable();
	bool is_enable();

	void insert_as_popup(client_managed_p c, xcb_timestamp_t time);
	void insert_as_dock(client_managed_p c, xcb_timestamp_t time);
	void insert_as_floating(client_managed_p c, xcb_timestamp_t time);
	void insert_as_fullscreen(client_managed_p c, xcb_timestamp_t time);
	void insert_as_notebook(client_managed_p c, xcb_timestamp_t time);

	void insert_as_fullscreen(shared_ptr<client_managed_t> c, viewport_p v);

	void unfullscreen(view_fullscreen_p view, xcb_timestamp_t time);

	void switch_view_to_fullscreen(view_p v, xcb_timestamp_t time);
	void switch_view_to_floating(view_p v, xcb_timestamp_t time);
	void switch_view_to_notebook(view_p mw, xcb_timestamp_t time);

	void switch_notebook_to_floating(view_notebook_p v, xcb_timestamp_t time);
	void switch_notebook_to_fullscreen(view_notebook_p v, xcb_timestamp_t time);

	void switch_floating_to_fullscreen(view_floating_p v, xcb_timestamp_t time);
	void switch_floating_to_notebook(view_floating_p v, xcb_timestamp_t time);

	void switch_fullscreen_to_floating(view_fullscreen_p v, xcb_timestamp_t time);
	void switch_fullscreen_to_notebook(view_fullscreen_p v, xcb_timestamp_t time);

	/* switch a fullscreened and managed window into floating or notebook window */
	void switch_fullscreen_to_prefered_view_mode(view_p c, xcb_timestamp_t time);
	void switch_fullscreen_to_prefered_view_mode(view_fullscreen_p c, xcb_timestamp_t time);

	void add_dock(shared_ptr<tree_t> c);
	void add_floating(shared_ptr<tree_t> c);
	void add_fullscreen(shared_ptr<tree_t> c);
	void add_overlay(shared_ptr<tree_t> c);
	void add_unknown(shared_ptr<tree_t> c);
	void add_tooltips(shared_ptr<tree_t> c);
	void add_notification(shared_ptr<tree_t> c);

	void set_name(string const & s);
	auto name() -> string const &;
	void set_to_default_name();

	auto client_focus_history() -> list<view_w>;
	bool client_focus_history_front(view_p & out);
	void client_focus_history_remove(view_p in);
	void client_focus_history_move_front(view_p in);
	bool client_focus_history_is_empty();

	auto lookup_view_for(client_managed_p c) const -> view_p;
	void set_focus(view_p new_focus, xcb_timestamp_t tfocus);
	void unmanage(client_managed_p mw);

	auto _find_viewport_of(tree_p t) -> viewport_p;
	void _insert_view_fullscreen(view_fullscreen_p vf, xcb_timestamp_t time);
	void _insert_view_floating(view_floating_p view, xcb_timestamp_t time);

	/**
	 * tree_t virtual API
	 **/

	using tree_t::hide;
	using tree_t::show;
	virtual auto get_node_name() const -> string;
	//virtual void remove(shared_ptr<tree_t> t);

	virtual void update_layout(time64_t const time);
	virtual void render(cairo_t * cr, region const & area);
	using tree_t::reconfigure;
	using tree_t::on_workspace_enable;
	using tree_t::on_workspace_disable;

	virtual auto get_opaque_region() -> region;
	virtual auto get_visible_region() -> region;
	virtual auto get_damaged() -> region;

	//virtual auto button_press(xcb_button_press_event_t const * ev)  -> button_action_e;
	//virtual auto button_release(xcb_button_release_event_t const * ev)  -> button_action_e;
	//virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	//virtual bool leave(xcb_leave_notify_event_t const * ev);
	//virtual bool enter(xcb_enter_notify_event_t const * ev);
	//virtual void expose(xcb_expose_event_t const * ev);
	//virtual void trigger_redraw();

	//virtual auto get_toplevel_xid() const -> xcb_window_t;
	//virtual rect get_window_position() const;
	//virtual void queue_redraw();

	/**
	 * page_component_t virtual API
	 **/

//	virtual void set_allocation(rect const & area);
//	virtual rect allocation() const;
//	virtual void replace(shared_ptr<page_component_t> src, shared_ptr<page_component_t> by);

};

}

#endif /* DESKTOP_HXX_ */
