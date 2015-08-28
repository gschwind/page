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
#include <xcb/damage.h>

#include <limits>
#include <utility>
#include <vector>
#include <list>
#include <iostream>
#include <set>
#include <algorithm>
#include <memory>

#include "atoms.hxx"
#include "region.hxx"
#include "motif_hints.hxx"
#include "properties.hxx"

namespace page {

class display_t;
class client_proxy_t;
struct pixmap_t;

class invalid_client_t {

};

using namespace std;

/**
 * client_view_t is handled by client if they keep a view of the
 * composite surface.
 **/
class client_view_t {
	friend class display_t;
	friend class client_proxy_t;

	weak_ptr<client_proxy_t> _parent;
	region _damaged;

public:

	client_view_t(shared_ptr<client_proxy_t> parent);
	auto get_pixmap() -> shared_ptr<pixmap_t>;
	void clear_damaged();
	auto get_damaged() -> region const &;
	bool has_damage();

};

class client_proxy_t : public enable_shared_from_this<client_proxy_t> {
	friend class display_t;
	friend class client_view_t;

private:
	display_t *                  _dpy;
	xcb_window_t                 _id;

	bool _destroyed;
	bool                         _need_update_type;
	xcb_atom_t                   _wm_type;

	xcb_get_window_attributes_reply_t _wa;
	xcb_get_geometry_reply_t _geometry;
	region * _shape;

	xcb_visualtype_t * _vis;
	xcb_damage_damage_t _damage;
	shared_ptr<pixmap_t> _pixmap;

	list<client_view_t *> _views;

	bool _need_pixmap_update;
	bool _is_redirected;

#define RO_PROPERTY(cxx_name, x11_name, x11_type, cxx_type) \
	private: cxx_name##_t _##cxx_name; \
	public:  cxx_type const * cxx_name(); \
	public:  void update_##cxx_name();

#define RW_PROPERTY(cxx_name, x11_name, x11_type, cxx_type) \
	private: cxx_name##_t _##cxx_name; \
	public:  cxx_type const * cxx_name(); \
	public:  void update_##cxx_name(); \
	public:  cxx_type const * cxx_name(cxx_type * x);

#include "client_property_list.hxx"

#undef RO_PROPERTY
#undef RW_PROPERTY

	/** short cut **/
	xcb_atom_t A(atom_e atom);
	xcb_atom_t B(atom_e atom);
	xcb_window_t xid();

private:
	client_proxy_t(client_proxy_t const &);
	client_proxy_t & operator=(client_proxy_t const &);

	bool _safe_pixmap_update();
	shared_ptr<pixmap_t> get_pixmap();

public:
	client_proxy_t(display_t * cnx, xcb_window_t id);
	~client_proxy_t();

	void read_all_properties();
	void delete_all_properties();
	bool read_window_attributes();
	void update_shape();
	void set_net_wm_desktop(unsigned int n);

public:

	void print_window_attributes();
	void print_properties();
	void update_type();

	xcb_atom_t wm_type();

	display_t *          cnx() const;
	xcb_window_t         id() const;

	auto wa() const -> xcb_get_window_attributes_reply_t const &;
	auto geometry() const -> xcb_get_geometry_reply_t const &;

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

	void process_event(xcb_damage_notify_event_t const * ev);
	void process_event(xcb_property_notify_event_t const * ev);
	void create_damage();
	void destroy_damage();
	void enable_redirect();
	void disable_redirect();
	void on_map();
	void add_damaged(region const & r);
	int depth();
	void apply_change();
	void destroy_pixmap();
	void freeze(bool x);

	auto create_view() -> client_view_t *;
	void remove_view(client_view_t * v);
	bool _has_views();

	bool destroyed();
	bool destroyed(bool x);

};

}




#endif /* CLIENT_PROPERTIES_HXX_ */
