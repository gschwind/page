/*
 * client_properties.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef CLIENT_PROPERTIES_HXX_
#define CLIENT_PROPERTIES_HXX_

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <xcb/xcb.h>

#include <limits>
#include <utility>
#include <vector>
#include <list>
#include <iostream>
#include <set>
#include <algorithm>


#include "utils.hxx"
#include "region.hxx"
#include "display.hxx"
#include "motif_hints.hxx"

#include "properties.hxx"

namespace page {

using namespace std;

class client_proxy_t {
private:
	display_t *                  _dpy;
	xcb_window_t                 _id;

	bool                         _need_update_type;
	xcb_atom_t                   _wm_type;

	xcb_get_window_attributes_reply_t * _wa;
	xcb_get_geometry_reply_t * _geometry;

	/**
	 * ICCCM properties read-only
	 **/
	wm_name_t                    _wm_name;
	wm_icon_name_t               _wm_icon_name;
	wm_normal_hints_t            _wm_normal_hints;
	wm_hints_t                   _wm_hints;
	wm_class_t                   _wm_class;
	wm_transient_for_t           _wm_transient_for;
	wm_protocols_t               _wm_protocols;
	wm_colormap_windows_t        _wm_colormap_windows;
	wm_client_machine_t          _wm_client_machine;

	/**
	 * ICCCM properties read-write
	 **/
	wm_state_t                   _wm_state;

	/**
	 * EWMH properties
	 **/
	net_wm_name_t                __net_wm_name;
	net_wm_visible_name_t        __net_wm_visible_name;
	net_wm_icon_name_t           __net_wm_icon_name;
	net_wm_visible_icon_name_t   __net_wm_visible_icon_name;
	net_wm_desktop_t             __net_wm_desktop;
	net_wm_window_type_t         __net_wm_window_type;
	net_wm_state_t               __net_wm_state;
	net_wm_allowed_actions_t     __net_wm_allowed_actions;
	net_wm_strut_t               __net_wm_strut;
	net_wm_strut_partial_t       __net_wm_strut_partial;
	net_wm_icon_geometry_t       __net_wm_icon_geometry;
	net_wm_icon_t                __net_wm_icon;
	net_wm_pid_t                 __net_wm_pid;
	//net_wm_handled_icons_t       __net_wm_handled_icons;
	net_wm_user_time_t           __net_wm_user_time;
	net_wm_user_time_window_t    __net_wm_user_time_window;
	net_frame_extents_t          __net_frame_extents;
	net_wm_opaque_region_t       __net_wm_opaque_region;
	net_wm_bypass_compositor_t   __net_wm_bypass_compositor;

	/**
	 * OTHERs
	 **/
	motif_hints_t                _motif_hints;

	region *                     _shape;

	/** short cut **/
	xcb_atom_t A(atom_e atom);
	xcb_atom_t B(atom_e atom);
	xcb_window_t xid();

private:
	client_proxy_t(client_proxy_t const &);
	client_proxy_t & operator=(client_proxy_t const &);
public:

	client_proxy_t(display_t * cnx, xcb_window_t id);

	~client_proxy_t();

	void read_all_properties();

	void delete_all_properties();

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

	/* EWMH */

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
	void update_net_wm_user_time();
	void update_net_wm_user_time_window();
	void update_net_frame_extents();
	void update_net_wm_opaque_region();
	void update_net_wm_bypass_compositor();
	void update_motif_hints();
	void update_shape();

	void set_net_wm_desktop(unsigned int n);

public:

	void print_window_attributes();


	void print_properties();

	void update_type();

	xcb_atom_t wm_type();

	display_t *          cnx() const;
	xcb_window_t         id() const;

	auto wa() const -> xcb_get_window_attributes_reply_t const *;
	auto geometry() const -> xcb_get_geometry_reply_t const *;

	/* ICCCM */

	string const *                     wm_name();
	string const *                     wm_icon_name();
	XSizeHints const *                 wm_normal_hints();
	XWMHints const *                   wm_hints();
	vector<string> const *             wm_class();
	xcb_window_t const *               wm_transient_for();
	list<xcb_atom_t> const *           wm_protocols();
	vector<xcb_window_t> const *       wm_colormap_windows();
	string const *                     wm_client_machine();

	/* wm_state is writen by WM */
	wm_state_data_t const *            wm_state();

	/* EWMH */

	string const *                     net_wm_name();
	string const *                     net_wm_visible_name();
	string const *                     net_wm_icon_name();
	string const *                     net_wm_visible_icon_name();
	unsigned int const *               net_wm_desktop();
	list<xcb_atom_t> const *           net_wm_window_type();
	list<xcb_atom_t> const *           net_wm_state();
	list<xcb_atom_t> const *           net_wm_allowed_actions();
	vector<int> const *                net_wm_strut();
	vector<int> const *                net_wm_strut_partial();
	vector<int> const *                net_wm_icon_geometry();
	vector<uint32_t> const *           net_wm_icon();
	unsigned int const *               net_wm_pid();
	uint32_t const *                   net_wm_user_time();
	xcb_window_t const *               net_wm_user_time_window();
	vector<int> const *                net_frame_extents();
	vector<int> const *                net_wm_opaque_region();
	unsigned int const *               net_wm_bypass_compositor();

	/* OTHERs */
	motif_wm_hints_t const *           motif_hints() const;

	region const *                     shape() const;

	void net_wm_state_add(atom_e atom);

	void net_wm_state_remove(atom_e atom);

	void net_wm_allowed_actions_add(atom_e atom);

	void net_wm_allowed_actions_set(list<atom_e> atom_list);

	void set_wm_state(int state);

	void process_event(xcb_configure_notify_event_t const * e);

	rect position() const;

	void set_focus_state(bool is_focused);
	void select_input(uint32_t mask);
	void select_input_shape(bool x);

	void grab_button (
	                 uint8_t           owner_events,
	                 uint16_t          event_mask,
	                 uint8_t           pointer_mode,
	                 uint8_t           keyboard_mode,
	                 xcb_window_t      confine_to,
	                 xcb_cursor_t      cursor,
	                 uint8_t           button,
	                 uint16_t          modifiers);

	void ungrab_button (uint8_t button, uint16_t modifiers);

	void send_event (
	                uint8_t           propagate  /**< */,
	                uint32_t          event_mask  /**< */,
	                const char       *event  /**< */);

	void set_input_focus(int revert_to, xcb_timestamp_t time);

	void move_resize(rect const & size);
	void fake_configure(rect const & location, int border_width);
	void delete_window(xcb_timestamp_t t);
	void set_border_width(uint32_t width);
	void xmap();
	void unmap();
	void reparentwindow(xcb_window_t parent, int x, int y);
	void delete_net_wm_state();
	void delete_wm_state();
	void add_to_save_set();

};

}



#endif /* CLIENT_PROPERTIES_HXX_ */
