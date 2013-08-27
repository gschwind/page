/*
 * page.hxx
 *
 * copyright (2010) Benoit Gschwind
 *
 */

#ifndef PAGE_HXX_
#define PAGE_HXX_


#include <glib.h>

#include <features.h>
#include <ctime>
#include <X11/X.h>
#include <X11/cursorfont.h>
#include <assert.h>
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

#include "compositor.hxx"
#include "xconnection.hxx"
#include "notebook.hxx"
#include "split.hxx"
#include "viewport.hxx"
#include "config_handler.hxx"

#include "unmanaged_window.hxx"
#include "managed_window.hxx"

#include "renderable_page.hxx"

#include "popup_notebook0.hxx"
#include "popup_frame_move.hxx"
#include "popup_split.hxx"

#include "simple_theme.hxx"

using namespace std;

namespace page {


typedef std::list<box_int_t> box_list_t;

inline void print_buffer__(const char * buf, int size) {
	for (int i = 0; i < size; ++i) {
		printf("%02x", buf[i]);
	}
	printf("\n");
}

class page_t {


public:

	typedef std::map<Window, Window> window_map_t;
	typedef std::set<notebook_t *> notebook_set_t;

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
	popup_notebook0_t * pn0;
	popup_frame_move_t * pfm;
	popup_split_t * ps;

	struct mode_data_split_t {
		split_t * split;
		box_int_t slider_area;
		double split_ratio;


		mode_data_split_t() {
			split = 0;
			slider_area = box_int_t();
			split_ratio = 0.5;
		}
	};

	struct mode_data_notebook_t {
		int start_x;
		int start_y;
		select_e zone;
		managed_window_t * c;
		notebook_t * from;
		notebook_t * ns;

		mode_data_notebook_t() {
			start_x = 0;
			start_y = 0;
			zone = SELECT_NONE;
			c = 0;
			from = 0;
			ns = 0;
		}

	};


	struct mode_data_bind_t {
		int start_x;
		int start_y;
		select_e zone;
		managed_window_t * c;
		notebook_t * ns;

		mode_data_bind_t() {
			start_x = 0;
			start_y = 0;
			zone = SELECT_NONE;
			c = 0;
			ns = 0;
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
		box_int_t original_position;
		managed_window_t * f;
		box_int_t popup_original_position;
		box_int_t final_position;

		mode_data_floating_t() {
			mode = RESIZE_NONE;
			x_offset = 0;
			y_offset = 0;
			x_root = 0;
			y_root = 0;
			original_position = box_int_t();
			f = 0;
			popup_original_position = box_int_t();
			final_position = box_int_t();
		}

	};

	struct mode_data_fullscreen_t {
		managed_window_t * mw;
		viewport_t * v;
	};

	struct fullscreen_data_t {
		managed_window_t * window;
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
		PROCESS_FULLSCREEN_MOVE // Alt + click to change fullscreen screen to another one
	};

	/* this define the current state of page */
	process_mode_e process_mode;


	enum key_press_mode_e {
		KEY_PRESS_NORMAL,			// Process event as usual
		KEY_PRESS_ALT_TAB,
	};

	key_press_mode_e key_press_mode;

	struct key_mode_data_t {
		managed_window_t * selected;
	};

	key_mode_data_t key_mode_data;

	bool use_internal_compositor;

	/* this data are used when you drag&drop a split slider */
	mode_data_split_t mode_data_split;
	/* this data are used when you drag&drop a notebook tab */
	mode_data_notebook_t mode_data_notebook;
	/* this data are used when you move/resize a floating window */
	mode_data_floating_t mode_data_floating;
	/* this data are used when you bind/drag&drop a floating window */
	mode_data_bind_t mode_data_bind;

	mode_data_fullscreen_t mode_data_fullscreen;

	xconnection_t * cnx;
	compositor_t * rnd;

	/* default cursor */
	Cursor cursor;
	Cursor cursor_fleur;

	bool running;

	config_handler_t conf;

	/** map viewport to real outputs **/
	map<RRCrtc, viewport_t *> viewport_outputs;

	/* all managed windows */
	set<managed_window_t *> managed_window;
	set<unmanaged_window_t *> unmanaged_window;

	/**
	 * Store data to allow proper revert fullscreen window to
	 * their original positions
	 **/
	map<managed_window_t *, fullscreen_data_t> fullscreen_client_to_viewport;

	list<Atom> supported_list;

	notebook_t * default_window_pop;

	string page_base_dir;

	map<Window, list<Window> > transient_for_tree;

	/**
	 * Cache transient for to avoid heavy read on safe_raise window
	 * key is a window, value is WM_TRANSIENT_FOR for this window.
	 **/
	map<Window, Window> transient_for_cache;

private:
	Time _last_focus_time;
	Time _last_button_press;
	list<managed_window_t *> _client_focused;

	/** this window is a mandatory window to handle the wm session **/
	Window wm_window;

	box_t<int> _root_position;

	vector<page_event_t> * page_areas;

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
	void set_focus(managed_window_t * w, Time tfocus);
	compositor_t * get_render_context();
	xconnection_t * get_xconnection();

	/** short cut **/
	Atom A(atom_e atom) {
		return cnx->A(atom);
	}

	void run();
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

	void update_client_list();
	void update_net_supported();

	void print_window_attributes(Window w, XWindowAttributes &wa);

	managed_window_t * manage(managed_window_type_e type, Atom net_wm_type, Window w, XWindowAttributes const & wa);
	void unmanage(managed_window_t * mw);

	void remove_window_from_tree(managed_window_t * x);
	void insert_window_in_tree(managed_window_t * x, notebook_t * n, bool prefer_activate);
	void iconify_client(managed_window_t * x);
	void update_allocation();

	void destroy(Window w);

	void fullscreen(managed_window_t * c, viewport_t * v = 0);
	void unfullscreen(managed_window_t * c);
	void toggle_fullscreen(managed_window_t * c);

	void split(notebook_t * nbk, split_type_e type);
	void split_left(notebook_t * nbk, managed_window_t * c);
	void split_right(notebook_t * nbk, managed_window_t * c);
	void split_top(notebook_t * nbk, managed_window_t * c);
	void split_bottom(notebook_t * nbk, managed_window_t * c);
	void notebook_close(notebook_t * src);

	void update_popup_position(popup_notebook0_t * p, box_int_t & position);
	void update_popup_position(popup_frame_move_t * p, box_int_t & position);

	void fix_allocation(viewport_t & v);

	split_t * new_split(split_type_e type);
	void destroy(split_t * x);

	notebook_t * new_notebook();
	void destroy(notebook_t * x);

	void destroy_managed_window(managed_window_t * mw);

	void process_net_vm_state_client_message(Window c, long type, Atom state_properties);

	void update_transient_for(Window w);
	void cleanup_transient_for_for_window(Window w);

	void safe_raise_window(Window w);
	void clear_transient_for_sibbling_child(Window w);

	Window find_root_window(Window w);
	Window find_client_window(Window w);

	void compute_client_size_with_constraint(Window c,
			unsigned int max_width, unsigned int max_height, unsigned int & width,
			unsigned int & height);


	void print_tree_windows();

	void bind_window(managed_window_t * mw);
	void unbind_window(managed_window_t * mw);

	void grab_pointer();

	void cleanup_grab(managed_window_t * mw);

	notebook_t * get_another_notebook(tree_t * x = 0);


	void get_notebooks(list<notebook_t *> & l);
	void get_splits(list<split_t *> & l);

	notebook_t * find_notebook_for(managed_window_t * mw);

	void get_managed_windows(list<managed_window_t *> & l);
	managed_window_t * find_managed_window_with(Window w);
	unmanaged_window_t * find_unmanaged_window_with(Window w);

	viewport_t * find_viewport_for(notebook_t * n);

	bool is_valid_notebook(notebook_t * n);

	void set_window_cursor(Window w, Cursor c);

	bool is_focussed(managed_window_t * mw);

	void update_windows_stack();

	void rr_update_viewport_layout();
	void destroy_viewport(viewport_t * v);

	Atom A(char const * aname) {
		return cnx->get_atom(aname);
	}

	Atom find_net_wm_type(Window w, bool override_redirect);

	bool onmap(Window w);

	void create_managed_window(Window w, Atom type, XWindowAttributes const & wa);

	void ackwoledge_configure_request(XConfigureRequestEvent const & e);

	void create_unmanaged_window(Window w, Atom type);
	viewport_t * find_mouse_viewport(int x, int y);

	bool get_safe_net_wm_user_time(Window w, Time & time);

	list<viewport_t *> viewports() {
		return list_values(viewport_outputs);
	}

	list<tree_t *> childs() {
		list<tree_t *> l;
		for (map<RRCrtc, viewport_t *>::iterator i = viewport_outputs.begin();
				i != viewport_outputs.end(); ++i) {
			i->second->get_childs(l);
		}
		return l;
	}

	void update_page_areas() {

		if (page_areas != 0) {
			delete page_areas;
		}

		list<tree_t *> xl = childs();
		list<tree_t const *> l(xl.begin(), xl.end());
		page_areas = theme->compute_page_areas(
				list<tree_t const *>(xl.begin(), xl.end()));

	}

	static managed_window_t * _upgrade(managed_window_base_t const * x) {
		return dynamic_cast<managed_window_t *>(const_cast<managed_window_base_t *>(x));
	}

	static notebook_t * _upgrade(notebook_base_t const * x) {
		return dynamic_cast<notebook_t *>(const_cast<notebook_base_t *>(x));
	}

	static split_t * _upgrade(split_base_t const * x) {
		return dynamic_cast<split_t *>(const_cast<split_base_t *>(x));
	}



};


}



#endif /* PAGE_HXX_ */
