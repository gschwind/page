/*
 * simple_window_test.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <unistd.h>

#include <X11/Xutil.h>

#include <X11/Xlib.h>
#include <stdexcept>
#include <cstdio>
#include <string>

int main(int argc, char ** argv) {

	Display * dpy = XOpenDisplay(0);
	Window root = XDefaultRootWindow(dpy);
	int screen = XDefaultScreen(dpy);

	XWindowAttributes wa;
	XGetWindowAttributes(dpy, root, &wa);

	Window w = XCreateSimpleWindow(dpy, root, 0, 0, 100, 100, 0, 0,
			XWhitePixel(dpy, screen));

	XSelectInput(dpy, w, SubstructureNotifyMask);

	printf("XMapWindow\n");
	XMapWindow(dpy, w);

	sleep(10);
	printf("XIconifyWindow\n");
	XIconifyWindow(dpy, w, screen);

	sleep(10);
	printf("XWithdrawWindow\n");
	XWithdrawWindow(dpy, w, screen);

	sleep(10);
	printf("XDestroyWindow\n");
	XDestroyWindow(dpy, w);

	return 0;
}
