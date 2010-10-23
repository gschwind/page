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

#ifndef TREE_H_
#define TREE_H_

#include <gdk/gdk.h>
#include <gtk/gtk.h>

typedef struct _dock dock;
struct _dock {
	GtkWidget * notebook;
	GtkWidget * hbox;
	GtkWidget * hsplit_button;
	GtkWidget * vsplit_button;
	GtkWidget * close_button;
	GtkWidget * exit_button;
	GtkWidget * label;
};

typedef struct _split split;
struct _split {
	GtkWidget * split_container;
};

typedef struct _root root;
struct _root {
	GtkWidget * window;
};

enum {
	TREE_VPANED,
	TREE_HPANED,
	TREE_NOTEBOOK,
	TREE_ROOT
};

typedef union {
	dock d;
	split s;
	root r;
} tree_content;

typedef struct _tree tree;
struct _tree {
	int mode;
	tree_content data;
	GtkWidget * w;
	void * ctx;
	tree * parent;
	tree * pack1;
	tree * pack2;
};

int tree_append_widget(tree * ths, GtkWidget * label, GtkWidget * content);
void tree_dock_init(tree * ths, void * ctx, tree * parent);
void tree_root_init(tree * ths, void * ctx);
void tree_dock_copy(tree * ths, tree * src);
gboolean tree_quit(GtkWidget * w, GdkEventButton * e, gpointer data);
gboolean tree_hsplit(GtkWidget * x, GdkEventButton * e, tree * ths);
gboolean tree_vsplit(GtkWidget * x, GdkEventButton * e, tree * ths);
gboolean tree_close(GtkWidget * x, GdkEventButton * e, tree * ths);
GtkWidget * tree_get_widget(tree * ths);


#endif /* TREE_H_ */
