/*
 * tree.cc
 *
 *  Created on: Sep 21, 2010
 *      Author: gschwind
 */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "tree.h"
#include "page.h"

void tree_build_control_tab(tree * ths) {
	ths->data.d.hbox = gtk_hbox_new(TRUE, 0);
	ths->data.d.split_button = gtk_button_new_with_label("S");
	ths->data.d.close_button = gtk_button_new_with_label("C");
	ths->data.d.exit_button = gtk_button_new_with_label("E");
	//g_signal_connect(GTK_OBJECT(exit_button), "button-release-event",
	//		GTK_SIGNAL_FUNC(page_quit), ths);
	gtk_box_pack_start(GTK_BOX(ths->data.d.hbox), ths->data.d.split_button,
			TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(ths->data.d.hbox), ths->data.d.close_button,
			TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(ths->data.d.hbox), ths->data.d.exit_button,
			TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(ths->data.d.split_button), "button-release-event",
			GTK_SIGNAL_FUNC(tree_split), ths);
	gtk_widget_show_all(ths->data.d.hbox);
}

void tree_dock_init(tree * ths, void * ctx, tree * parent) {
	ths->ctx = ctx;
	ths->mode = TREE_NOTEBOOK;
	ths->data.d.notebook = gtk_notebook_new();
	ths->w = ths->data.d.notebook;
	gtk_notebook_set_group_id(GTK_NOTEBOOK(ths->data.d.notebook), 1928374);
	ths->data.d.label = gtk_label_new("hello world 0");
	tree_build_control_tab(ths);
	gtk_notebook_append_page(GTK_NOTEBOOK(ths->data.d.notebook),
			ths->data.d.label, ths->data.d.hbox);
	ths->pack1 = NULL;
	ths->pack2 = NULL;
	ths->parent = parent;
	gtk_widget_show_all(ths->w);
}

void tree_root_init(tree * ths, void * _ctx) {
	page * ctx = _ctx;
	ths->ctx = ctx;
	ths->mode = TREE_ROOT;
	ths->parent = NULL;
	ctx->gtk_main_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(ctx->gtk_main_win), ctx->sw, ctx->sh);
	gtk_widget_show_all(ctx->gtk_main_win);
	ctx->gdk_main_win = gtk_widget_get_window(ctx->gtk_main_win);
	ths->data.r.window = ctx->gtk_main_win;
	ths->w = ths->data.r.window;
	ctx->main_cursor = gdk_cursor_new(GDK_PLUS);
	gdk_window_set_cursor(ctx->gdk_main_win, ctx->main_cursor);
	ths->pack1 = (tree *) malloc(sizeof(tree));
	tree_dock_init(ths->pack1, _ctx, ths);
	gtk_container_add(GTK_CONTAINER(ctx->gtk_main_win), tree_get_widget(
			ths->pack1));
}

void tree_dock_copy(tree * ths, tree * src) {
	memcpy(ths, src, sizeof(tree));
	ths->parent = src;
}

int tree_append_widget(tree * ths, GtkWidget * label, GtkWidget * content) {
	if (ths->mode == TREE_NOTEBOOK) {
		/* will generate page-added event */
		gtk_notebook_prepend_page(GTK_NOTEBOOK(ths->data.d.notebook), content,
				label);
		gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(ths->data.d.notebook),
				content, TRUE);
		gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(ths->data.d.notebook),
				content, TRUE);
		gtk_widget_show_all(GTK_WIDGET(ths->data.d.notebook));
		gtk_widget_queue_draw(GTK_WIDGET(ths->data.d.notebook));
		return 1;
	} else if (ths->mode == TREE_ROOT) {
		return tree_append_widget(ths->pack1, label, content);
	} else {
		if (!tree_append_widget(ths->pack1, label, content)) {
			return tree_append_widget(ths->pack2, label, content);
		} else {
			return 1;
		}
	}

	return 0;
}

gboolean tree_split(GtkWidget * x, GdkEventButton * e, tree * ths) {
	printf("call %s\n", __FUNCTION__);
	tree * split = (tree *) malloc(sizeof(tree));
	split->parent = ths->parent;
	ths->parent = split;
	split->mode = TREE_HPANED;
	split->pack1 = ths;
	split->pack2 = (tree *) malloc(sizeof(tree));
	tree_dock_init(split->pack2, ths->ctx, split);
	split->data.s.split_container = gtk_hpaned_new();
	split->w = split->data.s.split_container;
	split->ctx = ths->ctx;

	if (split->parent->mode == TREE_ROOT) {
		printf("find tree root \n");
		/* add reference of the widget, avoid remove to destroy it */
		g_object_ref(G_OBJECT(split->pack1->data.d.notebook));
		/* remove the widget from the parent */
		gtk_container_remove(GTK_CONTAINER(split->parent->data.r.window),
				GTK_WIDGET(split->pack1->data.d.notebook));
		gtk_paned_pack1(GTK_PANED(split->data.s.split_container),
				GTK_WIDGET(split->pack1->data.d.notebook), FALSE, FALSE);
		gtk_paned_pack2(GTK_PANED(split->data.s.split_container),
				GTK_WIDGET(split->pack2->data.d.notebook), FALSE, FALSE);
		gtk_container_add(GTK_CONTAINER(split->parent->data.r.window),
				GTK_WIDGET(split->data.s.split_container));
		/* now the widget is handled by split_container */
		g_object_unref(G_OBJECT(split->pack1->data.d.notebook));
		split->parent->pack1 = split;
	} else {
		g_object_ref(G_OBJECT(split->pack1->data.d.notebook));
		gtk_container_remove(
				GTK_CONTAINER(split->parent->data.s.split_container),
				GTK_WIDGET(split->pack1->data.d.notebook));
		gtk_paned_pack1(GTK_PANED(split->data.s.split_container),
				GTK_WIDGET(split->pack1->data.d.notebook), FALSE, FALSE);
		gtk_paned_pack2(GTK_PANED(split->data.s.split_container),
				GTK_WIDGET(split->pack2->data.d.notebook), FALSE, FALSE);
		if (split->parent->pack1 == split->pack1) {
			gtk_paned_pack1(GTK_PANED(split->parent->data.s.split_container),
					GTK_WIDGET(split->data.s.split_container), FALSE, FALSE);
			split->parent->pack1 = split;
		} else {
			gtk_paned_pack2(GTK_PANED(split->parent->data.s.split_container),
					GTK_WIDGET(split->data.s.split_container), FALSE, FALSE);
			split->parent->pack2 = split;
		}
		g_object_unref(G_OBJECT(split->pack1->data.d.notebook));
	}
	/* now the widget is handled by split_container */
	gtk_widget_show_all(GTK_WIDGET(split->parent->w));
	gtk_widget_queue_draw(GTK_WIDGET(split->parent->w));
	return TRUE;
}

GtkWidget * tree_get_widget(tree * ths) {
	return ths->w;
}
