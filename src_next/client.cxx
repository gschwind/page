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
	XMapWindow(dpy, clipping_window);
}
void client_t::unmap() {
	XUnmapWindow(dpy, clipping_window);
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

}
