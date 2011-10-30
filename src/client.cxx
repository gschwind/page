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
#include <stdint.h>
#include <stdlib.h>
#include <cstring>
#include "client.hxx"

namespace page_next {

void client_t::map() {
	is_map = true;
	XMapWindow(cnx.dpy, xwin);
	XMapWindow(cnx.dpy, clipping_window);
}

void client_t::unmap() {
	if (is_map) {
		is_map = false;
		unmap_pending += 1;
		printf("Unmap of #%lu\n", xwin);
		/* ICCCM require that WM unmap client window to change client state from
		 * Normal to Iconic state
		 * in PAGE all unviewable window are in iconic state */
		XUnmapWindow(cnx.dpy, clipping_window);
		XUnmapWindow(cnx.dpy, xwin);
	}
}

void client_t::update_client_size(int w, int h) {

	if (is_fullscreen()) {
		height = cnx.root_size.h;
		width = cnx.root_size.w;
	}

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

	printf("Update #%p window size %dx%d\n", (void *) xwin, width, height);
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
	return true;
}

void client_t::unlock_client() {
	cnx.ungrab();
}

void client_t::focus() {
	if (is_map) {
		XRaiseWindow(cnx.dpy, clipping_window);
		XRaiseWindow(cnx.dpy, xwin);
		XSetInputFocus(cnx.dpy, xwin, RevertToNone, CurrentTime);
	}
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
	int n;
	XTextProperty name;
	XGetTextProperty(cnx.dpy, xwin, &name, cnx.atoms.WM_NAME);
	if (!name.nitems)
		return;
	wm_name_is_valid = true;
	wm_name = (char const *) name.value;
	XFree(name.value);
}

void client_t::update_net_vm_name() {
	net_wm_name_is_valid = false;
	char **list = NULL;
	int n;
	XTextProperty name;
	XGetTextProperty(cnx.dpy, xwin, &name, cnx.atoms._NET_WM_NAME);
	if (!name.nitems)
		return;
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
		int y = 0;

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
			if (icons.size() > 0) {
				selected = icons.front();
				has_icon = true;
			} else {
				has_icon = false;
			}

		}

		if (has_icon) {
			int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
					selected.width);

			icon.width = selected.width;
			icon.height = selected.height;
			icon.data = (unsigned char *) malloc(stride * selected.height);

			for (int i = 0; i < selected.height; ++i) {
				memcpy(icon.data + stride * i,
						selected.data + (sizeof(uint32_t) * selected.width * i),
						sizeof(uint32_t) * selected.width);
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
	unsigned int num, i;
	long * val = get_properties32(cnx.atoms._NET_WM_WINDOW_TYPE, cnx.atoms.ATOM,
			&num);
	if (val) {
		type.clear();
		/* use the first value that we know about in the array */
		for (i = 0; i < num; ++i) {
			type.insert(val[i]);
		}
		delete[] val;
	}
}

void client_t::read_wm_state() {
	/* take _NET_WM_STATE */
	unsigned int n;
	long * net_wm_state = get_properties32(cnx.atoms._NET_WM_STATE,
			cnx.atoms.ATOM, &n);
	if (net_wm_state) {
		this->net_wm_state.clear();
		for (int i = 0; i < n; ++i) {
			this->net_wm_state.insert(net_wm_state[i]);
		}
	}
}

void client_t::write_wm_state() {
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
	write_wm_state();

	XReparentWindow(cnx.dpy, clipping_window, cnx.xroot, 0, 0);
	XMoveResizeWindow(cnx.dpy, xwin, 0, 0, cnx.root_size.w, cnx.root_size.h);
	XMoveResizeWindow(cnx.dpy, clipping_window, 0, 0, cnx.root_size.w,
			cnx.root_size.h);
	/* will set full screen, parameters will be ignored*/
	update_client_size(0, 0);
	map();
	focus();
}

void client_t::unset_fullscreen() {
	/* update window state */
	net_wm_state.erase(cnx.atoms._NET_WM_STATE_FULLSCREEN);
	write_wm_state();
	XReparentWindow(cnx.dpy, clipping_window, page_window, 0, 0);
	unmap();
}

}
