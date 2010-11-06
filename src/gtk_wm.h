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

#ifndef GTK_WM_H_
#define GTK_WM_H_

#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include "client.h"

G_BEGIN_DECLS

#define GTK_TYPE_WM                   (gtk_wm_get_type ())
#define GTK_WM(obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_WM, GtkWM))
#define GTK_WM_CLASS(klass)           (G_TYPE_CHECK_CLASS_CAST ((klass) , GTK_TYPE_WM, GtkWMClass))
#define GTK_IS_WM(obj)                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_WM))
#define GTK_IS_WM_CLASS(klass)        (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_WM))
#define GTK_WM_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_WM, GtkWMClass))

typedef struct _GtkWM       GtkWM;
typedef struct _GtkWMClass  GtkWMClass;

struct _GtkWM {
  GtkWidget widget;
  client * c;
};

struct _GtkWMClass {
  GtkWidgetClass parent_class;
};

GType gtk_wm_get_type(void);
GtkWidget * gtk_wm_new(client * c);
void gtk_wm_set_client(GtkWM * w, client *c);

G_END_DECLS


#endif /* GTK_XWINDOW_HANDLER_H_ */
