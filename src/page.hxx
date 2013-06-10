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

#include "tree.hxx"
#include "renderable.hxx"
#include "render_context.hxx"
#include "box.hxx"
#include "icon.hxx"
#include "xconnection.hxx"
#include "window.hxx"
#include "region.hxx"
#include "viewport.hxx"
#include "split.hxx"
#include "notebook.hxx"
#include "popup_split.hxx"
#include "popup_notebook0.hxx"
#include "popup_notebook1.hxx"
#include "page_base.hxx"
#include "default_theme.hxx"
#include "renderable_page.hxx"
#include "managed_window.hxx"
#include "theme.hxx"
#include "minimal_theme.hxx"
#include "config_handler.hxx"

using namespace std;

namespace page {

template<typename T, typename _>
bool has_key(std::map<T, _> const & x, T const & key) {
	typename std::map<T, _>::const_iterator i = x.find(key);
	return i != x.end();
}

template<typename T>
bool has_key(std::set<T> const & x, T const & key) {
	typename std::set<T>::const_iterator i = x.find(key);
	return i != x.end();
}

template<typename T>
bool has_key(std::list<T> const & x, T const & key) {
	typename std::list<T>::const_iterator i = std::find(x.begin(), x.end(), key);
	return i != x.end();
}

template<typename _0, typename _1>
_1 get_safe_value(std::map<_0, _1> & map, _0 key, _1 def) {
	typename std::map<_0, _1>::iterator i = map.find(key);
	if(i != map.end())
		return i->second;
	else
		return def;
}

typedef std::list<box_int_t> box_list_t;

inline void print_buffer__(const char * buf, int size) {
	for (int i = 0; i < size; ++i) {
		printf("%02x", buf[i]);
	}
	printf("\n");
}

class page_t : public xevent_handler_t {

	static long const MANAGED_WINDOW_EVENT_MASK = (ButtonPressMask | ButtonReleaseMask
			| StructureNotifyMask | PropertyChangeMask
			| SubstructureRedirectMask | ExposureMask);

public:

	typedef std::map<Window, window_t *> window_map_t;
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

	struct mode_data_split_t {
		split_t * split;
		box_int_t slider_area;
		double split_ratio;
		popup_split_t * p;
	};

	struct mode_data_notebook_t {
		int start_x;
		int start_y;
		select_e zone;
		managed_window_t * c;
		notebook_t * from;
		notebook_t * ns;
		popup_notebook0_t * pn0;
		popup_notebook1_t * pn1;
		bool popup_is_added;
	};


	struct mode_data_bind_t {
		int start_x;
		int start_y;
		select_e zone;
		managed_window_t * c;
		notebook_t * ns;
		popup_notebook0_t * pn0;
		popup_notebook1_t * pn1;
		bool popup_is_added;
	};

	enum resize_mode_e {
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
		popup_notebook0_t * pn0;
		box_int_t final_position;
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
		PROCESS_FLOATING_GRAB,	// Process event when a floating window is moved
		PROCESS_FLOATING_RESIZE,
		PROCESS_FLOATING_CLOSE,
		PROCESS_FLOATING_BIND
	};

	/* this define the current state of page */
	process_mode_e process_mode;

	/* this data are used when you drag&drop a split slider */
	mode_data_split_t mode_data_split;
	/* this data are used when you drag&drop a notebook tab */
	mode_data_notebook_t mode_data_notebook;
	/* this data are used when you move/resize a floating window */
	mode_data_floating_t mode_data_floating;
	/* this data are used when you bind/drag&drop a floating window */
	mode_data_bind_t mode_data_bind;


	static double const OPACITY = 0.95;

	xconnection_t * cnx;
	render_context_t * rnd;

	renderable_list_t popups;

	/* default cursor */
	Cursor cursor;
	Cursor cursor_fleur;

	bool running;

	config_handler_t conf;

	std::list<Window> _root_window_stack;

	std::map<window_t *, renderable_window_t *> window_to_renderable_context;

	/* list of view ports */
	list<viewport_t *> viewport_list;

	/* main window data base */
	window_map_t xwindow_to_window;

	/* floating windows */
	set<managed_window_t *> managed_window;

	/* store data to allow proper revert fullscreen window to
	 * their original positions */
	map<managed_window_t *, fullscreen_data_t> fullscreen_client_to_viewport;

	list<Atom> supported_list;

	notebook_t * default_window_pop;

	string page_base_dir;
	string font;
	string font_bold;

	list<window_t *> root_stack;

	map<Window, list<Window> > transient_for_map;

	double menu_opacity;

private:
	Time _last_focus_time;
	list<managed_window_t *> _client_focused;

	Window wm_window;

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
	void set_focus(managed_window_t * w);
	render_context_t * get_render_context();
	xconnection_t * get_xconnection();


	void render(cairo_t * cr);
	void render();
	void run();

	void scan();

	long get_window_state(Window w);
	bool get_text_prop(Window w, Atom atom, std::string & text);

	void update_page_aera();

	bool get_all(Window win, Atom prop, Atom type, int size,
			unsigned char **data, unsigned int *num);

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

	void drag_and_drop_loop();

	void update_client_list();
	void update_net_supported();

	void insert_client(window_t * c);

	void print_window_attributes(Window w, XWindowAttributes &wa);

	void update_focus(window_t * c);
	void read_viewport_layout();

	managed_window_t * manage(managed_window_type_e type, window_t * w);
	void unmanage(managed_window_t * mw);

	void print_state();

	window_t * get_window_t(Window w);

	void insert_window_above_of(window_t * w, Window above);

	void remove_window_from_tree(managed_window_t * x);
	void activate_client(managed_window_t * x);
	void insert_window_in_tree(managed_window_t * x, notebook_t * n, bool prefer_activate);
	void iconify_client(managed_window_t * x);
	void update_allocation();

	void insert_window_in_stack(renderable_window_t * x);

	void delete_window(window_t * w);
	void destroy(window_t * w);

	void fullscreen(managed_window_t * c);
	void unfullscreen(managed_window_t * c);
	void toggle_fullscreen(managed_window_t * c);

	static bool user_time_comp(window_t *, window_t *);
	void restack_all_window();


	bool check_for_start_split(XButtonEvent const & e);
	bool check_for_start_notebook(XButtonEvent const & e);

	void split(notebook_t * nbk, split_type_e type);
	void split_left(notebook_t * nbk, managed_window_t * c);
	void split_right(notebook_t * nbk, managed_window_t * c);
	void split_top(notebook_t * nbk, managed_window_t * c);
	void split_bottom(notebook_t * nbk, managed_window_t * c);
	void notebook_close(notebook_t * src);

	void update_popup_position(popup_notebook0_t * p, box_int_t & position, bool show_popup);

	void fix_allocation(viewport_t & v);

	split_t * new_split(split_type_e type);
	void destroy(split_t * x);

	notebook_t * new_notebook(viewport_t * v);
	void destroy(notebook_t * x);

	viewport_t * new_viewport(box_int_t & area);

	void new_renderable_window(window_t * w);
	void destroy_renderable(window_t * w);

	managed_window_t * new_managed_window(managed_window_type_e type, window_t * orig);
	void destroy_managed_window(managed_window_t * mw);

	viewport_t * find_viewport(window_t * w);

	void process_net_vm_state_client_message(window_t * c, long type, Atom state_properties);

	void update_transient_for(window_t * w);

	void safe_raise_window(window_t * w);
	void clear_sibbling_child(Window w);

	std::string safe_get_window_name(Window w);

	bool check_manage(window_t * x);


	window_t * find_root_window(window_t * w);
	window_t * find_client_window(window_t * w);

	void compute_client_size_with_constraint(window_t * c,
			unsigned int max_width, unsigned int max_height, unsigned int & width,
			unsigned int & height);

	void apply_transient_for(list<window_t *> & l);

	void print_tree_windows();

	void bind_window(managed_window_t * mw);
	void unbind_window(managed_window_t * mw);

	void grab_pointer();

	void cleanup_grab(managed_window_t * mw);

	managed_window_t * get_managed(Window w);

	void update_viewport_layout();

	/* cleanup unknown reference type from all indexes/cache
	 * the ugly safe way.
	 **/
	void cleanup_reference(void * ref);

	notebook_t * get_another_notebook(tree_t * x = 0);


	void get_notebooks(list<notebook_t *> & l);
	void get_splits(list<split_t *> & l);

	notebook_t * find_notebook_for(managed_window_t * mw);

	void get_managed_windows(list<managed_window_t *> & l);
	managed_window_t * find_managed_window_with(Window w);

	viewport_t * find_viewport_for(notebook_t * n);

	bool is_valid_notebook(notebook_t * n);

	void update_viewport_number(unsigned int n);

	void set_window_cursor(Window w, Cursor c);

	bool is_focussed(managed_window_t * mw);

};


}



#endif /* PAGE_HXX_ */
