/*
 * page_root.hxx
 *
 *  Created on: 12 août 2015
 *      Author: gschwind
 */

#ifndef SRC_PAGE_ROOT_HXX_
#define SRC_PAGE_ROOT_HXX_

#include "page_context.hxx"
#include "page_component.hxx"
#include "client_not_managed.hxx"
#include "client_managed.hxx"
#include "compositor_overlay.hxx"

namespace page {

class page_root_t : public tree_t {
	friend class page_t;


	page_context_t * _ctx;

	rect _root_position;

	unsigned int _current_desktop;
	vector<shared_ptr<workspace_t>> _desktop_list;

	shared_ptr<tree_t> below;
	shared_ptr<tree_t> root_subclients;
	shared_ptr<tree_t> tooltips;
	shared_ptr<tree_t> notifications;
	shared_ptr<tree_t> above;

	shared_ptr<compositor_overlay_t> _fps_overlay;
	/** store the order of last shown desktop **/
	shared_ptr<tree_t> _desktop_stack;

	shared_ptr<tree_t> _overlays;

public:

	page_root_t(page_context_t * ctx);
	~page_root_t();

	/**
	 * tree_t virtual API
	 **/

	//virtual void hide();
	//virtual void show();
	virtual auto get_node_name() const -> string;
	//virtual void remove(shared_ptr<tree_t> t);

	//virtual void append_children(vector<shared_ptr<tree_t>> & out) const;
	//virtual void update_layout(time64_t const time);
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



#endif /* SRC_PAGE_ROOT_HXX_ */
