/*
 * gtk_xwindow_handler.cc
 *
 *  Created on: 5 sept. 2010
 *      Author: gschwind
 */

#include <gdk/gdkx.h>
#include "gtk_xwindow_handler.h"

static void gtk_xwindow_handler_class_init(GtkXWindowHandlerClass *klass);
static void gtk_xwindow_handler_init(GtkXWindowHandler *cpu);
static void gtk_xwindow_handler_size_request(GtkWidget *widget,
		GtkRequisition *requisition);
static void gtk_xwindow_handler_size_allocate(GtkWidget *widget,
		GtkAllocation *allocation);
static void gtk_xwindow_handler_realize(GtkWidget *widget);
static gboolean gtk_xwindow_handler_expose(GtkWidget *widget,
		GdkEventExpose *event);
static gboolean gtk_xwindow_handler_no_expose(GtkWidget *widget,
		GdkEventAny * event);

static gboolean gtk_xwindow_handler_map_event(GtkWidget *widget,
		GdkEventAny * event);
static gboolean gtk_xwindow_handler_configure_event(GtkWidget *widget,
		GdkEventConfigure * event);
static gboolean gtk_xwindow_handler_unmap_event(GtkWidget *widget,
		GdkEventAny * event);
static void gtk_xwindow_handler_map(GtkWidget *widget);
static void gtk_xwindow_handler_unmap(GtkWidget *widget);
static void gtk_xwindow_handler_paint(GtkWidget *widget);
static void gtk_xwindow_handler_destroy(GtkObject *object);

GtkType gtk_xwindow_handler_get_type(void) {
	static GtkType gtk_xwindow_handler_type = 0;
	if (!gtk_xwindow_handler_type) {
		static const GtkTypeInfo gtk_xwindow_handler_info = { g_strdup(
				"GtkXWindowHandler"), sizeof(GtkXWindowHandler),
				sizeof(GtkXWindowHandlerClass),
				(GtkClassInitFunc) gtk_xwindow_handler_class_init,
				(GtkObjectInitFunc) gtk_xwindow_handler_init, NULL, NULL,
				(GtkClassInitFunc) NULL };
		gtk_xwindow_handler_type = gtk_type_unique(GTK_TYPE_WIDGET,
				&gtk_xwindow_handler_info);
	}
	return gtk_xwindow_handler_type;
}

void gtk_xwindow_handler_set_gwindow(GtkXWindowHandler * xwindow_handler,
		GdkWindow * w) {
	xwindow_handler->gwindow = w;
	gdk_window_hide(xwindow_handler->gwindow);
}

GtkWidget * gtk_xwindow_handler_new() {
	return GTK_WIDGET(gtk_type_new(gtk_xwindow_handler_get_type()));
}

static void gtk_xwindow_handler_class_init(GtkXWindowHandlerClass *klass) {
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;

	widget_class = (GtkWidgetClass *) klass;
	object_class = (GtkObjectClass *) klass;

	widget_class->realize = gtk_xwindow_handler_realize;
	widget_class->size_allocate = gtk_xwindow_handler_size_allocate;
	widget_class->expose_event = gtk_xwindow_handler_expose;
	widget_class->unmap_event = gtk_xwindow_handler_unmap_event;
	widget_class->map_event = gtk_xwindow_handler_map_event;
	widget_class->configure_event = gtk_xwindow_handler_configure_event;
}

static void gtk_xwindow_handler_init(GtkXWindowHandler * xwindow_handler) {
	printf("call %s\n", __FUNCTION__);
	gtk_widget_set_has_window(GTK_WIDGET (xwindow_handler), TRUE);
	xwindow_handler->gwindow = NULL;
}

static void gtk_xwindow_handler_size_allocate(GtkWidget *widget,
		GtkAllocation *allocation) {
	printf("call %s\n", __FUNCTION__);
	g_return_if_fail(widget != NULL);
	g_return_if_fail(GTK_IS_XWINDOW_HANDLER(widget));
	g_return_if_fail(allocation != NULL);

	widget->allocation = *allocation;

	printf("allocation %dx%d+%d+%d\n", allocation->width, allocation->height,
			allocation->x, allocation->y);
	if (GTK_WIDGET_REALIZED(widget)) {
		gdk_window_move_resize(widget->window, allocation->x, allocation->y,
				allocation->width, allocation->height);
		gdk_window_resize(GTK_XWINDOW_HANDLER(widget)->gwindow,
				widget->allocation.width, widget->allocation.height);
	}
}

static void gtk_xwindow_handler_realize(GtkWidget *widget) {
	printf("call %s\n", __FUNCTION__);
	GdkWindowAttr attributes;
	guint attributes_mask;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(GTK_IS_XWINDOW_HANDLER(widget));

	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;

	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.event_mask = gtk_widget_get_events(widget) | GDK_ALL_EVENTS_MASK;

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
	gtk_style_set_background(widget->style, widget->window, GTK_STATE_NORMAL);

	g_return_if_fail(GDK_IS_WINDOW(widget->window));
	g_return_if_fail(GDK_IS_WINDOW(GTK_XWINDOW_HANDLER(widget)->gwindow));

	gdk_window_reparent(GTK_XWINDOW_HANDLER(widget)->gwindow, widget->window,
			0, 0);
}

static gboolean gtk_xwindow_handler_expose(GtkWidget *widget,
		GdkEventExpose *event) {
	printf("call %s\n", __FUNCTION__);
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(GTK_IS_XWINDOW_HANDLER(widget), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	/* call parent handler */
	return FALSE;
}

static gboolean gtk_xwindow_handler_configure_event(GtkWidget *widget,
		GdkEventConfigure * event) {
	printf("call %s\n", __FUNCTION__);
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(GTK_IS_XWINDOW_HANDLER(widget), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	gdk_window_move_resize(widget->window, event->x, event->y, event->width,
			event->height);
	gdk_window_resize(GTK_XWINDOW_HANDLER(widget)->gwindow,
			event->width, event->height);
	/* event done */
	return TRUE;
}

static gboolean gtk_xwindow_handler_unmap_event(GtkWidget *widget,
		GdkEventAny *event) {
	printf("call %s\n", __FUNCTION__);
	gdk_window_hide(GTK_XWINDOW_HANDLER(widget)->gwindow);
	/* the event is done */
	return TRUE;
}

static gboolean gtk_xwindow_handler_map_event(GtkWidget *widget,
		GdkEventAny *event) {
	printf("call %s\n", __FUNCTION__);
	gdk_window_show(GTK_XWINDOW_HANDLER(widget)->gwindow);
	/* the event is done */
	return TRUE;
}

static void gtk_xwindow_handler_destroy(GtkObject *object) {
	printf("call %s\n", __FUNCTION__);
	GtkXWindowHandler * xwindow_handler;
	GtkXWindowHandlerClass * klass;

	g_return_if_fail(object != NULL);
	g_return_if_fail(GTK_IS_XWINDOW_HANDLER(object));

	xwindow_handler = GTK_XWINDOW_HANDLER(object);

	klass = GTK_XWINDOW_HANDLER_CLASS(gtk_type_class(gtk_widget_get_type()));

	if (GTK_OBJECT_CLASS(klass)->destroy) {
		(*GTK_OBJECT_CLASS(klass)->destroy)(object);
	}
}
