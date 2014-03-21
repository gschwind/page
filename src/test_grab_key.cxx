/*
 * test_grab_key.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/keysym.h>
#include <cstdio>

int main() {

	Display * dpy;
	dpy = XOpenDisplay(0);
	Window root = XDefaultRootWindow(dpy);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_y), Mod4Mask, root, 0, GrabModeAsync, GrabModeSync);

	while(true) {

		XEvent ev;
		XNextEvent(dpy, &ev);
		if(ev.type == KeyPress or ev.type == KeyRelease) {

			if (ev.xkey.keycode == 29
					and ev.type == KeyPress and (ev.xkey.state == Mod4Mask)) {
				XGrabKeyboard(dpy, root, 0, GrabModeAsync, GrabModeAsync,
						ev.xkey.time);
			}

			if (ev.xkey.keycode == 133 and ev.type == KeyRelease) {
				XUngrabKeyboard(dpy, ev.xkey.time);
			}

			XAllowEvents(dpy, AsyncKeyboard, ev.xkey.time);

			if (ev.type == KeyPress) {
				printf("KeyPress key = %d, mod4 = %s, mod1 = %s\n", ev.xkey.keycode,
						ev.xkey.state & Mod4Mask ? "true" : "false",
						ev.xkey.state & Mod1Mask ? "true" : "false");
			} else {
				printf("KeyRelease key = %d, mod4 = %s, mod1 = %s\n",
						ev.xkey.keycode, ev.xkey.state & Mod4Mask ? "true" : "false",
						ev.xkey.state & Mod1Mask ? "true" : "false");
			}


		}

	}




}
