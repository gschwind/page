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

#include <xcb/xcb.h>

#include "icon_handler.hxx"
#include "theme.hxx"

#include "floating_event.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "renderable_pixmap.hxx"

namespace page {

using namespace std;

enum managed_window_type_e {
	MANAGED_FLOATING,
	MANAGED_NOTEBOOK,
	MANAGED_FULLSCREEN,
	MANAGED_DOCK
};

class client_managed_t : public client_base_t {

	static long const MANAGED_BASE_WINDOW_EVENT_MASK = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
	static long const MANAGED_DECO_WINDOW_EVENT_MASK = XCB_EVENT_MASK_EXPOSURE;
	static long const MANAGED_ORIG_WINDOW_EVENT_MASK = XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW;
	static uint32_t const DEFAULT_BUTTON_EVENT_MASK =  XCB_EVENT_MASK_BUTTON_PRESS|XCB_EVENT_MASK_BUTTON_MOTION|XCB_EVENT_MASK_BUTTON_RELEASE;

	managed_window_type_e _managed_type;
	xcb_atom_t _net_wm_type;

	shared_ptr<client_view_t> _client_view;

	/** hold floating position **/
	rect _floating_wished_position;

	/** hold notebook position **/
	rect _notebook_wished_position;

	/** the absolute position without border **/
	rect _wished_position;

	rect _orig_position;
	rect _base_position;

	// the output surface (i.e. surface where we write things)
	cairo_surface_t * _surf;

	// border surface of floating window
	shared_ptr<pixmap_t> _top_buffer;
	shared_ptr<pixmap_t> _bottom_buffer;
	shared_ptr<pixmap_t> _left_buffer;
	shared_ptr<pixmap_t> _right_buffer;


	// window title cache
	string _title;

	// icon cache
	shared_ptr<icon16> _icon;

	xcb_visualid_t _orig_visual;
	int _orig_depth;

	xcb_visualid_t _deco_visual;
	int _deco_depth;

	//xcb_window_t _orig;
	xcb_window_t _base;
	xcb_window_t _deco;

	xcb_window_t _input_top;
	xcb_window_t _input_left;
	xcb_window_t _input_right;
	xcb_window_t _input_bottom;
	xcb_window_t _input_top_left;
	xcb_window_t _input_top_right;
	xcb_window_t _input_bottom_left;
	xcb_window_t _input_bottom_right;

	rect _area_top;
	rect _area_left;
	rect _area_right;
	rect _area_bottom;
	rect _area_top_left;
	rect _area_top_right;
	rect _area_bottom_left;
	rect _area_bottom_right;

	struct floating_area_t {
		rect close_button;
		rect bind_button;
		rect title_button;
		rect grip_top;
		rect grip_bottom;
		rect grip_left;
		rect grip_right;
		rect grip_top_left;
		rect grip_top_right;
		rect grip_bottom_left;
		rect grip_bottom_right;
	};

	floating_area_t _floating_area;

	bool _has_focus;
	bool _is_iconic;
	bool _demands_attention;
	bool _is_resized;
	bool _is_exposed;
	bool _has_change;

	mutable region _opaque_region_cache;
	mutable region _visible_region_cache;
	mutable region _damage_cache;

	/* private to avoid copy */
	client_managed_t(client_managed_t const &);
	client_managed_t & operator=(client_managed_t const &);

	void init_managed_type(managed_window_type_e type);

	void fake_configure_unsafe();
	void set_wished_position(rect const & position);
	rect const & get_wished_position() const;

	cairo_t * get_cairo_context();

	void update_icon();
	void set_theme(theme_t const * theme);

	xcb_window_t deco() const;
	xcb_atom_t A(atom_e atom);
	void icccm_focus_unsafe(xcb_timestamp_t t);

	void net_wm_allowed_actions_add(atom_e atom);

	void map_unsafe();
	void unmap_unsafe();
	void ungrab_all_button_unsafe();
	void select_inputs_unsafe();
	void unselect_inputs_unsafe();

	void _update_title();
	void _update_visible_region();
	void _update_opaque_region();
	void _apply_floating_hints_constraint();

	auto shared_from_this() -> shared_ptr<client_managed_t>;

	xcb_atom_t net_wm_type();
	bool get_wm_normal_hints(XSizeHints * size_hints);

	void destroy_back_buffer();
	void create_back_buffer();
	void update_floating_areas();
	void set_opaque_region(xcb_window_t w, region & region);
	display_t * cnx();

	void compute_floating_areas();
	rect compute_floating_bind_position(rect const & allocation) const;
	rect compute_floating_close_position(rect const & allocation) const;

	void update_title();
	bool prefer_window_border() const;

	void _update_backbuffers();
	void _paint_exposed();

public:

	client_managed_t(page_context_t * ctx, xcb_window_t w, xcb_atom_t net_wm_type);
	virtual ~client_managed_t();

	signal_t<client_managed_t *> on_destroy;
	signal_t<shared_ptr<client_managed_t>> on_title_change;
	signal_t<shared_ptr<client_managed_t>> on_focus_change;

	bool is(managed_window_type_e type);
	auto title() const -> string const &;
	auto create_view() -> shared_ptr<client_view_t>;
	auto get_wished_position() -> rect const &;
	void set_floating_wished_position(rect const & pos);
	rect get_base_position() const;
	void reconfigure();
	void normalize();
	void iconify();
	bool has_focus() const;
	bool is_iconic();
	void delete_window(xcb_timestamp_t);
	auto icon() const -> shared_ptr<icon16>;
	void set_notebook_wished_position(rect const & pos);
	void set_current_desktop(unsigned int n);
	bool is_stiky();
	bool is_modal();
	void net_wm_state_add(atom_e atom);
	void net_wm_state_remove(atom_e atom);
	void net_wm_state_delete();
	void wm_state_delete();
	bool is_fullscreen();
	bool skip_task_bar();
	auto get_floating_wished_position() -> rect const & ;
	bool lock();
	void unlock();
	void set_focus_state(bool is_focused);
	void set_demands_attention(bool x);
	bool demands_attention();
	void focus(xcb_timestamp_t t);
	auto get_type() -> managed_window_type_e;
	void set_managed_type(managed_window_type_e type);
	void grab_button_focused_unsafe();
	void grab_button_unfocused_unsafe();

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

	virtual void activate();
	//virtual void activate(shared_ptr<tree_t> t);
	virtual bool button_press(xcb_button_press_event_t const * ev);
	// virtual bool button_release(xcb_button_release_event_t const * ev);
	// virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	// virtual bool leave(xcb_leave_notify_event_t const * ev);
	// virtual bool enter(xcb_enter_notify_event_t const * ev);
	virtual void expose(xcb_expose_event_t const * ev);

	// virtual auto get_xid() const -> xcb_window_t;
	// virtual auto get_parent_xid() const -> xcb_window_t;
	// virtual rect get_window_position() const;

	virtual void queue_redraw();
	virtual void trigger_redraw();

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


using client_managed_p = shared_ptr<client_managed_t>;
using client_managed_w = weak_ptr<client_managed_t>;

}


#endif /* MANAGED_WINDOW_HXX_ */
