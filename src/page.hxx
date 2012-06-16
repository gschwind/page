/*
 * page.hxx
 *
 * copyright (2010) Benoit Gschwind
 *
 */

#ifndef PAGE_HXX_
#define PAGE_HXX_

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

#include "tree.hxx"
#include "client.hxx"
#include "box.hxx"
#include "icon.hxx"
#include "xconnection.hxx"
#include "popup.hxx"

namespace page_next {

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

	tree_t * tree_root;
	/* managed clients */
	client_list_t clients;
	std::list<popup_t *> popups;
	/* default cursor */
	Cursor cursor;

	int running;
	int selected;

	box_t<int> page_area;
	int start_x, end_x;
	int start_y, end_y;

	cairo_surface_t * composite_overlay_s;
	cairo_t * composite_overlay_cr;

	/* the main window */
	Window main_window;
	cairo_surface_t * main_window_s;
	cairo_t * main_window_cr;

	/* the main window attributes */
	XWindowAttributes wa;

	client_t * focuced;

	int damage_event, damage_error; // The event base is important here
	Damage damage;

	main_t(main_t const &);
	main_t &operator=(main_t const &);
public:
	main_t();
	~main_t();
	void render(cairo_t * cr);
	void render();
	void run();

	Window get_window() {
		return main_window;
	}

	Display * get_dpy() {
		return cnx.dpy;
	}

	cairo_t * get_cairo() {
		cairo_surface_t * surf;
		XGetWindowAttributes(cnx.dpy, main_window, &(wa));
		surf = cairo_xlib_surface_create(cnx.dpy, main_window, wa.visual, wa.width,
				wa.height);
		cairo_t * cr = cairo_create(surf);
		return cr;
	}

	void scan();
	long get_window_state(Window w);
	bool manage(Window w, XWindowAttributes & wa);
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
	void process_damage_event(XEvent * ev);

	void drag_and_drop_loop();

	void update_client_list();
	void update_net_supported();

	void fullscreen(client_t * c);
	void unfullscreen(client_t * c);
	void toggle_fullscreen(client_t * c);

	void print_window_attributes(Window w, XWindowAttributes &wa);

};

}

#endif /* PAGE_HXX_ */
