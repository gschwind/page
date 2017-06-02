/*
 * managed_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef CLIENT_MANAGED_HXX_
#define CLIENT_MANAGED_HXX_

#include <string>
#include <vector>
#include <set>
#include <map>

#include <xcb/xcb.h>

#include "icon_handler.hxx"
#include "theme.hxx"

#include "floating_event.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "renderable_pixmap.hxx"
#include "properties.hxx"
#include "client_proxy.hxx"

namespace page {

using namespace std;

enum managed_window_type_e {
	MANAGED_FLOATING,
	MANAGED_NOTEBOOK,
	MANAGED_FULLSCREEN,
	MANAGED_DOCK,
	MANAGED_POPUP
};

struct client_managed_t : public enable_shared_from_this<client_managed_t>, public connectable_t {
	static long const MANAGED_ORIG_WINDOW_EVENT_MASK =
			  XCB_EVENT_MASK_STRUCTURE_NOTIFY
			| XCB_EVENT_MASK_PROPERTY_CHANGE
			| XCB_EVENT_MASK_FOCUS_CHANGE
			| XCB_EVENT_MASK_ENTER_WINDOW
			| XCB_EVENT_MASK_LEAVE_WINDOW;
	static long const UNMANAGED_ORIG_WINDOW_EVENT_MASK =
			  XCB_EVENT_MASK_STRUCTURE_NOTIFY
			| XCB_EVENT_MASK_PROPERTY_CHANGE;

	page_t * _ctx;

	/* handle properties of client */
	client_proxy_p _client_proxy;

	managed_window_type_e _managed_type;
	xcb_atom_t _net_wm_type;

	/**
	 * hold floating position of the client window relative to root window,
	 * even if rebased to another window. This used as default floating position
	 * when the user switch the window between fullscreen or notebook to
	 * floating.
	 **/
	rect _floating_wished_position;

	/**
	 * The position of the client window relative to root window, even if the
	 * client is rebased to another window.
	 **/
	rect _absolute_position;

	// window title cache
	string _title;

	// icon cache
	shared_ptr<icon16> _icon;

	// track the current view that own the window.
	view_t * _current_owner_view;

	bool _has_focus;
	bool _demands_attention;
	bool _has_input_focus;
	bool _has_take_focus;

	shared_ptr<vector<int>> _net_wm_strut;
	shared_ptr<vector<int>> _net_wm_strut_partial;
	shared_ptr<XWMHints> _wm_hints;
	shared_ptr<list<xcb_atom_t>> _wm_protocols;

	shared_ptr<list<xcb_atom_t>> _net_wm_state;

	int _views_count;

	/* private to avoid copy */
	client_managed_t(client_managed_t const &) = delete;
	client_managed_t & operator=(client_managed_t const &) = delete;

	void init_managed_type(managed_window_type_e type);

	void fake_configure_unsafe(rect const & location);
	void set_wished_position(rect const & position);
	rect const & get_wished_position() const;

	cairo_t * get_cairo_context();

	void update_icon();
	void set_theme(theme_t const * theme);

	xcb_atom_t A(atom_e atom);
	void icccm_focus_unsafe(xcb_timestamp_t t);

	void net_wm_allowed_actions_add(atom_e atom);

	void map_unsafe();
	void unmap_unsafe();

	void _update_title();
	void _apply_floating_hints_constraint();

	xcb_atom_t net_wm_type();
	bool get_wm_normal_hints(XSizeHints * size_hints);

	void set_opaque_region(xcb_window_t w, region & region);
	display_t * cnx();

	void update_title();
	bool prefer_window_border() const;

	auto current_owner_view() const -> view_t *;
	// this two functions is to ensure correct ownership.
	void acquire(view_t * v);
	void release(view_t * v);

	client_managed_t(page_t * ctx, client_proxy_p proxy);
	~client_managed_t();

	signal_t<client_managed_t *> on_destroy;
	signal_t<client_managed_t *> on_title_change;
	signal_t<client_managed_t *> on_configure_notify;
	signal_t<client_managed_t *> on_strut_change;
	signal_t<client_managed_t *> on_opaque_region_change;
	signal_t<client_managed_t *> on_unmanage;
	signal_t<client_managed_t *, xcb_configure_request_event_t const *> on_configure_request;

	bool is(managed_window_type_e type);
	auto title() const -> string const &;
	auto get_wished_position() -> rect const &;
	void set_floating_wished_position(rect const & pos);
	rect get_base_position() const;
	void delete_window(xcb_timestamp_t);
	auto icon() const -> shared_ptr<icon16>;
	void set_current_workspace(unsigned int n);

	void net_wm_state_add(atom_e atom);
	void net_wm_state_remove(atom_e atom);
	void wm_state_delete();
	bool has_wm_state_fullscreen();
	bool has_wm_state_stiky();
	bool has_wm_state_modal();

	bool skip_task_bar();
	auto get_floating_wished_position() -> rect const & ;
	bool lock();
	void unlock();
	void set_demands_attention(bool x);
	bool demands_attention();
	void focus(xcb_timestamp_t t);
	auto get_type() -> managed_window_type_e;
	void set_managed_type(managed_window_type_e type);
	auto ensure_workspace() -> unsigned;

	void read_all_properties();
	void update_shape();
	bool has_motif_border();
	void set_net_wm_desktop(unsigned long n);
	bool is_window(xcb_window_t w);
	auto wm_type() -> xcb_atom_t;
	void print_window_attributes();
	void print_properties();

	void process_event(xcb_configure_notify_event_t const * e);
	auto cnx() const -> display_t *;
	auto transient_for() -> client_managed_p;
	auto shape() const -> region const *;
	auto position() -> rect;
	auto compute_size_with_constrain(unsigned w, unsigned h) -> dimention_t<unsigned>;

	template<int const ID>
	auto get() -> shared_ptr<typename ptype<ID>::type::cxx_type>
	{
		return _client_proxy->get<ID>();
	}

	void on_property_notify(xcb_property_notify_event_t const * e);
	void singal_configure_request(xcb_configure_request_event_t const * e);

	auto create_surface(xcb_window_t base) -> client_view_p;
	void destroy_view(client_view_t * c);

};


using client_managed_p = shared_ptr<client_managed_t>;
using client_managed_w = weak_ptr<client_managed_t>;

}


#endif /* MANAGED_WINDOW_HXX_ */
