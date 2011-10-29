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
#include "atoms.hxx"
#include "client.hxx"
#include "box.hxx"
#include "icon.hxx"

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

typedef std::list<client_t *> client_list_t;

inline void print_buffer__(const char * buf, int size) {
	for (int i = 0; i < size; ++i) {
		printf("%02x", buf[i]);
	}

	printf("\n");

}

class main_t {

	tree_t * tree_root;
	/* managed clients */
	client_list_t clients;
	/* default cursor */
	Cursor cursor;

	int running;
	int selected;

	/* main display */
	Display *dpy;
	/* main screen */
	int screen;
	/* the root window */
	Window xroot;
	/* root window atributes */
	XWindowAttributes root_wa;
	/* size of default root window */
	int sw, sh, sx, sy;
	box_t<int> page_area;
	int start_x, end_x;
	int start_y, end_y;

	atoms_t atoms;

	/* the main window */
	Window main_window;
	/* the main window attributes */
	XWindowAttributes wa;

	cairo_surface_t * surf;
	cairo_t * cr;

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
		return dpy;
	}

	cairo_t * get_cairo() {
		cairo_surface_t * surf;
		XGetWindowAttributes(dpy, main_window, &(wa));
		surf = cairo_xlib_surface_create(dpy, main_window, wa.visual, wa.width,
				wa.height);
		cairo_t * cr = cairo_create(surf);
		return cr;
	}

	void scan();
	long get_window_state(Window w);
	bool manage(Window w, XWindowAttributes * wa);
	client_t * find_client_by_xwindow(Window w);
	client_t * find_client_by_clipping_window(Window w);
	bool get_text_prop(Window w, Atom atom, std::string & text);

	void update_page_aera();

	void client_update_size_hints(client_t * ths);
	bool client_is_dock(client_t * c);
	bool get_all(Window win, Atom prop, Atom type, int size,
			unsigned char **data, unsigned int *num);

	void process_map_request_event(XEvent * e);
	void process_map_notify_event(XEvent * e);
	void process_unmap_notify_event(XEvent * e);
	void process_property_notify_event(XEvent * ev);
	void process_destroy_notify_event(XEvent * e);
	void process_client_message_event(XEvent * e);

	void update_vm_name(client_t &c);
	void update_net_vm_name(client_t &c);
	void update_title(client_t &c);
	void update_vm_hints(client_t &c);

	void update_client_list();
	void update_net_supported();

	template<typename T, unsigned int SIZE>
	T * get_properties(Window win, Atom prop, Atom type, unsigned int *num) {
		bool ret = false;
		int res;
		unsigned char * xdata = 0;
		Atom ret_type;
		int ret_size;
		unsigned long int ret_items, bytes_left;
		T * result = 0;
		T * data;

		res = XGetWindowProperty(dpy, win, prop, 0L,
				std::numeric_limits<int>::max(), False, type, &ret_type,
				&ret_size, &ret_items, &bytes_left, &xdata);
		if (res == Success) {
			if(bytes_left != 0)
				printf("some bits lefts\n");
			if (ret_size == SIZE && ret_items > 0) {
				result = new T[ret_items];
				data = reinterpret_cast<T*>(xdata);
				for (unsigned int i = 0; i < ret_items; ++i) {
					result[i] = data[i];
					//printf("%d %p\n", data[i], &data[i]);
				}
			}
			if(num)
				*num = ret_items;
			XFree(xdata);
		}
		return result;
	}

	void fullscreen(client_t * c);
	void unfullscreen(client_t * c);
	void toggle_fullscreen(client_t * c);

	long * get_properties32(Window win, Atom prop, Atom type, unsigned int *num);
	short * get_properties16(Window win, Atom prop, Atom type, unsigned int *num);
	char * get_properties8(Window win, Atom prop, Atom type, unsigned int *num);

	void parse_icons(client_t * c);

};

}

#endif /* PAGE_HXX_ */
