/*
 * desktop.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef DESKTOP_HXX_
#define DESKTOP_HXX_

#include <X11/extensions/Xrandr.h>

#include <map>
#include <vector>

#include "utils.hxx"
#include "page_context.hxx"
#include "viewport.hxx"
#include "client_managed.hxx"
#include "client_not_managed.hxx"
#include "renderable_pixmap.hxx"

namespace page {

using namespace std;

enum workspace_switch_direction_e {
	WORKSPACE_SWITCH_LEFT,
	WORKSPACE_SWITCH_RIGHT
};

struct workspace_t: public tree_t {

private:
	page_context_t * _ctx;

	//rect _allocation;
	rect _workarea;

	unsigned const _id;

	/* list of viewports in creation order, to make a sane reconfiguration */
	vector<shared_ptr<viewport_t>> _viewport_outputs;


	list<shared_ptr<tree_t>> _viewport_layer;

	/* floating, dock belong this layer */
	list<shared_ptr<tree_t>> _floating_layer;
	list<shared_ptr<tree_t>> _fullscreen_layer;


	weak_ptr<viewport_t> _primary_viewport;
	weak_ptr<notebook_t> _default_pop;

	workspace_t(workspace_t const & v);
	workspace_t & operator= (workspace_t const &);

	static time64_t const _switch_duration;

	time64_t _switch_start_time;
	shared_ptr<pixmap_t> _switch_screenshot;
	shared_ptr<renderable_pixmap_t> _switch_renderable;

	workspace_switch_direction_e _switch_direction;

	list<weak_ptr<client_managed_t>> _client_focus_history;

public:

	workspace_t(page_context_t * ctx, unsigned id);
	~workspace_t() { }


	auto get_any_viewport() const -> shared_ptr<viewport_t>;
	void set_default_pop(shared_ptr<notebook_t> n);
	auto default_pop() -> shared_ptr<notebook_t>;
	int  id();
	auto primary_viewport() const -> shared_ptr<viewport_t>;
	void start_switch(workspace_switch_direction_e direction);
	void set_workarea(rect const & r);
	auto workarea() -> rect const &;
	auto get_viewports() const -> vector<shared_ptr<viewport_t>> ;
	void update_default_pop();
	auto get_viewport_map() const -> vector<shared_ptr<viewport_t>>;
	void set_primary_viewport(shared_ptr<viewport_t> v);
	auto set_layout(vector<shared_ptr<viewport_t>> const & new_layout) -> void;
	void attach(shared_ptr<client_managed_t> c);

	bool client_focus_history_front(shared_ptr<client_managed_t> & out);
	void client_focus_history_remove(shared_ptr<client_managed_t> in);
	void client_focus_history_move_front(shared_ptr<client_managed_t> in);
	bool client_focus_history_is_empty();

	/**
	 * tree_t virtual API
	 **/

	virtual void hide();
	virtual void show();
	virtual auto get_node_name() const -> string;
	virtual void remove(shared_ptr<tree_t> t);

	virtual void append_children(vector<shared_ptr<tree_t>> & out) const;
	virtual void update_layout(time64_t const time);
	virtual void render(cairo_t * cr, region const & area);

	virtual auto get_opaque_region() -> region;
	virtual auto get_visible_region() -> region;
	virtual auto get_damaged() -> region;

	virtual void activate();
	virtual void activate(shared_ptr<tree_t> t);
	//virtual bool button_press(xcb_button_press_event_t const * ev);
	//virtual bool button_release(xcb_button_release_event_t const * ev);
	//virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	//virtual bool leave(xcb_leave_notify_event_t const * ev);
	//virtual bool enter(xcb_enter_notify_event_t const * ev);
	//virtual void expose(xcb_expose_event_t const * ev);
	//virtual void trigger_redraw();

	//virtual auto get_xid() const -> xcb_window_t;
	//virtual auto get_parent_xid() const -> xcb_window_t;
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
