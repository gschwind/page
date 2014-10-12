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


#include "config.hxx"

#include <glib.h>

#include <features.h>
#include <ctime>
#include <X11/X.h>
#include <X11/cursorfont.h>
#include <unistd.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include <stdio.h>

#include <list>
#include <string>
#include <iostream>
#include <limits>
#include <cstring>
#include <map>

/* According to POSIX.1-2001 */
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#include "utils.hxx"
#include "box.hxx"

#include "client_properties.hxx"
#include "client_base.hxx"
#include "time.hxx"

#include "compositor.hxx"
#include "display.hxx"
#include "notebook.hxx"
#include "split.hxx"
#include "viewport.hxx"
#include "config_handler.hxx"

#include "client_not_managed.hxx"
#include "client_managed.hxx"

#include "renderable_page.hxx"

#include "popup_notebook0.hxx"
#include "popup_frame_move.hxx"
#include "popup_split.hxx"
#include "popup_alt_tab.hxx"
#include "dropdown_menu.hxx"

#include "simple2_theme.hxx"

#include "page_exception.hxx"

#include "window_handler.hxx"
#include "keymap.hxx"
#include "page_component.hxx"


namespace page {

using namespace std;

typedef list<i_rect> box_list_t;

class page_t : public page_component_t {

	static long const ROOT_EVENT_MASK = SubstructureNotifyMask | SubstructureRedirectMask | PropertyChangeMask;
	static time_t const default_wait;

public:

	/*damage event handler */
	int damage_opcode, damage_event, damage_error;

	/* composite event handler */
	int composite_opcode, composite_event, composite_error;

	/* fixes event handler */
	int fixes_opcode, fixes_event, fixes_error;

	/* xshape extension handler */
	int xshape_opcode, xshape_event, xshape_error;

	/* xrandr extension handler */
	int xrandr_opcode, xrandr_event, xrandr_error;

	/* GLX extension handler */
	int glx_opcode, glx_event, glx_error;

	enum select_e {
		SELECT_NONE,
		SELECT_TAB,
		SELECT_TOP,
		SELECT_BOTTOM,
		SELECT_LEFT,
		SELECT_RIGHT,
		SELECT_CENTER
	};

	/* popups (overlay) */
	ptr<popup_notebook0_t> pn0;
	ptr<popup_frame_move_t> pfm;
	ptr<popup_split_t> ps;
	ptr<popup_alt_tab_t> pat;
	ptr<dropdown_menu_t> menu;

	struct mode_data_split_t {
		split_t * split;
		i_rect slider_area;
		double split_ratio;

		mode_data_split_t() {
			reset();
		}

		void reset() {
			split = nullptr;
			slider_area = i_rect();
			split_ratio = 0.5;
		}

	};

	struct mode_data_notebook_t {
		int start_x;
		int start_y;
		select_e zone;
		client_managed_t * c;
		notebook_t * from;
		notebook_t * ns;
		page_event_t ev;

		mode_data_notebook_t() {
			reset();
		}

		void reset() {
			start_x = 0;
			start_y = 0;
			zone = SELECT_NONE;
			c = nullptr;
			from = nullptr;
			ns = nullptr;
		}

	};

	struct mode_data_notebook_menu_t {
		notebook_t * from;
		bool active_grab;
		i_rect b;

		mode_data_notebook_menu_t() {
			reset();
		}

		void reset() {
			from = nullptr;
			active_grab = false;
		}

	};


	struct mode_data_bind_t {
		int start_x;
		int start_y;
		select_e zone;
		client_managed_t * c;
		notebook_t * ns;

		mode_data_bind_t() {
			reset();
		}

		void reset() {
			start_x = 0;
			start_y = 0;
			zone = SELECT_NONE;
			c = nullptr;
			ns = nullptr;
		}

	};

	enum resize_mode_e {
		RESIZE_NONE,
		RESIZE_TOP_LEFT,
		RESIZE_TOP,
		RESIZE_TOP_RIGHT,
		RESIZE_LEFT,
		RESIZE_RIGHT,
		RESIZE_BOTTOM_LEFT,
		RESIZE_BOTTOM,
		RESIZE_BOTTOM_RIGHT
	};

	struct mode_data_floating_t {
		resize_mode_e mode;
		int x_offset;
		int y_offset;
		int x_root;
		int y_root;
		i_rect original_position;
		client_managed_t * f;
		i_rect popup_original_position;
		i_rect final_position;
		unsigned int button;

		mode_data_floating_t() {
			reset();
		}

		void reset() {
			mode = RESIZE_NONE;
			x_offset = 0;
			y_offset = 0;
			x_root = 0;
			y_root = 0;
			original_position = i_rect();
			f = nullptr;
			popup_original_position = i_rect();
			final_position = i_rect();
			button = Button1;
		}

	};

	struct mode_data_fullscreen_t {
		client_managed_t * mw;
		viewport_t * v;

		mode_data_fullscreen_t() {
			reset();
		}

		void reset() {
			mw = nullptr;
			v = nullptr;
		}

	};

	struct fullscreen_data_t {
		viewport_t * viewport;
		managed_window_type_e revert_type;
		notebook_t * revert_notebook;

	};

	enum process_mode_e {
		PROCESS_NORMAL,			// Process event as usual
		PROCESS_SPLIT_GRAB,		// Process event when split is moving
		PROCESS_NOTEBOOK_GRAB,	// Process event when notebook tab is moved
		PROCESS_NOTEBOOK_BUTTON_PRESS,
		PROCESS_FLOATING_MOVE,	// Process event when a floating window is moved
		PROCESS_FLOATING_RESIZE,
		PROCESS_FLOATING_CLOSE,
		PROCESS_FLOATING_BIND,
		PROCESS_FULLSCREEN_MOVE, // Alt + click to change fullscreen screen to another one
		PROCESS_FLOATING_MOVE_BY_CLIENT, // Move requested by client
		PROCESS_FLOATING_RESIZE_BY_CLIENT, // Resize requested by client
		PROCESS_NOTEBOOK_MENU
	};

	/* this define the current state of page */
	process_mode_e process_mode;


	enum key_press_mode_e {
		KEY_PRESS_NORMAL,			// Process event as usual
		KEY_PRESS_ALT_TAB,
	};

	key_press_mode_e key_press_mode;

	struct key_mode_data_t {
		client_managed_t * selected;
	};

	key_mode_data_t key_mode_data;

	bool use_internal_compositor;
	bool replace_wm;

	/** window that handle page identity for others clients */
	Window identity_window;

	/* this data are used when you drag&drop a split slider */
	mode_data_split_t mode_data_split;
	/* this data are used when you drag&drop a notebook tab */
	mode_data_notebook_t mode_data_notebook;
	/* this data are used when you move/resize a floating window */
	mode_data_floating_t mode_data_floating;
	/* this data are used when you bind/drag&drop a floating window */
	mode_data_bind_t mode_data_bind;

	mode_data_fullscreen_t mode_data_fullscreen;

	mode_data_notebook_menu_t mode_data_notebook_menu;

	display_t * cnx;
	compositor_t * rnd;

	map<Window, client_base_t *> clients;

	list<client_not_managed_t *> below;
	list<client_base_t *> root_subclients;
	list<client_not_managed_t *> docks;
	list<client_not_managed_t *> tooltips;
	list<client_not_managed_t *> notifications;
	list<client_not_managed_t *> above;

	Cursor default_cursor;

	bool running;
	bool _need_render;

	config_handler_t conf;

	/** map viewport to real outputs **/
	map<RRCrtc, viewport_t *> viewport_outputs;

	/**
	 * Store data to allow proper revert fullscreen window to
	 * their original positions
	 **/
	map<client_managed_t *, fullscreen_data_t> fullscreen_client_to_viewport;

	list<Atom> supported_list;

	notebook_t * default_window_pop;

	string page_base_dir;

	map<Window, window_handler_t *> window_list;

	key_desc_t bind_page_quit;
	key_desc_t bind_toggle_fullscreen;
	key_desc_t bind_close;
	//key_desc_t bind_toggle_bind;
	//key_desc_t bind_vert_split;
	//key_desc_t bind_horz_split;

	key_desc_t bind_debug_1;
	key_desc_t bind_debug_2;
	key_desc_t bind_debug_3;
	key_desc_t bind_debug_4;

	keymap_t * keymap;

private:

	time_t _next_frame;
	time_t _max_wait;

	Time _last_focus_time;
	Time _last_button_press;
	list<client_managed_t *> _client_focused;

	i_rect _root_position;

	vector<page_event_t> * page_areas;


	Cursor xc_left_ptr;
	Cursor xc_fleur;

	Cursor xc_bottom_left_corner;
	Cursor xc_bottom_righ_corner;
	Cursor xc_bottom_side;

	Cursor xc_left_side;
	Cursor xc_right_side;

	Cursor xc_top_right_corner;
	Cursor xc_top_left_corner;
	Cursor xc_top_side;

	i_rect _allocation;

	mutable vector<tree_t *> _all_children_cache;

public:

	theme_t * theme;
	renderable_page_t * rpage;

private:
	page_t(page_t const &);
	page_t &operator=(page_t const &);
public:
	page_t(int argc, char ** argv);
	virtual ~page_t();

	void set_default_pop(notebook_t * x);
	void set_focus(client_managed_t * w, Time tfocus);
	compositor_t * get_render_context();
	display_t * get_xconnection();

	/** short cut **/
	Atom A(atom_e atom) {
		return cnx->A(atom);
	}

	/* run page main loop */
	void run();

	/* scan current root window status, finding mapped windows */
	void scan();

	void process_event(XKeyEvent const & e);
	void process_event_press(XButtonEvent const & e);
	void process_event_release(XButtonEvent const & e);
	void process_event(XMotionEvent const & e);
	/* SubstructureNotifyMask */
	void process_event(XCirculateEvent const & e);
	void process_event(XConfigureEvent const & e);
	void process_event(XCreateWindowEvent const & e);
	void process_event(XDestroyWindowEvent const & e);
	void process_event(XGravityEvent const & e);
	void process_event(XMapEvent const & e);
	void process_event(XReparentEvent const & e);
	void process_event(XUnmapEvent const & e);

	/* SubstructureRedirectMask */
	void process_event(XCirculateRequestEvent const & e);
	void process_event(XConfigureRequestEvent const & e);
	void process_event(XMapRequestEvent const & e);

	/* PropertyChangeMask */
	void process_event(XPropertyEvent const & e);

	/* Unmaskable Events */
	void process_event(XClientMessageEvent const & e);

	/* extension events */
	void process_event(XDamageNotifyEvent const & ev);

	void process_event(XEvent const & e);

	/* update _NET_CLIENT_LIST_STACKING and _NET_CLIENT_LIST */
	void update_client_list();

	/* update _NET_SUPPORTED */
	void update_net_supported();

	/* print some window attributes for debuging */
	void debug_print_window_attributes(Window w, XWindowAttributes &wa);

	/* setup and create managed window */
	client_managed_t * manage(Atom net_wm_type, shared_ptr<client_properties_t> wa);

	/* unmanage a managed window */
	void unmanage(client_managed_t * mw);

	/* put a managed window into a given notebook */
	void insert_window_in_notebook(client_managed_t * x, notebook_t * n, bool prefer_activate);

	/* update viewport and childs allocation */
	void update_allocation();

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

	void update_popup_position(ptr<popup_notebook0_t> p, i_rect & position);
	void update_popup_position(ptr<popup_frame_move_t> p, i_rect & position);

	/* compute the allocation of viewport taking in account DOCKs */
	void compute_viewport_allocation(viewport_t & v);

	void cleanup_client(client_base_t * c);

	void process_net_vm_state_client_message(Window c, long type, Atom state_properties);

	void insert_in_tree_using_transient_for(client_base_t * c);
	void safe_update_transient_for(client_base_t * c);

	client_base_t * get_transient_for(client_base_t * c);
	void logical_raise(client_base_t * c);
	void detach(tree_t * t);

	void safe_raise_window(client_base_t * c);

	/** move to client_base **/
	void compute_client_size_with_constraint(Window c,
			unsigned int max_width, unsigned int max_height, unsigned int & width,
			unsigned int & height);
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
	vector<notebook_t *> get_notebooks(tree_t * base = nullptr);
	/* find where the managed window is */
	notebook_t * find_parent_notebook_for(client_managed_t * mw);
	vector<client_managed_t*> get_managed_windows();
	client_managed_t * find_managed_window_with(Window w);
	viewport_t * find_viewport_for(notebook_t * n);
	void set_window_cursor(Window w, Cursor c);
	void update_windows_stack();
	void update_viewport_layout();
	void remove_viewport(viewport_t * v);
	void destroy_viewport(viewport_t * v);
	void onmap(Window w);
	void create_managed_window(shared_ptr<client_properties_t> c, Atom type);
	void manage_managed_window(client_managed_t * mw, Atom type);
	void ackwoledge_configure_request(XConfigureRequestEvent const & e);
	void create_unmanaged_window(shared_ptr<client_properties_t> c, Atom type);
	void create_dock_window(shared_ptr<client_properties_t> c, Atom type);
	viewport_t * find_mouse_viewport(int x, int y);
	bool get_safe_net_wm_user_time(client_base_t * c, Time & time);
	void update_page_areas();
	void set_desktop_geometry(long width, long height);
	client_base_t * find_client_with(Window w);
	client_base_t * find_client(Window w);
	void remove_client(client_base_t * c);
	void add_client(client_base_t * c);
	vector<tree_t *> childs() const;
	string get_node_name() const;
	void replace(page_component_t * src, page_component_t * by);
	void raise_child(tree_t * t);
	void remove(tree_t * t);

	void attach_dock(client_not_managed_t * uw) {
		docks.push_back(uw);
		uw->set_parent(this);
	}

	void detach_dock(client_not_managed_t * uw) {
		docks.remove(uw);
		uw->set_parent(nullptr);
	}

	void check_x11_extension();

	void create_identity_window();
	void register_wm();
	void register_cm();

	void render(cairo_t * cr, page::time_t time);
	bool need_render(time_t time);

	bool check_for_managed_window(Window w);
	bool check_for_destroyed_window(Window w);

	void update_keymap();
	void update_grabkey();

	client_managed_t * find_hidden_client_with(Window w);

	void prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time);

	vector<page_event_t> * compute_page_areas(
			list<tree_t const *> const & page) const;

	page_component_t * parent() const;
	void set_parent(tree_t * parent);
	void set_parent(page_component_t * parent);

	void set_allocation(i_rect const & r);

	i_rect allocation() const {
		return _allocation;
	}

	void render_legacy(cairo_t * cr, i_rect const & area) const { }


	void update_children_cache() const;
	void get_all_children(vector<tree_t *> & out) const;

	void render();
};


}



#endif /* PAGE_HXX_ */
