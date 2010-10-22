/*
 * gtk_xwindow_handler.h
 *
 *  Created on: 5 sept. 2010
 *      Author: gschwind
 */

#ifndef GTK_XWINDOW_HANDLER_H_
#define GTK_XWINDOW_HANDLER_H_

#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include "client.h"

G_BEGIN_DECLS

#define GTK_TYPE_XWINDOW_HANDLER (gtk_xwindow_handler_get_type ())
#define GTK_XWINDOW_HANDLER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_XWINDOW_HANDLER, GtkXWindowHandler))
#define GTK_XWINDOW_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass) , GTK_TYPE_XWINDOW_HANDLER, GtkXWindowHandlerClass))
#define GTK_IS_XWINDOW_HANDLER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_XWINDOW_HANDLER))
#define GTK_IS_XWINDOW_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_XWINDOW_HANDLER))
#define GTK_XWINDOW_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_XWINDOW_HANDLER, GtkXWindowHandlerClass))

typedef struct _GtkXWindowHandler GtkXWindowHandler;
typedef struct _GtkXWindowHandlerClass GtkXWindowHandlerClass;

struct _GtkXWindowHandler {
  GtkWidget widget;
  client * c;
};

struct _GtkXWindowHandlerClass {
  GtkWidgetClass parent_class;
};

GtkType gtk_xwindow_handler_get_type(void);
void gtk_xwindow_handler_set_client(GtkXWindowHandler * ths, client * c);
client * gtk_xwindow_handler_get_client(GtkXWindowHandler * ths);
GtkWidget * gtk_xwindow_handler_new();

G_END_DECLS


#endif /* GTK_XWINDOW_HANDLER_H_ */
