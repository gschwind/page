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

GType gtk_wm_get_type(void) {
	static GType gtk_wm_type = 0;
	if (!gtk_wm_type) {
		const GTypeInfo gtk_wm_info = { sizeof(GtkWMClass), NULL, NULL,
				(GClassInitFunc) gtk_wm_class_init, NULL, NULL, sizeof(GtkWM),
				0, (GInstanceInitFunc) gtk_wm_init, };
		gtk_wm_type = g_type_register_static(GTK_TYPE_WIDGET, "GtkWM",
				&gtk_wm_info, 0);

	}
	return gtk_wm_type;
}

GtkWidget * gtk_wm_new(client * c) {
	GtkWM * w = g_object_new(GTK_TYPE_WM, NULL);
	gtk_wm_set_client(w, c);
	return GTK_WIDGET(w);
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
	GTK_WM(ths)->c = NULL;
	printf("return %s #%p\n", __FUNCTION__, ths);
}

void gtk_wm_set_client(GtkWM * w, client *c) {
	GTK_WM(w)->c = c;
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

		if (c->incw != 0) {
			w -= ((w - c->basew) % c->incw);
		}

		if (c->inch != 0) {
			h -= ((h - c->baseh) % c->inch);
		}

		/* TODO respect Aspect */
		c->height = h;
		c->width = w;

		printf("Update #%p window size %dx%d\n", (void *) c->xwin, c->width,
				c->height);

	}
}

static void gtk_wm_size_allocate(GtkWidget *widget, GtkAllocation *allocation) {
	fprintf(stderr, "Entering %s #%p\n", __FUNCTION__, widget);
	g_return_if_fail(widget != NULL);
	g_return_if_fail(GTK_IS_WM(widget));
	g_return_if_fail(allocation != NULL);
	widget->allocation = *allocation;
	client * c = GTK_WM(widget)->c;

	fprintf(stderr, "allocate %dx%d+%d+%d\n", allocation->width,
			allocation->height, allocation->x, allocation->y);
	XMoveResizeWindow(c->ctx->xdpy, c->clipping_window, allocation->x,
			allocation->y, allocation->width, allocation->height);
	gtk_wm_update_client_size(c, allocation->width, allocation->height);
	XMoveResizeWindow(c->ctx->xdpy, c->xwin, 0, 0, c->width, c->height);
	fprintf(stderr, "Return %s #%p\n", __FUNCTION__, widget);
}

/* undo what was done in realize */
static void gtk_wm_unrealize(GtkWidget *widget) {
	printf("Enterring %s #%p\n", __FUNCTION__, widget);
	if (gtk_widget_get_realized(widget)) {
		client * c = GTK_WM(widget)->c;
		//GTK_WIDGET_CLASS(widget)->unmap(widget);
		XReparentWindow(c->ctx->xdpy, c->clipping_window, c->ctx->xroot, 0, 0);
		gtk_widget_set_realized(widget, FALSE);
		c->content = NULL;
	}
	printf("Return %s #%p\n", __FUNCTION__, widget);
}

static void gtk_wm_realize(GtkWidget *widget) {
	printf("Entering in %s #%p\n", __FUNCTION__, widget);
	//GTK_WIDGET_CLASS(g_type_class_peek_parent(GTK_WM_GET_CLASS(widget)))->realize(widget);
	GtkWM * wm = GTK_WM(widget);
	client * c = wm->c;
	gtk_widget_set_realized(widget, TRUE);
	gtk_widget_set_has_window(widget, FALSE);
	widget->window = gtk_widget_get_parent_window(widget);

	XReparentWindow(c->ctx->xdpy, c->clipping_window,
			GDK_WINDOW_XID(widget->window), widget->allocation.x,
			widget->allocation.y);
	XMoveResizeWindow(c->ctx->xdpy, c->clipping_window, widget->allocation.x,
			widget->allocation.y, widget->allocation.width,
			widget->allocation.height);
	XSelectInput(c->ctx->xdpy, c->xwin, StructureNotifyMask
			| PropertyChangeMask);
	c->content = widget;
	printf("Return in %s #%p\n", __FUNCTION__, widget);
}

static void gtk_wm_unmap(GtkWidget *widget) {
	printf("Entering in %s #%p\n", __FUNCTION__, widget);
	/* gtk check if we are mapped to send event, noto switch this value cause
	 * nevar call map.
	 */
	gtk_widget_set_mapped(widget, FALSE);
	client * c = GTK_WM(widget)->c;
	XUnmapWindow(c->ctx->xdpy, c->xwin);
	XUnmapWindow(c->ctx->xdpy, c->clipping_window);
	printf("Return from %s #%p\n", __FUNCTION__, widget);
}

static void gtk_wm_map(GtkWidget * widget) {
	printf("Enter %s #%p\n", __FUNCTION__, widget);
	gtk_widget_set_mapped(widget, TRUE);
	client * c = GTK_WM(widget)->c;
	XMapWindow(c->ctx->xdpy, c->clipping_window);
	XMapWindow(c->ctx->xdpy, c->xwin);
	XRaiseWindow(c->ctx->xdpy, c->clipping_window);
	//XSetInputFocus(c->ctx->xdpy, c->xwin, RevertToNone, CurrentTime);
	printf("Return %s #%p\n", __FUNCTION__, widget);
}

