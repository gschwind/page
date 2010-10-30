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

#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include "page.h"
#include "client.h"
#include "tree.h"
#include "gtk_wm.h"

client * page_find_client_by_widget(page * ths, GtkWidget * w);
client * page_find_client_by_gwindow(page * ths, GdkWindow * w);

long page_get_window_state(page * ths, Window w) {
	printf("call %s\n", __FUNCTION__);
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(ths->xdpy, w, ths->wmatom[WMState], 0L, 2L, False,
			ths->wmatom[WMState], &real, &format, &n, &extra,
			(unsigned char **) &p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

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
	if (GTK_IS_XWINDOW_HANDLER(child)) {
		client * c = gtk_xwindow_handler_get_client(GTK_XWINDOW_HANDLER(child));
		if (c == NULL)
			return;
		c->notebook_parent = notebook;
		printf("page %p added to %p\n", child, notebook);
		gtk_widget_queue_draw(GTK_WIDGET(notebook));
	}

}

/* inspired from dwm */
gboolean page_get_text_prop(page * ths, Window w, Atom atom, gchar ** text) {
	char **list = NULL;
	int n;
	XTextProperty name;
	if (!text)
		return False;
	XGetTextProperty(gdk_x11_display_get_xdisplay(ths->dpy), w, &name, atom);
	if (!name.nitems)
		return False;
	if (name.encoding == XA_STRING) {
		*text = g_strdup((gchar const *) name.value);
	} else {
		if (XmbTextPropertyToTextList(gdk_x11_display_get_xdisplay(ths->dpy),
				&name, &list, &n) == Success) {
			if (n > 0) {
				if (list[0]) {
					*text = g_strdup(list[0]);
				}
			}
			XFreeStringList(list);
		}
	}
	XFree(name.value);
	return True;
}

/* inspired from dwm */
void page_update_title(page * ths, client * c) {
	if (!page_get_text_prop(ths, c->xwin, ths->netatom[NetWMName], &c->name))
		if (!page_get_text_prop(ths, c->xwin, XA_WM_NAME, &c->name)) {
			c->name = g_strdup_printf("%p (noname)", (gpointer) c->xwin);
		}
	if (c->name[0] == '\0') { /* hack to mark broken clients */
		g_free(c->name);
		c->name = g_strdup_printf("%p (broken)", (gpointer) c->xwin);
	}
}

void page_update_size_hints(page * ths, client * c) {
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(gdk_x11_display_get_xdisplay(ths->dpy), c->xwin,
			&size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;

	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else {
		c->basew = 0;
		c->baseh = 0;
	}

	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else {
		c->incw = 0;
		c->inch = 0;
	}

	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else {
		c->maxw = 0;
		c->maxh = 0;
	}

	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else {
		c->minw = 0;
		c->minh = 0;
	}

	if (size.flags & PAspect) {
		if (size.min_aspect.x != 0 && size.max_aspect.y != 0) {
			c->mina = (gdouble) size.min_aspect.y / (gdouble) size.min_aspect.x;
			c->maxa = (gdouble) size.max_aspect.x / (gdouble) size.max_aspect.y;
		}
	} else {
		c->maxa = 0.0;
		c->mina = 0.0;
	}
	c->is_fixed_size = (c->maxw && c->minw && c->maxh && c->minh && c->maxw
			== c->minw && c->maxh == c->minh);
}

void page_init(page * ths, int * argc, char *** argv) {
	printf("call %s\n", __FUNCTION__);
	gtk_init(argc, argv);
	ths->dpy = gdk_display_open(NULL);
	ths->scn = gdk_display_get_default_screen(ths->dpy);
	ths->root = gdk_screen_get_root_window(ths->scn);
	ths->xdpy = gdk_x11_display_get_xdisplay(ths->dpy);
	ths->xroot = GDK_WINDOW_XID(ths->root);
	gdk_window_get_geometry(ths->root, NULL, NULL, &ths->sw, &ths->sh, NULL);
	printf("display size %d %d\n", ths->sw, ths->sh);

	ths->wmatom[WMState] = XInternAtom((ths->xdpy), "WM_STATE", False);

	ths->netatom[NetSupported] = XInternAtom((ths->xdpy), "_NET_SUPPORTED",
			False);
	ths->netatom[NetWMName] = XInternAtom((ths->xdpy), "_NET_WM_NAME", False);
	ths->netatom[NetWMState] = XInternAtom((ths->xdpy), "_NET_WM_STATE", False);
	ths->netatom[NetWMFullscreen] = XInternAtom((ths->xdpy),
			"_NET_WM_STATE_FULLSCREEN", False);

	ths->clients = NULL;

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

	ths->t = (tree *) malloc(sizeof(tree));
	tree_root_init(ths->t, ths);
	gtk_widget_show_all(ths->gtk_main_win);
	gtk_widget_queue_draw(ths->gtk_main_win);
	gdk_window_set_events(ths->gdk_main_win, GDK_ALL_EVENTS_MASK);
	fprintf(stderr, "Create %p\n", (void *) GDK_WINDOW_XID(ths->gdk_main_win));

	page_scan(ths);

	/* listen for new windows */
	/* there is no gdk equivalent to the folowing */
	XSelectInput(gdk_x11_display_get_xdisplay(ths->dpy), GDK_WINDOW_XID(
			ths->root), ExposureMask | SubstructureRedirectMask
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
	if (e->type == MapRequest)
		return ths->event_handler[e->type](ths, e);
	else {
		client * c = NULL;
		c = page_find_client_by_xwindow(ths, e->xany.window);
		if (!c)
			return GDK_FILTER_CONTINUE;
		fprintf(stderr, "process event %d %d : %s\n", (int) e->xany.window,
				(int) e->xany.serial, event_name[e->type]);
		if (ths->event_handler[e->type])
			return ths->event_handler[e->type](ths, e); /* call handler */
		return GDK_FILTER_CONTINUE;
	}
}

void page_scan(page * ths) {
	printf("call %s\n", __FUNCTION__);
	unsigned int i, num;
	Window d1, d2, *wins = 0;
	XWindowAttributes wa;

	/* ask for child of current root window, use Xlib here since gdk only know windows it
	 * have created. */
	if (XQueryTree(ths->xdpy, ths->xroot, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; ++i) {
			if (!XGetWindowAttributes(ths->xdpy, wins[i], &wa))
				continue;
			if (wa.override_redirect)
				continue;
			if ((page_get_window_state(ths, wins[i]) == IconicState
					|| wa.map_state == IsViewable))
				page_manage(ths, wins[i]);
		}

		if (wins)
			XFree(wins);
	}
}

/* this function is call when a client want map a new window
 * the evant is on root window, e->xmaprequest.window is the window that request the map */
#if 0
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
#endif

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

/* this function is call when a client want map a new window
 * the event is on root window, e->xmaprequest.window is the window that request the map */
GdkFilterReturn page_process_map_request_event(page * ths, XEvent * e) {
	printf("Entering in %s #%p\n", __FUNCTION__, (void *) e->xmaprequest.window);
	Window w = e->xmaprequest.window;
	/* should never happen */

	static XWindowAttributes wa;
	if (!XGetWindowAttributes(ths->xdpy, w, &wa))
		return GDK_FILTER_REMOVE;
	if (wa.override_redirect)
		return GDK_FILTER_REMOVE;
	page_manage(ths, w);

	exit: printf("Return from %s #%p\n", __FUNCTION__,
			(void *) e->xmaprequest.window);
	return GDK_FILTER_REMOVE;

}

#if 0
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
#endif

void page_init_event_hander(page * ths) {
	gint i;
	for (i = 0; i < LASTEvent; ++i) {
		ths->event_handler[i] = NULL;
	}

	ths->event_handler[MapRequest] = page_process_map_request_event;
	ths->event_handler[ConfigureRequest] = page_process_configure_request_event;
	ths->event_handler[CreateNotify] = page_process_create_notify_event;
}

void page_manage(page * ths, Window w) {
	fprintf(stderr, "Call %s on %p\n", __FUNCTION__, (void *) w);
	if (GDK_WINDOW_XID(ths->gdk_main_win) == w)
		return;

	client * c = 0;
	c = page_find_client_by_xwindow(ths, w);
	if (c != NULL) {
		printf("Window %p is already managed\n", c);
		return;
	}
	c = malloc(sizeof(client));
	c->xwin = w;
	c->root = ths->xroot;
	c->dpy = ths->xdpy;
	c->notebook_parent = 0;
	c->unmap_pending = 0;
	/* before page prepend !! */
	ths->clients = g_slist_prepend(ths->clients, c);

	page_update_title(ths, c);
	page_update_size_hints(ths, c);

	GtkWidget * label = gtk_label_new(c->name);
	GtkWidget * content = gtk_wm_new();
	gtk_wm_set_client(GTK_WM(content), c);
	tree_append_widget(ths->t, label, content);

	/* this window will not be destroyed on page close (one bug less)
	 * TODO check gdk equivalent */
	XAddToSaveSet(c->dpy, w);
	/* listen for new windows */
	/* there is no gdk equivalent to the folowing */
	XSelectInput(c->dpy, w, StructureNotifyMask | PropertyChangeMask);
	XFlush(c->dpy);
	fprintf(stderr, "Return %s on %p\n", __FUNCTION__, (void *) w);
}

GdkFilterReturn page_process_create_notify_event(page * ths, XEvent * ev) {
	fprintf(stderr, "Entering in %s on %p\n", __FUNCTION__,
			(void *) ev->xcreatewindow.window);

	fprintf(stderr, "Return from %s on %p\n", __FUNCTION__,
			(void *) ev->xcreatewindow.window);
	return GDK_FILTER_CONTINUE;
}

GdkFilterReturn page_process_configure_request_event(page * ths, XEvent * ev) {
	printf("Entering in %s on %p\n", __FUNCTION__, (void *) ev);
	unsigned long int mask = ev->xconfigurerequest.value_mask;
	client * c = page_find_client_by_xwindow(ths, ev->xconfigurerequest.window);
	if (c)
		return GDK_FILTER_REMOVE;

	/* no notebook mean not handled by widget, size is free */
	XWindowChanges wc;
	wc.x = ev->xconfigurerequest.x;
	wc.y = ev->xconfigurerequest.y;
	wc.width = ev->xconfigurerequest.width;
	wc.height = ev->xconfigurerequest.height;
	wc.border_width = ev->xconfigurerequest.border_width;
	wc.sibling = ev->xconfigurerequest.above;
	wc.stack_mode = ev->xconfigurerequest.detail;
	XConfigureWindow(ths->xdpy, ev->xconfigurerequest.window,
			ev->xconfigurerequest.value_mask, &wc);
	return GDK_FILTER_REMOVE;
}

GdkFilterReturn page_process_unmap_notify_event(page * ths, XEvent * ev) {
	printf("Entering in %s on %p\n", __FUNCTION__, (void *) ev);
	client * c = page_find_client_by_xwindow(ths, ev->xconfigurerequest.window);
	if (!c)
		return GDK_FILTER_CONTINUE;
	c->unmap_pending -= 1;
	if (c->unmap_pending == 0) {
		/* TODO remove notebook */
	}

	return GDK_FILTER_REMOVE;
}

