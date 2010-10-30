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

#include <gdk/gdkx.h>

#include "gtk_wm.h"

static void gtk_wm_class_init(GtkWMClass *klass);
static void gtk_wm_init(GtkWM *);

static void gtk_wm_unrealize(GtkWidget *widget);
static void gtk_wm_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void gtk_wm_realize(GtkWidget *widget);
static void gtk_wm_unmap(GtkWidget *widget);
static void gtk_wm_map(GtkWidget * widget);

GtkType gtk_wm_get_type(void) {
	static GtkType gtk_wm_type = 0;
	if (!gtk_wm_type) {
		static const GtkTypeInfo gtk_wm_info = { "GtkWM", sizeof(GtkWM),
				sizeof(GtkWMClass), (GtkClassInitFunc) gtk_wm_class_init,
				(GtkObjectInitFunc) gtk_wm_init, NULL, NULL,
				(GtkClassInitFunc) NULL };
		gtk_wm_type = gtk_type_unique(GTK_TYPE_WIDGET, &gtk_wm_info);
	}
	return gtk_wm_type;
}

GtkWidget * gtk_wm_new() {
	return GTK_WIDGET(gtk_type_new(gtk_wm_get_type()));
}

static void gtk_wm_class_init(GtkWMClass *klass) {
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;

	widget_class = (GtkWidgetClass *) klass;
	object_class = (GtkObjectClass *) klass;

	widget_class->realize = gtk_wm_realize;
	widget_class->unrealize = gtk_wm_unrealize;
	widget_class->size_allocate = gtk_wm_size_allocate;
	widget_class->unmap = gtk_wm_unmap;
	widget_class->map = gtk_wm_map;

}

static void gtk_wm_init(GtkWM * ths) {
	printf("call %s #%p\n", __FUNCTION__, ths);
	g_return_if_fail(ths != NULL);
	g_return_if_fail(GTK_IS_WM(ths));

	GTK_WM(ths)->need_resize = FALSE;
	GTK_WM(ths)->c = NULL;
}

void gtk_wm_set_client(GtkWidget * w, client *c) {
	g_return_if_fail(GTK_IS_WM(w));
	GTK_WM(w)->c = c;
	GTK_WM(w)->need_reparent = TRUE;
}

void gtk_wm_update_client_size(client * c, gint w, gint h) {
	if (c) {
		if (c->maxw != 0 && w > c->maxw) {
			w = c->maxw;
		}

		if (c->maxh != 0 && h > c->maxh) {
			h = c->maxh;
		}

		if (c->minw != 0 && w < c->minw) {
			w = c->minw;
		}

		if (c->minh != 0 && h < c->minh) {
			h = c->minh;
		}

		if (c->basew != 0) {
			w -= ((w - c->basew) % c->incw);
		}

		if (c->baseh != 0) {
			h -= ((h - c->baseh) % c->inch);
		}

		/* TODO respect Aspect */
		c->height = h;
		c->width = w;

		printf("Update #%p window size %dx%d\n", c->xwin, c->width, c->height);

	}
}

static void gtk_wm_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
	fprintf(stderr, "Entering %s #%p\n", __FUNCTION__, widget);
	g_return_if_fail(widget != NULL);
	g_return_if_fail(GTK_IS_WM(widget));
	g_return_if_fail(allocation != NULL);

	widget->allocation = *allocation;
	if (gtk_widget_get_has_window(widget)) {
		fprintf(stderr, "allocate %dx%d+%d+%d\n", allocation->width,
				allocation->height, allocation->x, allocation->y);
		gdk_window_move_resize(widget->window, allocation->x, allocation->y,
				allocation->width, allocation->height);
		client * c = GTK_WM(widget)->c;
		if (c) {
			gtk_wm_update_client_size(c, allocation->width, allocation->height);
			XMoveResizeWindow(c->dpy, c->xwin, allocation->x, allocation->y, c->width, c->height);
		}
	} else {
		GTK_WM(widget)->need_resize = TRUE;
	}
	fprintf(stderr, "Return %s #%p\n", __FUNCTION__, widget);
}

/* undo what was done in realize */
static void gtk_wm_unrealize(GtkWidget *widget) {
	printf("Enterring %s #%p\n", __FUNCTION__, widget);
	if (gtk_widget_get_realized(widget)) {

		if (gtk_widget_get_mapped(widget)) {
			GTK_WIDGET_GET_CLASS(widget)->unmap(widget);
		}

		client * c = GTK_WM(widget)->c;
		if (!GTK_WM(widget)->need_reparent && c) {
			XReparentWindow(c->dpy, c->xwin, c->root, 0, 0);
		}

		gdk_window_destroy(widget->window);
		GTK_WM(widget)->need_reparent = FALSE;
		gtk_widget_set_realized(widget, FALSE);
		gtk_widget_set_has_window(widget, FALSE);

	}
	printf("Return %s #%p\n", __FUNCTION__, widget);
}

static void gtk_wm_realize(GtkWidget *widget) {
	printf("Entering in %s #%p\n", __FUNCTION__, widget);
	g_return_if_fail(widget != NULL);
	g_return_if_fail(GTK_IS_WM(widget));

	if (!gtk_widget_get_realized(widget)) {
		gtk_widget_set_realized(widget, TRUE);
		gtk_widget_set_has_window(widget, TRUE);
		GTK_WM(widget)->need_resize = FALSE;
		GdkWindowAttr attributes;
		guint attributes_mask;

		if (GTK_WM(widget)->c)
			GTK_WM(widget)->need_reparent = TRUE;

		attributes.window_type = GDK_WINDOW_CHILD;
		attributes.x = widget->allocation.x;
		attributes.y = widget->allocation.y;
		attributes.width = widget->allocation.width;
		attributes.height = widget->allocation.height;
		attributes.wclass = GDK_INPUT_OUTPUT;
		attributes.event_mask = gtk_widget_get_events(widget)
				| GDK_ALL_EVENTS_MASK;
		attributes_mask = GDK_WA_X | GDK_WA_Y;

		widget->window = gdk_window_new(gtk_widget_get_parent_window(widget),
				&attributes, attributes_mask);
		/* X window seems to not be created on gdk_window_new
		 * Here we force the creation of this window to allow
		 * reparrent. This behaviour is versatil :/ (more than 4 hours to find the
		 * issue and solve it).
		 * there is no probleme with the child window since it is on the
		 *  way to be managed
		 */
		g_return_if_fail(gdk_window_ensure_native(widget->window));

		/* Not clear */
		gdk_window_set_user_data(widget->window, widget);

		widget->style = gtk_style_attach(widget->style, widget->window);
		gtk_style_set_background(widget->style, widget->window,
				GTK_STATE_PRELIGHT);

	}
	printf("Return in %s #%p\n", __FUNCTION__, widget);
}

static void gtk_wm_unmap(GtkWidget *widget) {
	printf("Entering in %s #%p\n", __FUNCTION__, widget);
	client * c = GTK_WM(widget)->c;
	if (gtk_widget_get_mapped(widget) && gtk_widget_get_has_window(widget)) {
		if (c)
			XUnmapWindow(c->dpy, c->xwin);
		gtk_widget_set_mapped(widget, FALSE);
		gdk_window_hide(widget->window);
	}
	printf("Return from %s #%p\n", __FUNCTION__, widget);
}

static void gtk_wm_map(GtkWidget * widget) {
	printf("Enter %s #%p\n", __FUNCTION__, widget);
	client * c = GTK_WM(widget)->c;
	g_assert (gtk_widget_get_realized (widget));
	if (!gtk_widget_get_mapped(widget) && gtk_widget_get_has_window(widget)) {
		if (GTK_WM(widget)->need_resize) {
			fprintf(stderr, "allocate %dx%d+%d+%d\n", widget->allocation.width,
					widget->allocation.height, widget->allocation.x,
					widget->allocation.y);
			gdk_window_move_resize(widget->window, widget->allocation.x,
					widget->allocation.y, widget->allocation.width,
					widget->allocation.height);

			if (c) {
				gtk_wm_update_client_size(c, widget->allocation.width,
						widget->allocation.height);
				XMoveResizeWindow(c->dpy, c->xwin, widget->allocation.x, widget->allocation.y, c->width, c->height);
				if (GTK_WM(widget)->need_reparent) {
					XReparentWindow(c->dpy, c->xwin,
							GDK_WINDOW_XID(gtk_widget_get_window(widget)),
							widget->allocation.x, widget->allocation.y);
					GTK_WM(widget)->need_reparent = FALSE;
				}
			}
			GTK_WM(widget)->need_resize = FALSE;
		}

		if (c)
			XMapWindow(c->dpy, c->xwin);
		gtk_widget_set_mapped(widget, TRUE);
		gdk_window_show(widget->window);
	}
	printf("Return %s #%p\n", __FUNCTION__, widget);
}

