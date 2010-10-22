/*
 * tree.h
 *
 *  Created on: Sep 21, 2010
 *      Author: gschwind
 */

#ifndef TREE_H_
#define TREE_H_

#include <gdk/gdk.h>
#include <gtk/gtk.h>

typedef struct _dock dock;
struct _dock {
	GtkWidget * notebook;
	GtkWidget * hbox;
	GtkWidget * split_button;
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
gboolean tree_split(GtkWidget * x, GdkEventButton * e, tree * ths);
gboolean tree_close(GtkWidget * x, GdkEventButton * e, tree * ths);
GtkWidget * tree_get_widget(tree * ths);


#endif /* TREE_H_ */
