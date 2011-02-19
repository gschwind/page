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

char const * x_event_name[LASTEvent] = { 0, 0, "KeyPress", "KeyRelease",
		"ButtonPress", "ButtonRelease", "MotionNotify", "EnterNotify",
		"LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose",
		"GraphicsExpose", "NoExpose", "VisibilityNotify", "CreateNotify",
		"DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
		"ReparentNotify", "ConfigureNotify", "ConfigureRequest",
		"GravityNotify", "ResizeRequest", "CirculateNotify",
		"CirculateRequest", "PropertyNotify", "SelectionClear",
		"SelectionRequest", "SelectionNotify", "ColormapNotify",
		"ClientMessage", "MappingNotify", "GenericEvent" };

client * page_find_client_by_xwindow(page * ths, Window w);

long page_get_window_state(page * ths, Window w) {
	printf("call %s\n", __FUNCTION__);
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(ths->xdpy, w, ths->atoms.WM_STATE, 0L, 2L, False,
			ths->atoms.WM_STATE, &real, &format, &n, &extra,
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

static gboolean page_get_all(page * ths, Window win, Atom prop, Atom type,
		gint size, guchar **data, guint *num) {
	gboolean ret = FALSE;
	gint res;
	guchar *xdata = NULL;
	Atom ret_type;
	gint ret_size;
	gulong ret_items, bytes_left;

	res = XGetWindowProperty(ths->xdpy, win, prop, 0l, G_MAXLONG, FALSE, type,
			&ret_type, &ret_size, &ret_items, &bytes_left, &xdata);
	if (res == Success) {
		if (ret_size == size && ret_items > 0) {
			guint i;

			*data = g_malloc(ret_items * (size / 8));
			for (i = 0; i < ret_items; ++i)
				switch (size) {
				case 8:
					(*data)[i] = xdata[i];
					break;
				case 16:
					((guint16*) *data)[i] = ((gushort*) xdata)[i];
					break;
				case 32:
					((guint32*) *data)[i] = ((gulong*) xdata)[i];
					break;
				default:
					g_assert_not_reached(); /* unhandled size */
				}
			*num = ret_items;
			ret = TRUE;
		}
		XFree(xdata);
	}
	return ret;
}

gboolean client_is_dock(page * ths, client * c) {
	guint num, i;
	guint32 *val;
	Window t;

	if (page_get_all(ths, c->xwin, ths->atoms._NET_WM_WINDOW_TYPE,
			ths->atoms.ATOM, 32, (guchar **) &val, &num)) {
		/* use the first value that we know about in the array */
		for (i = 0; i < num; ++i) {
			if (val[i] == ths->atoms._NET_WM_WINDOW_TYPE_DOCK)
				return True;
		}
		g_free(val);
	}

	return False;
}

/* inspired from dwm */
gboolean page_get_text_prop(page * ths, Window w, Atom atom, gchar ** text) {
	char **list = NULL;
	int n;
	XTextProperty name;
	if (!text)
		return False;
	XGetTextProperty(ths->xdpy, w, &name, atom);
	if (!name.nitems)
		return False;
	if (name.encoding == XA_STRING) {
		*text = g_strdup((gchar const *) name.value);
	} else {
		if (XmbTextPropertyToTextList(ths->xdpy, &name, &list, &n) == Success) {
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
	if (!page_get_text_prop(ths, c->xwin, ths->atoms._NET_WM_NAME, &c->name))
		if (!page_get_text_prop(ths, c->xwin, XA_WM_NAME, &c->name)) {
			c->name = g_strdup_printf("%p (noname)", (gpointer) c->xwin);
		}
	if (c->name[0] == '\0') { /* hack to mark broken clients */
		g_free(c->name);
		c->name = g_strdup_printf("%p (broken)", (gpointer) c->xwin);
	}
}

void page_init(page * ths, int * argc, char *** argv) {
	printf("Entering in %s\n", __FUNCTION__);
	XWindowAttributes wa;
	gtk_init(argc, argv);
	/* Youhou totaly undocumented ? found in metacity sources */
	ths->xdpy = gdk_display;
	ths->xscr = XDefaultScreen(ths->xdpy);
	ths->xroot = XDefaultRootWindow(ths->xdpy);
	XGetWindowAttributes(ths->xdpy, ths->xroot, &wa);
	fprintf(stderr, "display size %d %d\n", wa.width, wa.height);
	ths->sw = wa.width;
	ths->sh = wa.height;
	ths->sx = 0;
	ths->sy = 0;

#define ATOM_INIT(name) ths->atoms.name = XInternAtom(ths->xdpy, #name, False)

	ATOM_INIT(ATOM);
	ATOM_INIT(CARDINAL);

	ATOM_INIT(WM_STATE);

	ATOM_INIT(_NET_SUPPORTED);
	ATOM_INIT(_NET_WM_NAME);
	ATOM_INIT(_NET_WM_STATE);
	ATOM_INIT(_NET_WM_STATE_FULLSCREEN);

	ATOM_INIT(_NET_WM_WINDOW_TYPE);
	ATOM_INIT(_NET_WM_WINDOW_TYPE_DOCK);

	ATOM_INIT(_NET_WM_STRUT_PARTIAL);

	ths->clients = NULL;

	page_init_event_hander(ths);
}

gboolean page_quit(GtkWidget * w, GdkEventButton * e, gpointer data) {
	printf("call %s\n", __FUNCTION__);
	page * ths = (page *) data;
	gtk_widget_destroy(GTK_WIDGET(ths->gtk_main_win));
	//g_main_loop_quit(ths->main_loop);
}

void page_run(page * ths) {
	gint x, y, width, height, depth;

	ths->t = (tree *) malloc(sizeof(tree));
	tree_root_init(ths->t, ths);
	gtk_widget_show_all(GTK_WIDGET(ths->gtk_main_win));
	gtk_widget_queue_draw(GTK_WIDGET(ths->gtk_main_win));
	ths->x_main_window
			= GDK_WINDOW_XID(gtk_widget_get_window(gtk_widget_get_toplevel(GTK_WIDGET(ths->gtk_main_win))));

	page_scan(ths);

	/* listen for new windows */
	/* there is no gdk equivalent to the folowing */
	XSelectInput(ths->xdpy, ths->xroot, ExposureMask | SubstructureRedirectMask
			| SubstructureNotifyMask);
	/* get all unhandled event */
	gdk_window_add_filter(NULL, page_filter_event, ths);

	//ths->main_loop = g_main_loop_new(NULL, FALSE);
	printf("start main loop\n");
	//g_main_loop_run(ths->main_loop);
	gtk_main();

}

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
				(int) e->xany.serial, x_event_name[e->type]);
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
				page_manage(ths, wins[i], &wa);
		}

		if (wins)
			XFree(wins);
	}
}

client * page_find_client_by_xwindow(page * ths, Window w) {
	GSList * i = ths->clients;
	while (i != NULL) {
		if (((client *) (i->data))->xwin == w)
			return (client *) i->data;
		i = i->next;
	}
	return NULL;
}

client * page_find_client_by_clipping_window(page * ths, Window w) {
	GSList * i = ths->clients;
	while (i != NULL) {
		if (((client *) (i->data))->clipping_window == w)
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
	page_manage(ths, w, &wa);

	exit: printf("Return from %s #%p\n", __FUNCTION__,
			(void *) e->xmaprequest.window);
	return GDK_FILTER_REMOVE;

}

void page_init_event_hander(page * ths) {
	gint i;
	for (i = 0; i < LASTEvent; ++i) {
		ths->event_handler[i] = NULL;
	}

	ths->event_handler[MapRequest] = page_process_map_request_event;
	ths->event_handler[DestroyNotify] = page_process_destroy_notify_event;
	ths->event_handler[ConfigureRequest] = page_process_configure_request_event;
	ths->event_handler[CreateNotify] = page_process_create_notify_event;
	ths->event_handler[PropertyNotify] = page_process_property_notify_event;
}

void page_manage(page * ths, Window w, XWindowAttributes * wa) {
	fprintf(stderr, "Call %s on %p\n", __FUNCTION__, (void *) w);
	if (ths->x_main_window == w)
		return;

	client * c = 0;
	c = page_find_client_by_xwindow(ths, w);
	if (c != NULL) {
		printf("Window %p is already managed\n", c);
		return;
	}

	if (page_find_client_by_clipping_window(ths, w))
		return;

	c = malloc(sizeof(client));
	c->xwin = w;
	c->ctx = ths;
	/* before page prepend !! */
	ths->clients = g_slist_prepend(ths->clients, c);

	page_update_title(ths, c);
	client_update_size_hints(c);

	gchar * type;

	if (client_is_dock(ths, c)) {
		printf("IsDock !\n");
		/* partial struct need to be read ! */
		gint32 * partial_struct;
		guint n;
		page_get_all(ths, c->xwin, ths->atoms._NET_WM_STRUT_PARTIAL,
				ths->atoms.CARDINAL, 32, (guchar **) &partial_struct, &n);

		ths->sw -= partial_struct[0] + partial_struct[1];
		ths->sh -= partial_struct[2] + partial_struct[3];
		if(ths->sx < partial_struct[0])
			ths->sx = partial_struct[0];
		if(ths->sy < partial_struct[2])
			ths->sy = partial_struct[2];

		gtk_window_move(GTK_WINDOW(ths->gtk_main_win), ths->sx, ths->sy);
		gtk_window_resize(GTK_WINDOW(ths->gtk_main_win), ths->sw, ths->sh);

		return;
	}

	/* this window will not be destroyed on page close (one bug less)
	 * TODO check gdk equivalent */
	XAddToSaveSet(ths->xdpy, w);

	//gdk_window_foreign_new(c->xwin);
	c->clipping_window = XCreateSimpleWindow(ths->xdpy, ths->xroot, 0, 0, 1, 1,
			0, XBlackPixel(ths->xdpy, 0), XWhitePixel(ths->xdpy, 0));
	XReparentWindow(c->ctx->xdpy, c->xwin, c->clipping_window, 0, 0);
	//gdk_window_foreign_new(c->clipping_window);
	GtkWidget * label = gtk_label_new(c->name);
	GtkWidget * content = gtk_wm_new(c);
	tree_append_widget(ths->t, label, content);

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

	return GDK_FILTER_CONTINUE;
}

GdkFilterReturn page_process_property_notify_event(page * ths, XEvent * ev) {
	printf("Entering in %s on %p\n", __FUNCTION__, (void *) ev);
	char * name = XGetAtomName(ths->xdpy, ev->xproperty.atom);
	printf("Atom Property Name %s\n", name);
	if(strcmp(name, "_NET_WM_USER_TIME") == 0) {
		XRaiseWindow(ths->xdpy, ev->xproperty.window);
		XSetInputFocus(ths->xdpy, ev->xproperty.window, RevertToNone, CurrentTime);
	}
	XFree(name);
	return GDK_FILTER_CONTINUE;
}

GdkFilterReturn page_process_destroy_notify_event(page * ths, XEvent * ev) {
	printf("Entering in %s on %p\n", __FUNCTION__, (void *) ev);
	client * c = page_find_client_by_xwindow(ths, ev->xunmap.window);
	if (!c)
		return GDK_FILTER_CONTINUE;

	if (c->content) {
		tree_remove_widget(ths->t, c->content);
		ths->clients = g_slist_remove(ths->clients, c);
		XDestroyWindow(ths->xdpy, c->clipping_window);
		g_free(c->name);
		free(c);
	}

	return GDK_FILTER_REMOVE;
}

