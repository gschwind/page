/*
 * page_context.c
 *
 *  Created on: Sep 20, 2010
 *      Author: gschwind
 */

#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "page.h"
#include "client.h"

page * page_new() {
	page * ths = 0;
	ths = (page *) malloc(sizeof(page));
	/* TODO: manage error */
	memset(ths, 0, sizeof(page));
	return ths;
}

void page_init(page * ths, int * argc, char *** argv) {
	gtk_init(argc, argv);
	ths->dpy = gdk_display_open(NULL);
	ths->scn = gdk_display_get_default_screen(ths->dpy);
	ths->root = gdk_screen_get_root_window(ths->scn);
	gdk_window_get_geometry(ths->root, NULL, NULL, &ths->sw, &ths->sh, NULL);
	printf("display size %d %d\n", ths->sw, ths->sh);

	ths->managed_windows = g_new(GList, 1);

	page_init_event_hander(ths);
}

gboolean page_quit(GtkWidget * w, GdkEventButton * e, gpointer data) {
	printf("call %s\n", __FUNCTION__);
	page * ths = (page *)data;
	g_main_loop_quit(ths->main_loop);
}

GtkWidget * build_control_tab(page * ths) {
	GtkWidget * hbox = gtk_hbox_new(TRUE, 0);
	GtkWidget * split_button = gtk_button_new_with_label("SPLIT");
	GtkWidget * close_button = gtk_button_new_with_label("CLOSE");
	GtkWidget * exit_button = gtk_button_new_with_label("EXIT");
	g_signal_connect(GTK_OBJECT(exit_button), "button-release-event", GTK_SIGNAL_FUNC(page_quit), ths);
	gtk_box_pack_start(GTK_BOX(hbox), split_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), close_button, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), exit_button, TRUE, TRUE, 0);
	gtk_widget_show_all(hbox);
	return hbox;

}

void page_run(page * ths) {
	gint x, y, width, height, depth;

	ths->gtk_main_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(ths->gtk_main_win), ths->sw, ths->sh);
	gtk_widget_show_all(ths->gtk_main_win);
	ths->gdk_main_win = gtk_widget_get_window(ths->gtk_main_win);
	ths->main_cursor = gdk_cursor_new(GDK_PLUS);
	gdk_window_set_cursor(ths->gdk_main_win, ths->main_cursor);

	ths->notebook = gtk_notebook_new();
	GtkWidget * label0 = gtk_label_new("hello world 0");
	GtkWidget * label1 = gtk_label_new("hello world 1");
	GtkWidget * label2 = gtk_label_new("hello world 2");
	GtkWidget * tab_label0 = build_control_tab(ths);
	GtkWidget * tab_label1 = gtk_label_new("label 1");
	GtkWidget * tab_label2 = gtk_label_new("label 2");

	gtk_notebook_append_page(GTK_NOTEBOOK(ths->notebook), label0, tab_label0);
	gtk_notebook_append_page(GTK_NOTEBOOK(ths->notebook), label1, tab_label1);
	gtk_notebook_append_page(GTK_NOTEBOOK(ths->notebook), label2, tab_label2);
	gtk_container_add(GTK_CONTAINER(ths->gtk_main_win), ths->notebook);

	page_scan(ths);
	gtk_widget_show_all(ths->gtk_main_win);
	gtk_widget_queue_draw(ths->gtk_main_win);

	/* listen for new windows */
	/* there is no gdk equivalent to the folowing */
	XSelectInput(gdk_x11_display_get_xdisplay(ths->dpy), GDK_WINDOW_XID(ths->root),
			SubstructureRedirectMask | SubstructureNotifyMask);
	/* get all unhandled event */
	gdk_window_add_filter(ths->root, page_event_callback, ths);

	ths->main_loop = g_main_loop_new(NULL, FALSE);
	printf("start main loop\n");
	g_main_loop_run(ths->main_loop);

}

char const * event_name[LASTEvent] = { 0, 0, "KeyPress", "KeyRelease",
		"ButtonPress", "ButtonRelease", "MotionNotify", "EnterNotify",
		"LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose",
		"GraphicsExpose", "NoExpose", "VisibilityNotify", "CreateNotify",
		"DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
		"ReparentNotify", "ConfigureNotify", "ConfigureRequest",
		"GravityNotify", "ResizeRequest", "CirculateNotify",
		"CirculateRequest", "PropertyNotify", "SelectionClear",
		"SelectionRequest", "SelectionNotify", "ColormapNotify",
		"ClientMessage", "MappingNotify", "GenericEvent" };

/* this function is call on each event of root window, it can filter, or translate event to another */
GdkFilterReturn page_event_callback(GdkXEvent * xevent, GdkEvent * event,
		gpointer data) {
	page * ths = (page *) data;
	XEvent * e = (XEvent *) (xevent);
	if (e->type != MotionNotify)
		printf("process event %d %d : %s\n", (int) e->xany.window,
				(int) e->xany.serial, event_name[e->type]);
	if (ths->event_handler[e->type])
		return ths->event_handler[e->type](ths, e); /* call handler */
	return GDK_FILTER_CONTINUE;
}

void page_scan(page * ths) {
	printf("call %s\n", __FUNCTION__);
	unsigned int i, num;
	Window d1, d2, *wins = 0;
	XWindowAttributes wa;
	Display * dpy;

	dpy = gdk_x11_display_get_xdisplay(ths->dpy);

	/* ask for child of current root window, use Xlib here since gdk only know windows it
	 * have created. */
	if (XQueryTree(dpy, GDK_WINDOW_XID(ths->root), &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; ++i) {
			/* try to get Window Atribute */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (wa.map_state == IsViewable)
				page_manage(ths, wins[i], &wa);
		}
	}

	if (wins)
		XFree(wins);
}

/* this function is call when a client want map a new window
 * the evant is on root window, e->xmaprequest.window is the window that request the map */
GdkFilterReturn tabwm_process_map_request(page * ths, XEvent * e) {
	printf("call %s\n", __FUNCTION__);
	if (e->xmaprequest.window == GDK_WINDOW_XID(ths->gdk_main_win)) {
		/* just map, do noting else */
		gdk_window_show(ths->gdk_main_win);
		gtk_widget_queue_draw(ths->gtk_main_win);
		printf("try mapping myself\n");
		return GDK_FILTER_REMOVE;
	}
	XWindowAttributes wa;
	if (!XGetWindowAttributes(e->xmaprequest.display, e->xmaprequest.window,
			&wa)) {
		/* cannot get attribute, just ignore this request */
		return GDK_FILTER_CONTINUE;
	}

	/* manage the window but do not map it
	 * Map will be made on widget show
	 */
	page_manage(ths, e->xmaprequest.window, &wa);
	return GDK_FILTER_REMOVE;

}

client * find_client(page * ths, Window w) {
	GList * i = ths->managed_windows;
	while (i != NULL) {
		if (((client *) (i->data))->xwin == w)
			return (client *) i->data;
		i = i->next;
	}
	return NULL;
}

GdkFilterReturn tabwm_process_destroy(page * ths, XEvent * e) {
	printf("call %s\n", __FUNCTION__);
	client * c = 0;
	if (c = find_client(ths, e->xdestroywindow.window)) {
		gtk_notebook_remove_page((c->notebook_parent), gtk_notebook_page_num(
				c->notebook_parent, GTK_WIDGET(c->xwindow_handler)));
		ths->managed_windows = g_list_remove(ths->managed_windows, c);
		delete c;
	}
	gtk_widget_queue_draw(GTK_WIDGET(ths->notebook));
	return GDK_FILTER_REMOVE;
}

void page_init_event_hander(page * ths) {
	for (gint i = 0; i < LASTEvent; ++i) {
		ths->event_handler[i] = NULL;
	}

	ths->event_handler[MapRequest] = tabwm_process_map_request;
	ths->event_handler[DestroyNotify] = tabwm_process_destroy;
}

void page_manage(page * ths, Window w, XWindowAttributes * wa) {
	printf("call %s\n", __FUNCTION__);
	if (w == GDK_WINDOW_XID(ths->gdk_main_win)) {
		printf("Do not manage myself\n");
		return;
	}

	client * c = 0;
	c = new client;

	c->xwin = w;
	c->gwin = gdk_window_foreign_new(w);

	printf("Manage %d %dx%d+%d+%d\n", (gint) w, wa->width, wa->height, wa->x,
			wa->y);

	gchar * title = g_strdup_printf("%d", (gint) w);
	GtkWidget * label = gtk_label_new(title);
	GtkWidget * content = gtk_xwindow_handler_new();
	gtk_xwindow_handler_set_gwindow(GTK_XWINDOW_HANDLER(content), c->gwin);
	gtk_notebook_prepend_page(GTK_NOTEBOOK(ths->notebook), content, label);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(ths->notebook), content, TRUE);
	gtk_widget_show_all(GTK_WIDGET(ths->notebook));
	gtk_widget_draw(GTK_WIDGET(ths->notebook), NULL);
	c->notebook_parent = GTK_NOTEBOOK(ths->notebook);
	c->xwindow_handler = GTK_XWINDOW_HANDLER(content);
	ths->managed_windows = g_list_append(ths->managed_windows, c);
}
