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
	if (!is_mapped) {
		printf("map %s\n", name.c_str());
		XMapWindow(dpy, clipping_window);
		is_mapped = true;
	}

}
void client_t::unmap() {
	if (is_mapped) {
		XUnmapWindow(dpy, clipping_window);
		is_mapped = false;
	}
}

}
