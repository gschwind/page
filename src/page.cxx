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

namespace page_next {

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

main_t::main_t(int argc, char ** argv) :
		cnx() {

	trace_init();

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

main_t::~main_t() {
	//cairo_destroy(main_window_cr);
	//cairo_surface_destroy(main_window_s);

	tree_root->delete_all();
	delete tree_root;

	client_set_t::iterator i = clients.begin();
	while (i != clients.end()) {
		delete (*i);
		++i;
	}

	g_key_file_free(conf);

}

/* update main window location */
void main_t::update_page_aera() {
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

void main_t::run() {
	XSetWindowAttributes swa;
	XWindowAttributes wa;

	//main_window = XCreateWindow(cnx.dpy, cnx.xroot, cnx.root_size.x,
	//		cnx.root_size.y, cnx.root_size.w, cnx.root_size.h, 0,
	//		cnx.root_wa.depth, InputOutput, cnx.root_wa.visual, 0, &swa);
	//XCompositeRedirectWindow(cnx.dpy, main_window, CompositeRedirectManual);
	//cursor = XCreateFontCursor(cnx.dpy, XC_left_ptr);
	//XDefineCursor(cnx.dpy, main_window, cursor);
	//printf("Created main window #%lu\n", main_window);
	//XGetWindowAttributes(cnx.dpy, main_window, &(wa));
	//main_window_s = cairo_xlib_surface_create(cnx.dpy, main_window, wa.visual,
	//		wa.width, wa.height);
	//main_window_cr = cairo_create(main_window_s);

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

		if (e.type == ConfigureNotify) {
			process_configure_notify_event(&e);
		} else if (e.type == Expose) {
			printf("Expose #%x\n", (unsigned int) e.xexpose.window);
			//if (e.xexpose.window == main_window)
			//	render();
		} else if (e.type == ButtonPress) {
			cnx.last_know_time = e.xbutton.time;
			client_t * c = find_client_by_clipping_window(e.xbutton.window);
			if (c) {
				/* the hidden focus parameter */
				c->focus();
				update_focus(c);
			} else if (e.xbutton.window == cnx.xroot) {
				tree_root->process_button_press_event(&e);
			}
			render();
		} else if (e.type == MapRequest) {
			//printf("MapRequest\n");
			process_map_request_event(&e);
		} else if (e.type == MapNotify) {
			process_map_notify_event(&e);
		} else if (e.type == UnmapNotify) {
			process_unmap_notify_event(&e);
		} else if (e.type == PropertyNotify) {
			process_property_notify_event(&e);
		} else if (e.type == DestroyNotify) {
			process_destroy_notify_event(&e);
		} else if (e.type == ClientMessage) {
			process_client_message_event(&e);
		} else if (e.type == CreateNotify) {
			process_create_window_event(&e);
		} else if (e.type == cnx.damage_event + XDamageNotify) {
			process_damage_event(&e);
		}

		if (!cnx.is_not_grab()) {
			fprintf(stderr, "SERVER IS GRAB WHERE IT SHOULDN'T");
			exit(EXIT_FAILURE);
		}
	}
}

void main_t::render() {
	//printf("Enter render\n");
	//XGetWindowAttributes(cnx.dpy, main_window, &wa);
	//box_t<int> b(page_area.x, page_area.y, page_area.w, page_area.h);
	//tree_root->update_allocation(b);

	if (fullscreen_client == 0) {
		//cairo_save(main_window_cr);
		tree_root->render();
		repair_overlay(cnx.root_size);
		//cairo_restore(main_window_cr);
	} else {
		move_fullscreen(fullscreen_client);
		fullscreen_client->map();
		//XRaiseWindow(cnx.dpy, fullscreen_client->clipping_window);
	}
	//printf("return render\n");
}

enum wm_mode_e {
	WM_MODE_IGNORE, WM_MODE_AUTO, WM_MODE_WITHDRAW, WM_MODE_POPUP, WM_MODE_ERROR
};

main_t::wm_mode_e main_t::guess_window_state(long know_state,
		Bool override_redirect, int map_state, int w_class) {
	if (w_class == InputOnly) {
		printf("> SET Ignore\n");
		return WM_MODE_IGNORE;
	} else {
		if (know_state == IconicState || know_state == NormalState) {
			printf("> SET Auto\n");
			return WM_MODE_AUTO;
		} else if (know_state == WithdrawnState) {
			printf("> SET Withdrawn\n");
			return WM_MODE_WITHDRAW;
		} else {
			if (override_redirect && map_state != IsUnmapped) {
				printf("> SET POPUP\n");
				return WM_MODE_POPUP;
			} else if (map_state == IsUnmapped && override_redirect == 0) {
				printf("> SET Withdrawn\n");
				return WM_MODE_WITHDRAW;
			} else if (override_redirect && map_state == IsUnmapped) {
				printf("> SET Ignore\n");
				return WM_MODE_IGNORE;
			} else if (!override_redirect && map_state != IsUnmapped) {
				printf("> SET Auto\n");
				return WM_MODE_AUTO;
			} else {
				printf("> OUPSSSSS\n");
				return WM_MODE_ERROR;
			}
		}
	}
}

void main_t::scan() {
	printf("call %s\n", __PRETTY_FUNCTION__);
	unsigned int num;
	Window d1, d2, *wins = 0;
	XWindowAttributes wa = { 0 };
	cnx.grab();
	if (XQueryTree(cnx.dpy, cnx.xroot, &d1, &d2, &wins, &num)) {
		for (unsigned i = 0; i < num; ++i) {
			client_t * c;
			popup_t * p;
			if (!XGetWindowAttributes(cnx.dpy, wins[i], &wa))
				continue;
			if (wa.c_class == InputOnly)
				continue;

			//print_window_attributes(wins[i], wa);

			long state = get_window_state(wins[i]);

			/* Folowing ICCCM the following rules are ok ... */
			switch (guess_window_state(state, wa.override_redirect,
					wa.map_state, wa.c_class)) {
			case WM_MODE_AUTO:
				if (find_client_by_xwindow(wins[i]) != 0) {
					printf("Window %p is already managed\n", c);
					continue;
				}
				/* do not manage clipping window */
				if (find_client_by_clipping_window(wins[i]) != 0)
					continue;
				c = new client_t(cnx, wins[i], wa, NormalState);
				withdraw_to_X(c);
				break;
			case WM_MODE_WITHDRAW:
				c = new client_t(cnx, wins[i], wa, WithdrawnState);
				clients.insert(c);
				break;
			case WM_MODE_POPUP:
				p = new popup_window_t(cnx.dpy, wins[i], wa);
				popups.push_back(p);
				break;
			case WM_MODE_IGNORE:
				break;
			case WM_MODE_ERROR:
			default:
				printf("Found an unexpected window state\n");
				break;
			}
		}

		XFree(wins);
	}
	/* scan is ended, start to listen event */
	XSelectInput(cnx.dpy, cnx.xroot,
			ButtonPressMask | SubstructureNotifyMask | SubstructureRedirectMask);
	cnx.ungrab();
	update_client_list();
}

client_t * main_t::find_client_by_xwindow(Window w) {
	client_set_t::iterator i = clients.begin();
	while (i != clients.end()) {
		if ((*i)->xwin == w)
			return (*i);
		++i;
	}
	return 0;
}

popup_t * main_t::find_popup_by_xwindow(Window w) {
	std::list<popup_t *>::iterator i = popups.begin();
	while (i != popups.end()) {
		if ((*i)->is_window(w))
			return (*i);
		++i;
	}
	return 0;
}

client_t * main_t::find_client_by_clipping_window(Window w) {
//	client_set_t::iterator i = clients.begin();
//	while (i != clients.end()) {
//		if ((*i)->clipping_window == w)
//			return (*i);
//		++i;
//	}

	return 0;
}

void main_t::update_net_supported() {

	const int N_SUPPORTED = 15;
	Atom supported_list[N_SUPPORTED];

	supported_list[0] = cnx.atoms._NET_WM_NAME;
	supported_list[1] = cnx.atoms._NET_WM_USER_TIME;
	supported_list[2] = cnx.atoms._NET_CLIENT_LIST;
	supported_list[3] = cnx.atoms._NET_WM_STRUT_PARTIAL;
	supported_list[4] = cnx.atoms._NET_NUMBER_OF_DESKTOPS;
	supported_list[5] = cnx.atoms._NET_DESKTOP_GEOMETRY;
	supported_list[6] = cnx.atoms._NET_DESKTOP_VIEWPORT;
	supported_list[7] = cnx.atoms._NET_CURRENT_DESKTOP;
	supported_list[8] = cnx.atoms._NET_ACTIVE_WINDOW;
	supported_list[9] = cnx.atoms._NET_WM_STATE_FULLSCREEN;
	supported_list[10] = cnx.atoms._NET_WM_STATE_FOCUSED;
	supported_list[11] = cnx.atoms._NET_FRAME_EXTENTS;
	supported_list[12] = cnx.atoms._NET_WM_ALLOWED_ACTIONS;
	supported_list[13] = cnx.atoms._NET_WM_ACTION_FULLSCREEN;
	supported_list[14] = cnx.atoms._NET_WM_ACTION_CLOSE;

	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_SUPPORTED,
			cnx.atoms.ATOM, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(supported_list), N_SUPPORTED);

}

void main_t::update_client_list() {

	Window * data = new Window[clients.size()];

	int k = 0;

	client_set_t::iterator i = clients.begin();
	while (i != clients.end()) {
		if ((*i)->wm_state != WithdrawnState) {
			data[k] = (*i)->xwin;
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

long main_t::get_window_state(Window w) {
	int format;
	long result = -1;
	long * p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(cnx.dpy, w, cnx.atoms.WM_STATE, 0L, 2L, False,
			cnx.atoms.WM_STATE, &real, &format, &n, &extra,
			(unsigned char **) &p) != Success)
		return -1;

	if (n != 0 && format == 32 && real == cnx.atoms.WM_STATE) {
		result = p[0];
	} else {
		printf("Error in WM_STATE %lu %d %lu\n", n, format, real);
		return -1;
	}
	XFree(p);
	return result;
}

/* inspired from dwm */
bool main_t::get_text_prop(Window w, Atom atom, std::string & text) {
	char **list = NULL;
	XTextProperty name;
	XGetTextProperty(cnx.dpy, w, &name, atom);
	if (!name.nitems)
		return false;
	text = (char const *) name.value;
	return true;
}

void main_t::process_map_request_event(XEvent * e) {
	printf("Entering in %s #%p\n", __PRETTY_FUNCTION__,
			(void *) e->xmaprequest.window);
	if (e->xmaprequest.parent != cnx.xroot)
		return;

	Window w = e->xmaprequest.window;
	/* secure the map request */
	//cnx.grab();
//	XEvent ev;
//	if (XCheckTypedWindowEvent(cnx.dpy, e->xmaprequest.window, DestroyNotify,
//			&ev)) {
//		/* the window is already destroyed, return */
//		cnx.ungrab();
//		return;
//	}
//
//	if (XCheckTypedWindowEvent(cnx.dpy, e->xmaprequest.window, UnmapNotify,
//			&ev)) {
//		/* the window is already unmapped, return */
//		cnx.ungrab();
//		return;
//	}
	/* should never happen */
	XWindowAttributes wa;
	if (!XGetWindowAttributes(cnx.dpy, w, &wa)) {
		//cnx.ungrab();
		return;
	}

	//print_window_attributes(w, wa);

	client_t * c = find_client_by_xwindow(w);
	if (c) {
		if (c->wm_state == WithdrawnState) {
			withdraw_to_X(c);
		} else {
			/* if window is in Iconic state, map mean go to Normal */
			c->set_wm_state(NormalState);
			tree_root->activate_client(c);
		}
	}

	render();
	update_client_list();
	//cnx.ungrab();
	printf("Return from %s #%p\n", __PRETTY_FUNCTION__,
			(void *) e->xmaprequest.window);
	return;

}

void main_t::process_map_notify_event(XEvent * e) {
	printf("MapNotify serial:#%lu win:#%lu\n", e->xmap.serial, e->xmap.window);
	if (e->xmap.event != cnx.xroot)
		return;

	client_t * c = find_client_by_xwindow(e->xmap.window);
	if (c) {
		if (client_focused == c) {
			c->focus();
		}

		if (c->wm_state != WithdrawnState)
			return;
	}

	if (e->xmap.event == cnx.xroot) {
		/* pop up menu do not throw map_request_notify */
		XWindowAttributes wa;
		if (!XGetWindowAttributes(cnx.dpy, e->xmap.window, &wa)) {
			return;
		}

		//print_window_attributes(e->xmap.window, wa);

		if (wa.override_redirect) {
			printf("add popup\n");
			popup_t * c = new popup_window_t(cnx.dpy, e->xmap.window, wa);
			popups.push_back(c);
			repair_back_buffer(c->get_absolute_extend());
			repair_overlay(c->get_absolute_extend());
		}

	}

	printf("Return MapNotify\n");
}

void main_t::process_unmap_notify_event(XEvent * e) {
	printf("UnmapNotify serial:#%lu event: #%lu win:#%lu \n", e->xunmap.serial,
			e->xunmap.window, e->xunmap.event);

	/* remove popup */
	popup_t * p = find_popup_by_xwindow(e->xunmap.window);
	if (p) {
		popups.remove(p);
		XDamageNotifyEvent ev;
		box_int_t area = p->get_absolute_extend();
		repair_back_buffer(area);
		repair_overlay(area);
		delete p;
		return;
	}

	if (e->xunmap.event == cnx.xroot)
		return;

	client_t * c = find_client_by_xwindow(e->xunmap.window);

	event_t ex;
	ex.serial = e->xunmap.serial;
	ex.type = e->xunmap.type;
	bool expected_event = cnx.find_pending_event(ex);

	if (!c)
		return;
	if (expected_event) {
		printf("Expected Unmap\n");
	} else {
		/* Syntetic unmap mean Normal/Iconic to WithDraw */
		/* some client do not honor syntetic unmap ... */
		//if (e->xunmap.send_event) {
		printf("Unmap form send event\n");
		cnx.reparentwindow(c->xwin, cnx.xroot, 0, 0);
		XRemoveFromSaveSet(cnx.dpy, c->xwin);
		if (fullscreen_client == c)
			fullscreen_client = 0;
		if (client_focused == c) {
			update_focus(0);
		}
		tree_root->remove_client(c);
		if (c->has_partial_struct)
			update_page_aera();
		if (!c->is_dock) {
			//XDestroyWindow(cnx.dpy, c->clipping_window);
			//c->clipping_window = None;
		}
		c->set_wm_state(WithdrawnState);
		update_client_list();
		render();
		//}
	}
}

void main_t::process_destroy_notify_event(XEvent * e) {
	//printf("DestroyNotify destroy : #%lu, event : #%lu\n", e->xunmap.window,
	//		e->xunmap.event);

	client_t * c = find_client_by_xwindow(e->xmap.window);
	if (c) {
		clients.erase(c);
		if (fullscreen_client == c)
			fullscreen_client = 0;
		if (client_focused == c) {
			update_focus(0);
		}
		tree_root->remove_client(c);
		if (c->has_partial_struct)
			update_page_aera();
		update_client_list();
//		if (c->clipping_window != None)
//			XDestroyWindow(cnx.dpy, c->clipping_window);
		delete c;
		render();
	}

	popup_t * p = find_popup_by_xwindow(e->xmap.window);

	if (p) {
		popups.remove(p);
		box_int_t area = p->get_absolute_extend();
		repair_back_buffer(area);
		repair_overlay(area);
		delete p;
	}
}

void main_t::process_property_notify_event(XEvent * ev) {
	//printf("Entering in %s on %lu\n", __PRETTY_FUNCTION__,
	//		ev->xproperty.window);

	//printf("%lu\n", ev->xproperty.atom);
	char * name = XGetAtomName(cnx.dpy, ev->xproperty.atom);
	printf("Atom Name = \"%s\"\n", name);
	XFree(name);

	client_t * c = find_client_by_xwindow(ev->xproperty.window);
	if (!c)
		return;
	if (ev->xproperty.atom == cnx.atoms._NET_WM_USER_TIME) {
		if (fullscreen_client == 0 || fullscreen_client == c) {
			tree_root->activate_client(c);
			/* the hidden parameter of focus */
			cnx.last_know_time = ev->xproperty.time;
			//c->focus();
			//update_focus(c);
		}
	} else if (ev->xproperty.atom == cnx.atoms._NET_WM_NAME) {
		c->update_net_vm_name();
		c->update_title();
		render();
	} else if (ev->xproperty.atom == cnx.atoms.WM_NAME) {
		c->update_vm_name();
		c->update_title();
		render();
	} else if (ev->xproperty.atom == cnx.atoms._NET_WM_STRUT_PARTIAL) {
		if (ev->xproperty.state == PropertyNewValue) {
			unsigned int n;
			long * partial_struct = c->get_properties32(
					cnx.atoms._NET_WM_STRUT_PARTIAL, cnx.atoms.CARDINAL, &n);

			if (partial_struct) {

				printf(
						"partial struct %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n",
						partial_struct[0], partial_struct[1], partial_struct[2],
						partial_struct[3], partial_struct[4], partial_struct[5],
						partial_struct[6], partial_struct[7], partial_struct[8],
						partial_struct[9], partial_struct[10],
						partial_struct[11]);

				c->has_partial_struct = true;
				memcpy(c->partial_struct, partial_struct, sizeof(long) * 12);
				delete[] partial_struct;
				update_page_aera();

			} else {
				c->has_partial_struct = false;
			}
		} else if (ev->xproperty.state == PropertyDelete) {
			c->has_partial_struct = false;
		}

	} else if (ev->xproperty.atom == cnx.atoms._NET_ACTIVE_WINDOW) {
		printf("request to activate %lu\n", ev->xproperty.window);

	} else if (ev->xproperty.atom == cnx.atoms.WM_NORMAL_HINTS) {
		c->client_update_size_hints();
		render();
	} else if (ev->xproperty.atom == cnx.atoms.WM_PROTOCOLS) {
		c->read_wm_protocols();
	}
}

void main_t::fullscreen(client_t *c) {

	printf("FUULLLLSCREEENNN\n");
	if (fullscreen_client != 0) {
		fullscreen_client->unmap();
		fullscreen_client->unset_fullscreen();
		insert_client(fullscreen_client);
		fullscreen_client = 0;
		update_focus(0);
	}

	fullscreen_client = c;
	tree_root->remove_client(c);
	c->set_fullscreen();
	move_fullscreen(c);
	c->map();
	update_focus(c);
	c->focus();
}

void main_t::unfullscreen(client_t * c) {
	if (fullscreen_client == c) {
		fullscreen_client = 0;
		c->unset_fullscreen();
		c->unmap();
		update_focus(0);
		insert_client(c);
		render();
	}
}

void main_t::toggle_fullscreen(client_t * c) {
	if (c == fullscreen_client) {
		unfullscreen(c);
	} else {
		fullscreen(c);
	}
}

void main_t::process_client_message_event(XEvent * ev) {
	printf("Entering in %s on %lu\n", __PRETTY_FUNCTION__,
			ev->xproperty.window);

	//printf("%lu\n", ev->xclient.message_type);
	char * name = XGetAtomName(cnx.dpy, ev->xclient.message_type);
	printf("Atom Name = \"%s\"\n", name);
	XFree(name);

	if (ev->xclient.message_type == cnx.atoms._NET_ACTIVE_WINDOW) {
		printf("request to activate %lu\n", ev->xclient.window);
		client_t * c = find_client_by_xwindow(ev->xclient.window);
		if (c) {
			tree_root->activate_client(c);
			render();
		}

	} else if (ev->xclient.message_type == cnx.atoms._NET_WM_STATE) {
		client_t * c = find_client_by_xwindow(ev->xclient.window);
		if (c) {
			if (ev->xclient.data.l[1] == cnx.atoms._NET_WM_STATE_FULLSCREEN
					|| ev->xclient.data.l[2]
							== cnx.atoms._NET_WM_STATE_FULLSCREEN) {
				switch (ev->xclient.data.l[0]) {
				case 0:
					printf("SET normal\n");
					unfullscreen(c);
					break;
				case 1:
					printf("SET fullscreen\n");
					fullscreen(c);
					break;
				case 2:
					printf("SET toggle\n");
					toggle_fullscreen(c);
					break;

				}
			}

		}

	} else if (ev->xclient.message_type == cnx.atoms.WM_CHANGE_STATE) {
		/* client should send this message to go iconic */
		if (ev->xclient.data.l[0] == IconicState) {
			client_t * c = find_client_by_xwindow(ev->xclient.window);
			if (c) {
				printf("Set to iconic %lu\n", c->xwin);
				c->set_wm_state(IconicState);
				tree_root->iconify_client(c);
			}
		}
	} else if (ev->xclient.message_type == cnx.atoms.PAGE_QUIT) {
		running = false;
	} else if (ev->xclient.message_type == cnx.atoms.WM_PROTOCOLS) {
		char * name = XGetAtomName(cnx.dpy, ev->xclient.data.l[0]);
		printf("PROTOCOL Atom Name = \"%s\"\n", name);
		XFree(name);
	} else if (ev->xclient.message_type == cnx.atoms._NET_CLOSE_WINDOW) {

		XEvent evx;
		evx.xclient.display = cnx.dpy;
		evx.xclient.type = ClientMessage;
		evx.xclient.format = 32;
		evx.xclient.message_type = cnx.atoms.WM_PROTOCOLS;
		evx.xclient.window = ev->xclient.window;
		evx.xclient.data.l[0] = cnx.atoms.WM_DELETE_WINDOW;
		evx.xclient.data.l[1] = ev->xclient.data.l[0];

		XSendEvent(cnx.dpy, ev->xclient.window, False, NoEventMask, &evx);
	}
}

void main_t::process_damage_event(XEvent * ev) {
	XDamageNotifyEvent * e = (XDamageNotifyEvent *) ev;

	XDamageSubtract(cnx.dpy, e->damage, None, None);

	/* printf here create recursive damage when ^^ */
	//printf("damage event win: #%lu %dx%d+%d+%d\n", e->drawable,
	//		(int) e->area.width, (int) e->area.height, (int) e->area.x,
	//		(int) e->area.y);
	/* if this is a popup, I find the coresponding area on
	 * main window, then I repair the main window.
	 * This avoid multiple repair method.
	 */

	client_t * c = find_client_by_xwindow(e->drawable);
	if (c) {

		cairo_surface_flush(c->window_surf);
		cairo_surface_mark_dirty_rectangle(c->window_surf, e->area.x, e->area.y,
				e->area.width, e->area.height);
		//printf("find clients damage\n");
		e->area.x += c->size.x;
		e->area.y += c->size.y;

	}

	popup_t * p = find_popup_by_xwindow(e->drawable);
	if (p) {
		box_int_t area = p->get_absolute_extend();
		e->area.x += area.x;
		e->area.y += area.y;

		/* need to avoid RTTI */
		popup_window_t * p1 = dynamic_cast<popup_window_t*>(p);
		if (p1) {
			cairo_surface_flush(p1->surf);
			cairo_surface_mark_dirty_rectangle(p1->surf, e->area.x, e->area.y,
					e->area.width, e->area.height);
		}
	}

	if (p != 0 || c != 0) {
		box_int_t x(e->area.x, e->area.y, e->area.width, e->area.height);
		pending_damage = substract_box(pending_damage, x);
		pending_damage.push_back(x);
	}

	if (!e->more) {
		while (merge_area_macro(pending_damage)) {
		}

		box_list_t::const_iterator i = pending_damage.begin();
		while (i != pending_damage.end()) {
			repair_back_buffer(*i);
			repair_overlay(*i);
			++i;
		}
		pending_damage.clear();

	}

}

void main_t::repair_back_buffer(box_int_t const & area) {

	/* complexe computation to avoid useless memory blit */
	box_list_t mask;
	for (client_set_t::iterator i = clients.begin(); i != clients.end(); ++i) {
		mask.push_back((*i)->size);
	}

	box_list_t gui_area;
	gui_area.push_back(area);

	for (box_list_t::iterator i = mask.begin(); i != mask.end(); ++i) {
		gui_area = substract_box(gui_area, *i);
	}

	while (merge_area_macro(gui_area))
		continue;

	cairo_reset_clip(back_buffer_cr);
	cairo_set_source_surface(back_buffer_cr, gui_s, 0., 0.);
	for (box_list_t::iterator i = gui_area.begin(); i != gui_area.end(); ++i) {
		cairo_rectangle(back_buffer_cr, area.x, area.y, area.w, area.h);
		cairo_fill(back_buffer_cr);
	}

	for (client_set_t::iterator i = clients.begin(); i != clients.end(); ++i) {
		client_t * c = *i;
		if (!c->is_dock)
			continue;
		box_int_t clip = area & c->size;
		if (clip.w > 0 && clip.h > 0) {
			//printf("draw client %dx%d+%d+%d\n", clip.w, clip.h, clip.x, clip.x,
			//		clip.y);
			cairo_set_source_surface(back_buffer_cr, c->window_surf, c->size.x,
					c->size.y);
			cairo_rectangle(back_buffer_cr, clip.x, clip.y, clip.w, clip.h);
			cairo_fill(back_buffer_cr);
		}
	}

	for (client_set_t::iterator i = clients.begin(); i != clients.end(); ++i) {
		client_t * c = *i;
		if (!c->is_map)
			continue;
		if (c->wm_state != NormalState)
			continue;
		box_int_t clip = area & c->size;
		if (clip.w > 0 && clip.h > 0) {
			//printf("draw client %dx%d+%d+%d\n", clip.w, clip.h, clip.x, clip.x,
			//		clip.y);
			cairo_set_source_surface(back_buffer_cr, c->window_surf, c->size.x,
					c->size.y);
			cairo_rectangle(back_buffer_cr, clip.x, clip.y, clip.w, clip.h);
			cairo_fill(back_buffer_cr);
		}
	}

	std::list<popup_t *>::iterator i = popups.begin();
	while (i != popups.end()) {
		popup_t * p = (*i);
		/* make intersection */
		p->repair1(back_buffer_cr, area);
		++i;
	}
}

void main_t::repair_overlay(box_int_t const & area) {

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

void main_t::print_window_attributes(Window w, XWindowAttributes & wa) {
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

void main_t::insert_client(client_t * c) {
	if (default_window_pop != 0) {
		if (!default_window_pop->add_client(c)) {
			throw std::runtime_error("Fail to add a client");
		}
	} else {
		if (!tree_root->add_client(c)) {
			throw std::runtime_error("Fail to add a client\n");
		}
	}
}

void main_t::update_focus(client_t * c) {

	if (client_focused == c)
		return;

	if (client_focused != 0) {
		client_focused->net_wm_state.erase(cnx.atoms._NET_WM_STATE_FOCUSED);
		client_focused->write_net_wm_state();
	}

	client_focused = c;

	if (client_focused != 0) {
		client_focused->net_wm_state.insert(cnx.atoms._NET_WM_STATE_FOCUSED);
		client_focused->write_net_wm_state();
	}
}

void main_t::process_create_window_event(XEvent * e) {
	XWindowAttributes wa;
	if(!XGetWindowAttributes(cnx.dpy, e->xcreatewindow.window, &wa))
		return;
	if (!e->xcreatewindow.override_redirect
			&& e->xcreatewindow.window != cnx.xroot
			&& e->xcreatewindow.parent == cnx.xroot) {
		client_t * c = new client_t(cnx, e->xcreatewindow.window, wa,
				WithdrawnState);
		clients.insert(c);
	}
}

void main_t::move_fullscreen(client_t * c) {
//	XMoveResizeWindow(cnx.dpy, c->clipping_window, fullscreen_position.x,
//			fullscreen_position.y, fullscreen_position.w,
//			fullscreen_position.h);
	XMoveResizeWindow(cnx.dpy, c->xwin, 0, 0, fullscreen_position.w,
			fullscreen_position.h);
}

void main_t::withdraw_to_X(client_t * c) {
	printf("Manage #%lu\n", c->xwin);

	/* this window will not be destroyed on page close (one bug less) */
	XAddToSaveSet(cnx.dpy, c->xwin);
	/* before page prepend !! */
	clients.insert(c);

	c->update_all();

	XWMHints * hints = XGetWMHints(cnx.dpy, c->xwin);
	if (hints) {
		if (hints->initial_state == IconicState) {
			c->set_wm_state(IconicState);
		} else {
			c->set_wm_state(NormalState);
		}
		XFree(hints);
	} else {
		c->set_wm_state(NormalState);
	}

	if (c->client_is_dock()) {
		c->is_dock = true;
		printf("IsDock !\n");
		unsigned int n;
		long * partial_struct = c->get_properties32(
				cnx.atoms._NET_WM_STRUT_PARTIAL, cnx.atoms.CARDINAL, &n);

		if (partial_struct) {

			printf(
					"partial struct %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n",
					partial_struct[0], partial_struct[1], partial_struct[2],
					partial_struct[3], partial_struct[4], partial_struct[5],
					partial_struct[6], partial_struct[7], partial_struct[8],
					partial_struct[9], partial_struct[10], partial_struct[11]);

			c->has_partial_struct = true;
			memcpy(c->partial_struct, partial_struct, sizeof(long) * 12);

			delete[] partial_struct;

			update_page_aera();

			c->net_wm_allowed_actions.insert(cnx.atoms._NET_WM_ACTION_CLOSE);
			c->update_net_wm_allowed_actions();

			XSelectInput(cnx.dpy, c->xwin, ClientEventMask);
			c->is_map = false;
			c->map();

			//XCompositeRedirectWindow(cnx.dpy, c->xwin, CompositeRedirectManual);
			//cnx.reparentwindow(c->xwin, main_window, c->wa.x, c->wa.y);
			return;
		} /* if has not partial struct threat it as normal window */

	} else {
		c->is_dock = false;
	}

	XSetWindowBorderWidth(cnx.dpy, c->xwin, 0);

	c->net_wm_allowed_actions.insert(cnx.atoms._NET_WM_ACTION_CLOSE);
	c->net_wm_allowed_actions.insert(cnx.atoms._NET_WM_ACTION_FULLSCREEN);
	c->update_net_wm_allowed_actions();

	//XSetWindowAttributes swa = { 0 };

	//swa.background_pixel = 0xeeU << 16 | 0xeeU << 8 | 0xecU;
	//swa.border_pixel = XBlackPixel(cnx.dpy, cnx.screen);
	//c->clipping_window = XCreateWindow(cnx.dpy, main_window, 0, 0, 300, 300, 0,
	//		cnx.root_wa.depth, InputOutput, cnx.root_wa.visual,
	//		CWBackPixel | CWBorderPixel, &swa);
	//XCompositeRedirectWindow(cnx.dpy, c->clipping_window,
	//		CompositeRedirectAutomatic);
	//XSelectInput(cnx.dpy, c->clipping_window,
	//		ButtonPressMask | ButtonRelease | ExposureMask);

	//printf("XReparentWindow(%p, #%lu, #%lu, %d, %d)\n", cnx.dpy, c->xwin,
	//		c->clipping_window, 0, 0);
	//cnx.reparentwindow(c->xwin, c->clipping_window, 0, 0);

	XSelectInput(cnx.dpy, c->xwin, ClientEventMask);
	//XCompositeRedirectWindow(cnx.dpy, c->xwin, CompositeRedirectManual);
	//c->damage = XDamageCreate(cnx.dpy, c->xwin, XDamageReportRawRectangles);

	if (c->is_fullscreen()) {
		move_fullscreen(c);
		fullscreen(c);
		update_focus(c);
	} else {
		insert_client(c);
	}

	printf("Return %s on %p\n", __PRETTY_FUNCTION__, (void *) c->xwin);
	return;

}

void main_t::process_configure_notify_event(XEvent * e) {
	printf("Configure %dx%d+%d+%d\n", e->xconfigure.width, e->xconfigure.height,
			e->xconfigure.x, e->xconfigure.y);
	popup_t * p = find_popup_by_xwindow(e->xconfigure.window);
	if (p) {
		box_int_t area = p->get_absolute_extend();
		box_int_t new_area(e->xconfigure.x, e->xconfigure.y,
				e->xconfigure.width, e->xconfigure.height);
		p->reconfigure(new_area);
		box_int_t full_extend = get_max_extand(area, new_area);

		if (full_extend.w > 0 && full_extend.h > 0) {
			repair_back_buffer(full_extend);
			repair_overlay(full_extend);
		}

	}

	/* Some client set size and position after map the window ... we need fix it */
	client_t * c = find_client_by_xwindow(e->xconfigure.window);
	if (c) {
		c->hints.height = e->xconfigure.height;
		c->hints.width = e->xconfigure.width;
		c->hints.x = e->xconfigure.x;
		c->hints.y = e->xconfigure.y;
		box_t<int> x;
		tree_root->update_allocation(x);

		cairo_xlib_surface_set_size(c->window_surf, e->xconfigure.width, e->xconfigure.height);
		cairo_surface_flush(c->window_surf);
		cairo_surface_mark_dirty(c->window_surf);

		repair_back_buffer(cnx.root_size);
		repair_overlay(cnx.root_size);
	}

}

/* box0 - box1 */
box_list_t main_t::substract_box(box_int_t const &box0, box_int_t const &box1) {
	box_list_t result;

	box_int_t inter_sec = box0 & box1;

	if (inter_sec.w > 0 && inter_sec.h > 0) {
		/* top box */
		{
			int left = inter_sec.x;
			int right = inter_sec.x + inter_sec.w;
			int top = box0.y;
			int bottom = inter_sec.y;

			if (right - left > 0 && bottom - top > 0) {
				result.push_back(
						box_int_t(left, top, right - left, bottom - top));
			}
		}

		/* bottom box */
		{
			int left = inter_sec.x;
			int right = inter_sec.x + inter_sec.w;
			int top = inter_sec.y + inter_sec.h;
			int bottom = box0.y + box0.h;

			if (right - left > 0 && bottom - top > 0) {
				result.push_back(
						box_int_t(left, top, right - left, bottom - top));
			}
		}

		/* left box */
		{
			int left = box0.x;
			int right = inter_sec.x;
			int top = inter_sec.y;
			int bottom = inter_sec.y + inter_sec.h;

			if (right - left > 0 && bottom - top > 0) {
				result.push_back(
						box_int_t(left, top, right - left, bottom - top));
			}
		}

		/* right box */
		{
			int left = inter_sec.x + inter_sec.w;
			int right = box0.x + box0.w;
			int top = inter_sec.y;
			int bottom = inter_sec.y + inter_sec.h;

			if (right - left > 0 && bottom - top > 0) {
				result.push_back(
						box_int_t(left, top, right - left, bottom - top));
			}
		}

		/* top left box */
		{
			int left = box0.x;
			int right = inter_sec.x;
			int top = box0.y;
			int bottom = inter_sec.y;

			if (right - left > 0 && bottom - top > 0) {
				result.push_back(
						box_int_t(left, top, right - left, bottom - top));
			}
		}

		/* top right box */
		{
			int left = inter_sec.x + inter_sec.w;
			int right = box0.x + box0.w;
			int top = box0.y;
			int bottom = inter_sec.y;

			if (right - left > 0 && bottom - top > 0) {
				result.push_back(
						box_int_t(left, top, right - left, bottom - top));
			}
		}

		/* bottom left box */
		{
			int left = box0.x;
			int right = inter_sec.x;
			int top = inter_sec.y + inter_sec.h;
			int bottom = box0.y + box0.h;

			if (right - left > 0 && bottom - top > 0) {
				result.push_back(
						box_int_t(left, top, right - left, bottom - top));
			}
		}

		/* bottom right box */
		{
			int left = inter_sec.x + inter_sec.w;
			int right = box0.x + box0.w;
			int top = inter_sec.y + inter_sec.h;
			int bottom = box0.y + box0.h;

			if (right - left > 0 && bottom - top > 0) {
				result.push_back(
						box_int_t(left, top, right - left, bottom - top));
			}
		}

	} else {
		result.push_back(box0);
	}

	return result;

}

box_list_t main_t::substract_box(box_list_t const &box_list,
		box_int_t const &box1) {
	box_list_t result;
	box_list_t::const_iterator i = box_list.begin();
	while (i != box_list.end()) {
		box_list_t x = substract_box(*i, box1);
		result.insert(result.end(), x.begin(), x.end());
		++i;
	}
	return result;
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

/* merge 2 reactangle, not efficiently */
bool main_t::merge_area_macro(box_list_t & list) {

	box_list_t result;
	box_list_t tmp;
	box_list_t::const_iterator i = list.begin();
	while (i != list.end()) {
		box_list_t::const_iterator j = list.begin();
		while (j != list.end()) {
			if (i != j) {
				if ((*i).x + (*i).w == (*j).x && (*i).y == (*j).y
						&& (*i).h == (*j).h) {
					copy_without(i, j, list, tmp);
					tmp.push_back(
							box_int_t((*i).x, (*i).y, (*j).w + (*i).w, (*i).h));
					list = tmp;
					return true;
				}

				if ((*i).x == (*j).x + (*j).w && (*i).y == (*j).y
						&& (*i).h == (*j).h) {
					copy_without(i, j, list, tmp);
					tmp.push_back(
							box_int_t((*j).x, (*j).y, (*j).w + (*i).w, (*j).h));
					list = tmp;
					return true;
				}

				if ((*i).y == (*j).y + (*j).h && (*i).x == (*j).x
						&& (*i).w == (*j).w) {
					copy_without(i, j, list, tmp);
					tmp.push_back(
							box_int_t((*j).x, (*j).y, (*j).w, (*j).h + (*i).h));
					list = tmp;
					return true;
				}

				if ((*i).y + (*i).h == (*j).y && (*i).x == (*j).x
						&& (*i).w == (*j).w) {
					copy_without(i, j, list, tmp);
					tmp.push_back(
							box_int_t((*i).x, (*i).y, (*i).w, (*j).h + (*i).h));
					list = tmp;
					return true;
				}

			}
			++j;
		}
		++i;
	}

	return false;

}

}
