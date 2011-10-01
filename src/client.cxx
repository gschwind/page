/*
 * client.cxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

#include <X11/Xlib.h>
#include <stdio.h>
#include "client.hxx"

namespace page_next {

void client_t::map() {
	is_map = true;
	XMapWindow(dpy, xwin);
	XMapWindow(dpy, clipping_window);
}
void client_t::unmap() {
	if (is_map) {
		is_map = false;
		unmap_pending += 1;
		printf("Unmap of #%lu\n", xwin);
		/* ICCCM require that WM unmap client window to change client state from
		 * Normal to Iconic state
		 * in PAGE all unviewable window are in iconic state */
		XUnmapWindow(dpy, clipping_window);
		XUnmapWindow(dpy, xwin);
	}
}

void client_t::update_client_size(int w, int h) {
	if (maxw != 0 && w > maxw) {
		w = maxw;
	}

	if (maxh != 0 && h > maxh) {
		h = maxh;
	}

	if (minw != 0 && w < minw) {
		w = minw;
	}

	if (minh != 0 && h < minh) {
		h = minh;
	}

	if (incw != 0) {
		w -= ((w - basew) % incw);
	}

	if (inch != 0) {
		h -= ((h - baseh) % inch);
	}

	/* TODO respect Aspect */
	height = h;
	width = w;

	printf("Update #%p window size %dx%d\n", (void *) xwin, width, height);
}

bool client_t::try_lock_client() {
	XGrabServer(dpy);
	XSync(dpy, False);
	XEvent e;
	if (XCheckTypedWindowEvent(dpy, xwin, DestroyNotify, &e)) {
		XPutBackEvent(dpy, &e);
		XUngrabServer(dpy);
		XFlush(dpy);
		return false;
	}
	return true;
}

void client_t::unlock_client() {
	XUngrabServer(dpy);
	XFlush(dpy);
}

}
