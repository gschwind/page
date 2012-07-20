/*
 * page.cxx
 *
 * copyright (2010) Benoit Gschwind
 *
 */

#include <X11/keysymdef.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xinerama.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <stdlib.h>
#include <cstring>

#include <sstream>
#include <limits>
#include <stdint.h>
#include <stdexcept>
#include <set>

#include "page.hxx"
#include "box.hxx"
#include "client.hxx"
#include "root.hxx"

namespace page {

long int const ClientEventMask = (StructureNotifyMask | PropertyChangeMask);

char const * x_event_name[LASTEvent] = { 0, 0, "KeyPress", "KeyRelease",
		"ButtonPress", "ButtonRelease", "MotionNotify", "EnterNotify",
		"LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose",
		"GraphicsExpose", "NoExpose", "VisibilityNotify", "CreateNotify",
		"DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
		"ReparentNotify", "ConfigureNotify", "ConfigureRequest",
		"GravityNotify", "ResizeRequest", "CirculateNotify", "CirculateRequest",
		"PropertyNotify", "SelectionClear", "SelectionRequest",
		"SelectionNotify", "ColormapNotify", "ClientMessage", "MappingNotify",
		"GenericEvent" };

page_t::page_t(int argc, char ** argv) :
		cnx(), rnd(cnx), event_handler(*this) {

#ifdef ENABLE_TRACE
	trace_init();
#endif

	conf = g_key_file_new();

	if (argc < 2) {
		throw std::runtime_error("usage : prg <config_file>");
	}

	if (!g_key_file_load_from_file(conf, argv[1], G_KEY_FILE_NONE, 0)) {
		throw std::runtime_error("could not load config file");
	}

	gchar * theme = g_key_file_get_string(conf, "default", "theme_dir", 0);
	if (theme == 0) {
		throw std::runtime_error("no theme_dir found in config file");
	}

	page_base_dir = theme;
	g_free(theme);

	gchar * sfont = g_key_file_get_string(conf, "default", "font_file", 0);
	if (theme == 0) {
		throw std::runtime_error("no font_file found in config file");
	}

	font = sfont;
	g_free(sfont);

	gchar * sfont_bold = g_key_file_get_string(conf, "default",
			"font_bold_file", 0);
	if (theme == 0) {
		throw std::runtime_error("no font_file found in config file");
	}

	font_bold = sfont_bold;
	g_free(sfont_bold);

	default_window_pop = 0;
	client_focused = 0;
	running = false;

	has_fullscreen_size = false;

	cursor = None;
}

page_t::~page_t() {
	window_list_t::iterator i = windows_stack.begin();
	while (i != windows_stack.end()) {
		rnd.remove(*i);
		delete (*i);
		++i;
	}

	if (conf) {
		g_key_file_free(conf);
		conf = 0;
	}

}

void page_t::run() {
	XSetWindowAttributes swa;
	XWindowAttributes wa;

	printf("root size: %d,%d\n", cnx.root_size.w, cnx.root_size.h);

	int n;
	XineramaScreenInfo * info = XineramaQueryScreens(cnx.dpy, &n);

	if (n < 1) {
		printf("NoScreen Found\n");
		exit(1);
	}

	for (int i = 0; i < n; ++i) {
		box_t<int> x(info[i].x_org, info[i].y_org, info[i].width,
				info[i].height);
		if (!has_fullscreen_size) {
			has_fullscreen_size = true;
			fullscreen_position = x;
		}

		viewport_t * v = new viewport_t(*this, x);
		viewport_list.push_back(v);
		v->z = -1;
		rnd.add(v);
	}

	XFree(info);

	update_net_supported();

	/* update number of desktop */
	int32_t number_of_desktop = 1;
	cnx.change_property(cnx.xroot, cnx.atoms._NET_NUMBER_OF_DESKTOPS,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&number_of_desktop), 1);

	/* define desktop geometry */
	long desktop_geometry[2];
	desktop_geometry[0] = cnx.root_size.w;
	desktop_geometry[1] = cnx.root_size.h;
	cnx.change_property(cnx.xroot, cnx.atoms._NET_DESKTOP_GEOMETRY,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(desktop_geometry), 2);

	/* set viewport */
	long viewport[2] = { 0, 0 };
	cnx.change_property(cnx.xroot, cnx.atoms._NET_DESKTOP_VIEWPORT,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(viewport), 2);

	/* set current desktop */
	long current_desktop = 0;
	cnx.change_property(cnx.xroot, cnx.atoms._NET_CURRENT_DESKTOP,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&current_desktop), 1);

	long showing_desktop = 0;
	cnx.change_property(cnx.xroot, cnx.atoms._NET_SHOWING_DESKTOP,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&showing_desktop), 1);

	scan();
	update_allocation();

	long workarea[4];
	workarea[0] = 0;
	workarea[1] = 0;
	workarea[2] = cnx.root_size.w;
	workarea[3] = cnx.root_size.h;
	cnx.change_property(cnx.xroot, cnx.atoms._NET_WORKAREA, cnx.atoms.CARDINAL,
			32, PropModeReplace, reinterpret_cast<unsigned char*>(workarea), 4);

	rnd.add_damage_area(cnx.root_size);
	rnd.render_flush();
	XSync(cnx.dpy, False);
	XGrabKey(cnx.dpy, XKeysymToKeycode(cnx.dpy, XK_f), Mod4Mask, cnx.xroot, True, GrabModeAsync, GrabModeAsync);

	/* add page event handler */
	cnx.add_event_handler(&event_handler);

	running = true;
	while (running) {
		cnx.process_next_event();
	}
}

void page_t::manage(window_t * w) {
	clients_map[w->get_xwin()] = w;
	managed_windows.push_back(w);
	w->read_all();
	if (w->is_dock()) {
		w->setup_extends();
		w->map();
	} else {
		insert_client(w);
	}
}

void page_t::scan() {
	printf("call %s\n", __PRETTY_FUNCTION__);
	unsigned int num;
	Window d1, d2, *wins = 0;
	XWindowAttributes wa = { 0 };
	cnx.grab();
	if (XQueryTree(cnx.dpy, cnx.xroot, &d1, &d2, &wins, &num)) {
		for (unsigned i = 0; i < num; ++i) {
			if (!cnx.get_window_attributes(wins[i], &wa))
				continue;
			print_window_attributes(wins[i], wa);
			window_t * w = create_window(wins[i], wa);

			if (!w->override_redirect() && !w->is_input_only() && w->is_map()) {
				/* ICCCM top level window */
				manage(w);
			} else

			/* special case were a window is on IconicState state */
			if (!w->override_redirect() && !w->is_input_only()
			&& !w->is_map()
			&& w->read_wm_state() == IconicState) {manage (w);
}			}
			XFree(wins);
		}

		/* scan is ended, start to listen root event */
	XSelectInput(cnx.dpy, cnx.xroot,
			ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask
					| SubstructureNotifyMask | SubstructureRedirectMask);
	cnx.ungrab();
	update_window_z();
	update_client_list();
	printf("return %s\n", __PRETTY_FUNCTION__);
}

void page_t::update_net_supported() {
	std::list<Atom> supported_list;

	supported_list.push_back(cnx.atoms._NET_WM_NAME);
	supported_list.push_back(cnx.atoms._NET_WM_USER_TIME);
	supported_list.push_back(cnx.atoms._NET_CLIENT_LIST);
	supported_list.push_back(cnx.atoms._NET_CLIENT_LIST_STACKING);
	supported_list.push_back(cnx.atoms._NET_WM_STRUT_PARTIAL);
	supported_list.push_back(cnx.atoms._NET_NUMBER_OF_DESKTOPS);
	supported_list.push_back(cnx.atoms._NET_DESKTOP_GEOMETRY);
	supported_list.push_back(cnx.atoms._NET_DESKTOP_VIEWPORT);
	supported_list.push_back(cnx.atoms._NET_CURRENT_DESKTOP);
	supported_list.push_back(cnx.atoms._NET_ACTIVE_WINDOW);
	supported_list.push_back(cnx.atoms._NET_WM_STATE_FULLSCREEN);
	supported_list.push_back(cnx.atoms._NET_WM_STATE_FOCUSED);
	supported_list.push_back(cnx.atoms._NET_FRAME_EXTENTS);

	supported_list.push_back(cnx.atoms._NET_WM_ALLOWED_ACTIONS);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_FULLSCREEN);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_ABOVE);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_BELOW);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_CHANGE_DESKTOP);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_CLOSE);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_FULLSCREEN);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_MAXIMIZE_HORZ);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_MAXIMIZE_VERT);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_MINIMIZE);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_MOVE);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_RESIZE);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_SHADE);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_STICK);

	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_COMBO);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_DESKTOP);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_DIALOG);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_DND);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_DOCK);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_DROPDOWN_MENU);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_MENU);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_NORMAL);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_NOTIFICATION);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_POPUP_MENU);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_SPLASH);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_TOOLBAR);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_TOOLTIP);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_UTILITY);

	long * list = new long[supported_list.size()];
	int k = 0;
	for (std::list<Atom>::iterator i = supported_list.begin();
			i != supported_list.end(); ++i, ++k) {
		list[k] = *i;
	}

	cnx.change_property(cnx.xroot, cnx.atoms._NET_SUPPORTED, cnx.atoms.ATOM, 32,
			PropModeReplace, reinterpret_cast<unsigned char *>(list), k);

	delete[] list;

}

void page_t::update_client_list() {

	Window * data = new Window[managed_windows.size() + 1];

	int k = 0;

	window_list_t::iterator i = managed_windows.begin();
	while (i != managed_windows.end()) {
		if ((*i)->get_wm_state() != WithdrawnState) {
			data[k] = (*i)->get_xwin();
			++k;
		}
		++i;
	}

	cnx.change_property(cnx.xroot, cnx.atoms._NET_CLIENT_LIST_STACKING,
			cnx.atoms.WINDOW, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(data), k);
	cnx.change_property(cnx.xroot, cnx.atoms._NET_CLIENT_LIST, cnx.atoms.WINDOW,
			32, PropModeReplace, reinterpret_cast<unsigned char *>(data), k);

	delete[] data;
}

/* inspired from dwm */
bool page_t::get_text_prop(Window w, Atom atom, std::string & text) {
	char **list = NULL;
	XTextProperty name;
	cnx.get_text_property(w, &name, atom);
	if (!name.nitems)
		return false;
	text = (char const *) name.value;
	return true;
}

void page_t::process_event(XKeyEvent const & e) {
	int n;
	KeySym * k = XGetKeyboardMapping(cnx.dpy, e.keycode, 1, &n);

	if(k == 0)
		return;
	if(n == 0) {
		XFree(k);
		return;
	}

	printf("key : %x\n", (unsigned)k[0]);

	if (XK_Super_L == k[0]) {
		if (e.type == KeyPress) {
			_super_is_pressed = true;
		} else {
			_super_is_pressed = false;
		}
	}

	if (XK_f == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
		if (client_focused != 0) {
			toggle_fullscreen(client_focused);
		}
	}

	XFree(k);

}

void page_t::process_event(XCirculateEvent const & e) {
	window_t * x = find_window(e.window);
	if (x) {
		if (e.place == PlaceOnTop) {
			windows_stack.remove(x);
			windows_stack.push_back(x);
			update_window_z();
			rnd.add_damage_area(x->get_absolute_extend());
		} else if (e.place == PlaceOnBottom) {
			windows_stack.remove(x);
			windows_stack.push_front(x);
			update_window_z();
			rnd.add_damage_area(x->get_absolute_extend());
		}
	}
}

void page_t::process_event(XConfigureEvent const & e) {
	printf("Configure %dx%d+%d+%d above:%lu, event:%lu, window:%lu \n", e.width,
			e.height, e.x, e.y, e.above, e.event, e.window);
	/* track window position and stacking */
	window_t * x = find_window(e.window);
	if (x) {
		/* restack x */
		set_window_above(x, e.above);
		/* mark as dirty all area */
		rnd.add_damage_area(x->get_absolute_extend());
		x->process_configure_notify_event(e);
		rnd.add_damage_area(x->get_absolute_extend());
	}
}

void page_t::process_event(XCreateWindowEvent const & e) {
	if (e.send_event == True)
		return;
	if (e.parent != cnx.xroot)
		return;
	/* track created window */
	XWindowAttributes wa;
	if (!cnx.get_window_attributes(e.window, &wa))
		return;

	/* for unkwown reason page get create window event
	 * from an already existing window.
	 */
	window_t * w = find_window(e.window);
	if (!w) {
		create_window(e.window, wa);
	}
}

void page_t::process_event(XDestroyWindowEvent const & e) {

	window_t * c = find_client(e.window);
	if (c) {
		remove_client(c);
		clients_map.erase(e.window);
		managed_windows.remove(c);
	}

	window_t * x = find_window(e.window);
	if (x) {
		rnd.add_damage_area(x->get_absolute_extend());
		windows_stack.remove(x);
		windows_map.erase(e.window);
		update_window_z();
		rnd.remove(x);
		delete x;
	}
}

void page_t::process_event(XGravityEvent const & e) {
	/* Ignore it */
}

void page_t::process_event(XMapEvent const & e) {
	if (e.event != cnx.xroot)
		return;

	window_t * x = find_window(e.window);
	if (!x)
		return;

	x->map_notify();
	if (x == client_focused) {
		x->focus();
	}

	rnd.add_damage_area(x->get_absolute_extend());

	window_t * c = find_client(e.window);
	if (!c) {
		if (!x->is_input_only() && !x->override_redirect()) {
			x->read_all();
			manage(x);
		}
	}

}

void page_t::process_event(XReparentEvent const & e) {
	/* TODO: track reparent */
	window_t * x = find_window(e.window);
	if (x) {
		/* the window is removed from root */
		if (e.parent != cnx.xroot) {
			windows_stack.remove(x);
			windows_map.erase(e.window);
			update_window_z();
			rnd.remove(x);
			remove_client(x);
		}
	} else {
		/* a new top level window */
		if (e.parent == cnx.xroot) {
			XWindowAttributes wa;
			if (cnx.get_window_attributes(e.window, &wa)) {
				create_window(e.window, wa);
			}
		}
	}
}

void page_t::process_event(XUnmapEvent const & e) {
	window_t * x = find_window(e.window);
	if (x) {
		x->unmap_notify();
		rnd.add_damage_area(x->get_absolute_extend());
	}

	event_t ex;
	ex.serial = e.serial;
	ex.type = e.type;
	bool expected_event = cnx.find_pending_event(ex);
	if (expected_event)
		return;

	window_t * c = find_client(e.window);
	if (c) {
		remove_client(c);
		managed_windows.remove(c);
		clients_map.erase(e.window);
	}
}

void page_t::process_event(XCirculateRequestEvent const & e) {
	/* will happpen ? */
	window_t * x = find_window(e.window);
	if (x) {
		if (e.place == PlaceOnTop) {
			cnx.raise_window(e.window);
		} else if (e.place == PlaceOnBottom) {
			cnx.lower_window(e.window);
		}
	}
}

void page_t::process_event(XConfigureRequestEvent const & e) {
	window_t * x = find_window(e.window);
	if (x) {
		rnd.add_damage_area(x->get_absolute_extend());
		window_t * w = find_client(e.window);
		if (w) {
			box_int_t size = x->get_absolute_extend();
			/* restack the window if needed */
			if ((e.value_mask & CWSibling)||(e.value_mask & CWStackMode)){

			XWindowChanges wc;
			long value_mask = 0;

			if(e.value_mask & CWSibling) {
				value_mask |= CWSibling;
				wc.sibling = e.above;
			}

			if (e.value_mask & CWStackMode) {
				value_mask |= CWStackMode;
				wc.stack_mode = e.detail;
			}

			cnx.configure_window(e.window, value_mask,
					&wc);

		}

			/* we never move a window, inform the window with that */
			XEvent ev;
			ev.xconfigure.type = ConfigureNotify;
			ev.xconfigure.display = cnx.dpy;
			ev.xconfigure.event = e.window;
			ev.xconfigure.x = size.x;
			ev.xconfigure.y = size.y;
			ev.xconfigure.width = size.w;
			ev.xconfigure.height = size.h;
			ev.xconfigure.above = None;
			ev.xconfigure.border_width = 0;
			ev.xconfigure.override_redirect = False;
			ev.xconfigure.send_event = True;
			ev.xconfigure.window = e.window;
			cnx.send_event(e.window, False, StructureNotifyMask, &ev);
		} else {
			XWindowChanges wc;
			wc.x = e.x;
			wc.y = e.y;
			wc.height = e.height;
			wc.width = e.width;
			wc.sibling = e.above;
			wc.stack_mode = e.detail;
			wc.border_width = e.border_width;
			cnx.configure_window(e.window, e.value_mask, &wc);
		}

	}
}

void page_t::process_event(XMapRequestEvent const & e) {
	printf("Entering in %s #%p\n", __PRETTY_FUNCTION__, (void *) e.window);
	window_t * x = find_window(e.window);
	if (x) {
		window_t * c = find_client(e.window);
		if (!c && !x->is_input_only()) {
			x->map();
			x->read_all();
			manage(x);
			activate_client(x);
		}
	}
}

void page_t::process_event(XPropertyEvent const & e) {
	char * name = cnx.get_atom_name(e.atom);
	printf("Atom Name = \"%s\"\n", name);
	XFree(name);

	cnx.last_know_time = e.time;

	window_t * x = find_window(e.window);
	if (!x)
		return;
	if (e.atom == cnx.atoms._NET_WM_USER_TIME) {
		window_t * c = find_client(e.window);
		if (c) {
			activate_client(c);
		}
	} else if (e.atom == cnx.atoms._NET_WM_NAME) {
		x->read_title();
	} else if (e.atom == cnx.atoms.WM_NAME) {
		x->read_title();
	} else if (e.atom == cnx.atoms._NET_WM_STRUT_PARTIAL) {
		if (e.state == PropertyNewValue) {
			x->read_partial_struct();
			update_allocation();
		} else if (e.state == PropertyDelete) {
			x->read_partial_struct();
			update_allocation();
		}
	} else if (e.atom == cnx.atoms._NET_WM_WINDOW_TYPE) {
		x->read_net_wm_type();
	} else if (e.atom == cnx.atoms.WM_NORMAL_HINTS) {
		x->read_wm_normal_hints();
		update_allocation();
	} else if (e.atom == cnx.atoms.WM_PROTOCOLS) {
		x->read_net_wm_protocols();
	}
}

void page_t::process_event(XClientMessageEvent const & e) {
	printf("Entering in %s on %lu\n", __PRETTY_FUNCTION__, e.window);

	//printf("%lu\n", ev->xclient.message_type);
	char * name = cnx.get_atom_name(e.message_type);
	printf("Atom Name = \"%s\"\n", name);
	XFree(name);

	if (e.message_type == cnx.atoms._NET_ACTIVE_WINDOW) {
		printf("request to activate %lu\n", e.window);
		window_t * w = find_client(e.window);
		if (w) {
			activate_client(w);
		}
	} else if (e.message_type == cnx.atoms._NET_WM_STATE) {
		window_t * c = find_client(e.window);
		if (c) {
			if (e.data.l[1] == cnx.atoms._NET_WM_STATE_FULLSCREEN
					|| e.data.l[2] == cnx.atoms._NET_WM_STATE_FULLSCREEN) {
				switch (e.data.l[0]) {
				case 0:
					printf("SET normal\n");
					if(!c->is_fullscreen())
						break;
					unfullscreen(c);
					break;
				case 1:
					printf("SET fullscreen\n");
					if(c->is_fullscreen())
						break;
					fullscreen(c);
					break;
				case 2:
					printf("SET toggle\n");
					toggle_fullscreen(c);
					break;

				}
			}
			update_allocation();
		}

	} else if (e.message_type == cnx.atoms.WM_CHANGE_STATE) {
		/* client should send this message to go iconic */
		if (e.data.l[0] == IconicState) {
			window_t * x = find_client(e.window);
			if (x) {
				printf("Set to iconic %lu\n", x->get_xwin());
				x->write_wm_state(IconicState);
				iconify_client(x);
			}
		}
	} else if (e.message_type == cnx.atoms.PAGE_QUIT) {
		running = false;
	} else if (e.message_type == cnx.atoms.WM_PROTOCOLS) {
		char * name = cnx.get_atom_name(e.data.l[0]);
		printf("PROTOCOL Atom Name = \"%s\"\n", name);
		XFree(name);
	} else if (e.message_type == cnx.atoms._NET_CLOSE_WINDOW) {

		XEvent evx;
		evx.xclient.display = cnx.dpy;
		evx.xclient.type = ClientMessage;
		evx.xclient.format = 32;
		evx.xclient.message_type = cnx.atoms.WM_PROTOCOLS;
		evx.xclient.window = e.window;
		evx.xclient.data.l[0] = cnx.atoms.WM_DELETE_WINDOW;
		evx.xclient.data.l[1] = e.data.l[0];

		cnx.send_event(e.window, False, NoEventMask, &evx);
	}
}

void page_t::process_event(XDamageNotifyEvent const & e) {

	/* create an empty region */
	XserverRegion region = XFixesCreateRegion(cnx.dpy, 0, 0);

	if (!region)
		throw std::runtime_error("could not create region");

	/* get damaged region and remove them from damaged status */
	XDamageSubtract(cnx.dpy, e.damage, None, region);

	window_t * x = find_window(e.drawable);
	if (x) {
		XRectangle * rects;
		int nb_rects;

		/* get all rectangles for the damaged region */
		rects = XFixesFetchRegion(cnx.dpy, region, &nb_rects);

		if (rects) {
			for (int i = 0; i < nb_rects; ++i) {
				box_int_t box(rects[i]);

				x->mark_dirty_retangle(box);
				box_int_t _ = x->get_absolute_extend();
				/* setup absolute damaged area */
				box.x += _.x;
				box.y += _.y;
				rnd.add_damage_area(box);

			}
			XFree(rects);
		}

		XFixesDestroyRegion(cnx.dpy, region);
	}

}

void page_t::fullscreen(window_t * c) {
	/* WARNING: Call order is important, change it with caution */
	std::list<viewport_t *>::iterator i = viewport_list.begin();
	while (i != viewport_list.end()) {
		viewport_t * v = (*i);
		window_list_t l = v->get_windows();
		window_t * x = find_window(l, c->get_xwin());
		if (x) {
			if (v->fullscreen_client != 0) {
				v->fullscreen_client->unmap();
				v->fullscreen_client->unset_fullscreen();
				v->add_client(v->fullscreen_client);
				v->fullscreen_client = 0;
				set_focus(0);
			}

			v->remove_client(c);
			v->fullscreen_client = c;
			c->set_fullscreen();
			c->map();
			c->move_resize(v->raw_aera);
			set_focus(c);
			c->focus();
			cnx.raise_window(c->get_xwin());
			return;
		}
		++i;
	}

	if (!viewport_list.empty()) {
		viewport_t * v = viewport_list.front();

		if (v->fullscreen_client != 0) {
			v->fullscreen_client->unmap();
			v->fullscreen_client->unset_fullscreen();
			v->add_client(v->fullscreen_client);
			v->fullscreen_client = 0;
			set_focus(0);
		}

		v->remove_client(c);
		v->fullscreen_client = c;
		c->set_fullscreen();
		c->map();
		c->move_resize(v->raw_aera);
		set_focus(c);
		c->focus();
		cnx.raise_window(c->get_xwin());
		return;
	}

}

void page_t::unfullscreen(window_t * c) {
	/* WARNING: Call order is important, change it with caution */
	std::list<viewport_t *>::iterator i = viewport_list.begin();
	while (i != viewport_list.end()) {
		viewport_t * v = (*i);
		if (v->fullscreen_client == c) {
			v->fullscreen_client->unmap();
			v->fullscreen_client->unset_fullscreen();
			set_focus(0);
			v->remove_client(v->fullscreen_client);
			v->add_client(c);
			update_allocation();
			rnd.add_damage_area(v->raw_aera);
			return;
		}
		++i;
	}
}

void page_t::toggle_fullscreen(window_t * c) {

	/* if is already full screen */
	std::list<viewport_t *>::iterator i = viewport_list.begin();
	while (i != viewport_list.end()) {
		viewport_t * v = (*i);
		if (v->fullscreen_client == c) {
			v->fullscreen_client->unmap();
			v->fullscreen_client->unset_fullscreen();
			set_focus(0);
			v->remove_client(v->fullscreen_client);
			v->add_client(c);
			update_allocation();
			rnd.add_damage_area(v->raw_aera);
			return;
		}
		++i;
	}

	/* if it is not fullscreen */
	fullscreen(c);
}

void page_t::print_window_attributes(Window w, XWindowAttributes & wa) {
	printf(">>> Window: #%lu\n", w);
	printf("> size: %dx%d+%d+%d\n", wa.width, wa.height, wa.x, wa.y);
//	printf("> border_width: %d\n", wa.border_width);
//	printf("> depth: %d\n", wa.depth);
//	printf("> visual #%p\n", wa.visual);
//	printf("> root: #%lu\n", wa.root);
	if (wa.c_class == CopyFromParent) {
		printf("> class: CopyFromParent\n");
	} else if (wa.c_class == InputOutput) {
		printf("> class: InputOutput\n");
	} else if (wa.c_class == InputOnly) {
		printf("> class: InputOnly\n");
	} else {
		printf("> class: Unknown\n");
	}

	if (wa.map_state == IsViewable) {
		printf("> map_state: IsViewable\n");
	} else if (wa.map_state == IsUnviewable) {
		printf("> map_state: IsUnviewable\n");
	} else if (wa.map_state == IsUnmapped) {
		printf("> map_state: IsUnmapped\n");
	} else {
		printf("> map_state: Unknown\n");
	}

//	printf("> bit_gravity: %d\n", wa.bit_gravity);
//	printf("> win_gravity: %d\n", wa.win_gravity);
//	printf("> backing_store: %dlx\n", wa.backing_store);
//	printf("> backing_planes: %lx\n", wa.backing_planes);
//	printf("> backing_pixel: %lx\n", wa.backing_pixel);
//	printf("> save_under: %d\n", wa.save_under);
//	printf("> colormap: ?\n");
//	printf("> all_event_masks: %08lx\n", wa.all_event_masks);
//	printf("> your_event_mask: %08lx\n", wa.your_event_mask);
//	printf("> do_not_propagate_mask: %08lx\n", wa.do_not_propagate_mask);
	printf("> override_redirect: %d\n", wa.override_redirect);
//	printf("> screen: %p\n", wa.screen);

}

void page_t::insert_client(window_t * w) {
	if (default_window_pop != 0) {
		if (!default_window_pop->add_client(w)) {
			throw std::runtime_error("Fail to add a client");
		}
	} else {
		add_client(w);
		if (w->is_fullscreen()) {
			fullscreen(w);
		}
	}
	update_client_list();
}

window_t * page_t::find_window(Window w) {
	return window_t::find_window(&cnx, w);
}

window_t * page_t::find_window(window_list_t const & list, Window w) {
	window_list_t::const_iterator i = list.begin();
	while (i != list.end()) {
		if ((*i)->is_window(w))
			return (*i);
		++i;
	}
	return 0;
}

void page_t::set_window_above(window_t * w, Window above) {
	assert(above != w->get_xwin());
	windows_stack.remove(w);
	window_list_t::reverse_iterator i = windows_stack.rbegin();
	while (i != windows_stack.rend()) {
		if ((*i)->get_xwin() == above) {
			break;
		}
		++i;
	}
	windows_stack.insert(i.base(), w);
	update_window_z();
}

void page_t::page_event_handler_t::process_event(XEvent const & e) {

	if (e.type == MapNotify) {
		printf("#%08lu %s: event = %lu, win = %lu\n", e.xmap.serial,
				x_event_name[e.type], e.xmap.event, e.xmap.window);
	} else if (e.type == DestroyNotify) {
		printf("#%08lu %s: event = %lu, win = %lu\n", e.xdestroywindow.serial,
				x_event_name[e.type], e.xdestroywindow.event,
				e.xdestroywindow.window);
	} else if (e.type == MapRequest) {
		printf("#%08lu %s: parent = %lu, win = %lu\n", e.xmaprequest.serial,
				x_event_name[e.type], e.xmaprequest.parent,
				e.xmaprequest.window);
	} else if (e.type == UnmapNotify) {
		printf("#%08lu %s: event = %lu, win = %lu\n", e.xunmap.serial,
				x_event_name[e.type], e.xunmap.event, e.xunmap.window);
	} else if (e.type == CreateNotify) {
		printf("#%08lu %s: parent = %lu, win = %lu\n", e.xcreatewindow.serial,
				x_event_name[e.type], e.xcreatewindow.parent,
				e.xcreatewindow.window);
	} else if (e.type < LASTEvent && e.type > 0) {
		printf("#%08lu %s: win: #%lu\n", e.xany.serial, x_event_name[e.type],
				e.xany.window);
	}

//	page.cnx.grab();
//
//	/* remove all destroyed windows */
//	XEvent tmp_ev;
//	while (XCheckTypedEvent(page.cnx.dpy, DestroyNotify, &tmp_ev)) {
//		page.process_event(e.xdestroywindow);
//	}

	if (e.type == ButtonPress || e.type == ButtonRelease) {
		page.cnx.last_know_time = e.xbutton.time;
		window_t * c = page.find_client(e.xbutton.window);
		if (c) {
			/* the hidden focus parameter */
			if (page.last_focus_time < page.cnx.last_know_time) {
				c->focus();
				page.set_focus(c);
			}
		} else if (e.xbutton.window == page.cnx.xroot) {
			//tree_root->process_button_press_event(&e);
		}
	}

	if (e.type == KeyPress || e.type == KeyRelease) {
		page.process_event(e.xkey);
	} else if (e.type == CirculateNotify) {
		page.process_event(e.xcirculate);
	} else if (e.type == ConfigureNotify) {
		page.process_event(e.xconfigure);
	} else if (e.type == CreateNotify) {
		page.process_event(e.xcreatewindow);
	} else if (e.type == DestroyNotify) {
		page.process_event(e.xdestroywindow);
	} else if (e.type == GravityNotify) {
		page.process_event(e.xgravity);
	} else if (e.type == MapNotify) {
		page.process_event(e.xmap);
	} else if (e.type == ReparentNotify) {
		page.process_event(e.xreparent);
	} else if (e.type == UnmapNotify) {
		page.process_event(e.xunmap);
	} else if (e.type == CirculateRequest) {
		page.process_event(e.xcirculaterequest);
	} else if (e.type == ConfigureRequest) {
		page.process_event(e.xconfigurerequest);
	} else if (e.type == MapRequest) {
		page.process_event(e.xmaprequest);
	} else if (e.type == PropertyNotify) {
		page.process_event(e.xproperty);
	} else if (e.type == ClientMessage) {
		page.process_event(e.xclient);
	} else if (e.type == page.cnx.damage_event + XDamageNotify) {
		page.process_event(reinterpret_cast<XDamageNotifyEvent const &>(e));
	}

	page.rnd.render_flush();

	//page.cnx.ungrab();

	if (!page.cnx.is_not_grab()) {
		fprintf(stderr, "SERVER IS GRAB WHERE IT SHOULDN'T");
		exit(EXIT_FAILURE);
	}

}

void page_t::remove_client(window_t * x) {
	if (client_focused == x)
		client_focused = 0;
	std::list<viewport_t *>::iterator i = viewport_list.begin();
	while (i != viewport_list.end()) {
		(*i)->remove_client(x);
		++i;
	}
}

void page_t::activate_client(window_t * x) {
	std::list<viewport_t *>::iterator i = viewport_list.begin();
	while (i != viewport_list.end()) {
		(*i)->activate_client(x);
		++i;
	}
}

void page_t::add_client(window_t * x) {
	if (!viewport_list.empty()) {
		viewport_list.front()->add_client(x);
	}
}

void page_t::iconify_client(window_t * x) {
	std::list<viewport_t *>::iterator i = viewport_list.begin();
	while (i != viewport_list.end()) {
		(*i)->iconify_client(x);
		++i;
	}
}

/* update viewport and childs allocation */
void page_t::update_allocation() {
	std::list<viewport_t *>::iterator i = viewport_list.begin();
	while (i != viewport_list.end()) {
		box_int_t x = (*i)->get_absolute_extend();
		(*i)->reconfigure(x);
		++i;
	}
}

void page_t::read_viewport_layout() {
	int n;
	XineramaScreenInfo * info = XineramaQueryScreens(cnx.dpy, &n);

	if (n < 1) {
		throw std::runtime_error("no Xinerama screen Found");
	}

	for (int i = 0; i < n; ++i) {
		box_t<int> x(info[i].x_org, info[i].y_org, info[i].width,
				info[i].height);
		viewport_list.push_back(new viewport_t(*this, x));
	}

	XFree(info);
}

void page_t::set_default_pop(tree_t * x) {
	default_window_pop = x;
}

void page_t::set_focus(window_t * w) {
	last_focus_time = cnx.last_know_time;
	if (client_focused == w)
		return;

	if (client_focused != 0) {
		client_focused->unset_focused();
	}

	client_focused = w;

	if (client_focused != 0) {
		client_focused->set_focused();
	}
}

render_context_t & page_t::get_render_context() {
	return rnd;
}

xconnection_t & page_t::get_xconnection() {
	return cnx;
}

/* update z in renderable according the staking order */
void page_t::update_window_z() {
	int z = 0;
	window_list_t::iterator i = windows_stack.begin();
	while (i != windows_stack.end()) {
		(*i)->z = z;
		++i;
		++z;
	}
}

/* create window handler */
window_t * page_t::create_window(Window w, XWindowAttributes const & wa) {
	window_t * x = new window_t(cnx, w, wa);
	windows_map[w] = x;
	windows_stack.push_back(x);
	update_window_z();
	rnd.add(x);
	return x;
}

/* delete window handler */
void page_t::delete_window(window_t * x) {
	rnd.add_damage_area(x->get_absolute_extend());
	windows_map.erase(x->get_xwin());
	windows_stack.remove(x);
	update_window_z();
	rnd.remove(x);
	managed_windows.remove(x);
	delete x;
}

}
