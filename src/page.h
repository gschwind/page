/*
 * Copyright (C) 2010 Benoit Gschwind
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef PAGE_CONTEXT_H_
#define PAGE_CONTEXT_H_

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include "tree.h"
#include "atoms.h"

enum {
	AtomType,
	WMProtocols,
	WMDelete,
	WMState,
	WMLast,
	NetSupported,
	NetWMName,
	NetWMState,
	NetWMFullscreen,
	NetWMType,
	NetWMTypeDock,
	NetLast,
	AtomLast
}; /* EWMH atoms */

typedef struct _page page;
struct _page {
	Display * xdpy;
	gint xscr;
	Window xroot;
	/* size of default root window */
	gint sw, sh, sx, sy;
	gint start_x, end_x;
	gint start_y, end_y;

	/* gtk window wich handle notebooks */
	GtkWidget * gtk_main_win;
	Window x_main_window;

	GMainLoop * main_loop;

	GSList * clients;

	/* events handlers */
	GdkFilterReturn (*event_handler[LASTEvent])(page *, XEvent *);

	/* tree of notebooks */
	tree * t;

	atoms_t atoms;
};

/* just create the page */
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
void page_manage(page * ths, Window w, XWindowAttributes * wa);
/* filter event, event can be removed, translated to gdk, or just forwarded
 * for normal processing */
GdkFilterReturn page_filter_event(GdkXEvent * xevent, GdkEvent * event,
		gpointer data);
GdkFilterReturn page_process_map_request(page * ths, XEvent * e);
/* clients create a window */
GdkFilterReturn page_process_create_notify_event(page * ths, XEvent * e);
/* clients configure is window */
GdkFilterReturn page_process_configure_request_event(page * ths, XEvent * e);
/* do unmap */
GdkFilterReturn page_process_unmap_notify_event(page * ths, XEvent * e);

GdkFilterReturn page_process_destroy_notify_event(page * ths, XEvent * ev);
GdkFilterReturn page_process_property_notify_event(page * ths, XEvent * ev);

#endif /* PAGE_CONTEXT_H_ */
