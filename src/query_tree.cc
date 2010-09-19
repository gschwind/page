/*
 * query_tree.cc
 *
 *  Created on: 10 sept. 2010
 *      Author: gschwind
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <list>
#include <map>
#include <string>
#include <pango/pangoxft.h>
#include <pango/pango.h>

#include <glib-object.h>

Display * dpy;
Atom atom_name;

bool get_text_prop(Window w, Atom atom, std::string & s) {
	char * * list = 0;
	int n;
	XTextProperty name;
	XGetTextProperty(dpy, w, &name, atom);
	if (!name.nitems)
		return false;
	if (name.encoding == XA_STRING)
		s = ((char *) name.value);
	else {
		if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success
				&& n > 0 && *list) {
			s = *list;
			XFreeStringList(list);
		}
	}
	XFree(name.value);
	return true;
}

std::string update_title(Window w) {
	std::string s;
	if (!get_text_prop(w, atom_name, s))
		get_text_prop(w, XA_WM_NAME, s);
	return s;
}

void scan(Window w, int level) {
	unsigned int i, num;
	Window d1, d2, *wins = 0;
	XWindowAttributes wa;

	char * escape = (char *)malloc(level + 1);
	for(int i = 0; i < level; ++i)
		escape[i] = '|';
	escape[level] = 0;
	std::string s = update_title(w);
	printf("%s+-%d %s\n", escape, (int)w, s.c_str());
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
