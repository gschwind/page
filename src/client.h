/*
 * client.h
 *
 *  Created on: Sep 20, 2010
 *      Author: gschwind
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include "gtk_xwindow_handler.h"

struct client {
	Window xwin;
	GdkWindow * gwin;
	GtkNotebook * notebook_parent;
	GtkXWindowHandler * xwindow_handler;
};

#endif /* CLIENT_H_ */
