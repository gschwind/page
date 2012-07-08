/*
 * xconnection.hxx
 *
 * copyright (2011) Benoit Gschwind
 *
 */

#ifndef XCONNECTION_HXX_
#define XCONNECTION_HXX_

#include "X11/extensions/Xcomposite.h"
#include "X11/extensions/Xdamage.h"
#include "X11/extensions/Xfixes.h"
#include "X11/extensions/shape.h"
#include <X11/extensions/Xinerama.h>

#include <stdlib.h>
#include <stdio.h>
#include <limits>
#include <algorithm>
#include <list>
#include "box.hxx"
#include "atoms.hxx"
#include <stdexcept>

namespace page {

static char const * const x_function_codes[] = { //
		"", //
				"X_CreateWindow", //   1
				"X_ChangeWindowAttributes", //   2
				"X_GetWindowAttributes", //   3
				"X_DestroyWindow", //   4
				"X_DestroySubwindows", //   5
				"X_ChangeSaveSet", //   6
				"X_ReparentWindow", //   7
				"X_MapWindow", //   8
				"X_MapSubwindows", //   9
				"X_UnmapWindow", //  10
				"X_UnmapSubwindows", //  11
				"X_ConfigureWindow", //  12
				"X_CirculateWindow", //  13
				"X_GetGeometry", //  14
				"X_QueryTree", //  15
				"X_InternAtom", //  16
				"X_GetAtomName", //  17
				"X_ChangeProperty", //  18
				"X_DeleteProperty", //  19
				"X_GetProperty", //  20
				"X_ListProperties", //  21
				"X_SetSelectionOwner", //  22
				"X_GetSelectionOwner", //  23
				"X_ConvertSelection", //  24
				"X_SendEvent", //  25
				"X_GrabPointer", //  26
				"X_UngrabPointer", //                27
				"X_GrabButton", //                   28
				"X_UngrabButton", //                 29
				"X_ChangeActivePointerGrab", //      30
				"X_GrabKeyboard", //                 31
				"X_UngrabKeyboard", //               32
				"X_GrabKey", //                      33
				"X_UngrabKey", //                    34
				"X_AllowEvents", //                  35
				"X_GrabServer", //                   36
				"X_UngrabServer", //                 37
				"X_QueryPointer", //                 38
				"X_GetMotionEvents", //              39
				"X_TranslateCoords", //              40
				"X_WarpPointer", //                  41
				"X_SetInputFocus", //                42
				"X_GetInputFocus", //                43
				"X_QueryKeymap", //                  44
				"X_OpenFont", //                     45
				"X_CloseFont", //                    46
				"X_QueryFont", //                    47
				"X_QueryTextExtents", //             48
				"X_ListFonts", //                    49
				"X_ListFontsWithInfo", //    	    50
				"X_SetFontPath", //                  51
				"X_GetFontPath", //                  52
				"X_CreatePixmap", //                 53
				"X_FreePixmap", //                   54
				"X_CreateGC", //                     55
				"X_ChangeGC", //                     56
				"X_CopyGC", //                       57
				"X_SetDashes", //                    58
				"X_SetClipRectangles", //            59
				"X_FreeGC", //                       60
				"X_ClearArea", //                    61
				"X_CopyArea", //                     62
				"X_CopyPlane", //                    63
				"X_PolyPoint", //                    64
				"X_PolyLine", //                     65
				"X_PolySegment", //                  66
				"X_PolyRectangle", //                67
				"X_PolyArc", //                      68
				"X_FillPoly", //                     69
				"X_PolyFillRectangle", //            70
				"X_PolyFillArc", //                  71
				"X_PutImage", //                     72
				"X_GetImage", //                     73
				"X_PolyText8", //                    74
				"X_PolyText16", //                   75
				"X_ImageText8", //                   76
				"X_ImageText16", //                  77
				"X_CreateColormap", //               78
				"X_FreeColormap", //                 79
				"X_CopyColormapAndFree", //          80
				"X_InstallColormap", //              81
				"X_UninstallColormap", //            82
				"X_ListInstalledColormaps", //       83
				"X_AllocColor", //                   84
				"X_AllocNamedColor", //              85
				"X_AllocColorCells", //              86
				"X_AllocColorPlanes", //             87
				"X_FreeColors", //                   88
				"X_StoreColors", //                  89
				"X_StoreNamedColor", //              90
				"X_QueryColors", //                  91
				"X_LookupColor", //                  92
				"X_CreateCursor", //                 93
				"X_CreateGlyphCursor", //            94
				"X_FreeCursor", //                   95
				"X_RecolorCursor", //                96
				"X_QueryBestSize", //                97
				"X_QueryExtension", //               98
				"X_ListExtensions", //               99
				"X_ChangeKeyboardMapping", //        100
				"X_GetKeyboardMapping", //           101
				"X_ChangeKeyboardControl", //        102
				"X_GetKeyboardControl", //           103
				"X_Bell", //                         104
				"X_ChangePointerControl", //         105
				"X_GetPointerControl", //            106
				"X_SetScreenSaver", //               107
				"X_GetScreenSaver", //               108
				"X_ChangeHosts", //                  109
				"X_ListHosts", //                    110
				"X_SetAccessControl", //             111
				"X_SetCloseDownMode", //             112
				"X_KillClient", //                   113
				"X_RotateProperties", //	            114
				"X_ForceScreenSaver", //	            115
				"X_SetPointerMapping", //            116
				"X_GetPointerMapping", //            117
				"X_SetModifierMapping", //	        118
				"X_GetModifierMapping", //	        119
				"X_NoOperation" //                   127

		};

struct event_t {
	unsigned long serial;
	int type;
};

struct xconnection_t {
	/* main display connection */
	Display * dpy;
	/* main screen */
	int screen;
	/* the root window ID */
	Window xroot;
	/* overlay composite */
	Window composite_overlay;
	/* root window atributes */
	XWindowAttributes root_wa;
	/* size of default root window */
	box_t<int> root_size;

	std::list<event_t> pending;

	int grab_count;

	Time last_know_time;

	int (*old_error_handler)(Display * dpy, XErrorEvent * ev);

	int connection_fd;

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
		Atom WM_TAKE_FOCUS;

		Atom WM_CHANGE_STATE;

		/* ICCCM extend atoms */
		Atom _NET_SUPPORTED;
		Atom _NET_WM_NAME;
		Atom _NET_WM_STATE;
		Atom _NET_WM_STRUT_PARTIAL;

		Atom _NET_WM_WINDOW_TYPE;
		Atom _NET_WM_WINDOW_TYPE_DESKTOP;
		Atom _NET_WM_WINDOW_TYPE_DOCK;
		Atom _NET_WM_WINDOW_TYPE_TOOLBAR;
		Atom _NET_WM_WINDOW_TYPE_MENU;
		Atom _NET_WM_WINDOW_TYPE_UTILITY;
		Atom _NET_WM_WINDOW_TYPE_SPLASH;
		Atom _NET_WM_WINDOW_TYPE_DIALOG;
		Atom _NET_WM_WINDOW_TYPE_DROPDOWN_MENU;
		Atom _NET_WM_WINDOW_TYPE_POPUP_MENU;
		Atom _NET_WM_WINDOW_TYPE_TOOLTIP;
		Atom _NET_WM_WINDOW_TYPE_NOTIFICATION;
		Atom _NET_WM_WINDOW_TYPE_COMBO;
		Atom _NET_WM_WINDOW_TYPE_DND;
		Atom _NET_WM_WINDOW_TYPE_NORMAL;

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
		Atom _NET_WM_STATE_FOCUSED;

		Atom _NET_WM_ICON;

		Atom _NET_WM_ALLOWED_ACTIONS;

		/* _NET_WM_ALLOWED_ACTIONS */
		Atom _NET_WM_ACTION_MOVE; /*never allowed */
		Atom _NET_WM_ACTION_RESIZE; /* never allowed */
		Atom _NET_WM_ACTION_MINIMIZE; /* never allowed */
		Atom _NET_WM_ACTION_SHADE; /* never allowed */
		Atom _NET_WM_ACTION_STICK; /* never allowed */
		Atom _NET_WM_ACTION_MAXIMIZE_HORZ; /* never allowed */
		Atom _NET_WM_ACTION_MAXIMIZE_VERT; /* never allowed */
		Atom _NET_WM_ACTION_FULLSCREEN; /* allowed */
		Atom _NET_WM_ACTION_CHANGE_DESKTOP; /* never allowed */
		Atom _NET_WM_ACTION_CLOSE; /* always allowed */
		Atom _NET_WM_ACTION_ABOVE; /* never allowed */
		Atom _NET_WM_ACTION_BELOW; /* never allowed */

		Atom _NET_FRAME_EXTENTS;

		Atom _NET_CLOSE_WINDOW;

		/* TODO Atoms for root window */
		Atom _NET_DESKTOP_NAMES;
		Atom _NET_ACTIVE_WINDOW;
		Atom _NET_SUPPORTING_WM_CHECK;
		Atom _NET_VIRTUAL_ROOTS;
		Atom _NET_DESKTOP_LAYOUT;

		/* page special protocol */
		Atom PAGE_QUIT; /* quit page */

	} atoms;

	/*damage event handler */
	int damage_event, damage_error;

	/* composite event handler */
	int composite_event, composite_error;

	/* fixes event handler */
	int fixes_event, fixes_error;

	/* xinerama extension handler */
	int xinerama_event, xinerama_error;

	xconnection_t() {
		old_error_handler = XSetErrorHandler(error_handler);

		dpy = XOpenDisplay(0);
		if (dpy == NULL) {
			throw std::runtime_error("Could not open display");
		} else {
			printf("Open display : Success\n");
		}

		connection_fd = ConnectionNumber(dpy);

		grab_count = 0;
		screen = DefaultScreen(dpy);
		xroot = DefaultRootWindow(dpy);
		if (!XGetWindowAttributes(dpy, xroot, &root_wa)) {
			throw std::runtime_error("Cannot get root window attributes");
		} else {
			printf("Get root windows attribute Success\n");
		}

		root_size.x = 0;
		root_size.y = 0;
		root_size.w = root_wa.width;
		root_size.h = root_wa.height;

		// Check if composite is supported.
		if (XCompositeQueryExtension(dpy, &composite_event, &composite_error)) {
			int major = 0, minor = 0; // The highest version we support
			XCompositeQueryVersion(dpy, &major, &minor);
			if (major != 0 || minor < 4) {
				throw std::runtime_error(
						"X Server doesn't support Composite 0.4");
			} else {
				printf("using composite %d.%d\n", major, minor);
			}
		} else {
			throw std::runtime_error("X Server doesn't support Composite");
		}

		// check/init Damage.
		if (!XDamageQueryExtension(dpy, &damage_event, &damage_error)) {
			throw std::runtime_error("Damage extension is not supported");
		} else {
			int major = 0, minor = 0;
			XDamageQueryVersion(dpy, &major, &minor);
			printf("Damage Extension version %d.%d found\n", major, minor);
		}

		if (!XineramaQueryExtension(dpy, &xinerama_event, &xinerama_error)) {
			throw std::runtime_error("Fixes extension is not supported");
		} else {
			int major = 0, minor = 0;
			XineramaQueryVersion(dpy, &major, &minor);
			printf("Xinerama Extension version %d.%d found\n", major, minor);
		}

		/* map & passtrough the overlay */
		composite_overlay = XCompositeGetOverlayWindow(dpy, xroot);
		allow_input_passthrough(composite_overlay);

		//XCompositeRedirectSubwindows(dpy, xroot, CompositeRedirectManual);

		/* initialize all atoms for this connection */
#define ATOM_INIT(name) atoms.name = XInternAtom(dpy, #name, False)

		ATOM_INIT(ATOM);
		ATOM_INIT(CARDINAL);
		ATOM_INIT(WINDOW);

		ATOM_INIT(WM_STATE);
		ATOM_INIT(WM_NAME);
		ATOM_INIT(WM_DELETE_WINDOW);
		ATOM_INIT(WM_PROTOCOLS);
		ATOM_INIT(WM_TAKE_FOCUS);

		ATOM_INIT(WM_NORMAL_HINTS);
		ATOM_INIT(WM_CHANGE_STATE);

		ATOM_INIT(_NET_SUPPORTED);
		ATOM_INIT(_NET_WM_NAME);
		ATOM_INIT(_NET_WM_STATE);
		ATOM_INIT(_NET_WM_STRUT_PARTIAL);

		ATOM_INIT(_NET_WM_WINDOW_TYPE);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_DOCK);

		ATOM_INIT(_NET_WM_WINDOW_TYPE);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_DESKTOP);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_DOCK);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_TOOLBAR);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_MENU);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_UTILITY);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_SPLASH);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_DIALOG);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_POPUP_MENU);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_TOOLTIP);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_NOTIFICATION);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_COMBO);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_DND);
		ATOM_INIT(_NET_WM_WINDOW_TYPE_NORMAL);

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
		ATOM_INIT(_NET_WM_STATE_FOCUSED);

		ATOM_INIT(_NET_WM_ALLOWED_ACTIONS);

		/* _NET_WM_ALLOWED_ACTIONS */
		ATOM_INIT(_NET_WM_ACTION_MOVE);
		/*never allowed */
		ATOM_INIT(_NET_WM_ACTION_RESIZE);
		/* never allowed */
		ATOM_INIT(_NET_WM_ACTION_MINIMIZE);
		/* never allowed */
		ATOM_INIT(_NET_WM_ACTION_SHADE);
		/* never allowed */
		ATOM_INIT(_NET_WM_ACTION_STICK);
		/* never allowed */
		ATOM_INIT(_NET_WM_ACTION_MAXIMIZE_HORZ);
		/* never allowed */
		ATOM_INIT(_NET_WM_ACTION_MAXIMIZE_VERT);
		/* never allowed */
		ATOM_INIT(_NET_WM_ACTION_FULLSCREEN);
		/* allowed */
		ATOM_INIT(_NET_WM_ACTION_CHANGE_DESKTOP);
		/* never allowed */
		ATOM_INIT(_NET_WM_ACTION_CLOSE);
		/* always allowed */
		ATOM_INIT(_NET_WM_ACTION_ABOVE);
		/* never allowed */
		ATOM_INIT(_NET_WM_ACTION_BELOW);
		/* never allowed */

		ATOM_INIT(_NET_CLOSE_WINDOW);

		ATOM_INIT(_NET_FRAME_EXTENTS);

		ATOM_INIT(_NET_WM_ICON);

		ATOM_INIT(PAGE_QUIT);

#undef ATOM_INIT

		/* try to register composite manager */
		if (!register_cm())
			throw std::runtime_error("Another compositor running");

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

	static int error_handler(Display * dpy, XErrorEvent * ev) {
		/* TODO: dump some use full information */
		char buffer[1024];
		XGetErrorText(dpy, ev->error_code, buffer, 1024);
		printf("\e[1;31m%s: %s %lu\e[m\n",
				(ev->request_code < 127 && ev->request_code > 0) ?
						x_function_codes[ev->request_code] : "Unknow", buffer,
				ev->serial);
		return 0;
	}

	void allow_input_passthrough(Window w) {
		// undocumented : http://lists.freedesktop.org/pipermail/xorg/2005-January/005954.html
		XserverRegion region = XFixesCreateRegion(dpy, NULL, 0);
		// Shape for the entire of window.
		XFixesSetWindowShapeRegion(dpy, w, ShapeBounding, 0, 0, 0);
		// input shape was introduced by Keith Packard to define an input area of window
		// by default is the ShapeBounding which is used.
		// here we set input area an empty region.
		XFixesSetWindowShapeRegion(dpy, w, ShapeInput, 0, 0, region);
		XFixesDestroyRegion(dpy, region);
	}

	void unmap(Window w) {
		unsigned long serial = XNextRequest(dpy);
		//printf("UnMap serial: #%lu win: #%lu\n", serial, w);
		XUnmapWindow(dpy, w);
		event_t e;
		e.serial = serial;
		e.type = UnmapNotify;
		pending.push_back(e);
	}

	void reparentwindow(Window w, Window parent, int x, int y) {
		unsigned long serial = XNextRequest(dpy);
		//printf("Reparent serial: #%lu win: #%lu\n", serial, w);
		XReparentWindow(dpy, w, parent, x, y);
		event_t e;
		e.serial = serial;
		e.type = UnmapNotify;
		pending.push_back(e);
	}

	void map(Window w) {
		unsigned long serial = XNextRequest(dpy);
		//printf("Map serial: #%lu win: #%lu\n", serial, w);
		XMapWindow(dpy, w);
		event_t e;
		e.serial = serial;
		e.type = MapNotify;
		pending.push_back(e);
	}

	bool find_pending_event(event_t & e) {
		std::list<event_t>::iterator i = pending.begin();
		while (i != pending.end()) {
			if (e.type == (*i).type && e.serial == (*i).serial)
				return true;
			++i;
		}
		return false;
	}

	static long int last_serial;

	static bool filter(event_t e) __attribute__((no_instrument_function));

	void xnextevent(XEvent * ev) {
		XNextEvent(dpy, ev);
		xconnection_t::last_serial = ev->xany.serial;
		//std::remove_if(pending.begin(), pending.end(), filter);
	}

	/* this fonction come from xcompmgr
	 * it is intend to make page as composite manager */
	bool register_cm() {
		Window w;
		Atom a;
		static char net_wm_cm[] = "_NET_WM_CM_Sxx";

		snprintf(net_wm_cm, sizeof(net_wm_cm), "_NET_WM_CM_S%d", screen);
		a = XInternAtom(dpy, net_wm_cm, False);

		w = XGetSelectionOwner(dpy, a);
		if (w != None) {
			XTextProperty tp;
			char **strs;
			int count;
			Atom winNameAtom = XInternAtom(dpy, "_NET_WM_NAME", False);

			if (!XGetTextProperty(dpy, w, &tp, winNameAtom)
					&& !XGetTextProperty(dpy, w, &tp, XA_WM_NAME)) {
				fprintf(stderr,
						"Another composite manager is already running (0x%lx)\n",
						(unsigned long) w);
				return false;
			}
			if (XmbTextPropertyToTextList(dpy, &tp, &strs, &count) == Success) {
				fprintf(stderr,
						"Another composite manager is already running (%s)\n",
						strs[0]);

				XFreeStringList(strs);
			}

			XFree(tp.value);

			return false;
		}

		w = XCreateSimpleWindow(dpy, RootWindow (dpy, screen), 0, 0, 1, 1, 0,
				None, None);

		Xutf8SetWMProperties(dpy, w, "page", "page", NULL, 0, NULL, NULL, NULL);

		XSetSelectionOwner(dpy, a, w, CurrentTime);

		return true;
	}

	void add_to_save_set(Window w) {
		XAddToSaveSet(dpy, w);
	}

	void select_input(Window w, long int mask) {
		XSelectInput(dpy, w, mask);
	}

	void move_resize(Window w, box_int_t const & size) {
		unsigned long serial = XNextRequest(dpy);
		//printf("Reparent serial: #%lu win: #%lu\n", serial, w);
		XMoveResizeWindow(dpy, w, size.x, size.y, size.w, size.h);
		event_t e;
		e.serial = serial;
		e.type = ConfigureNotify;
		pending.push_back(e);
	}

	void set_window_border_width(Window w, unsigned int width) {
		unsigned long serial = XNextRequest(dpy);
		//printf("Reparent serial: #%lu win: #%lu\n", serial, w);
		XSetWindowBorderWidth(dpy, w, width);
		event_t e;
		e.serial = serial;
		e.type = ConfigureNotify;
		pending.push_back(e);
	}

	void raise_window(Window w) {
		unsigned long serial = XNextRequest(dpy);
		//printf("Reparent serial: #%lu win: #%lu\n", serial, w);
		XRaiseWindow(dpy, w);
		event_t e;
		e.serial = serial;
		e.type = ConfigureNotify;
		pending.push_back(e);
	}


};

}

#endif /* XCONNECTION_HXX_ */
