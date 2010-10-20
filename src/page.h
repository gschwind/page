/*
 * page_context.h
 *
 *  Created on: Sep 19, 2010
 *      Author: gschwind
 */

#ifndef PAGE_CONTEXT_H_
#define PAGE_CONTEXT_H_

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include "tree.h"

enum { WMProtocols, WMDelete, WMState, WMLast };        /* default atoms */

typedef struct _page page;
struct _page {
	GdkDisplay * dpy;
	GdkScreen * scn;
	GdkWindow * root;

	/* size of scn */
	gint sw, sh;

	GdkWindow * gdk_main_win;
	GtkWidget * gtk_main_win;

	GMainLoop * main_loop;
	GtkWidget * notebook1;
	GtkWidget * notebook2;
	GtkWidget * hpaned;
	GdkCursor * main_cursor;

	GSList * clients;

	/* events handlers */
	GdkFilterReturn (*event_handler[LASTEvent])(page *, XEvent *);

	tree * t;

};

/* juste create the page */
page * page_new();
/* init, must be call before other function of page */
void page_init(page * ths, int * argc, char *** argv);
/* link event to corresponding function */
void page_init_event_hander(page * ths);
/* run loop till exit */
void page_run(page * ths);
/* destroy the current context */
void page_destroy(page * ths);
/* this function scan for each sub window of root. */
void page_scan(page * ths);
/* manage a new visible window */
void page_manage(page * ths, GdkWindow * w);
/* filter event, event can be removed, translated to gdk, or just forwarded
 * for normal processing */
GdkFilterReturn page_filter_event(GdkXEvent * xevent, GdkEvent * event,
		gpointer data);
GdkFilterReturn page_process_map_request(page * ths, XEvent * e);

#endif /* PAGE_CONTEXT_H_ */
