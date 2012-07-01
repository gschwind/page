/*
 * client.cxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

#include <stdio.h>
#include <iostream>
#include <sstream>
#include <cairo.h>
#include <cairo-xlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <cstring>
#include "client.hxx"

namespace page_next {

client_t::client_t(xconnection_t &cnx, Window w,
		XWindowAttributes &wa, long wm_state) :
		cnx(cnx), xwin(w), wa(wa), is_dock(false), has_partial_struct(false), height(
				wa.height), width(wa.width), lock_count(
				0), is_lock(false), icon_surf(0) {

	icon.data = 0;

	set_wm_state(wm_state);

	if (wa.map_state == IsUnmapped) {
		is_map = false;
	} else {
		is_map = true;
	}

	memset(partial_struct, 0, sizeof(partial_struct));

	window_surf = cairo_xlib_surface_create(cnx.dpy, xwin, wa.visual, wa.width, wa.height);
	damage = XDamageCreate(cnx.dpy, xwin, XDamageReportRawRectangles);
	XCompositeRedirectWindow(cnx.dpy, xwin, CompositeRedirectManual);

	size.x = wa.x;
	size.y = wa.y;
	size.w = wa.width;
	size.h = wa.height;

}

void client_t::update_all() {

	wm_input_focus = false;
	XWMHints * hints = XGetWMHints(cnx.dpy, xwin);
	if (hints) {
		if ((hints->flags & InputHint) && hints->input == True)
			wm_input_focus = true;
		XFree(hints);
	}

	update_net_vm_name();
	update_vm_name();
	update_title();
	client_update_size_hints();
	update_type();
	read_net_wm_state();
	read_wm_protocols();

	init_icon();

}

long client_t::get_window_state() {
	int format;
	long result = -1;
	long * p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(cnx.dpy, xwin, cnx.atoms.WM_STATE, 0L, 2L, False,
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

void client_t::map() {
	// generate a map request event.
	is_map = true;
	cnx.map(xwin);
	//cnx.map(clipping_window);
}

void client_t::unmap() {
	if (is_map) {
		is_map = false;
		//unmap_pending += 1;
		/* ICCCM require that WM unmap client window to change client state from
		 * Normal to Iconic state
		 * in PAGE all unviewable window are in iconic state */
		//cnx.unmap(clipping_window);
		cnx.unmap(xwin);
	}
}

void client_t::update_client_size(int w, int h) {

	//if (is_fullscreen()) {
	//	height = cnx.root_size.h;
	//	width = cnx.root_size.w;
	//}

	if (hints.flags & PMaxSize) {
		if (w > hints.max_width)
			w = hints.max_width;
		if (h > hints.max_height)
			h = hints.max_height;
	}

	if (hints.flags & PBaseSize) {
		if (w < hints.base_width)
			w = hints.base_width;
		if (h < hints.base_height)
			h = hints.base_height;
	} else if (hints.flags & PMinSize) {
		if (w < hints.min_width)
			w = hints.min_width;
		if (h < hints.min_height)
			h = hints.min_height;
	}

	if (hints.flags & PAspect) {
		if (hints.flags & PBaseSize) {
			/* ICCCM say if base is set substract base before aspect checking ref : ICCCM*/
			if ((w - hints.base_width) * hints.min_aspect.y
					< (h - hints.base_height) * hints.min_aspect.x) {
				/* reduce h */
				h = hints.base_height
						+ ((w - hints.base_width) * hints.min_aspect.y)
								/ hints.min_aspect.x;

			} else if ((w - hints.base_width) * hints.max_aspect.y
					> (h - hints.base_height) * hints.max_aspect.x) {
				/* reduce w */
				w = hints.base_width
						+ ((h - hints.base_height) * hints.max_aspect.x)
								/ hints.max_aspect.y;
			}
		} else {
			if (w * hints.min_aspect.y < h * hints.min_aspect.x) {
				/* reduce h */
				h = (w * hints.min_aspect.y) / hints.min_aspect.x;

			} else if (w * hints.max_aspect.y > h * hints.max_aspect.x) {
				/* reduce w */
				w = (h * hints.max_aspect.x) / hints.max_aspect.y;
			}
		}

	}

	if (hints.flags & PResizeInc) {
		w -= ((w - hints.base_width) % hints.width_inc);
		h -= ((h - hints.base_height) % hints.height_inc);
	}

	height = h;
	width = w;

	printf("Update #%lu window size %dx%d\n", xwin, width, height);
}

/* check if client is still alive */
bool client_t::try_lock_client() {
	cnx.grab();
	XEvent e;
	if (XCheckTypedWindowEvent(cnx.dpy, xwin, DestroyNotify, &e)) {
		XPutBackEvent(cnx.dpy, &e);
		cnx.ungrab();
		return false;
	}
	is_lock = true;
	return true;
}

void client_t::unlock_client() {
	is_lock = false;
	cnx.ungrab();
}

void client_t::focus() {
	if (is_map && wm_input_focus) {
		//if (!is_lock)
		//	printf("warning: client isn't locked.\n");
		printf("Focus #%x\n", (unsigned int) xwin);
		//XRaiseWindow(cnx.dpy, clipping_window);
		XRaiseWindow(cnx.dpy, xwin);
		XSetInputFocus(cnx.dpy, xwin, RevertToNone, CurrentTime);
	}

	if (wm_protocols.find(cnx.atoms.WM_TAKE_FOCUS) != wm_protocols.end()) {
		printf("TAKE_FOCUS\n");
		//XRaiseWindow(cnx.dpy, clipping_window);
		XRaiseWindow(cnx.dpy, xwin);
		XEvent ev;
		ev.xclient.display = cnx.dpy;
		ev.xclient.type = ClientMessage;
		ev.xclient.format = 32;
		ev.xclient.message_type = cnx.atoms.WM_PROTOCOLS;
		ev.xclient.window = xwin;
		ev.xclient.data.l[0] = cnx.atoms.WM_TAKE_FOCUS;
		ev.xclient.data.l[1] = cnx.last_know_time;
		XSendEvent(cnx.dpy, xwin, False, NoEventMask, &ev);
	}

	net_wm_state.insert(cnx.atoms._NET_WM_STATE_FOCUSED);
	write_net_wm_state();

	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_ACTIVE_WINDOW,
			cnx.atoms.WINDOW, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&(xwin)), 1);

}

void client_t::client_update_size_hints() {
	long msize;
	XSizeHints &size = hints;
	if (!XGetWMNormalHints(cnx.dpy, xwin, &size, &msize)) {
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
		printf("no WMNormalHints\n");
	}
}

void client_t::update_vm_name() {
	wm_name_is_valid = false;
	char **list = NULL;
	XTextProperty name;
	XGetTextProperty(cnx.dpy, xwin, &name, cnx.atoms.WM_NAME);
	if (!name.nitems) {
		XFree(name.value);
		return;
	}
	wm_name_is_valid = true;
	wm_name = (char const *) name.value;
	XFree(name.value);
}

void client_t::update_net_vm_name() {
	net_wm_name_is_valid = false;
	char **list = NULL;
	XTextProperty name;
	XGetTextProperty(cnx.dpy, xwin, &name, cnx.atoms._NET_WM_NAME);
	if (!name.nitems) {
		XFree(name.value);
		return;
	}
	net_wm_name_is_valid = true;
	net_wm_name = (char const *) name.value;
	XFree(name.value);
}

/* inspired from dwm */
void client_t::update_title() {
	if (net_wm_name_is_valid) {
		name = net_wm_name;
		return;
	}
	if (wm_name_is_valid) {
		name = wm_name;
	}
	std::stringstream s(std::stringstream::in | std::stringstream::out);
	s << "#" << (xwin) << " (noname)";
	name = s.str();
}

void client_t::init_icon() {

	if (icon_surf != 0) {
		cairo_surface_destroy(icon_surf);
		icon_surf = 0;
	}

	if(icon.data != 0) {
		free(icon.data);
		icon.data = 0;
	}

	long * icon_data;
	int32_t * icon_data32;
	int icon_data_size;
	icon_t selected;
	std::list<struct icon_t> icons;
	bool has_icon = false;
	unsigned int n;
	icon_data = get_properties32(cnx.atoms._NET_WM_ICON, cnx.atoms.CARDINAL,
			&n);
	icon_data_size = n;

	if (icon_data != 0) {
		icon_data32 = new int32_t[n];
		for (int i = 0; i < n; ++i)
			icon_data32[i] = icon_data[i];

		int offset = 0;
		while (offset < icon_data_size) {
			icon_t tmp;
			tmp.width = icon_data[offset + 0];
			tmp.height = icon_data[offset + 1];
			tmp.data = (unsigned char *) &icon_data32[offset + 2];
			offset += 2 + tmp.width * tmp.height;
			icons.push_back(tmp);
		}

		icon_t ic;
		int x = 0;
		/* find a icon */
		std::list<icon_t>::iterator i = icons.begin();
		while (i != icons.end()) {
			if ((*i).width <= 16 && x < (*i).width) {
				x = (*i).width;
				ic = (*i);
				has_icon = true;
			}
			++i;
		}

		if (has_icon) {
			selected = ic;
		} else {
			if (!icons.empty()) {
				selected = icons.front();
				has_icon = true;
			} else {
				has_icon = false;
			}

		}

		if (has_icon) {

			int target_height;
			int target_width;
			double ratio;

			if (selected.width > selected.height) {
				target_width = 16;
				target_height = (double) selected.height
						/ (double) selected.width * 16;
				ratio = (double) target_width / (double) selected.width;
			} else {
				target_height = 16;
				target_width = (double) selected.width
						/ (double) selected.height * 16;
				ratio = (double) target_height / (double) selected.height;
			}

			int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
					target_width);

			printf("reformat from %dx%d to %dx%d %f\n", selected.width,
					selected.height, target_width, target_height, ratio);

			icon.width = target_width;
			icon.height = target_height;
			icon.data = (unsigned char *) malloc(stride * target_height);

			for (int j = 0; j < target_height; ++j) {
				for (int i = 0; i < target_width; ++i) {
					int x = i / ratio;
					int y = j / ratio;
					if (x < selected.width && x >= 0 && y < selected.height
							&& y >= 0) {
						((uint32_t *) (icon.data + stride * j))[i] =
								((uint32_t*) selected.data)[x
										+ selected.width * y];
					}
				}
			}

			icon_surf = cairo_image_surface_create_for_data(
					(unsigned char *) icon.data, CAIRO_FORMAT_ARGB32,
					icon.width, icon.height, stride);

		} else {
			icon_surf = 0;
		}

		delete[] icon_data;
		delete[] icon_data32;

	} else {
		icon_surf = 0;
	}

}

void client_t::update_type() {
	unsigned int num;
	long * val = get_properties32(cnx.atoms._NET_WM_WINDOW_TYPE, cnx.atoms.ATOM,
			&num);
	if (val) {
		type.clear();
		/* use the first value that we know about in the array */
		for (unsigned i = 0; i < num; ++i) {
			type.insert(val[i]);
		}
		delete[] val;
	}
}

void client_t::read_net_wm_state() {
	/* take _NET_WM_STATE */
	unsigned int n;
	long * net_wm_state = get_properties32(cnx.atoms._NET_WM_STATE,
			cnx.atoms.ATOM, &n);
	if (net_wm_state) {
		this->net_wm_state.clear();
		for (int i = 0; i < n; ++i) {
			this->net_wm_state.insert(net_wm_state[i]);
		}
		delete[] net_wm_state;
	}
}

void client_t::read_wm_protocols() {
	/* take _NET_WM_STATE */
	unsigned int n;
	long * wm_protocols = get_properties32(cnx.atoms.WM_PROTOCOLS,
			cnx.atoms.ATOM, &n);
	if (wm_protocols) {
		this->wm_protocols.clear();
		for (int i = 0; i < n; ++i) {
			this->wm_protocols.insert(wm_protocols[i]);
		}
		delete[] wm_protocols;
	}
}

void client_t::write_net_wm_state() {
	int size = net_wm_state.size();
	long * new_state = new long[size];
	std::set<Atom>::iterator iter = net_wm_state.begin();
	int i = 0;
	while (iter != net_wm_state.end()) {
		new_state[i] = *iter;
		++iter;
		++i;
	}

	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_WM_STATE, cnx.atoms.ATOM,
			32, PropModeReplace, reinterpret_cast<unsigned char *>(new_state),
			size);
	delete[] new_state;
}

void client_t::set_fullscreen() {
	/* update window state */
	net_wm_state.insert(cnx.atoms._NET_WM_STATE_FULLSCREEN);
	write_net_wm_state();

	//XReparentWindow(cnx.dpy, clipping_window, cnx.composite_overlay, 0, 0);
//	XMoveResizeWindow(cnx.dpy, xwin, 0, 0, cnx.root_size.w, cnx.root_size.h);
//	XMoveResizeWindow(cnx.dpy, clipping_window, 0, 0, cnx.root_size.w,
//			cnx.root_size.h);
//	/* will set full screen, parameters will be ignored*/
//	update_client_size(0, 0);
//	map();
	//cnx.focuced = this;
	//focus();
}

void client_t::unset_fullscreen() {
	/* update window state */
	net_wm_state.erase(cnx.atoms._NET_WM_STATE_FULLSCREEN);
	write_net_wm_state();
	//XReparentWindow(cnx.dpy, clipping_window, page_window, 0, 0);
	//unmap();
}

}
