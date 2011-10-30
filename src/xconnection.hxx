/*
 * xconnection.hxx
 *
 * copyright (2011) Benoit Gschwind
 *
 */

#ifndef XCONNECTION_HXX_
#define XCONNECTION_HXX_

#include <stdio.h>
#include <limits>
#include "box.hxx"
#include "atoms.hxx"

namespace page_next {

struct xconnection_t {
	/* main display connection */
	Display * dpy;
	/* main screen */
	int screen;
	/* the root window ID */
	Window xroot;
	/* root window atributes */
	XWindowAttributes root_wa;
	/* size of default root window */
	box_t<int> root_size;

	int grab_count;

	struct {
		/* properties type */
		Atom CARDINAL;
		Atom ATOM;
		Atom WINDOW;

		/* ICCCM atoms */
		Atom WM_STATE;
		Atom WM_NAME;
		Atom WM_DELETE_WINDOW;
		Atom WM_PROTOCOLS;
		Atom WM_NORMAL_HINTS;

		/* ICCCM extend atoms */
		Atom _NET_SUPPORTED;
		Atom _NET_WM_NAME;
		Atom _NET_WM_STATE;
		Atom _NET_WM_STRUT_PARTIAL;

		Atom _NET_WM_WINDOW_TYPE;
		Atom _NET_WM_WINDOW_TYPE_DOCK;

		Atom _NET_WM_USER_TIME;

		Atom _NET_CLIENT_LIST;
		Atom _NET_CLIENT_LIST_STACKING;

		Atom _NET_NUMBER_OF_DESKTOPS;
		Atom _NET_DESKTOP_GEOMETRY;
		Atom _NET_DESKTOP_VIEWPORT;
		Atom _NET_CURRENT_DESKTOP;

		Atom _NET_SHOWING_DESKTOP;
		Atom _NET_WORKAREA;

		/* _NET_WM_STATE */
		Atom _NET_WM_STATE_MODAL;
		Atom _NET_WM_STATE_STICKY;
		Atom _NET_WM_STATE_MAXIMIZED_VERT;
		Atom _NET_WM_STATE_MAXIMIZED_HORZ;
		Atom _NET_WM_STATE_SHADED;
		Atom _NET_WM_STATE_SKIP_TASKBAR;
		Atom _NET_WM_STATE_SKIP_PAGER;
		Atom _NET_WM_STATE_HIDDEN;
		Atom _NET_WM_STATE_FULLSCREEN;
		Atom _NET_WM_STATE_ABOVE;
		Atom _NET_WM_STATE_BELOW;
		Atom _NET_WM_STATE_DEMANDS_ATTENTION;

		Atom _NET_WM_ICON;

		/*TODO Atoms for root window */
		Atom _NET_DESKTOP_NAMES;
		Atom _NET_ACTIVE_WINDOW;
		Atom _NET_SUPPORTING_WM_CHECK;
		Atom _NET_VIRTUAL_ROOTS;
		Atom _NET_DESKTOP_LAYOUT;

	} atoms;

	xconnection_t() {
		dpy = XOpenDisplay(0);
		screen = DefaultScreen(dpy);
		xroot = DefaultRootWindow(dpy);
		XGetWindowAttributes(dpy, xroot, &root_wa);
		root_size.x = 0;
		root_size.y = 0;
		root_size.w = root_wa.width;
		root_size.h = root_wa.height;

		/* initialize all atoms for this connection */
#define ATOM_INIT(name) atoms.name = XInternAtom(dpy, #name, False)

		ATOM_INIT(ATOM);
		ATOM_INIT(CARDINAL);
		ATOM_INIT(WINDOW);

		ATOM_INIT(WM_STATE);
		ATOM_INIT(WM_NAME);
		ATOM_INIT(WM_DELETE_WINDOW);
		ATOM_INIT(WM_PROTOCOLS);

		ATOM_INIT(WM_NORMAL_HINTS);

		ATOM_INIT(_NET_SUPPORTED);
		ATOM_INIT(_NET_WM_NAME);
		ATOM_INIT(_NET_WM_STATE);
		ATOM_INIT(_NET_WM_STRUT_PARTIAL);

		ATOM_INIT(_NET_WM_WINDOW_TYPE);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_DOCK);

		ATOM_INIT(_NET_WM_USER_TIME);

		ATOM_INIT(_NET_CLIENT_LIST);
		ATOM_INIT(_NET_CLIENT_LIST_STACKING);

		ATOM_INIT(_NET_NUMBER_OF_DESKTOPS);
		ATOM_INIT(_NET_DESKTOP_GEOMETRY);
		ATOM_INIT(_NET_DESKTOP_VIEWPORT);
		ATOM_INIT(_NET_CURRENT_DESKTOP);

		ATOM_INIT(_NET_SHOWING_DESKTOP);
		ATOM_INIT(_NET_WORKAREA);

		ATOM_INIT(_NET_ACTIVE_WINDOW);

		ATOM_INIT(_NET_WM_STATE_MODAL);
		ATOM_INIT(_NET_WM_STATE_STICKY);
		ATOM_INIT(_NET_WM_STATE_MAXIMIZED_VERT);
		ATOM_INIT(_NET_WM_STATE_MAXIMIZED_HORZ);
		ATOM_INIT(_NET_WM_STATE_SHADED);
		ATOM_INIT(_NET_WM_STATE_SKIP_TASKBAR);
		ATOM_INIT(_NET_WM_STATE_SKIP_PAGER);
		ATOM_INIT(_NET_WM_STATE_HIDDEN);
		ATOM_INIT(_NET_WM_STATE_FULLSCREEN);
		ATOM_INIT(_NET_WM_STATE_ABOVE);
		ATOM_INIT(_NET_WM_STATE_BELOW);
		ATOM_INIT(_NET_WM_STATE_DEMANDS_ATTENTION);

		ATOM_INIT(_NET_WM_ICON);

	}

	void grab() {
		if (grab_count == 0) {
			XGrabServer(dpy);
			XSync(dpy, False);
		}
		++grab_count;
	}

	void ungrab() {
		if (grab_count == 0) {
			fprintf(stderr, "TRY TO UNGRAB NOT GRABBED CONNECTION!\n");
			return;
		}
		--grab_count;
		if (grab_count == 0) {
			XUngrabServer(dpy);
			XFlush(dpy);
		}
	}

	inline bool is_not_grab() {
		return grab_count == 0;
	}

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
			if (bytes_left != 0)
				printf("some bits lefts\n");
			if (ret_size == SIZE && ret_items > 0) {
				result = new T[ret_items];
				data = reinterpret_cast<T*>(xdata);
				for (unsigned int i = 0; i < ret_items; ++i) {
					result[i] = data[i];
					//printf("%d %p\n", data[i], &data[i]);
				}
			}
			if (num)
				*num = ret_items;
			XFree(xdata);
		}
		return result;
	}

	long * get_properties32(Window win, Atom prop, Atom type,
			unsigned int *num) {
		return this->get_properties<long, 32>(win, prop, type, num);
	}

	short * get_properties16(Window win, Atom prop, Atom type,
			unsigned int *num) {
		return this->get_properties<short, 16>(win, prop, type, num);
	}

	char * get_properties8(Window win, Atom prop, Atom type,
			unsigned int *num) {
		return this->get_properties<char, 8>(win, prop, type, num);
	}

};

}

#endif /* XCONNECTION_HXX_ */
