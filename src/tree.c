/*
 * tree.cc
 *
 *  Created on: Sep 21, 2010
 *      Author: gschwind
 */

#include <stdlib.h>
#include <gtk/gtk.h>
#include "tree.h"

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
	ths->w = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(ths->w), GTK_WIDGET(ths->data.d.notebook));
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

void tree_dock_copy(tree * ths, tree * src) {
	*ths = *src;
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
	}
}

gboolean tree_split(GtkWidget * x, GdkEventButton * e, tree * ths) {
	printf("call %s\n", __FUNCTION__);
	if (ths->mode == TREE_NOTEBOOK) {
		gtk_container_remove(GTK_CONTAINER(ths->w), GTK_WIDGET(ths->data.d.notebook));
		ths->pack1 = (tree *) malloc(sizeof(tree));
		ths->pack2 = (tree *) malloc(sizeof(tree));
		tree_dock_copy(ths->pack1, ths);
		tree_dock_init(ths->pack2, ths, NULL);
		ths->data.s.split_container = gtk_hpaned_new();
		gtk_paned_pack1(GTK_PANED(ths->data.s.split_container),
				tree_get_widget(ths->pack1), 0, 0);
		gtk_paned_pack2(GTK_PANED(ths->data.s.split_container),
				tree_get_widget(ths->pack2), 0, 0);
		gtk_container_remove(GTK_CONTAINER(ths->w), GTK_WIDGET(ths->data.s.split_container));
		gtk_widget_show_all(GTK_WIDGET(ths->data.s.split_container));
		gtk_widget_show_all(GTK_WIDGET(ths->w));
		gtk_widget_queue_draw(GTK_WIDGET(ths->w));
		ths->mode = TREE_VPANED;
		return TRUE;
	}
}

GtkWidget * tree_get_widget(tree * ths) {
	return ths->w;
}
