/*
 * listen_xdamage.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <X11/Xutil.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xdamage.h>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <limits>
#include <list>

int main(int argc, char ** argv) {
	Display * dpy = XOpenDisplay(0);

	Window xroot = DefaultRootWindow(dpy) ;

	unsigned int num;
	Window * wins;
	Window d1, d2;
	if (XQueryTree(dpy, xroot, &d1, &d2, &wins, &num)) {
		for (unsigned i = 0; i < num; ++i) {
			if(wins[i] == 0x1a00001)
				continue;
			XWindowAttributes a;
			XGetWindowAttributes(dpy, wins[i], &a);
			if (a.c_class == InputOutput) {
				XDamageCreate(dpy, wins[i], XDamageReportRawRectangles);
			}
		}

		XFree(wins);
	}

	int damage_opcode, damage_event, damage_error;

	// check/init Damage.
	if (!XQueryExtension(dpy, DAMAGE_NAME, &damage_opcode, &damage_event,
			&damage_error)) {
		throw std::runtime_error("Damage extension is not supported");
	} else {
		int major = 0, minor = 0;
		XDamageQueryVersion(dpy, &major, &minor);
		printf("Damage Extension version %d.%d found\n", major, minor);
		printf("Damage error %d, Damage event %d\n", damage_error,
				damage_event);
	}

	while (true) {
		XEvent ev;
		XNextEvent(dpy, &ev);

		if (ev.type == damage_event + XDamageNotify) {
			XDamageNotifyEvent &e = (XDamageNotifyEvent&) ev;
			printf("%ld (%d,%d,%d,%d)\n", e.damage, e.area.x, e.area.y,
					e.area.width, e.area.height);
		}

	}

	return 0;
}

