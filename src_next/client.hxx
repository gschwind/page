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
	Window xwin;
	Window clipping_window;

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
