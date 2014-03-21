/*
 * simple_window_test.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

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

	Atom PAGE_QUIT = XInternAtom(dpy, "PAGE_QUIT", False);
	XEvent ev = { 0 };
	ev.xclient.display = dpy;
	ev.xclient.type = ClientMessage;
	ev.xclient.format = 32;
	ev.xclient.message_type = PAGE_QUIT;
	ev.xclient.window = root;
	ev.xclient.data.l[0] = 0;
	ev.xclient.data.l[1] = 0;
	XSendEvent(dpy, root, False, 0xffffff, &ev);

	XFlush(dpy);

	XCloseDisplay(dpy);

	return 0;
}
