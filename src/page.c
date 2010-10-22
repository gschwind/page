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
#include "tree.h"

static GdkAtom wmatom[WMLast];

client * page_find_client_by_widget(page * ths, GtkWidget * w);
client * page_find_client_by_gwindow(page * ths, GdkWindow * w);

page * page_new() {
	page * ths = 0;
	ths = (page *) malloc(sizeof(page));
	/* TODO: manage error */
	memset(ths, 0, sizeof(page));
	return ths;
}

static void page_page_added(GtkNotebook *notebook, GtkWidget *child,
		guint page_num, gpointer user_data) {
	printf("call %s\n", __FUNCTION__);
	printf("try to page %p added to %p\n", child, notebook);
	/* register this page to the notebook */

	page * ths = (page *) user_data;
	client * c = page_find_client_by_widget(ths, child);
	if (c == NULL)
		return;
	c->notebook_parent = notebook;
	printf("page %p added to %p\n", child, notebook);
	gtk_widget_queue_draw(GTK_WIDGET(notebook));

}

void page_init(page * ths, int * argc, char *** argv) {
	printf("call %s\n", __FUNCTION__);
	gtk_init(argc, argv);
	ths->dpy = gdk_display_open(NULL);
	ths->scn = gdk_display_get_default_screen(ths->dpy);
	ths->root = gdk_screen_get_root_window(ths->scn);
	gdk_window_get_geometry(ths->root, NULL, NULL, &ths->sw, &ths->sh, NULL);
	printf("display size %d %d\n", ths->sw, ths->sh);

	wmatom[WMState] = gdk_atom_intern("WM_STATE", False);
	ths->clients = 0;

	page_init_event_hander(ths);
}

gboolean page_quit(GtkWidget * w, GdkEventButton * e, gpointer data) {
	printf("call %s\n", __FUNCTION__);
	page * ths = (page *) data;
	gtk_widget_destroy(GTK_WIDGET(ths->gtk_main_win));
	//g_main_loop_quit(ths->main_loop);
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

	//ths->gtk_main_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	//gtk_window_set_default_size(GTK_WINDOW(ths->gtk_main_win), ths->sw, ths->sh);
	//gtk_widget_show_all(ths->gtk_main_win);
	//ths->gdk_main_win = gtk_widget_get_window(ths->gtk_main_win);
	//ths->main_cursor = gdk_cursor_new(GDK_PLUS);
	//gdk_window_set_cursor(ths->gdk_main_win, ths->main_cursor);

	//ths->hpaned = gtk_hpaned_new();

	//ths->notebook1 = gtk_notebook_new();
	//ths->notebook2 = gtk_notebook_new();

	//gtk_notebook_set_group_id(GTK_NOTEBOOK(ths->notebook1), 1928374);
	//gtk_notebook_set_group_id(GTK_NOTEBOOK(ths->notebook2), 1928374);

	//gtk_paned_pack1(GTK_PANED(ths->hpaned), ths->notebook1, 0, 0);
	//gtk_paned_pack2(GTK_PANED(ths->hpaned), ths->notebook2, 0, 0);

	//g_signal_connect(GTK_OBJECT(ths->notebook1), "page-added", GTK_SIGNAL_FUNC(page_page_added), ths);
	//g_signal_connect(GTK_OBJECT(ths->notebook2), "page-added", GTK_SIGNAL_FUNC(page_page_added), ths);

	//GtkWidget * label0 = gtk_label_new("hello world 0");
	//GtkWidget * label1 = gtk_label_new("hello world 1");
	//GtkWidget * label2 = gtk_label_new("hello world 2");
	//GtkWidget * tab_label0 = build_control_tab(ths);
	//GtkWidget * tab_label1 = gtk_label_new("label 1");
	//GtkWidget * tab_label2 = gtk_label_new("label 2");

	//gtk_notebook_append_page(GTK_NOTEBOOK(ths->notebook1), label0, tab_label0);
	//gtk_notebook_append_page(GTK_NOTEBOOK(ths->notebook1), label1, tab_label1);
	//gtk_notebook_append_page(GTK_NOTEBOOK(ths->notebook2), label2, tab_label2);

	ths->t = (tree *)malloc(sizeof(tree));
	tree_root_init(ths->t, ths);

	page_scan(ths);
	gtk_widget_show_all(ths->gtk_main_win);
	gtk_widget_queue_draw(ths->gtk_main_win);
	gdk_window_set_events(ths->gdk_main_win, GDK_ALL_EVENTS_MASK);

	/* listen for new windows */
	/* there is no gdk equivalent to the folowing */
	XSelectInput(gdk_x11_display_get_xdisplay(ths->dpy),
			GDK_WINDOW_XID(ths->root), ExposureMask | SubstructureRedirectMask
					| SubstructureNotifyMask);
	/* get all unhandled event */
	gdk_window_add_filter(ths->root, page_filter_event, ths);

	//ths->main_loop = g_main_loop_new(NULL, FALSE);
	printf("start main loop\n");
	//g_main_loop_run(ths->main_loop);
	gtk_main();

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
GdkFilterReturn page_filter_event(GdkXEvent * xevent, GdkEvent * event,
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
			GdkWindow * w = gdk_window_foreign_new(wins[i]);
			GdkWindowState s = gdk_window_get_state(w);
			if (s != GDK_WINDOW_STATE_WITHDRAWN)
				page_manage(ths, w);
		}
	}

	if (wins)
		XFree(wins);
}

/* this function is call when a client want map a new window
 * the evant is on root window, e->xmaprequest.window is the window that request the map */
GdkFilterReturn page_process_destroy_notify_event(page * ths, XEvent * e) {
	printf("call %s\n", __FUNCTION__);
	GdkWindow * w = gdk_window_foreign_new(e->xdestroywindow.window);
	/* should never happen */
	client * c = page_find_client_by_gwindow(ths, w);
	if (c == NULL) {
		/* just map, do noting else */
		printf("This window (%p) is not managed\n", w);
		return GDK_FILTER_REMOVE;
	}
	gint n = gtk_notebook_page_num(c->notebook_parent,
			GTK_WIDGET(c->xwindow_handler));
	printf("remove %d of %p\n", n, c->notebook_parent);
	if (n < 0)
		return GDK_FILTER_CONTINUE;
	gtk_notebook_remove_page((c->notebook_parent), n);
	gtk_widget_queue_draw(GTK_WIDGET(c->notebook_parent));
	ths->clients = g_slist_remove(ths->clients, c);
	free(c);
	gtk_widget_queue_draw(GTK_WIDGET(ths->notebook1));
	return GDK_FILTER_REMOVE;

}

/* this function is call when a client want map a new window
 * the evant is on root window, e->xmaprequest.window is the window that request the map */
GdkFilterReturn page_process_map_request_event(page * ths, XEvent * e) {
	printf("call %s\n", __FUNCTION__);
	GdkWindow * w = gdk_window_foreign_new(e->xmaprequest.window);
	if (w == ths->gdk_main_win) {
		/* just map, do noting else */
		gdk_window_show(ths->gdk_main_win);
		gtk_widget_queue_draw(ths->gtk_main_win);
		printf("try mapping myself\n");
		return GDK_FILTER_REMOVE;
	}

	/* should never happen */
	client * c = page_find_client_by_gwindow(ths, w);
	if (c) {
		/* just map, do noting else */
		gdk_window_show(ths->gdk_main_win);
		gtk_widget_queue_draw(ths->gtk_main_win);
		printf("Already mapped\n");
		return GDK_FILTER_REMOVE;
	}

	GdkWindowState s = gdk_window_get_state(w);
	if (s != GDK_WINDOW_STATE_WITHDRAWN)
		page_manage(ths, w);

	/* manage the window but do not map it
	 * Map will be made on widget show
	 */
	page_manage(ths, w);
	return GDK_FILTER_REMOVE;

}

client * page_find_client_by_xwindow(page * ths, Window w) {
	printf("call %s\n", __FUNCTION__);
	GSList * i = ths->clients;
	while (i != NULL) {
		if (((client *) (i->data))->xwin == w)
			return (client *) i->data;
		i = i->next;
	}
	return NULL;
}

client * page_find_client_by_gwindow(page * ths, GdkWindow * w) {
	printf("call %s\n", __FUNCTION__);
	GSList * i = ths->clients;
	while (i != NULL) {
		if (((client *) (i->data))->gwin == w)
			return (client *) i->data;
		i = i->next;
	}
	return NULL;
}

client * page_find_client_by_widget(page * ths, GtkWidget * w) {
	printf("call %s\n", __FUNCTION__);
	if (!GTK_IS_XWINDOW_HANDLER(w))
		return NULL;
	GSList * i = ths->clients;
	while (i != NULL) {
		if (((client *) (i->data))->xwindow_handler == GTK_XWINDOW_HANDLER(w))
			return (client *) i->data;
		i = i->next;
	}
	return NULL;
}

GdkFilterReturn page_process_window_destroy_event(page * ths, XEvent * e) {
	printf("call %s %d\n", __FUNCTION__, (int) e->xdestroywindow.window);
	client * c = 0;
	c = page_find_client_by_xwindow(ths, e->xdestroywindow.window);
	if (c != 0) {
		gint n = gtk_notebook_page_num(c->notebook_parent,
				GTK_WIDGET(c->xwindow_handler));
		printf("remove %d of %p\n", n, c->notebook_parent);
		if (n < 0)
			return GDK_FILTER_CONTINUE;
		gtk_notebook_remove_page((c->notebook_parent), n);
		gtk_widget_queue_draw(GTK_WIDGET(c->notebook_parent));
		ths->clients = g_slist_remove(ths->clients, c);
		free(c);
	}
	//gtk_widget_queue_draw(GTK_WIDGET(ths->notebook1));
	return GDK_FILTER_REMOVE;
}

void page_init_event_hander(page * ths) {
	gint i;
	for (i = 0; i < LASTEvent; ++i) {
		ths->event_handler[i] = NULL;
	}

	ths->event_handler[DestroyNotify] = page_process_destroy_notify_event;
	ths->event_handler[MapRequest] = page_process_map_request_event;
	ths->event_handler[DestroyNotify] = page_process_window_destroy_event;
}

void page_manage(page * ths, GdkWindow * w) {
	printf("call %s on %p\n", __FUNCTION__, w);
	gint width, height, x, y, depth;
	if (w == ths->gdk_main_win) {
		printf("Do not manage myself\n");
		return;
	}

	client * c = 0;
	c = page_find_client_by_gwindow(ths, w);
	if (c != NULL) {
		printf("Window %p is already managed\n", c);
		return;
	}
	c = malloc(sizeof(client));
	c->xwin = GDK_WINDOW_XID(w);
	c->gwin = w;
	c->notebook_parent = 0;

	gdk_window_get_geometry(w, &x, &y, &width, &height, &depth);
	printf("manage %p %dx%d+%d+%d(%d)\n", w, width, height, x, y, depth);

	gchar * title = g_strdup_printf("%p", w);
	GtkWidget * label = gtk_label_new(title);
	GtkWidget * content = gtk_xwindow_handler_new();
	gtk_xwindow_handler_set_gwindow(GTK_XWINDOW_HANDLER(content), c->gwin);

	c->xwindow_handler = GTK_XWINDOW_HANDLER(content);
	/* before page prepend !! */
	ths->clients = g_slist_prepend(ths->clients, c);

	tree_append_widget(ths->t, label, content);

	/* listen for new windows */
	/* there is no gdk equivalent to the folowing */
	XSelectInput(gdk_x11_display_get_xdisplay(ths->dpy),
			GDK_WINDOW_XID(w), StructureNotifyMask | PropertyChangeMask);
	gdk_flush();
	/* will be setup in page_added event */

}
