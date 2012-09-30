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

#ifdef ENABLE_TRACE
#include "ftrace_function.hxx"
#endif

#include "tree.hxx"
#include "renderable.hxx"
#include "render_context.hxx"
#include "box.hxx"
#include "icon.hxx"
#include "xconnection.hxx"
#include "window.hxx"
#include "region.hxx"
#include "viewport.hxx"
#include "page_base.hxx"

namespace page {

typedef std::list<box_int_t> box_list_t;

inline void print_buffer__(const char * buf, int size) {
	for (int i = 0; i < size; ++i) {
		printf("%02x", buf[i]);
	}
	printf("\n");
}

class page_t : public page_base_t {

public:

	struct page_event_handler_t : public xevent_handler_t {
		page_t & page;
		page_event_handler_t(page_t & page) : page(page) { }
		virtual ~page_event_handler_t() { }
		virtual void process_event(XEvent const & e);
	};

	static double const OPACITY = 0.95;

	xconnection_t cnx;
	render_context_t rnd;

	renderable_list_t popups;

	/* default cursor */
	Cursor cursor;

	bool running;

	bool has_fullscreen_size;
	box_t<int> fullscreen_position;

	GKeyFile * conf;

	page_event_handler_t event_handler;

	std::list<viewport_t *> viewport_list;

	std::list<Atom> supported_list;

private:
	window_t * client_focused;
public:

	page_t(page_t const &);
	page_t &operator=(page_t const &);
public:
	page_t(int argc, char ** argv);
	~page_t();

	virtual void set_default_pop(tree_t * x);
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
	void process_event(XDamageNotifyEvent const & ev)
			__attribute__((no_instrument_function));


	void drag_and_drop_loop();

	void update_client_list();
	void update_net_supported();

	void insert_client(window_t * c);

	void print_window_attributes(Window w, XWindowAttributes &wa);

	void update_focus(window_t * c);
	void read_viewport_layout();

	void manage(window_t * w);

	enum wm_mode_e {
		WM_MODE_IGNORE,
		WM_MODE_AUTO,
		WM_MODE_WITHDRAW,
		WM_MODE_POPUP,
		WM_MODE_ERROR
	};

	wm_mode_e guess_window_state(long know_state, Bool override_redirect,
			bool map_state);

	//void withdraw_to_X(client_t * c);

	//void move_fullscreen(client_t * c);

	box_list_t substract_box(box_int_t const &box0, box_int_t const &box1);
	box_list_t substract_box(box_list_t const &box_list, box_int_t const &box1);

	void repair_back_buffer(box_int_t const & area);
	void repair_overlay(box_int_t const & area);

	bool merge_area_macro(box_list_t & list);

	window_t * find_window(Window w);
	window_t * find_window(window_list_t const & list, Window w);

	window_t * find_client(Window w) {
		return find_window(managed_windows, w);
	}

	void add_damage_area(box_int_t const & box);

	void render_flush();

	void insert_window_above_of(window_t * w, Window above);

	void remove_client(window_t * x);
	void activate_client(window_t * x);
	void add_client(window_t * x);
	void iconify_client(window_t * x);
	void update_allocation();

	void update_window_z();

	window_t * insert_new_window(Window w, XWindowAttributes const & wa);
	void delete_window(window_t * w);

	void fullscreen(window_t * c);
	void unfullscreen(window_t * c);
	void toggle_fullscreen(window_t * c);

	window_list_t get_windows();

};

}

#endif /* PAGE_HXX_ */
