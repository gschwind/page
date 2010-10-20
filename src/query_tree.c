/*
 * query_tree.cc
 *
 *  Created on: 10 sept. 2010
 *      Author: gschwind
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <pango/pangoxft.h>
#include <pango/pango.h>

#include <glib-object.h>

Display * dpy;
Atom atom_name;

int get_text_prop(Window w, Atom atom, char * * s) {
	char * * list = 0;
	int n;
	XTextProperty name;
	XGetTextProperty(dpy, w, &name, atom);
	if (!name.nitems)
		return 0;
	if (name.encoding == XA_STRING)
		*s = g_strdup(name.value);
	else {
		if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success
				&& n > 0 && *list) {
			*s = g_strdup(*list);
			XFreeStringList(list);
		}
	}
	XFree(name.value);
	return 1;
}

char * update_title(Window w) {
	char * s;
	if (!get_text_prop(w, atom_name, &s))
		get_text_prop(w, XA_WM_NAME, &s);
	return s;
}

void scan(Window w, int level) {
	unsigned int i, num;
	Window d1, d2, *wins = 0;
	XWindowAttributes wa;

	char * escape = (char *)malloc(level + 1);
	for(i = 0; i < level; ++i)
		escape[i] = '|';
	escape[level] = 0;
	char * s = update_title(w);
	printf("%s+-%d %s\n", escape, (int)w, s);
	/* ask for child of current root window */
	if (XQueryTree(dpy, w,
			&d1, &d2, &wins, &num)) {

		for (i = 0; i < num; ++i) {
			scan(wins[i], level + 1);
		}
	}

	if (wins)
		XFree(wins);
	free(escape);
}

int main(int argc, char * * argv) {
	dpy = XOpenDisplay(NULL);
	atom_name = XInternAtom(dpy, "_NET_WM_NAME", False);
	scan(XDefaultRootWindow(dpy), 0);
	return 0;
}
