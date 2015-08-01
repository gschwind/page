/*
 * page.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef PAGE_HXX_
#define PAGE_HXX_

#include <memory>
#include <list>
#include <vector>
#include <string>
#include <map>

#include "config.hxx"

#include "time.hxx"
#include "display.hxx"
#include "compositor.hxx"

#include "config_handler.hxx"

#include "key_desc.hxx"
#include "keymap.hxx"

#include "theme.hxx"

#include "client_base.hxx"
#include "client_managed.hxx"
#include "client_not_managed.hxx"

#include "popup_alt_tab.hxx"
#include "popup_notebook0.hxx"
#include "popup_split.hxx"
#include "popup_frame_move.hxx"

#include "dropdown_menu.hxx"

#include "page_component.hxx"
#include "notebook.hxx"
#include "split.hxx"
#include "viewport.hxx"
#include "desktop.hxx"

#include "page_event.hxx"
#include "renderable_page.hxx"

#include "mainloop.hxx"

namespace page {

struct fullscreen_data_t {
	workspace_t * desktop;
	viewport_t * viewport;
	managed_window_type_e revert_type;
	notebook_t * revert_notebook;
};

/* process_mode_e define possible state of page */
enum process_mode_e {
	PROCESS_NORMAL,						// default evant processing
	PROCESS_SPLIT_GRAB,					// when split is moving
	PROCESS_NOTEBOOK_GRAB,				// when notebook tab is moved
	PROCESS_NOTEBOOK_BUTTON_PRESS,		// when click on close/unbind button
	PROCESS_FLOATING_MOVE,				// when a floating window is moved
	PROCESS_FLOATING_RESIZE,			// when resizing a floating window
	PROCESS_FLOATING_CLOSE,				// when clicking close button of floating window
	PROCESS_FLOATING_BIND,				// when clicking bind button
	PROCESS_FULLSCREEN_MOVE,			// when mod4+click to change fullscreen window screen
	PROCESS_FLOATING_MOVE_BY_CLIENT,	// when moving a floating window started by client himself
	PROCESS_FLOATING_RESIZE_BY_CLIENT,	// when resizing a floating window started by client himself
	PROCESS_NOTEBOOK_MENU,				// when notebook menu is shown
	PROCESS_NOTEBOOK_CLIENT_MENU,		// when switch desktop menu is shown
	PROCESS_ALT_TAB						// when alt-tab running
};

struct key_bind_cmd_t {
	key_desc_t key;
	std::string cmd;
};

class page_t : public page_component_t, public mainloop_t, public page_context_t {
	static uint32_t const DEFAULT_BUTTON_EVENT_MASK = XCB_EVENT_MASK_BUTTON_PRESS|XCB_EVENT_MASK_BUTTON_RELEASE|XCB_EVENT_MASK_BUTTON_MOTION;
	static uint32_t const ROOT_EVENT_MASK = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE;
	static time_t const default_wait;

	/** define callback function type for event handler **/
	using callback_event_t = void (page_t::*) (xcb_generic_event_t const *);

	std::map<int, callback_event_t> _event_handlers;

	void _event_handler_bind(int type, callback_event_t f);
	void _bind_all_default_event();

public:

	/* store all managed client */
	std::list<client_managed_t *> _clients_list;

	/* auto-refocus a client if the current one is closed */
	bool _auto_refocus;

	bool _mouse_focus;

	/* enable-shade */
	bool _enable_shade_windows;

private:
	grab_handler_t * _grab_handler;

public:
	bool replace_wm;

	/** window that handle page identity for others clients */
	xcb_window_t identity_window;

	display_t * cnx;
	compositor_t * rnd;
	composite_surface_manager_t * cmgr;

	std::list<client_not_managed_t *> below;
	std::list<client_base_t *> root_subclients;
	std::list<client_not_managed_t *> tooltips;
	std::list<client_not_managed_t *> notifications;
	std::list<client_not_managed_t *> above;

	bool _need_render;
	bool _need_restack;
	bool _need_update_client_list;
	bool _menu_drop_down_shadow;

	config_handler_t conf;

	unsigned int _current_desktop;
	std::vector<workspace_t *> _desktop_list;

	/** store the order of last shown desktop **/
	std::list<workspace_t *> _desktop_stack;

	/**
	 * Store data to allow proper revert fullscreen window to
	 * their original positions
	 **/
	std::map<client_managed_t *, fullscreen_data_t> _fullscreen_client_to_viewport;

	std::list<xcb_atom_t> supported_list;

	std::string page_base_dir;
	std::string _theme_engine;

	key_desc_t bind_page_quit;
	key_desc_t bind_toggle_fullscreen;
	key_desc_t bind_toggle_compositor;
	key_desc_t bind_close;

	key_desc_t bind_exposay_all;

	key_desc_t bind_right_desktop;
	key_desc_t bind_left_desktop;

	key_desc_t bind_bind_window;
	key_desc_t bind_fullscreen_window;
	key_desc_t bind_float_window;

	keymap_t * _keymap;

	key_desc_t bind_debug_1;
	key_desc_t bind_debug_2;
	key_desc_t bind_debug_3;
	key_desc_t bind_debug_4;

	std::array<key_bind_cmd_t, 10> bind_cmd;

private:

	xcb_timestamp_t _last_focus_time;
	xcb_timestamp_t _last_button_press;

	std::list<client_managed_t *> _global_client_focus_history;

	std::list<std::shared_ptr<overlay_t>> _overlays;

	i_rect _root_position;
	theme_t * _theme;

	int _left_most_border;
	int _top_most_border;

public:

private:
	page_t(page_t const &);
	page_t &operator=(page_t const &);
public:
	page_t(int argc, char ** argv);
	virtual ~page_t();

	void set_default_pop(notebook_t * x);
	void set_focus(client_managed_t * w, xcb_timestamp_t tfocus);
	compositor_t * get_render_context();
	display_t * get_xconnection();

	/** short cut **/
	xcb_atom_t A(atom_e atom) {
		return cnx->A(atom);
	}

	/* run page main loop */
	void run();

	/* scan current root window status, finding mapped windows */
	void scan();

	/** user inputs **/
	void process_key_press_event(xcb_generic_event_t const * e);
	void process_key_release_event(xcb_generic_event_t const * e);
	void process_button_press_event(xcb_generic_event_t const * e);
	void process_motion_notify(xcb_generic_event_t const * _e);
	void process_button_release(xcb_generic_event_t const * _e);
	/* SubstructureNotifyMask */
	//void process_event(xcb_circulate_notify_event_t const * e);
	void process_configure_notify_event(xcb_generic_event_t const * e);
	void process_create_notify_event(xcb_generic_event_t const * e);
	void process_destroy_notify_event(xcb_generic_event_t const * e);
	void process_gravity_notify_event(xcb_generic_event_t const * e);
	void process_map_notify_event(xcb_generic_event_t const * e);
	void process_reparent_notify_event(xcb_generic_event_t const * e);
	void process_unmap_notify_event(xcb_generic_event_t const * e);
	void process_fake_unmap_notify_event(xcb_generic_event_t const * e);
	void process_mapping_notify_event(xcb_generic_event_t const * e);
	void process_selection_clear_event(xcb_generic_event_t const * e);
	void process_focus_in_event(xcb_generic_event_t const * e);
	void process_focus_out_event(xcb_generic_event_t const * e);
	void process_enter_window_event(xcb_generic_event_t const * e);
	void process_leave_window_event(xcb_generic_event_t const * e);

	void process_expose_event(xcb_generic_event_t const * e);

	void process_randr_notify_event(xcb_generic_event_t const * e);
	void process_shape_notify_event(xcb_generic_event_t const * e);

	/* SubstructureRedirectMask */
	void process_circulate_request_event(xcb_generic_event_t const * e);
	void process_configure_request_event(xcb_generic_event_t const * e);
	void process_map_request_event(xcb_generic_event_t const * e);

	/* PropertyChangeMask */
	void process_property_notify_event(xcb_generic_event_t const * e);

	/* Unmaskable Events */
	void process_fake_client_message_event(xcb_generic_event_t const * e);

	/* extension events */
	void process_damage_notify_event(xcb_generic_event_t const * ev);

	void process_event(xcb_generic_event_t const * e);

	/* update _NET_CLIENT_LIST_STACKING and _NET_CLIENT_LIST */
	void update_client_list();

	/* update _NET_SUPPORTED */
	void update_net_supported();

	/* setup and create managed window */
	client_managed_t * manage(xcb_atom_t net_wm_type, std::shared_ptr<client_properties_t> wa);

	/* unmanage a managed window */
	void unmanage(client_managed_t * mw);

	/* put a managed window into a given notebook */
	void insert_window_in_notebook(client_managed_t * x, notebook_t * n, bool prefer_activate);

	/* update viewport and childs allocation */
	void update_workarea();

	/* turn a managed window into fullscreen */
	void fullscreen(client_managed_t * c, viewport_t * v = nullptr);

	/* switch a fullscreened and managed window into floating or notebook window */
	void unfullscreen(client_managed_t * c);

	/* toggle fullscreen */
	void toggle_fullscreen(client_managed_t * c);

	/* split a notebook into two notebook */
	void split(notebook_t * nbk, split_type_e type);
	void split_left(notebook_t * nbk, client_managed_t * c);
	void split_right(notebook_t * nbk, client_managed_t * c);
	void split_top(notebook_t * nbk, client_managed_t * c);
	void split_bottom(notebook_t * nbk, client_managed_t * c);

	/* close a notebook and unsplit the parent */
	void notebook_close(notebook_t * src);

	/* compute the allocation of viewport taking in account DOCKs */
	void compute_viewport_allocation(workspace_t * d, viewport_t * v);

	void cleanup_not_managed_client(client_not_managed_t * c);

	void process_net_vm_state_client_message(xcb_window_t c, long type, xcb_atom_t state_properties);

	void insert_in_tree_using_transient_for(client_base_t * c);
	void insert_in_tree_using_transient_for(client_managed_t * c);
	void safe_update_transient_for(client_base_t * c);

	client_base_t * get_transient_for(client_base_t * c);
	void logical_raise(client_base_t * c);
	void detach(tree_t * t);

	void safe_raise_window(client_base_t * c);

	/* attach floating window in a notebook */
	void bind_window(client_managed_t * mw, bool activate);
	/* detach notebooked window to a floating window */
	void unbind_window(client_managed_t * mw);
	void grab_pointer();
	/* if grab is linked to a given window remove this grab */
	void cleanup_grab(client_managed_t * mw);
	/* find a valid notebook, that is in subtree base and that is no nbk */
	notebook_t * get_another_notebook(tree_t * base = nullptr, tree_t * nbk = nullptr);
	/* get all available notebooks with page */
	std::vector<notebook_t *> get_notebooks(tree_t * base = nullptr);
	std::vector<viewport_t *> get_viewports(tree_t * base = nullptr);
	/* find where the managed window is */
	notebook_t * find_parent_notebook_for(client_managed_t * mw);
	client_managed_t * find_managed_window_with(xcb_window_t w);
	static viewport_t * find_viewport_of(tree_t * n);
	static workspace_t * find_desktop_of(tree_t * n);
	void set_window_cursor(xcb_window_t w, xcb_cursor_t c);
	void update_windows_stack();
	void update_viewport_layout();
	void remove_viewport(workspace_t * d, viewport_t * v);
	void destroy_viewport(viewport_t * v);
	void onmap(xcb_window_t w);
	void create_managed_window(std::shared_ptr<client_properties_t> c, xcb_atom_t type);
	void manage_client(client_managed_t * mw, xcb_atom_t type);
	void ackwoledge_configure_request(xcb_configure_request_event_t const * e);
	void create_unmanaged_window(std::shared_ptr<client_properties_t> c, xcb_atom_t type);
	void create_dock_window(std::shared_ptr<client_properties_t> c, xcb_atom_t type);
	viewport_t * find_mouse_viewport(int x, int y) const;
	bool get_safe_net_wm_user_time(client_base_t * c, xcb_timestamp_t & time);
	void update_page_areas();
	void set_desktop_geometry(long width, long height);
	client_base_t * find_client_with(xcb_window_t w);
	client_base_t * find_client(xcb_window_t w);
	void remove_client(client_base_t * c);
	std::string get_node_name() const;
	void replace(page_component_t * src, page_component_t * by);
	void raise_child(tree_t * t);
	void activate(tree_t * t = nullptr);
	void remove(tree_t * t);

	void fullscreen_client_to_viewport(client_managed_t * c, viewport_t * v);

	void attach_dock(client_not_managed_t * uw) {
		get_current_workspace()->add_dock_client(uw);
	}

	void process_notebook_client_menu(client_managed_t * c, int selected);

	void check_x11_extension();

	void create_identity_window();
	void register_wm();
	void register_cm();

	void render(cairo_t * cr, page::time_t time);
	bool need_render(time_t time);

	bool check_for_managed_window(xcb_window_t w);
	bool check_for_destroyed_window(xcb_window_t w);

	void update_keymap();
	void update_grabkey();

	client_managed_t * find_hidden_client_with(xcb_window_t w);

	void prepare_render(std::vector<std::shared_ptr<renderable_t>> & out, page::time_t const & time);

	std::vector<page_event_t> compute_page_areas(
			viewport_t * v) const;

	page_component_t * parent() const;
	void set_parent(tree_t * parent);

	void set_allocation(i_rect const & r);

	i_rect allocation() const {
		return _root_position;
	}

	void render_legacy(cairo_t * cr, i_rect const & area) const { }


	void children(std::vector<tree_t *> & out) const;
	void get_visible_children(std::vector<tree_t *> & out);

	void render();

	/** debug function that try to print the state of page in stdout **/
	void print_state() const;

	void update_current_desktop() const;

	void hide();
	void show();

	void switch_to_desktop(unsigned int desktop);

	void update_fullscreen_clients_position();
	void update_desktop_visibility();

	void process_error(xcb_generic_event_t const * e);

	void add_compositor_damaged(region const & r);

	void start_compositor();
	void stop_compositor();

	void run_cmd(std::string const & cmd_with_args);

	std::vector<client_managed_t *> get_sticky_client_managed(tree_t * base);
	void reconfigure_docks(workspace_t * d);

	void mark_durty(tree_t * t);

	unsigned int find_current_desktop(client_base_t * c);

	void process_pending_events();
	bool render_timeout();

	/**
	 * The page_context API
	 **/

	virtual theme_t const * theme() const;
	virtual composite_surface_manager_t * csm() const;
	virtual display_t * dpy() const;
	virtual compositor_t * cmp() const;

	virtual workspace_t * get_current_workspace() const;
	virtual workspace_t * get_workspace(int id) const;
	virtual int get_workspace_count() const;

	virtual void grab_start(grab_handler_t * handler);
	virtual void grab_stop();
	virtual void overlay_add(std::shared_ptr<overlay_t> x);
	virtual void overlay_remove(std::shared_ptr<overlay_t> x);

	virtual void add_global_damage(region const & r);
	virtual int left_most_border();
	virtual int top_most_border();

	virtual std::list<client_managed_t *> global_client_focus_history();
	virtual std::list<client_managed_t *> clients_list();

	virtual keymap_t const * keymap() const;
	virtual bool menu_drop_down_shadow() const;

};


}



#endif /* PAGE_HXX_ */
