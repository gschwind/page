/*
 * tree.h
 *
 *  Created on: Sep 21, 2010
 *      Author: gschwind
 */

#ifndef TREE_H_
#define TREE_H_

#include "gtk/gtk.h"

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

enum {
	TREE_VPANED,
	TREE_HPANED,
	TREE_NOTEBOOK
};

typedef union {
	dock d;
	split s;
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
void tree_dock_copy(tree * ths, tree * src);
void tree_split(tree * ths);
GtkWidget * tree_get_widget(tree * ths);


#endif /* TREE_H_ */
