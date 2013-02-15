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
#include "render_tree.hxx"
#include "renderable_page.hxx"

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

class page_t {

public:

	typedef std::map<Window, window_t *> window_map_t;
	typedef std::set<notebook_t *> notebook_set_t;

	enum select_e {
		SELECT_NONE,
		SELECT_TAB,
		SELECT_TOP,
		SELECT_BOTTOM,
		SELECT_LEFT,
		SELECT_RIGHT
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
		window_t * c;
		notebook_t * from;
		notebook_t * ns;
		popup_notebook0_t * pn0;
		popup_notebook1_t * pn1;
		std::string name;
		bool popup_is_added;
	};

	enum process_mode_e {
		NORMAL_PROCESS,			// Process event as usual
		SPLIT_GRAB_PROCESS,		// Process event when split is moving
		NOTEBOOK_GRAB_PROCESS	// Process event when notebook tab is moved
	};



	/* this define the current state of page */
	process_mode_e process_mode;

	/* this data are used when you drag&drop a split slider */
	mode_data_split_t mode_data_split;

	/* this data are used when you drag&drop a notebook tab */
	mode_data_notebook_t mode_data_notebook;


	struct page_event_handler_t : public xevent_handler_t {
		page_t & page;
		page_event_handler_t(page_t & page) : page(page) { }
		virtual ~page_event_handler_t() { }
		virtual void process_event(XEvent const & e);
	};

	static double const OPACITY = 0.95;

	xconnection_t cnx;
	render_context_t rnd;

	render_tree_t * rndt;
	renderable_page_t * rpage;

	renderable_list_t popups;

	/* default cursor */
	Cursor cursor;
	Cursor cursor_fleur;

	bool running;

	bool has_fullscreen_size;
	box_t<int> fullscreen_position;

	GKeyFile * conf;

	page_event_handler_t event_handler;

	/* track viewport, split and notebook */
	std::set<viewport_t *> viewport_list;
	std::set<split_t *> split_list;
	notebook_set_t notebook_list;

	// track where a client is stored
	std::map<window_t *, notebook_t *> client_to_notebook_index;
	std::map<notebook_t *, viewport_t *> notebook_to_viewport_index;
	std::map<window_t *, std::pair<viewport_t *, notebook_t *> > fullscreen_client_to_viewport;
	std::map<viewport_t *, notebook_set_t > viewport_to_notebooks_index;

	std::list<Atom> supported_list;


	window_map_t xwindow_to_window_index;

	/* top_level_windows */
	window_set_t all_windows;
	window_set_t normal_clients;
	window_set_t dock_clients;
	window_set_t other_windows;

	/* all know windows in x11 stack order */
	window_list_t windows_stack;

	notebook_t * default_window_pop;
	std::string page_base_dir;
	std::string font;
	std::string font_bold;

	Time last_focus_time;

private:
	window_t * client_focused;
public:

	page_t(page_t const &);
	page_t &operator=(page_t const &);
public:
	page_t(int argc, char ** argv);
	virtual ~page_t();

	virtual void set_default_pop(notebook_t * x);
	virtual void set_focus(window_t * w);
	virtual render_context_t & get_render_context();
	virtual xconnection_t & get_xconnection();


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


	void drag_and_drop_loop();

	void update_client_list();
	void update_net_supported();

	void insert_client(window_t * c);

	void print_window_attributes(Window w, XWindowAttributes &wa);

	void update_focus(window_t * c);
	void read_viewport_layout();

	void manage(window_t * w);
	void unmanage(window_t * w);

	void print_state();

	enum wm_mode_e {
		WM_MODE_IGNORE,
		WM_MODE_AUTO,
		WM_MODE_WITHDRAW,
		WM_MODE_POPUP,
		WM_MODE_ERROR
	};

	wm_mode_e guess_window_state(long know_state, Bool override_redirect,
			bool map_state);

	box_list_t substract_box(box_int_t const &box0, box_int_t const &box1);
	box_list_t substract_box(box_list_t const &box_list, box_int_t const &box1);

	void repair_back_buffer(box_int_t const & area);
	void repair_overlay(box_int_t const & area);

	bool merge_area_macro(box_list_t & list);

	window_t * get_window_t(Window w) {
		window_map_t::iterator i = xwindow_to_window_index.find(w);
		if(i != xwindow_to_window_index.end())
			return i->second;
		else
			return 0;
	}

	window_t * find_client(Window w);


	void add_damage_area(box_int_t const & box);

	void render_flush();

	void insert_window_above_of(window_t * w, Window above);

	void remove_window_from_tree(window_t * x);
	void activate_client(window_t * x);
	void insert_window_in_tree(window_t * x, notebook_t * n);
	void iconify_client(window_t * x);
	void update_allocation();

	void update_window_z();

	void insert_window_in_stack(window_t * x);

	window_t * new_window(Window const w, XWindowAttributes const & wa);
	void delete_window(window_t * w);
	void destroy(window_t * w);

	void fullscreen(window_t * c);
	void unfullscreen(window_t * c);
	void toggle_fullscreen(window_t * c);

	window_list_t get_windows();

	static bool user_time_comp(window_t *, window_t *);
	void restack_all_window();


	bool check_for_start_split(XButtonEvent const & e);
	bool check_for_start_notebook(XButtonEvent const & e);

	void split(notebook_t * nbk, split_type_e type);
	void split_left(notebook_t * nbk, window_t * c);
	void split_right(notebook_t * nbk, window_t * c);
	void split_top(notebook_t * nbk, window_t * c);
	void split_bottom(notebook_t * nbk, window_t * c);
	void notebook_close(notebook_t * src);

	void update_popup_position(popup_notebook0_t * p, int x, int y,
			int w, int h, bool show_popup);

	void fix_allocation(viewport_t & v);

	split_t * new_split(split_type_e type);
	void destroy(split_t * x);

	notebook_t * new_notebook(viewport_t * v);
	void destroy(notebook_t * x);

	viewport_t * new_viewport(box_int_t & area);

	void unmap_set(window_set_t & set);
	void map_set(window_set_t & set);

	viewport_t * find_viewport(window_t * w);

	void process_net_vm_state_client_messate(window_t * c, long type, Atom state_properties);

	void update_transient_for(window_t * w);

	void safe_raise_window(window_t * w);
	void clear_sibbling_child(window_t * w);

	std::string safe_get_window_name(Window w);

};

}

#endif /* PAGE_HXX_ */
