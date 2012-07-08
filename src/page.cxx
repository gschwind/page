/*
 * page.cxx
 *
 * copyright (2010) Benoit Gschwind
 *
 */

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
		cnx() {

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
	fullscreen_client = 0;
	client_focused = 0;
	running = false;

	gui_s = 0;
	gui_cr = 0;

	back_buffer_s = 0;
	back_buffer_cr = 0;

	composite_overlay_s = 0;
	composite_overlay_cr = 0;
	has_fullscreen_size = false;
}

page_t::~page_t() {
	tree_root->delete_all();
	delete tree_root;
	window_list_t::iterator i = top_level_windows.begin();
	while (i != top_level_windows.end()) {
		delete (*i);
		++i;
	}

	g_key_file_free(conf);



}

/* update main window location */
void page_t::update_page_aera() {
//	int left = 0, right = 0, top = 0, bottom = 0;
//	std::list<client_t *>::iterator i = clients.begin();
//	while (i != clients.end()) {
//		if ((*i)->has_partial_struct) {
//			client_t * c = (*i);
//			if (left < c->struct_left)
//				left = c->struct_left;
//			if (right < c->struct_right)
//				right = c->struct_right;
//			if (top < c->struct_top)
//				top = c->struct_top;
//			if (bottom < c->struct_bottom)
//				bottom = c->struct_bottom;
//		}
//		++i;
//	}
//
//	page_area.x = screen_area.x + left;
//	page_area.y = screen_area.y + top;
//	page_area.w = screen_area.w - (left + right);
//	page_area.h = screen_area.h - (top + bottom);
//
	box_t<int> b(0, 0, cnx.root_size.w, cnx.root_size.h);
	tree_root->update_allocation(b);

}

void page_t::run() {
	XSetWindowAttributes swa;
	XWindowAttributes wa;

	XGetWindowAttributes(cnx.dpy, cnx.composite_overlay, &wa);
	composite_overlay_s = cairo_xlib_surface_create(cnx.dpy,
			cnx.composite_overlay, cnx.root_wa.visual, cnx.root_wa.width,
			cnx.root_wa.height);
	composite_overlay_cr = cairo_create(composite_overlay_s);

	gui_s = cairo_surface_create_similar(composite_overlay_s,
			CAIRO_CONTENT_COLOR, cnx.root_wa.width, cnx.root_wa.height);
	gui_cr = cairo_create(gui_s);

	back_buffer_s = cairo_surface_create_similar(composite_overlay_s,
			CAIRO_CONTENT_COLOR, cnx.root_wa.width, cnx.root_wa.height);
	back_buffer_cr = cairo_create(back_buffer_s);

	printf("root size: %d,%d\n", cnx.root_size.w, cnx.root_size.h);

	tree_root = new root_t(*this);

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

		tree_root->add_aera(x);
	}
	XFree(info);

	update_net_supported();

	/* update number of desktop */
	int32_t number_of_desktop = 1;
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_NUMBER_OF_DESKTOPS,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&number_of_desktop), 1);

	/* define desktop geometry */
	long desktop_geometry[2];
	desktop_geometry[0] = cnx.root_size.w;
	desktop_geometry[1] = cnx.root_size.h;
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_DESKTOP_GEOMETRY,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(desktop_geometry), 2);

	/* set viewport */
	long viewport[2] = { 0, 0 };
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_DESKTOP_VIEWPORT,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(viewport), 2);

	/* set current desktop */
	long current_desktop = 0;
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_CURRENT_DESKTOP,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&current_desktop), 1);

	long showing_desktop = 0;
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_SHOWING_DESKTOP,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&showing_desktop), 1);

	scan();
	update_page_aera();
	render();

	long workarea[4];
	workarea[0] = 0;
	workarea[1] = 0;
	workarea[2] = cnx.root_size.w;
	workarea[3] = cnx.root_size.h;
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_WORKAREA,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char*>(workarea), 4);

	//XSelectInput(cnx.dpy, cnx.xroot, ButtonPressMask | SubstructureNotifyMask | SubstructureRedirectMask);
	//cnx.map(main_window);
	//damage = XDamageCreate(cnx.dpy, main_window, XDamageReportRawRectangles);

	render();
	repair_back_buffer(cnx.root_size);
	repair_overlay(cnx.root_size);

	running = true;
	while (running) {

		XEvent e;
		cnx.xnextevent(&e);
		//printf("##%lu\n", e.xany.serial);
		if (e.type < LASTEvent && e.type > 0) {
			//printf("##%lu\n", e.xany.serial);
			printf("%s serial: #%lu win: #%lu\n", x_event_name[e.type],
					e.xany.serial, e.xany.window);
		}

		if (e.type == ButtonPress) {
			cnx.last_know_time = e.xbutton.time;
			window_t * c = find_client(e.xbutton.window);
			if (c) {
				/* the hidden focus parameter */
				c->focus();
				//update_focus(c);
			} else if (e.xbutton.window == cnx.xroot) {
				tree_root->process_button_press_event(&e);
				render_flush();
			}
			render_flush();
		}

		if (e.type == CirculateNotify) {
			process_event(e.xcirculate);
		} else if (e.type == ConfigureNotify) {
			process_event(e.xconfigure);
		} else if (e.type == CreateNotify) {
			process_event(e.xcreatewindow);
		} else if (e.type == DestroyNotify) {
			process_event(e.xdestroywindow);
		} else if (e.type == GravityNotify) {
			process_event(e.xgravity);
		} else if (e.type == MapNotify) {
			process_event(e.xmap);
		} else if (e.type == ReparentNotify) {
			process_event(e.xreparent);
		} else if (e.type == UnmapNotify) {
			process_event(e.xunmap);
		} else if (e.type == CirculateRequest) {
			process_event(e.xcirculaterequest);
		} else if (e.type == ConfigureRequest) {
			process_event(e.xconfigurerequest);
		} else if (e.type == MapRequest) {
			process_event(e.xmaprequest);
		} else if (e.type == PropertyNotify) {
			process_event(e.xproperty);
		} else if (e.type == ClientMessage) {
			process_event(e.xclient);
		} else if (e.type == cnx.damage_event + XDamageNotify) {
			process_event(reinterpret_cast<XDamageNotifyEvent&>(e));
		}

		if (!cnx.is_not_grab()) {
			fprintf(stderr, "SERVER IS GRAB WHERE IT SHOULDN'T");
			exit(EXIT_FAILURE);
		}
	}
}

void page_t::render() {
	if (fullscreen_client == 0) {
		//cairo_save(main_window_cr);
		tree_root->render();
		repair_overlay(cnx.root_size);
		//cairo_restore(main_window_cr);
	} else {
		//move_fullscreen(fullscreen_client);
		//fullscreen_client->map();
	}
}

enum wm_mode_e {
	WM_MODE_IGNORE, WM_MODE_AUTO, WM_MODE_WITHDRAW, WM_MODE_POPUP, WM_MODE_ERROR
};

page_t::wm_mode_e page_t::guess_window_state(long know_state,
		Bool override_redirect, bool is_map) {
//	if (w_class == InputOnly) {
//		printf("> SET Ignore\n");
//		return WM_MODE_IGNORE;
//	} else {
	if (know_state == IconicState || know_state == NormalState) {
		printf("> SET Auto\n");
		return WM_MODE_AUTO;
	} else if (know_state == WithdrawnState) {
		printf("> SET Withdrawn\n");
		return WM_MODE_WITHDRAW;
	} else {
		if (override_redirect && is_map) {
			printf("> SET POPUP\n");
			return WM_MODE_POPUP;
		} else if (!is_map && override_redirect == 0) {
			printf("> SET Withdrawn\n");
			return WM_MODE_WITHDRAW;
		} else if (override_redirect && !is_map) {
			printf("> SET Ignore\n");
			return WM_MODE_IGNORE;
		} else if (!override_redirect && is_map) {
			printf("> SET Auto\n");
			return WM_MODE_AUTO;
		} else {
			printf("> OUPSSSSS\n");
			return WM_MODE_ERROR;
		}
	}
//	}
}

void page_t::manage(window_t * w) {
	/* WM must ignore this window */
	if (w->is_input_only())
		return;
	w->print_net_wm_window_type();

	if (w->get_wm_state() != WithdrawnState) {
		if (w->is_dock()) {
			w->setup_extends();
			w->map();
		} else if (w->is_popup()) {
			w->set_opacity(OPACITY);
			/* ignore */
		} else if (w->is_normal()) {
			insert_client(w);
		}
		return;
	}

	if (!w->is_map())
		return;

	if (w->is_dock()) {
		w->setup_extends();
		w->map();
	} else if (w->is_popup()) {
		w->set_opacity(OPACITY);
		/* ignore */
	} else if (w->is_normal()) {
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
			if (!XGetWindowAttributes(cnx.dpy, wins[i], &wa))
				continue;
			window_t * w = new window_t(cnx, wins[i], wa);
			top_level_windows.push_back(w);
			w->read_all();
			manage(w);
		}

		XFree(wins);
	}

	/* scan is ended, start to listen root event */
	XSelectInput(cnx.dpy, cnx.xroot,
			ButtonPressMask | SubstructureNotifyMask | SubstructureRedirectMask);
	cnx.ungrab();

	update_client_list();
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

	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_SUPPORTED,
			cnx.atoms.ATOM, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(list), k);

}

void page_t::update_client_list() {

	Window * data = new Window[clients.size()];

	int k = 0;

	window_set_t::iterator i = clients.begin();
	while (i != clients.end()) {
		if ((*i)->get_wm_state() != WithdrawnState) {
			data[k] = (*i)->get_xwin();
			++k;
		}
		++i;
	}

	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_CLIENT_LIST,
			cnx.atoms.WINDOW, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(data), k);
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_CLIENT_LIST_STACKING,
			cnx.atoms.WINDOW, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(data), k);

	delete[] data;
}

/* inspired from dwm */
bool page_t::get_text_prop(Window w, Atom atom, std::string & text) {
	char **list = NULL;
	XTextProperty name;
	XGetTextProperty(cnx.dpy, w, &name, atom);
	if (!name.nitems)
		return false;
	text = (char const *) name.value;
	return true;
}

void page_t::process_event(XCirculateEvent const & e) {
	/* will happpen ? */
	window_t * x = find_window(e.window);
	if (x) {
		if (e.place == PlaceOnTop) {
			top_level_windows.remove(x);
			top_level_windows.push_back(x);
		} else if (e.place == PlaceOnBottom) {
			top_level_windows.remove(x);
			top_level_windows.push_front(x);
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
		add_damage_area(x->get_absolute_extend());
		x->process_configure_notify_event(e);
		add_damage_area(x->get_absolute_extend());
		render_flush();
	}

	window_t * c = find_client(e.window);
	if (c) {
		/* apply page size constraints */
		box_t<int> _;
		tree_root->update_allocation(_);
		return;
	}

}

void page_t::process_event(XCreateWindowEvent const & e) {
	XWindowAttributes wa;
	if (!XGetWindowAttributes(cnx.dpy, e.window, &wa))
		return;
	window_t * w = new window_t(cnx, e.window, wa);
	top_level_windows.push_back(w);
}

void page_t::process_event(XDestroyWindowEvent const & e) {
	window_t * x = find_window(e.window);
	if (x) {
		add_damage_area(x->get_absolute_extend());
		render_flush();
		tree_root->remove_client(x);
		top_level_windows.remove(x);
		clients.erase(x);
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

	if (x->is_popup())
		x->set_opacity(0.90);
	add_damage_area(x->get_absolute_extend());
	render_flush();

	event_t ex;
	ex.serial = e.serial;
	ex.type = e.type;
	bool expected_event = cnx.find_pending_event(ex);

	if (expected_event)
		return;
}

void page_t::process_event(XReparentEvent const & e) {
	/* TODO: track reparent */
	window_t * x = find_window(e.window);
	if (x) {
		/* the window is removed from root */
		if (e.parent != cnx.xroot) {
			top_level_windows.remove(x);
			clients.erase(x);
			tree_root->remove_client(x);
		}
	} else {
		/* a new top level window */
		if (e.parent == cnx.xroot) {
			XWindowAttributes wa;
			if (XGetWindowAttributes(cnx.dpy, e.window, &wa)) {
				window_t * w = new window_t(cnx, e.window, wa);
				top_level_windows.push_back(w);
			}
		}
	}
}

void page_t::process_event(XUnmapEvent const & e) {
	printf("UnmapNotify serial:#%lu event: #%lu win:#%lu \n", e.serial,
			e.window, e.event);

	window_t * x = find_window(e.window);
	if (x) {
		x->unmap_notify();
		add_damage_area(x->get_absolute_extend());
		render_flush();
	}

	event_t ex;
	ex.serial = e.serial;
	ex.type = e.type;
	bool expected_event = cnx.find_pending_event(ex);
	if (expected_event)
		return;

	window_t * c = find_client(e.window);
	if (c) {
		tree_root->remove_client(c);
		clients.erase(c);
	}

}

void page_t::process_event(XCirculateRequestEvent const & e) {
	/* will happpen ? */
	window_t * x = find_window(e.window);
	if (x) {
		if (e.place == PlaceOnTop) {
			top_level_windows.remove(x);
			top_level_windows.push_back(x);
		} else if (e.place == PlaceOnBottom) {
			top_level_windows.remove(x);
			top_level_windows.push_front(x);
		}
	}
}

void page_t::process_event(XConfigureRequestEvent const & e) {

	window_t * x = find_window(e.window);
	if (x) {
		/* restack x */
		set_window_above(x, e.above);
		add_damage_area(x->get_absolute_extend());
		x->process_configure_request_event(e);
		add_damage_area(x->get_absolute_extend());
		render_flush();

	}

	window_t * c = find_client(e.window);
	if (c) {
		/* apply page size constraints */
		box_t<int> _;
		tree_root->update_allocation(_);
		return;
	}

}

void page_t::process_event(XMapRequestEvent const & e) {
	printf("Entering in %s #%p\n", __PRETTY_FUNCTION__, (void *) e.window);
	window_t * x = find_window(e.window);
	if (x) {
		x->read_all();
		x->map();
		manage(x);
	}
}

void page_t::process_event(XPropertyEvent const & e) {
	//printf("Entering in %s on %lu\n", __PRETTY_FUNCTION__,
	//		ev->xproperty.window);

	//printf("%lu\n", ev->xproperty.atom);
	char * name = XGetAtomName(cnx.dpy, e.atom);
	printf("Atom Name = \"%s\"\n", name);
	XFree(name);

	window_t * x = find_window(e.window);
	if (!x)
		return;
	if (e.atom == cnx.atoms._NET_WM_USER_TIME) {
		x->focus();
		update_focus(x);
//		window_t * x = find_client_by_xwindow(x->get_xwin());
//		if (fullscreen_client == 0 || fullscreen_client == c) {
//			tree_root->activate_client(c);
//			/* the hidden parameter of focus */
//			cnx.last_know_time = ev->xproperty.time;
//			c->focus();
//			update_focus(c);
//		}
	} else if (e.atom == cnx.atoms._NET_WM_NAME) {
		x->read_title();
		render();
	} else if (e.atom == cnx.atoms.WM_NAME) {
		x->read_title();
		render();
	} else if (e.atom == cnx.atoms._NET_WM_STRUT_PARTIAL) {
		if (e.state == PropertyNewValue) {
			x->read_partial_struct();
			update_page_aera();
		} else if (e.state == PropertyDelete) {
			x->read_partial_struct();
		}
	} else if (e.atom == cnx.atoms._NET_WM_WINDOW_TYPE) {
		x->read_net_wm_type();
	} else if (e.atom == cnx.atoms._NET_ACTIVE_WINDOW) {
		printf("request to activate %lu\n", e.window);

	} else if (e.atom == cnx.atoms.WM_NORMAL_HINTS) {
		x->read_size_hints();
		render();
	} else if (e.atom == cnx.atoms.WM_PROTOCOLS) {
		x->read_net_wm_protocols();
	}
}

void page_t::process_event(XClientMessageEvent const & e) {
	printf("Entering in %s on %lu\n", __PRETTY_FUNCTION__, e.window);

	//printf("%lu\n", ev->xclient.message_type);
	char * name = XGetAtomName(cnx.dpy, e.message_type);
	printf("Atom Name = \"%s\"\n", name);
	XFree(name);

	if (e.message_type == cnx.atoms._NET_ACTIVE_WINDOW) {
		printf("request to activate %lu\n", e.window);
//		client_t * c = find_client_by_xwindow(ev->xclient.window);
//		if (c) {
//			tree_root->activate_client(c);
//			render();
//		}

	} else if (e.message_type == cnx.atoms._NET_WM_STATE) {
//		client_t * c = find_client_by_xwindow(ev->xclient.window);
//		if (c) {
//			if (ev->xclient.data.l[1] == cnx.atoms._NET_WM_STATE_FULLSCREEN
//					|| ev->xclient.data.l[2]
//							== cnx.atoms._NET_WM_STATE_FULLSCREEN) {
//				switch (ev->xclient.data.l[0]) {
//				case 0:
//					printf("SET normal\n");
//					unfullscreen(c);
//					break;
//				case 1:
//					printf("SET fullscreen\n");
//					fullscreen(c);
//					break;
//				case 2:
//					printf("SET toggle\n");
//					toggle_fullscreen(c);
//					break;
//
//				}
//			}
//
//		}

	} else if (e.message_type == cnx.atoms.WM_CHANGE_STATE) {
		/* client should send this message to go iconic */
		if (e.data.l[0] == IconicState) {
//			window_t * x = find_window(ev->xclient.window);
//			if (x) {
//				printf("Set to iconic %lu\n", x->get_xwin());
//				x->write_wm_state(IconicState);
//				client_t * c = find_client_by_xwindow(ev->xclient.window);
//				if (c)
//					tree_root->iconify_client(c);
//			}
		}
	} else if (e.message_type == cnx.atoms.PAGE_QUIT) {
		running = false;
	} else if (e.message_type == cnx.atoms.WM_PROTOCOLS) {
		char * name = XGetAtomName(cnx.dpy, e.data.l[0]);
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

		XSendEvent(cnx.dpy, e.window, False, NoEventMask, &evx);
	}
}

void page_t::process_event(XDamageNotifyEvent const & e) {
	/* printf here create recursive damage when ^^ */
	//printf("damage event win: #%lu %dx%d+%d+%d\n", e->drawable,
	//		(int) e->area.width, (int) e->area.height, (int) e->area.x,
	//		(int) e->area.y);
	/* if this is a popup, I find the coresponding area on
	 * main window, then I repair the main window.
	 * This avoid multiple repair method.
	 */

	/* create an empty region */
	XserverRegion region = XFixesCreateRegion(cnx.dpy, 0, 0);

	if (!region)
		throw std::runtime_error("could not create region");

	/* get damaged region and remove them from damaged status */
	XDamageSubtract(cnx.dpy, e.damage, None, region);

	window_t * x = find_window(e.drawable);

	XRectangle * rects;
	int nb_rects;

	/* get all rectangles for the damaged region */
	rects = XFixesFetchRegion(cnx.dpy, region, &nb_rects);

	if (rects) {
		for (int i = 0; i < nb_rects; ++i) {
			box_int_t box(rects[i]);
			if (x) {
				x->mark_dirty_retangle(box);
				box_int_t _ = x->get_absolute_extend();
				/* setup absolute damaged area */
				box.x += _.x;
				box.y += _.y;
				pending_damage += box;
			}
		}
		XFree(rects);
	}

	XFixesDestroyRegion(cnx.dpy, region);

	/* if no more damage pending, process repair */
	if (!e.more) {
		render_flush();
	}

}

void page_t::fullscreen(client_t * c) {

//	printf("FUULLLLSCREEENNN\n");
//	if (fullscreen_client != 0) {
//		fullscreen_client->unmap();
//		fullscreen_client->unset_fullscreen();
//		insert_client(fullscreen_client);
//		fullscreen_client = 0;
//		update_focus(0);
//	}
//
//	fullscreen_client = c;
//	tree_root->remove_client(c);
//	c->set_fullscreen();
//	//move_fullscreen(c);
//	c->map();
//	update_focus(c);
//	c->focus();
}

void page_t::unfullscreen(client_t * c) {
//	if (fullscreen_client == c) {
//		fullscreen_client = 0;
//		c->unset_fullscreen();
//		c->unmap();
//		update_focus(0);
//		insert_client(c);
//		render();
//	}
}

void page_t::toggle_fullscreen(client_t * c) {
//	if (c == fullscreen_client) {
//		unfullscreen(c);
//	} else {
//		fullscreen(c);
//	}
}

void page_t::render_flush() {

	box_list_t area = pending_damage.get_area();
	/* update back buffer */
	{
		box_list_t::const_iterator i = area.begin();
		while (i != area.end()) {
			repair_back_buffer(*i);
			++i;
		}
		cairo_surface_flush(back_buffer_s);
	}
	/* update front buffer */
	{
		box_list_t::const_iterator i = area.begin();
		while (i != area.end()) {
			repair_overlay(*i);
			++i;
		}
		cairo_surface_flush(composite_overlay_s);
	}
	pending_damage.clear();

}

void page_t::add_damage_area(box_int_t const & box) {
	pending_damage += box;
}

void page_t::repair_back_buffer(box_int_t const & area) {

	/* complex computation to avoid useless memory blit */

	/* get area of normal window */
	/* remove normal window area from gui repair area */
	region_t<int> gui_area = area;
	for (window_set_t::iterator i = clients.begin(); i != clients.end(); ++i) {
		if ((*i)->is_map()) {
			gui_area -= ((*i)->get_absolute_extend());
		}
	}

	/* render gui */
	cairo_surface_flush(gui_s);
	cairo_reset_clip(back_buffer_cr);
	cairo_set_source_surface(back_buffer_cr, gui_s, 0., 0.);
	for (box_list_t::const_iterator i = gui_area.begin(); i != gui_area.end(); ++i) {
		box_int_t b = (*i);
		cairo_rectangle(back_buffer_cr, b.x, b.y, b.w, b.h);
		cairo_fill(back_buffer_cr);
	}

	/* draw top-level windows, from bottom most to top most */
	cairo_reset_clip(back_buffer_cr);
	/* draw dock normal window */
	for (window_list_t::iterator i = top_level_windows.begin();
			i != top_level_windows.end(); ++i) {
		window_t * c = *i;
		if (!c->is_map())
			continue;
		if (c->is_input_only())
			continue;
		box_int_t clip = area & c->get_absolute_extend();
		if (clip.w > 0 && clip.h > 0) {
			//printf("draw client %dx%d+%d+%d\n", clip.w, clip.h, clip.x, clip.x,
			//		clip.y);
			c->repair1(back_buffer_cr, clip);
		}
	}

	/* draw popups (custom objects) over the screen */
	cairo_reset_clip(back_buffer_cr);
	/* draw dock normal window */
	for (renderable_list_t::iterator i = popups.begin();
			i != popups.end(); ++i) {
		renderable_t * r = *i;
		box_int_t clip = area & r->get_absolute_extend();
		if (clip.w > 0 && clip.h > 0) {
			//printf("draw client %dx%d+%d+%d\n", clip.w, clip.h, clip.x, clip.x,
			//		clip.y);
			r->repair1(back_buffer_cr, clip);
		}
	}

}

void page_t::repair_overlay(box_int_t const & area) {

	cairo_reset_clip(composite_overlay_cr);
	cairo_set_source_surface(composite_overlay_cr, back_buffer_s, 0, 0);
	cairo_rectangle(composite_overlay_cr, area.x, area.y, area.w, area.h);
	cairo_fill(composite_overlay_cr);

	/* for debug purpose */
	if (false) {
		static int color = 0;
		switch (color % 3) {
		case 0:
			cairo_set_source_rgb(composite_overlay_cr, 1.0, 0.0, 0.0);
			break;
		case 1:
			cairo_set_source_rgb(composite_overlay_cr, 1.0, 1.0, 0.0);
			break;
		case 2:
			cairo_set_source_rgb(composite_overlay_cr, 1.0, 0.0, 1.0);
			break;
		}
		++color;
		cairo_set_line_width(composite_overlay_cr, 1.0);
		cairo_rectangle(composite_overlay_cr, area.x + 0.5, area.y + 0.5,
				area.w - 1.0, area.h - 1.0);
		cairo_stroke(composite_overlay_cr);
	}

	cairo_surface_flush(composite_overlay_s);

}

void page_t::print_window_attributes(Window w, XWindowAttributes & wa) {
	//return;
	printf(">>> Window: #%lu\n", w);
	printf("> size: %dx%d+%d+%d\n", wa.width, wa.height, wa.x, wa.y);
	printf("> border_width: %d\n", wa.border_width);
	printf("> depth: %d\n", wa.depth);
	printf("> visual #%p\n", wa.visual);
	printf("> root: #%lu\n", wa.root);
	printf("> class: %d\n", wa.c_class);
	printf("> bit_gravity: %d\n", wa.bit_gravity);
	printf("> win_gravity: %d\n", wa.win_gravity);
	printf("> backing_store: %dlx\n", wa.backing_store);
	printf("> backing_planes: %lx\n", wa.backing_planes);
	printf("> backing_pixel: %lx\n", wa.backing_pixel);
	printf("> save_under: %d\n", wa.save_under);
	printf("> colormap: ?\n");
	printf("> all_event_masks: %08lx\n", wa.all_event_masks);
	printf("> your_event_mask: %08lx\n", wa.your_event_mask);
	printf("> do_not_propagate_mask: %08lx\n", wa.do_not_propagate_mask);
	printf("> override_redirect: %d\n", wa.override_redirect);
	printf("> screen: %p\n", wa.screen);

}

void page_t::insert_client(window_t * w) {
	if (clients.find(w) != clients.end())
		return;
	clients.insert(w);

	if (default_window_pop != 0) {
		if (!default_window_pop->add_client(w)) {
			throw std::runtime_error("Fail to add a client");
		}
	} else {
		if (!tree_root->add_client(w)) {
			throw std::runtime_error("Fail to add a client\n");
		}
	}

	update_client_list();
}

void page_t::update_focus(window_t * c) {

	if (client_focused == c)
		return;

	if (client_focused != 0) {
		client_focused->unset_focused();
	}

	client_focused = c;

	if (client_focused != 0) {
		client_focused->set_focused();
	}
}

void page_t::move_fullscreen(client_t * c) {
//	XMoveResizeWindow(cnx.dpy, c->clipping_window, fullscreen_position.x,
//			fullscreen_position.y, fullscreen_position.w,
//			fullscreen_position.h);
	box_int_t x(0, 0, fullscreen_position.w, fullscreen_position.h);
	c->get_window()->move_resize(x);
}

void page_t::withdraw_to_X(client_t * c) {
//	c->withdraw_to_X();
//	if (c->is_fullscreen()) {
//		move_fullscreen(c);
//		fullscreen(c);
//		update_focus(c);
//	} else {
//		insert_client(c);
//	}
//	return;
}


inline void copy_without(box_list_t::const_iterator x,
		box_list_t::const_iterator y, box_list_t const & list,
		box_list_t & out) {
	box_list_t::const_iterator i = list.begin();
	while (i != list.end()) {
		if (i != x && i != y) {
			out.push_back(*i);
		}
		++i;
	}
}

window_t * page_t::find_window(Window w) {
	return window_t::find_window(&cnx, w);
}

window_t * page_t::find_window(window_set_t const & list, Window w) {
	window_set_t::iterator i = list.begin();
	while (i != list.end()) {
		if ((*i)->is_window(w))
			return (*i);
		++i;
	}
	return 0;
}

void page_t::set_window_above(window_t * w, Window above) {
	assert(above != w->get_xwin());
	top_level_windows.remove(w);
	window_list_t::reverse_iterator i = top_level_windows.rbegin();
	while (i != top_level_windows.rend()) {
		if ((*i)->get_xwin() == above) {
			break;
		}
		++i;
	}
	top_level_windows.insert(i.base(), w);
}

}
