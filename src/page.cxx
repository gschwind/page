/*
 * page.cxx
 *
 * copyright (2010) Benoit Gschwind
 *
 */

#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <cstring>

#include <sstream>
#include <limits>
#include <stdint.h>

#include "page.hxx"
#include "box.hxx"
#include "client.hxx"
#include "root.hxx"

namespace page_next {

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

main_t::main_t() {
	XSetWindowAttributes swa;
	XWindowAttributes wa;
	XSelectInput(cnx.dpy, cnx.xroot, SubstructureNotifyMask | SubstructureRedirectMask);

	main_window = XCreateWindow(cnx.dpy, cnx.xroot, cnx.root_size.x, cnx.root_size.y, cnx.root_size.w, cnx.root_size.h, 0, cnx.root_wa.depth,
			InputOutput, cnx.root_wa.visual, 0, &swa);
	cursor = XCreateFontCursor(cnx.dpy, XC_left_ptr);
	XDefineCursor(cnx.dpy, main_window, cursor);
	XSelectInput(cnx.dpy, main_window,
			StructureNotifyMask | ButtonPressMask | ExposureMask);
	XMapWindow(cnx.dpy, main_window);

	printf("Created main window #%lu\n", main_window);

	XGetWindowAttributes(cnx.dpy, main_window, &(wa));
	surf = cairo_xlib_surface_create(cnx.dpy, main_window, wa.visual, wa.width,
			wa.height);
	cr = cairo_create(surf);

	box_t<int> a(0, 0, cnx.root_size.w, cnx.root_size.h);
	tree_root = new root_t(cnx.dpy, main_window, a);

}

main_t::~main_t() {
	cairo_destroy(cr);
	cairo_surface_destroy(surf);
}

long * main_t::get_properties32(Window win, Atom prop, Atom type,
		unsigned int *num) {
	return this->get_properties<long, 32>(win, prop, type, num);
}

short * main_t::get_properties16(Window win, Atom prop, Atom type,
		unsigned int *num) {
	return this->get_properties<short, 16>(win, prop, type, num);
}

char * main_t::get_properties8(Window win, Atom prop, Atom type,
		unsigned int *num) {
	return this->get_properties<char, 8>(win, prop, type, num);
}

/* update main window location */
void main_t::update_page_aera() {
	int left = 0, right = 0, top = 0, bottom = 0;
	std::list<client_t *>::iterator i = clients.begin();
	while (i != clients.end()) {
		if ((*i)->has_partial_struct) {
			client_t * c = (*i);
			if (left < c->struct_left)
				left = c->struct_left;
			if (right < c->struct_right)
				right = c->struct_right;
			if (top < c->struct_top)
				top = c->struct_top;
			if (bottom < c->struct_bottom)
				bottom = c->struct_bottom;
		}
		++i;
	}

	page_area.x = left;
	page_area.y = top;
	page_area.w = cnx.root_size.w - (left + right);
	page_area.h = cnx.root_size.h - (top + bottom);

	box_t<int> b(0, 0, cnx.root_size.w, cnx.root_size.h);
	tree_root->update_allocation(b);
	XMoveResizeWindow(cnx.dpy, main_window, page_area.x, page_area.y, page_area.w,
			page_area.h);

}

void main_t::run() {
	update_net_supported();

	/* update number of desktop */
	int32_t number_of_desktop = 1;
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_NUMBER_OF_DESKTOPS, cnx.atoms.CARDINAL,
			32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&number_of_desktop), 1);

	/* define desktop geometry */
	long desktop_geometry[2];
	desktop_geometry[0] = cnx.root_size.w;
	desktop_geometry[1] = cnx.root_size.h;
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_DESKTOP_GEOMETRY, cnx.atoms.CARDINAL, 32,
			PropModeReplace,
			reinterpret_cast<unsigned char *>(desktop_geometry), 2);

	/* set viewport */
	long viewport[2] = { 0, 0 };
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_DESKTOP_VIEWPORT, cnx.atoms.CARDINAL, 32,
			PropModeReplace, reinterpret_cast<unsigned char *>(viewport), 2);

	/* set current desktop */
	long current_desktop = 0;
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_CURRENT_DESKTOP, cnx.atoms.CARDINAL, 32,
			PropModeReplace,
			reinterpret_cast<unsigned char *>(&current_desktop), 1);

	long showing_desktop = 0;
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_SHOWING_DESKTOP, cnx.atoms.CARDINAL, 32,
			PropModeReplace,
			reinterpret_cast<unsigned char *>(&showing_desktop), 1);

	scan();
	update_page_aera();

	long workarea[4];
	workarea[0] = page_area.x;
	workarea[1] = page_area.y;
	workarea[2] = page_area.w;
	workarea[3] = page_area.h;

	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_WORKAREA, cnx.atoms.CARDINAL, 32,
			PropModeReplace, reinterpret_cast<unsigned char*>(workarea), 4);

	render();
	running = 1;
	while (running) {
		XEvent e;
		XNextEvent(cnx.dpy, &e);

		printf("#%lu event: %s window: %lu\n", e.xany.serial,
				x_event_name[e.type], e.xany.window);
		if (e.type == MapNotify) {
		} else if (e.type == Expose) {
			if (e.xmapping.window == main_window)
				render();
		} else if (e.type == ButtonPress) {
			client_t * c = find_client_by_clipping_window(e.xbutton.window);
			if (c) {
				if (c->try_lock_client()) {
					c->focus();
					c->unlock_client();
				}
			} else {
				tree_root->process_button_press_event(&e);
			}
			render();
		} else if (e.type == MapRequest) {
			printf("MapRequest\n");
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
		}

		if(!cnx.is_not_grab()) {
			fprintf(stderr, "SERVEUR IS GRAB WHERE IT SHOULDN'T");
			exit(EXIT_FAILURE);
		}
	}
}

void main_t::render(cairo_t * cr) {
	tree_root->render(cr);
}

void main_t::render() {
	XGetWindowAttributes(cnx.dpy, main_window, &wa);
	box_t<int> b(0, 0, wa.width, wa.height);
	tree_root->update_allocation(b);
	render(cr);
}

void main_t::scan() {
	printf("call %s\n", __PRETTY_FUNCTION__);
	unsigned int i, num;
	Window d1, d2, *wins = 0;
	XWindowAttributes wa;

	cnx.grab();
	/* ask for child of current root window, use Xlib here since gdk
	 * only know windows it have created.
	 */
	if (XQueryTree(cnx.dpy, cnx.xroot, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; ++i) {
			if (!XGetWindowAttributes(cnx.dpy, wins[i], &wa))
				continue;
			if (wa.override_redirect)
				continue;
			if ((get_window_state(wins[i]) == IconicState
					|| wa.map_state == IsViewable))
				manage(wins[i], &wa);
		}

		if (wins)
			XFree(wins);
	}

	update_client_list();
	cnx.ungrab();
}

client_t * main_t::find_client_by_xwindow(Window w) {
	std::list<client_t *>::iterator i = clients.begin();
	while (i != clients.end()) {
		if ((*i)->xwin == w)
			return (*i);
		++i;
	}

	return 0;
}

client_t * main_t::find_client_by_clipping_window(Window w) {
	std::list<client_t *>::iterator i = clients.begin();
	while (i != clients.end()) {
		if ((*i)->clipping_window == w)
			return (*i);
		++i;
	}

	return 0;
}

void main_t::update_net_supported() {

	Atom supported_list[9];

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
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_SUPPORTED, cnx.atoms.ATOM, 32,
			PropModeReplace, reinterpret_cast<unsigned char *>(supported_list),
			10);

}

void main_t::update_client_list() {

	Window * data = new Window[clients.size()];

	int k = 0;

	std::list<client_t *>::iterator i = clients.begin();
	while (i != clients.end()) {
		data[k] = (*i)->xwin;
		++i;
		++k;
	}

	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_CLIENT_LIST, cnx.atoms.WINDOW, 32,
			PropModeReplace, reinterpret_cast<unsigned char *>(data),
			clients.size());
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_CLIENT_LIST_STACKING, cnx.atoms.WINDOW,
			32, PropModeReplace, reinterpret_cast<unsigned char *>(data),
			clients.size());

	delete[] data;
}

bool main_t::manage(Window w, XWindowAttributes * wa) {
	printf("Manage #%lu\n", w);
	if (main_window == w)
		return false;

	client_t * c = find_client_by_xwindow(w);
	if (c != NULL) {
		printf("Window %p is already managed\n", c);
		return false;
	}

	/* do not manage clipping window */
	if (find_client_by_clipping_window(w))
		return false;

	c = new client_t(cnx);
	c->is_dock = false;
	c->has_partial_struct = false;
	c->xwin = w;
	c->height = wa->height;
	c->width = wa->width;
	printf("Map stase : %d\n", wa->map_state);
	/* if the client is mapped, the reparent will unmap the window
	 * The client is mapped if the manage occur on start of
	 * page.
	 */
	if (wa->map_state == IsUnmapped) {
		c->is_map = false;
		c->unmap_pending = 0;
	} else {
		c->is_map = true;
		c->unmap_pending = 1;
	}
	/* before page prepend !! */
	clients.push_back(c);

	c->update_net_vm_name();
	c->update_vm_name();
	c->update_title();
	c->client_update_size_hints();

	if (client_is_dock(c)) {
		c->is_dock = true;
		printf("IsDock !\n");
		unsigned int n;
		long * partial_struct = get_properties32(c->xwin,
				cnx.atoms._NET_WM_STRUT_PARTIAL, cnx.atoms.CARDINAL, &n);

		if (partial_struct) {

			printf("partial struct %ld %ld %ld %ld\n", partial_struct[0],
					partial_struct[1], partial_struct[2], partial_struct[3]);

			c->has_partial_struct = true;
			c->struct_left = partial_struct[0];
			c->struct_right = partial_struct[1];
			c->struct_top = partial_struct[2];
			c->struct_bottom = partial_struct[3];

			delete[] partial_struct;

		}
		return true;
	}

	/* take _NET_WM_STATE */
	{
		unsigned int n;
		long * net_wm_state = get_properties32(c->xwin, cnx.atoms._NET_WM_STATE,
				cnx.atoms.ATOM, &n);

		c->is_modal = false;
		c->is_sticky = false;
		c->is_maximized_vert = false;
		c->is_maximized_horz = false;
		c->is_is_shaded = false;
		c->is_skip_taskbar = false;
		c->is_skip_pager = false;
		c->is_hidden = false;
		c->is_fullscreen = false;
		c->is_above = false;
		c->is_below = false;
		c->is_demands_attention = false;

		for (int i = 0; i < n; ++i) {
			if (net_wm_state[i] == cnx.atoms._NET_WM_STATE_MODAL) {
				c->is_modal = true;
			} else if (net_wm_state[i] == cnx.atoms._NET_WM_STATE_STICKY) {
				c->is_sticky = true;
			} else if (net_wm_state[i] == cnx.atoms._NET_WM_STATE_MAXIMIZED_VERT) {
				c->is_maximized_vert = true;
			} else if (net_wm_state[i] == cnx.atoms._NET_WM_STATE_MAXIMIZED_HORZ) {
				c->is_maximized_horz = true;
			} else if (net_wm_state[i] == cnx.atoms._NET_WM_STATE_SHADED) {
				c->is_is_shaded = true;
			} else if (net_wm_state[i] == cnx.atoms._NET_WM_STATE_SKIP_TASKBAR) {
				c->is_skip_taskbar = true;
			} else if (net_wm_state[i] == cnx.atoms._NET_WM_STATE_SKIP_PAGER) {
				c->is_skip_pager = true;
			} else if (net_wm_state[i] == cnx.atoms._NET_WM_STATE_HIDDEN) {
				c->is_hidden = true;
			} else if (net_wm_state[i] == cnx.atoms._NET_WM_STATE_FULLSCREEN) {
				c->is_fullscreen = true;
			} else if (net_wm_state[i] == cnx.atoms._NET_WM_STATE_ABOVE) {
				c->is_above = true;
			} else if (net_wm_state[i] == cnx.atoms._NET_WM_STATE_BELOW) {
				c->is_below = true;
			} else if (net_wm_state[i]
					== cnx.atoms._NET_WM_STATE_DEMANDS_ATTENTION) {
				c->is_demands_attention = true;
			}

		}
	}

	{
		unsigned int n;
		c->icon_data = get_properties32(c->xwin, cnx.atoms._NET_WM_ICON,
				cnx.atoms.CARDINAL, &n);
		c->icon_data_size = n;

		if (n > 0) {
			c->icon_data32 = new int32_t[n];
			for (int i = 0; i < n; ++i)
				c->icon_data32[i] = c->icon_data[i];
		}
		parse_icons(c);

		icon ic;
		int x = 0;
		int y = 0;
		bool has_icon = false;
		/* find a icon */
		std::list<icon>::iterator i = c->icons.begin();
		while (i != c->icons.end()) {
			if ((*i).width <= 16 && x < (*i).width) {
				x = (*i).width;
				ic = (*i);
				has_icon = true;
			}
			++i;
		}

		if (has_icon)
			c->selected = ic;
		else
			c->selected = c->icons.front();

		c->icon_surf = cairo_image_surface_create_for_data(
				(unsigned char *) c->selected.data,
				CAIRO_FORMAT_ARGB32,
				c->selected.width,
				c->selected.height,
				cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
						c->selected.width));
	}
	/* this window will not be destroyed on page close (one bug less) */
	XAddToSaveSet(cnx.dpy, w);
	XSetWindowBorderWidth(cnx.dpy, w, 0);

	XSetWindowAttributes swa;

	swa.background_pixel = 0xeeU << 16 | 0xeeU << 8 | 0xecU;
	swa.border_pixel = XBlackPixel(cnx.dpy, cnx.screen);
	c->clipping_window = XCreateWindow(cnx.dpy, main_window, 0, 0, 1, 1, 0,
			cnx.root_wa.depth, InputOutput, cnx.root_wa.visual,
			CWBackPixel | CWBorderPixel, &swa);
	XSelectInput(cnx.dpy, c->clipping_window, ButtonPressMask | ButtonRelease);

	printf("XReparentWindow(%p, #%lu, #%lu, %d, %d)\n", cnx.dpy, c->xwin,
			c->clipping_window, 0, 0);
	XSelectInput(cnx.dpy, c->xwin, StructureNotifyMask | PropertyChangeMask);
	XReparentWindow(cnx.dpy, c->xwin, c->clipping_window, 0, 0);

	if (!tree_root->add_notebook(c)) {
		printf("Fail to add a client\n");
		exit(EXIT_FAILURE);
	}

	if (c->is_fullscreen) {
		XReparentWindow(cnx.dpy, c->clipping_window, cnx.xroot, 0, 0);
		XMoveResizeWindow(cnx.dpy, c->xwin, 0, 0, cnx.root_size.w, cnx.root_size.h);
		XMoveResizeWindow(cnx.dpy, c->clipping_window, 0, 0, cnx.root_size.w, cnx.root_size.h);
		c->fullscreen(cnx.root_size.w, cnx.root_size.h);
		c->map();
		c->focus();
	}

	printf("Return %s on %p\n", __PRETTY_FUNCTION__, (void *) w);
	return true;
}

long main_t::get_window_state(Window w) {
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(cnx.dpy, w, cnx.atoms.WM_STATE, 0L, 2L, False,
			cnx.atoms.WM_STATE, &real, &format, &n, &extra,
			(unsigned char **) &p) != Success
			)
		return -1;

	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

/* inspired from dwm */
bool main_t::get_text_prop(Window w, Atom atom, std::string & text) {
	char **list = NULL;
	int n;
	XTextProperty name;
	XGetTextProperty(cnx.dpy, w, &name, atom);
	if (!name.nitems)
		return false;
	text = (char const *) name.value;
	return true;
}

bool main_t::client_is_dock(client_t * c) {
	unsigned int num, i;
	long * val = get_properties32(c->xwin, cnx.atoms._NET_WM_WINDOW_TYPE,
			cnx.atoms.ATOM, &num);
	Window t;

	if (val) {
		/* use the first value that we know about in the array */
		for (i = 0; i < num; ++i) {
			if (val[i] == cnx.atoms._NET_WM_WINDOW_TYPE_DOCK)
				return true;
		}
		free(val);
	}

	return false;
}

void main_t::process_map_request_event(XEvent * e) {
	printf("Entering in %s #%p\n", __PRETTY_FUNCTION__,
			(void *) e->xmaprequest.window);
	Window w = e->xmaprequest.window;
	/* secure the map request */
	//XGrabServer(cnx.dpy);
	//XSync(cnx.dpy, False);
	XEvent ev;
	if (XCheckTypedWindowEvent(cnx.dpy, e->xunmap.window, DestroyNotify, &ev)) {
		/* the window is already destroyed, return */
		//XUngrabServer(cnx.dpy);
		//XFlush(cnx.dpy);
		return;
	}

	if (XCheckTypedWindowEvent(cnx.dpy, e->xunmap.window, UnmapNotify, &ev)) {
		/* the window is already unmapped, return */
		//XUngrabServer(cnx.dpy);
		//XFlush(cnx.dpy);
		return;
	}

	/* should never happen */
	XWindowAttributes wa;
	if (!XGetWindowAttributes(cnx.dpy, w, &wa)) {
		XMapWindow(cnx.dpy, w);
		return;
	}
	if (wa.override_redirect) {
		XMapWindow(cnx.dpy, w);
		return;
	}
	manage(w, &wa);
	XMapWindow(cnx.dpy, w);
	render();
	update_client_list();
	//XUngrabServer(cnx.dpy);
	//XFlush(cnx.dpy);
	printf("Return from %s #%p\n", __PRETTY_FUNCTION__,
			(void *) e->xmaprequest.window);
	return;

}

void main_t::process_map_notify_event(XEvent * e) {

}

void main_t::process_unmap_notify_event(XEvent * e) {
	printf("UnmapNotify #%lu #%lu\n", e->xunmap.window, e->xunmap.event);
	client_t * c = find_client_by_xwindow(e->xmap.window);
	/* unmap can be received twice time but are unique per window event.
	 * so this remove multiple events.
	 */
	if (e->xunmap.window != e->xunmap.event) {
		printf("Ignore this unmap #%lu #%lu\n", e->xunmap.window,
				e->xunmap.event);
		return;
	}
	if (!c)
		return;
	if (c->unmap_pending > 0) {
		printf("Expected Unmap\n");
		c->unmap_pending -= 1;
	} else {
		/**
		 * The client initiate an unmap.
		 * That mean the window go in WithDrawn state, PAGE must forget this window.
		 * Unmap is often followed by destroy window, If this is the case we try to
		 * reparent a destroyed window. Therefore we grab the server and check if the
		 * window is not already destroyed.
		 */
		//XGrabServer(cnx.dpy);
		//XSync(cnx.dpy, False);
		XEvent ev;
		if (XCheckTypedWindowEvent(cnx.dpy, e->xunmap.window, DestroyNotify, &ev)) {
			process_destroy_notify_event(&ev);
		} else {
			tree_root->remove_client(c->xwin);
			clients.remove(c);
			XReparentWindow(cnx.dpy, c->xwin, cnx.xroot, 0, 0);
			XRemoveFromSaveSet(cnx.dpy, c->xwin);
			XDestroyWindow(cnx.dpy, c->clipping_window);
			delete c;
			render();
		}
		//XUngrabServer(cnx.dpy);
		//XFlush(cnx.dpy);
	}
}

void main_t::process_destroy_notify_event(XEvent * e) {
	printf("DestroyNotify destroy : #%lu, event : #%lu\n", e->xunmap.window,
			e->xunmap.event);
	client_t * c = find_client_by_xwindow(e->xmap.window);
	if (c) {
		tree_root->remove_client(c->xwin);
		clients.remove(c);
		if (c->has_partial_struct)
			update_page_aera();
		update_client_list();
		if (!c->is_dock)
			XDestroyWindow(cnx.dpy, c->clipping_window);
		delete c;
		render();
	}
}

void main_t::process_property_notify_event(XEvent * ev) {
	//printf("Entering in %s on %lu\n", __PRETTY_FUNCTION__,
	//		ev->xproperty.window);

	//printf("%lu\n", ev->xproperty.atom);
	char * name = XGetAtomName(cnx.dpy, ev->xproperty.atom);
	//printf("Atom Name = \"%s\"\n", name);
	XFree(name);

	client_t * c = find_client_by_xwindow(ev->xproperty.window);
	if (!c)
		return;
	if (c->try_lock_client()) {
		if (ev->xproperty.atom == cnx.atoms._NET_WM_USER_TIME) {
			tree_root->activate_client(c);
			render();
			c->focus();
			XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_ACTIVE_WINDOW, cnx.atoms.WINDOW,
					32, PropModeReplace,
					reinterpret_cast<unsigned char *>(&(ev->xproperty.window)),
					1);
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
				long * partial_struct = get_properties32(c->xwin,
						cnx.atoms._NET_WM_STRUT_PARTIAL, cnx.atoms.CARDINAL, &n);

				if (partial_struct) {

					printf("partial struct %ld %ld %ld %ld\n",
							partial_struct[0], partial_struct[1],
							partial_struct[2], partial_struct[3]);

					c->has_partial_struct = true;
					c->struct_left = partial_struct[0];
					c->struct_right = partial_struct[1];
					c->struct_top = partial_struct[2];
					c->struct_bottom = partial_struct[3];

					delete[] partial_struct;

				}
			} else if (ev->xproperty.state == PropertyDelete) {
				c->has_partial_struct = false;
			}

		} else if (ev->xproperty.atom == cnx.atoms._NET_ACTIVE_WINDOW) {
			printf("request to activate %lu\n", ev->xproperty.window);

		} else if (ev->xproperty.atom == cnx.atoms.WM_NORMAL_HINTS) {
			c->client_update_size_hints();
			render();
		}
		c->unlock_client();
	}
}

void main_t::fullscreen(client_t *c) {
	printf("TOGGLE FULLSCREEN\n");
	c->is_fullscreen = true;
	/* update window state */
	long new_state[1];
	new_state[0] = cnx.atoms._NET_WM_STATE_FULLSCREEN;
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_WM_STATE, cnx.atoms.ATOM, 32,
			PropModeReplace, reinterpret_cast<unsigned char *>(new_state), 1);

	XReparentWindow(cnx.dpy, c->clipping_window, cnx.xroot, 0, 0);
	XMoveResizeWindow(cnx.dpy, c->xwin, 0, 0, cnx.root_size.w, cnx.root_size.h);
	XMoveResizeWindow(cnx.dpy, c->clipping_window, 0, 0, cnx.root_size.w, cnx.root_size.h);
	c->fullscreen(cnx.root_size.w, cnx.root_size.h);
	c->map();
	c->focus();
}

void main_t::unfullscreen(client_t * c) {
	printf("TOGGLE WINDOWED\n");
	c->is_fullscreen = false;
	long new_state[1];
	new_state[0] = cnx.atoms._NET_WM_STATE_FULLSCREEN;
	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_WM_STATE, cnx.atoms.ATOM, 32,
			PropModeReplace, reinterpret_cast<unsigned char *>(new_state), 0);
	XReparentWindow(cnx.dpy, c->clipping_window, main_window, 0, 0);
	c->unmap();
	render();
}

void main_t::toggle_fullscreen(client_t * c) {
	if (c->is_fullscreen) {
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
			if (c->try_lock_client()) {
				if (ev->xclient.data.l[1] == cnx.atoms._NET_WM_STATE_FULLSCREEN
						|| ev->xclient.data.l[2]
								== cnx.atoms._NET_WM_STATE_FULLSCREEN) {
					switch (ev->xclient.data.l[0]) {
					case 0:
						printf("SET normal\n");
						if (c->is_fullscreen) {
							toggle_fullscreen(c);
						}
						break;
					case 1:
						printf("SET fullscreen\n");
						if (!c->is_fullscreen) {
							toggle_fullscreen(c);
						}
						break;
					case 2:
						printf("SET toggle\n");
						toggle_fullscreen(c);
						break;

					}
				}
				c->unlock_client();
			}

		}

	}
}

void main_t::update_vm_hints(client_t &c) {

}

void main_t::parse_icons(client_t * c) {
	int offset = 0;
	while (offset < c->icon_data_size) {
		icon tmp;
		tmp.width = c->icon_data[offset + 0];
		tmp.height = c->icon_data[offset + 1];
		printf("FIND ICON%d %d\n", tmp.width, tmp.height);
		tmp.data = &c->icon_data32[offset + 3];
		offset += 2 + tmp.width * tmp.height;
		c->icons.push_back(tmp);
	}
}

}
