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

	if (is_fullscreen)
		return;

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

void client_t::fullscreen(int w, int h) {
	width = w;
	height = h;
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

void client_t::parse_icons() {
	int offset = 0;
	while (offset < icon_data_size) {
		icon tmp;
		tmp.width = icon_data[offset + 0];
		tmp.height = icon_data[offset + 1];
		printf("FIND ICON%d %d\n", tmp.width, tmp.height);
		tmp.data = &icon_data32[offset + 3];
		offset += 2 + tmp.width * tmp.height;
		icons.push_back(tmp);
	}
}

void client_t::init_icon() {
	unsigned int n;
	icon_data = cnx.get_properties32(xwin, cnx.atoms._NET_WM_ICON,
			cnx.atoms.CARDINAL, &n);
	icon_data_size = n;

	if (n > 0) {
		icon_data32 = new int32_t[n];
		for (int i = 0; i < n; ++i)
			icon_data32[i] = icon_data[i];
	}
	parse_icons();

	icon ic;
	int x = 0;
	int y = 0;
	bool has_icon = false;
	/* find a icon */
	std::list<icon>::iterator i = icons.begin();
	while (i != icons.end()) {
		if ((*i).width <= 16 && x < (*i).width) {
			x = (*i).width;
			ic = (*i);
			has_icon = true;
		}
		++i;
	}

	if (has_icon)
		selected = ic;
	else
		selected = icons.front();

	icon_surf = cairo_image_surface_create_for_data(
			(unsigned char *) selected.data, CAIRO_FORMAT_ARGB32,
			selected.width, selected.height,
			cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, selected.width));
}

}
