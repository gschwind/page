/*
 * page.hxx
 *
 * copyright (2010) Benoit Gschwind
 *
 */

#ifndef PAGE_HXX_
#define PAGE_HXX_

//#define _POSIX_C_SOURCE 199309L

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

/* According to POSIX.1-2001 */
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "tree.hxx"
#include "client.hxx"
#include "box.hxx"
#include "icon.hxx"
#include "xconnection.hxx"
#include "popup.hxx"
#include "root.hxx"
#include "ftrace_function.hxx"

namespace page_next {

typedef std::set<client_t *> client_set_t;
typedef std::list<box_int_t> box_list_t;

enum {
	AtomType,
	WMProtocols,
	WMDelete,
	WMState,
	WMLast,
	NetSupported,
	NetWMName,
	NetWMState,
	NetWMFullscreen,
	NetWMType,
	NetWMTypeDock,
	NetLast,
	AtomLast
};
/* EWMH atoms */

inline void print_buffer__(const char * buf, int size) {
	for (int i = 0; i < size; ++i) {
		printf("%02x", buf[i]);
	}
	printf("\n");
}

class main_t {
public:

	static double const OPACITY = 0.95;

	/* connection will be start, as soon as main is created. */
	xconnection_t cnx;

	std::string page_base_dir;

	client_t * fullscreen_client;
	root_t * tree_root;
	tree_t * default_window_pop;
	/* managed clients */
	client_set_t clients;
	std::list<popup_t *> popups;
	/* default cursor */
	Cursor cursor;

	bool running;

	cairo_surface_t * composite_overlay_s;
	cairo_t * composite_overlay_cr;

	/* the main window */
	//Window main_window;
	//cairo_surface_t * main_window_s;
	//cairo_t * main_window_cr;

	cairo_surface_t * back_buffer_s;
	cairo_t * back_buffer_cr;


	bool has_fullscreen_size;
	box_t<int> fullscreen_position;
	box_list_t pending_damage;

	GKeyFile * conf;

	std::string font;
	std::string font_bold;

private:
	client_t * client_focused;
public:

	Damage damage;

	main_t(main_t const &);
	main_t &operator=(main_t const &);
public:
	main_t(int argc, char ** argv);
	~main_t();
	void render(cairo_t * cr);
	void render();
	void run();

	void scan();
	long get_window_state(Window w);
	client_t * find_client_by_xwindow(Window w);
	popup_t * find_popup_by_xwindow(Window w);
	client_t * find_client_by_clipping_window(Window w);
	bool get_text_prop(Window w, Atom atom, std::string & text);

	void update_page_aera();

	bool get_all(Window win, Atom prop, Atom type, int size,
			unsigned char **data, unsigned int *num);

	void process_map_request_event(XEvent * e);
	void process_map_notify_event(XEvent * e);
	void process_unmap_notify_event(XEvent * e);
	void process_property_notify_event(XEvent * ev);
	void process_destroy_notify_event(XEvent * e);
	void process_client_message_event(XEvent * e);
	void process_damage_event(XEvent * ev)
			__attribute__((no_instrument_function));
	void process_create_window_event(XEvent * e);

	void process_configure_notify_event(XEvent * e);

	void drag_and_drop_loop();

	void update_client_list();
	void update_net_supported();

	void fullscreen(client_t * c);
	void unfullscreen(client_t * c);
	void toggle_fullscreen(client_t * c);

	void insert_client(client_t * c);

	void print_window_attributes(Window w, XWindowAttributes &wa);

	void update_focus(client_t * c);

	enum wm_mode_e {
		WM_MODE_IGNORE,
		WM_MODE_AUTO,
		WM_MODE_WITHDRAW,
		WM_MODE_POPUP,
		WM_MODE_ERROR
	};

	wm_mode_e guess_window_state(long know_state, Bool override_redirect,
			int map_state, int w_class);

	void withdraw_to_X(client_t * c);

	void move_fullscreen(client_t * c);

	box_list_t substract_box(box_int_t const &box0, box_int_t const &box1);
	box_list_t substract_box(box_list_t &box_list, box_int_t &box1);

	void repair_back_buffer(box_int_t const & area);
	void repair_overlay(box_int_t const & area);

	bool merge_area_macro(box_list_t & list);

};

}

#endif /* PAGE_HXX_ */
