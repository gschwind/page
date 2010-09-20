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

struct page {
	GdkDisplay * dpy;
	GdkScreen * scn;
	GdkWindow * root;
	gint sw, sh;

	GdkWindow * gdk_main_win;
	GtkWidget * gtk_main_win;

	GMainLoop * main_loop;
	GtkWidget * notebook;

	GdkCursor * main_cursor;
	GList * managed_windows;

	/* event handler */
	GdkFilterReturn (*event_handler[LASTEvent])(page *, XEvent *);

};

page * page_new();
void page_init(page * ths, int * argc, char *** argv);
void page_init_event_hander(page * ths);
void page_run(page * ths);
void page_destroy(page * ths);
void page_scan(page * ths);
void page_manage(page * ths, Window w, XWindowAttributes * wa);
GdkFilterReturn page_event_callback(GdkXEvent * xevent, GdkEvent * event,
		gpointer data);
GdkFilterReturn page_process_map_request(page * ths, XEvent * e);

#endif /* PAGE_CONTEXT_H_ */
