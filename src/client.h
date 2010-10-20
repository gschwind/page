/*
 * client.h
 *
 *  Created on: Sep 20, 2010
 *      Author: gschwind
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include "gtk_xwindow_handler.h"

typedef struct _client client;

struct _client {
	Window xwin;
	GdkWindow * gwin;
	GtkXWindowHandler * xwindow_handler;
	GtkNotebook * notebook_parent;
};

#endif /* CLIENT_H_ */
