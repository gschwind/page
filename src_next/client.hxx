/*
 * client.hxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

#ifndef CLIENT_HXX_
#define CLIENT_HXX_

#include <X11/Xlib.h>
#include <string>

namespace page_next {

struct client_t {
	Display * dpy;
	/* the name of window */
	std::string name;

	bool wm_name_is_valid;
	std::string wm_name;
	bool net_wm_name_is_valid;
	std::string net_wm_name;

	Window xwin;
	Window clipping_window;

	/* store the map/unmap stase from the point of view of PAGE */
	bool is_map;
	/* this is used to distinguish if unmap is initiated by client or by PAGE
	 * PAGE unmap mean Normal to Iconic
	 * client unmap mean Normal to WithDrawn */
	int unmap_pending;

	bool as_icon;
	Pixmap pixmap_icon;
	Window w_icon;

	/* the desired width/heigth */
	int width;
	int height;

	/* Hints */
	int basew;
	int baseh;
	int minw;
	int minh;
	int maxw;
	int maxh;
	int incw;
	int inch;
	double mina;
	double maxa;
	bool is_fixed_size;

	void map();
	void unmap();
	void update_client_size(int w, int h);

};

}

#endif /* CLIENT_HXX_ */
