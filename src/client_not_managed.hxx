/*
 * unmanaged_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef CLIENT_NOT_MANAGED_HXX_
#define CLIENT_NOT_MANAGED_HXX_

#include <X11/Xlib.h>

#include <memory>


#include "client_base.hxx"
#include "display.hxx"

#include "renderable.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "renderable_unmanaged_gaussian_shadow.hxx"
#include "renderable_pixmap.hxx"

namespace page {

class client_not_managed_t : public client_base_t {
private:

	static unsigned long const UNMANAGED_ORIG_WINDOW_EVENT_MASK =
	XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE;

	xcb_atom_t _net_wm_type;

	mutable rect _base_position;

	mutable region _opaque_region_cache;
	mutable region _visible_region_cache;
	mutable region _damage_cache;

	shared_ptr<client_view_t> _client_view;

	/* avoid copy */
	client_not_managed_t(client_not_managed_t const &);
	client_not_managed_t & operator=(client_not_managed_t const &);

	void _update_visible_region();
	void _update_opaque_region();

public:

	client_not_managed_t(page_context_t * ctx, xcb_window_t w, xcb_atom_t type);
	~client_not_managed_t();

	auto net_wm_type() -> xcb_atom_t;

	/**
	 * tree_t virtual API
	 **/

	virtual void hide();
	virtual void show();
	virtual auto get_node_name() const -> string;
	// virtual void remove(shared_ptr<tree_t> t);

	// virtual void children(vector<shared_ptr<tree_t>> & out) const;
	virtual void update_layout(time64_t const time);
	virtual void render(cairo_t * cr, region const & area);
	virtual void render_finished();

	virtual auto get_opaque_region() -> region;
	virtual auto get_visible_region() -> region;
	virtual auto get_damaged() -> region;

	// virtual void activate();
	// virtual void activate(shared_ptr<tree_t> t);
	// virtual bool button_press(xcb_button_press_event_t const * ev);
	// virtual bool button_release(xcb_button_release_event_t const * ev);
	// virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	// virtual bool leave(xcb_leave_notify_event_t const * ev);
	// virtual bool enter(xcb_enter_notify_event_t const * ev);
	// virtual void expose(xcb_expose_event_t const * ev);
	// virtual void trigger_redraw();

	// virtual auto get_xid() const -> xcb_window_t;
	// virtual auto get_parent_xid() const -> xcb_window_t;
	// virtual rect get_window_position() const;
	// virtual void queue_redraw();

	/**
	 * client base API
	 **/

	virtual bool has_window(xcb_window_t w) const;
	virtual auto base() const -> xcb_window_t;
	virtual auto orig() const -> xcb_window_t;
	virtual auto base_position() const -> rect const &;
	virtual auto orig_position() const -> rect const &;
	virtual void on_property_notify(xcb_property_notify_event_t const * e);

};

}

#endif /* CLIENT_NOT_MANAGED_HXX_ */
