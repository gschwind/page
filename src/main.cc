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

#include "gtk_xwindow_handler.h"
#include "page.h"

int main(int argc, char * * argv) {
	page p;
	page_init(&p, &argc, &argv);
	page_run(&p);
}

