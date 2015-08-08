/*
 * client.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 * client_base store/cache all client windows properties define in ICCCM or EWMH.
 *
 * most of them are store with pointer which is null if the properties is not set on client.
 *
 */

#ifndef CLIENT_BASE_HXX_
#define CLIENT_BASE_HXX_

#include <cassert>
#include <memory>

#include "utils.hxx"
#include "region.hxx"

#include "atoms.hxx"
#include "exception.hxx"

#include "client_properties.hxx"
#include "display.hxx"

#include "tree.hxx"
#include "page_context.hxx"

namespace page {

using namespace std;

using card32 = long;


/**
 * client_base_t handle all foreign windows, it's the base of
 * client_managed_t and client_not_managed_t.
 **/
class client_base_t : public tree_t {
protected:
	page_context_t * _ctx;

	/* handle properties of client */
	shared_ptr<client_properties_t> _properties;

	/* sub-clients */
	list<shared_ptr<tree_t>> _children;

	/** short cut **/
	auto A(atom_e atom) -> xcb_atom_t;

public:

	client_base_t(client_base_t const & c);
	client_base_t(page_context_t * ctx, shared_ptr<client_properties_t> props);

	virtual ~client_base_t();

	void read_all_properties();
	bool read_window_attributes();

	void update_wm_name();
	void update_wm_icon_name();
	void update_wm_normal_hints();
	void update_wm_hints();
	void update_wm_class();
	void update_wm_transient_for();
	void update_wm_protocols();
	void update_wm_colormap_windows();
	void update_wm_client_machine();
	void update_wm_state();

	void update_net_wm_name();
	void update_net_wm_visible_name();
	void update_net_wm_icon_name();
	void update_net_wm_visible_icon_name();
	void update_net_wm_desktop();
	void update_net_wm_window_type();
	void update_net_wm_state();
	void update_net_wm_allowed_actions();
	void update_net_wm_struct();
	void update_net_wm_struct_partial();
	void update_net_wm_icon_geometry();
	void update_net_wm_icon();
	void update_net_wm_pid();
	void update_net_wm_handled_icons();
	void update_net_wm_user_time();
	void update_net_wm_user_time_window();
	void update_net_frame_extents();
	void update_net_wm_opaque_region();
	void update_net_wm_bypass_compositor();

	void update_motif_hints();
	void update_shape();

	bool has_motif_border();

	void set_net_wm_desktop(unsigned long n);

	void add_subclient(shared_ptr<client_base_t> s);
	bool is_window(xcb_window_t w);

	xcb_atom_t wm_type();
	void print_window_attributes();
	void print_properties();

	void process_event(xcb_configure_notify_event_t const * e);
	auto cnx() const -> display_t *;
	auto wa() const -> xcb_get_window_attributes_reply_t const * ;
	auto geometry() const -> xcb_get_geometry_reply_t const *;

	/* ICCCM (read-only properties for WM) */
	auto wm_name() const -> string const *;
	auto wm_icon_name() const -> string const *;
	auto wm_normal_hints() const -> XSizeHints const *;
	auto wm_hints() const -> XWMHints const *;
	auto wm_class() const -> vector<string> const *;
	auto wm_transient_for() const -> xcb_window_t const *;
	auto wm_protocols() const -> list<xcb_atom_t> const *;
	auto wm_colormap_windows() const -> vector<xcb_window_t> const *;
	auto wm_client_machine() const -> string const *;

	/* ICCCM (read-write properties for WM) */
	auto wm_state() const -> wm_state_data_t const *;

	/* EWMH (read-only properties for WM) */
	auto net_wm_name() const -> string const *;
	auto net_wm_visible_name() const -> string const * ;
	auto net_wm_icon_name() const -> string const * ;
	auto net_wm_visible_icon_name() const -> string const * ;
	auto net_wm_window_type() const -> list<xcb_atom_t> const * ;
	auto net_wm_allowed_actions() const -> list<xcb_atom_t> const *;
	auto net_wm_strut() const -> vector<int> const *;
	auto net_wm_strut_partial() const -> vector<int> const *;
	auto net_wm_icon_geometry() const -> vector<int> const *;
	auto net_wm_icon() const -> vector<uint32_t> const *;
	auto net_wm_pid() const -> unsigned int const *;
	auto net_wm_handled_icons() const -> bool;
	auto net_wm_user_time() const -> uint32_t const *;
	auto net_wm_user_time_window() const -> xcb_window_t const *;
	auto net_wm_opaque_region() const -> vector<int> const *;
	auto net_wm_bypass_compositor() const -> unsigned int const *;

	/* EWMH (read-write properties for WM) */
	auto net_wm_desktop() const -> unsigned int const *;
	auto net_wm_state() const -> list<xcb_atom_t> const * ;
	auto net_frame_extents() const -> vector<int> const *;

	/* OTHERs */
	auto motif_hints() const -> motif_wm_hints_t const *;
	auto shape() const -> region const *;
	auto position() -> rect;


	void on_property_notify(xcb_property_notify_event_t const * e);

	/* find the bigger window that is smaller than w and h */
	dimention_t<unsigned> compute_size_with_constrain(unsigned w, unsigned h);


	/**
	 * tree_t virtual API
	 **/

	using tree_t::children;

	// virtual void hide();
	// virtual void show();
	virtual auto get_node_name() const -> string;
	virtual void remove(shared_ptr<tree_t> t);

	virtual void children(vector<shared_ptr<tree_t>> & out) const;
	// virtual void update_layout(time64_t const time);
	// virtual void render(cairo_t * cr, region const & area);

	// virtual auto get_opaque_region() -> region;
	// virtual auto get_visible_region() -> region;
	// virtual auto get_damaged() -> region;

	using tree_t::activate; // virtual void activate();
	virtual void activate(shared_ptr<tree_t> t);
	// virtual bool button_press(xcb_button_press_event_t const * ev);
	// virtual bool button_release(xcb_button_release_event_t const * ev);
	// virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	// virtual bool leave(xcb_leave_notify_event_t const * ev);
	// virtual bool enter(xcb_enter_notify_event_t const * ev);
	// virtual void expose(xcb_expose_event_t const * ev);
	// virtual void trigger_redraw();

	virtual auto get_xid() const -> xcb_window_t;
	virtual auto get_parent_xid() const -> xcb_window_t;
	// virtual rect get_window_position() const;
	// virtual void queue_redraw();

	/**
	 * client base API
	 **/

	virtual bool has_window(xcb_window_t w) const = 0;
	virtual auto base() const -> xcb_window_t = 0;
	virtual auto orig() const -> xcb_window_t = 0;
	virtual auto base_position() const -> rect const & = 0;
	virtual auto orig_position() const -> rect const & = 0;

};

}

#endif /* CLIENT_HXX_ */
