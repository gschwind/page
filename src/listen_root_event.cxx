/*
 * listen_root_event.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include "X11/Xlib.h"
#include <cstdio>

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

int main(int argc, char ** argv) {

	Display * dpy = XOpenDisplay(0);
	Window root = XDefaultRootWindow(dpy);
	int screen = XDefaultScreen(dpy);

	Window w = XCreateSimpleWindow(dpy, root, 0, 0, 300, 300, 0, 0, 0);

	//XSelectInput(dpy, root, SubstructureNotifyMask | SubstructureRedirectMask);

	//XSelectInput(dpy, w, SubstructureNotifyMask | SubstructureRedirectMask | ButtonPressMask | ButtonMotionMask | ButtonReleaseMask);

	XGrabButton(dpy, AnyButton, AnyModifier, w, False, ButtonPressMask | ButtonMotionMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync, None, None);

	XMapWindow(dpy, w);

	while (true) {
		XEvent ev;
		XNextEvent(dpy, &ev);

		if (ev.type == MapNotify) {
			XMapEvent & e = ev.xmap;
			printf("%s event: #%lu event: #%lu redirect:%d window: #%lu send_event: %d\n",
					x_event_name[e.type], e.event, e.window,
					e.override_redirect, e.window, e.send_event);
		} else if (ev.type == UnmapNotify) {
			XUnmapEvent & e = ev.xunmap;
			printf("%s event: #%lu event: #%lu from_configure:%d window: #%lu send_event: %d\n",
					x_event_name[e.type], e.event, e.window,
					e.from_configure, e.window, e.send_event);
		} else if (ev.type == CreateNotify) {
			XCreateWindowEvent & e = ev.xcreatewindow;
			printf("%s parent: #%lu window #%lu overide_redirect:%d send_event: %d\n",
					x_event_name[e.type], e.parent, e.window,
					e.override_redirect, e.send_event);
		} else if (ev.type == ReparentNotify) {
			XReparentEvent & e = ev.xreparent;
			printf("%s parent: #%lu window #%lu overide_redirect:%d send_event: %d\n",
					x_event_name[e.type], e.parent, e.window,
					e.override_redirect, e.send_event);
		} else if (ev.type == MapRequest) {
			XMapRequestEvent & e = ev.xmaprequest;
			printf("%s parent: #%lu window #%lu send_event: %d\n",
					x_event_name[e.type], e.parent, e.window, e.send_event);
			//XMapWindow(dpy, e.window);
		} else if (ev.type == DestroyNotify) {
			XDestroyWindowEvent & e = ev.xdestroywindow;
			printf("%s event: #%lu window #%lu send_event: %d\n",
					x_event_name[e.type], e.event, e.window, e.send_event);
		} else if (ev.type == ButtonPress) {
			printf("%s event: #%lu window #%lu send_event: %d\n",
					x_event_name[ev.xany.type], ev.xany.serial, ev.xany.window, ev.xany.send_event);
			XAllowEvents(dpy, AsyncPointer, ev.xbutton.time);
		} else {
			printf("%s event: #%lu window #%lu send_event: %d\n",
					x_event_name[ev.xany.type], ev.xany.serial, ev.xany.window, ev.xany.send_event);
		}
	}

}
