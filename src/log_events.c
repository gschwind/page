/*
 * log_events.c
 *
 *  Created on: Oct 29, 2010
 *      Author: gschwind
 */

#include <X11/Xlib.h>
#include <stdio.h>

Display * dpy;

char const * x_event_name[LASTEvent] = { 0, 0, "KeyPress", "KeyRelease",
		"ButtonPress", "ButtonRelease", "MotionNotify", "EnterNotify",
		"LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose",
		"GraphicsExpose", "NoExpose", "VisibilityNotify", "CreateNotify",
		"DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
		"ReparentNotify", "ConfigureNotify", "ConfigureRequest",
		"GravityNotify", "ResizeRequest", "CirculateNotify",
		"CirculateRequest", "PropertyNotify", "SelectionClear",
		"SelectionRequest", "SelectionNotify", "ColormapNotify",
		"ClientMessage", "MappingNotify", "GenericEvent" };

void run(void) {
	XEvent ev;
	/* main event loop */
	XSync(dpy, False);
	while (!XNextEvent(dpy, &ev)) {
		if (x_event_name[ev.type])
			printf("Event no %d : %s on #%p\n", ev.xany.serial,
					x_event_name[ev.type], (void *) ev.xany.window);
		if (ev.type == MapRequest)
			XMapWindow(dpy, ev.xmaprequest.window);
		if (ev.type == ConfigureRequest) {
			XWindowChanges wc;
			wc.x = ev.xconfigurerequest.x;
			wc.y = ev.xconfigurerequest.y;
			wc.width = ev.xconfigurerequest.width;
			wc.height = ev.xconfigurerequest.height;
			wc.border_width = ev.xconfigurerequest.border_width;
			wc.sibling = ev.xconfigurerequest.above;
			wc.stack_mode = ev.xconfigurerequest.detail;
			XConfigureWindow(dpy, ev.xconfigurerequest.window,
					ev.xconfigurerequest.value_mask, &wc);
		}
	}
}

int main(int argc, char * * argv) {
	dpy = XOpenDisplay(NULL);
	XSelectInput(dpy, XDefaultRootWindow(dpy), ExposureMask
			| SubstructureRedirectMask | SubstructureNotifyMask);
	run();
	return 0;
}
