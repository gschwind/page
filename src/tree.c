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

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "tree.h"
#include "page.h"

gboolean tree_quit(GtkWidget * w, GdkEventButton * e, gpointer data) {
	printf("call %s\n", __FUNCTION__);
	page * ths = (page *) data;
	gtk_widget_destroy(GTK_WIDGET(ths->gtk_main_win));
	while(gtk_events_pending())
		gtk_main_iteration();
	gtk_main_quit();
	return TRUE;
}

void tree_build_control_tab(tree * ths) {
	ths->data.d.hbox = gtk_hbox_new(FALSE, 0);
	ths->data.d.hsplit_button = gtk_button_new_with_label("HS");
	ths->data.d.vsplit_button = gtk_button_new_with_label("VS");
	ths->data.d.close_button = gtk_button_new_with_label("C");
	ths->data.d.exit_button = gtk_button_new_with_label("E");
	//g_signal_connect(GTK_OBJECT(exit_button), "button-release-event",
	//		GTK_SIGNAL_FUNC(page_quit), ths);
	gtk_box_pack_start(GTK_BOX(ths->data.d.hbox), ths->data.d.hsplit_button,
			TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(ths->data.d.hbox), ths->data.d.vsplit_button,
			TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(ths->data.d.hbox), ths->data.d.close_button,
			TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(ths->data.d.hbox), ths->data.d.exit_button,
			TRUE, TRUE, 0);
	g_signal_connect(GTK_OBJECT(ths->data.d.hsplit_button), "button-release-event",
			GTK_SIGNAL_FUNC(tree_hsplit), ths);
	g_signal_connect(GTK_OBJECT(ths->data.d.vsplit_button), "button-release-event",
			GTK_SIGNAL_FUNC(tree_vsplit), ths);
	g_signal_connect(GTK_OBJECT(ths->data.d.close_button), "button-release-event",
			GTK_SIGNAL_FUNC(tree_close), ths);
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

gboolean tree_split_generic(tree * ths, int mode) {
	printf("call %s\n", __FUNCTION__);
	tree * split = (tree *) malloc(sizeof(tree));
	split->parent = ths->parent;
	ths->parent = split;
	split->mode = mode;
	split->pack1 = ths;
	split->pack2 = (tree *) malloc(sizeof(tree));
	tree_dock_init(split->pack2, ths->ctx, split);
	if (mode == TREE_HPANED) {
		split->data.s.split_container = gtk_hpaned_new();
	} else {
		split->data.s.split_container = gtk_vpaned_new();
	}
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

gboolean tree_hsplit(GtkWidget * x, GdkEventButton * e, tree * ths) {
	printf("call %s #%p\n", __FUNCTION__, ths);
	return tree_split_generic(ths, TREE_HPANED);
}

gboolean tree_vsplit(GtkWidget * x, GdkEventButton * e, tree * ths) {
	printf("call %s #%p\n", __FUNCTION__, ths);
	return tree_split_generic(ths, TREE_VPANED);
}

gboolean tree_close(GtkWidget * x, GdkEventButton * e, tree * ths) {
	printf("Enter %s #%p\n", __FUNCTION__, ths);
	tree * tab_dst = NULL;
	if (ths->parent->mode != TREE_ROOT) {
		if (ths == ths->parent->pack1) {
			tab_dst = ths->parent->pack2;
		} else {
			tab_dst = ths->parent->pack1;
		}

		int i = gtk_notebook_get_n_pages(GTK_NOTEBOOK(ths->data.d.notebook));
		while (i--) {
			GtkWidget * content = gtk_notebook_get_nth_page(
					GTK_NOTEBOOK(ths->data.d.notebook), i);
			GtkWidget * label = gtk_notebook_get_tab_label(
					GTK_NOTEBOOK(ths->data.d.notebook), content);
			if (label != ths->data.d.hbox) {
				g_object_ref(G_OBJECT(content));
				g_object_ref(G_OBJECT(label));
				gtk_notebook_remove_page(GTK_NOTEBOOK(ths->data.d.notebook), i);
				tree_append_widget(tab_dst, label, content);
				g_object_unref(G_OBJECT(content));
				g_object_unref(G_OBJECT(label));
			} else {
				gtk_notebook_remove_page(GTK_NOTEBOOK(ths->data.d.notebook), i);
			}
		}

		g_object_ref(G_OBJECT(tab_dst->w));
		/* remove the widget from the parent */
		gtk_container_remove(
				GTK_CONTAINER(ths->parent->data.s.split_container),
				GTK_WIDGET(tab_dst->w));
		gtk_container_remove(GTK_CONTAINER(ths->parent->parent->w),
				GTK_WIDGET(ths->parent->w));
		tab_dst->parent = ths->parent->parent;
		/* now we rebuild gtk_tree */
		if (ths->parent->parent->mode == TREE_ROOT) {
			printf("find tree root \n");
			/* add reference of the widget, avoid remove to destroy it */
			gtk_container_add(
					GTK_CONTAINER(ths->parent->parent->data.r.window),
					GTK_WIDGET(tab_dst->w));
			free(ths->parent->parent->pack1);
			ths->parent->parent->pack1 = tab_dst;
		} else {
			if (ths->parent->parent->pack1 == ths->parent) {
				gtk_paned_pack1(
						GTK_PANED(ths->parent->parent->data.s.split_container),
						GTK_WIDGET(tab_dst->w), FALSE, FALSE);
				free(ths->parent->parent->pack1);
				ths->parent->parent->pack1 = tab_dst;
			} else {
				gtk_paned_pack2(
						GTK_PANED(ths->parent->parent->data.s.split_container),
						GTK_WIDGET(tab_dst->w), FALSE, FALSE);
				free(ths->parent->parent->pack2);
				ths->parent->parent->pack2 = tab_dst;
			}

		}

		g_object_unref(G_OBJECT(tab_dst->w));
		gtk_widget_show_all(tab_dst->parent->w);
		gtk_widget_queue_draw(tab_dst->parent->w);
		free(ths);
		printf("Exit %s #%p\n", __FUNCTION__, ths);
		return TRUE;

	} else {
		tree_quit(x, e, ths->ctx);
		printf("Exit %s #%p\n", __FUNCTION__, ths);
		return TRUE;
	}
	printf("Exit %s #%p\n", __FUNCTION__, ths);
	return TRUE;
}

GtkWidget * tree_get_widget(tree * ths) {
	return ths->w;
}
