/*
 * client.cxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

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

	if(is_fullscreen)
		return;


	if(hints.flags & PMaxSize) {
		if(w > hints.max_width)
			w = hints.max_width;
		if(h > hints.max_height)
			h = hints.max_height;
	}


	if(hints.flags & PBaseSize) {
		if(w < hints.base_width)
			w = hints.base_width;
		if(h < hints.base_height)
			h = hints.base_height;
	} else if (hints.flags & PMinSize) {
		if(w < hints.min_width)
			w = hints.min_width;
		if(h < hints.min_height)
			h = hints.min_height;
	}

	if(hints.flags & PAspect) {
		if(hints.flags & PBaseSize) {
			/* ICCCM say if base is set substract base before aspect checking ref : ICCCM*/
			if((w-hints.base_width) * hints.min_aspect.y < (h-hints.base_height) * hints.min_aspect.x) {
				/* reduce h */
				h = hints.base_height + ((w-hints.base_width) * hints.min_aspect.y) / hints.min_aspect.x;

			} else if ((w-hints.base_width) * hints.max_aspect.y > (h-hints.base_height) * hints.max_aspect.x) {
				/* reduce w */
				w = hints.base_width + ((h-hints.base_height) * hints.max_aspect.x) / hints.max_aspect.y;
			}
		} else {
			if(w * hints.min_aspect.y < h * hints.min_aspect.x) {
				/* reduce h */
				h = (w * hints.min_aspect.y) / hints.min_aspect.x;

			} else if (w * hints.max_aspect.y > h * hints.max_aspect.x) {
				/* reduce w */
				w = (h * hints.max_aspect.x) / hints.max_aspect.y;
			}
		}

	}

	if(hints.flags & PResizeInc) {
		w -= ((w - hints.base_width) % hints.width_inc);
		h -= ((h - hints.base_height) % hints.height_inc);
	}

	height = h;
	width = w;

	printf("Update #%p window size %dx%d\n", (void *) xwin, width, height);
}

/* check if client is still alive */
bool client_t::try_lock_client() {
	//XGrabServer(dpy);
	//XSync(dpy, False);
	XEvent e;
	if (XCheckTypedWindowEvent(dpy, xwin, DestroyNotify, &e)) {
		XPutBackEvent(dpy, &e);
		//XUngrabServer(dpy);
		//XFlush(dpy);
		return false;
	}
	return true;
}

void client_t::unlock_client() {
	//XUngrabServer(dpy);
	//XFlush(dpy);
}

void client_t::focus() {
	if(is_map) {
		XRaiseWindow(dpy, clipping_window);
		XRaiseWindow(dpy, xwin);
		XSetInputFocus(dpy, xwin, RevertToNone, CurrentTime);
	}
}

void client_t::fullscreen(int w, int h) {
	width = w;
	height = h;
}

}
