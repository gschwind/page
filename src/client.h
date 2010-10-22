/*
 * client.h
 *
 *  Created on: Sep 20, 2010
 *      Author: gschwind
 */

#ifndef CLIENT_H_
#define CLIENT_H_

typedef struct _client client;
struct _client {
	Window xwin;
	GdkWindow * gwin;
	GtkNotebook * notebook_parent;

	guint orig_width;
	guint orig_height;
	guint orig_x;
	guint orig_y;
	guint orig_depth;

};

#endif /* CLIENT_H_ */
