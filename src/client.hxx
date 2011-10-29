/*
 * client.hxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

#ifndef CLIENT_HXX_
#define CLIENT_HXX_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string>
#include "atoms.hxx"

namespace page_next {

struct client_t {
	/* the Xlib context */
	Display * dpy;
	Window xroot;
	atoms_t * atoms;
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

	XSizeHints hints;

	/* _NET_WM_STATE */
	bool is_modal; /* has no effect */
	bool is_sticky; /* has no effect */
	bool is_maximized_vert; /* has no effect */
	bool is_maximized_horz; /* has no effect */
	bool is_is_shaded; /* has no effect */
	bool is_skip_taskbar; /* has no effect */
	bool is_skip_pager; /* has no effect */
	bool is_hidden; /* has no effect */
	bool is_fullscreen; /* has no effect */
	bool is_above; /* has no effect */
	bool is_below; /* has no effect */
	bool is_demands_attention; /* has no effect */

	bool has_partial_struct;
	int struct_left;
	int struct_right;
	int struct_top;
	int struct_bottom;

	void map();
	void unmap();
	void update_client_size(int w, int h);

	bool try_lock_client();
	void unlock_client();
	void focus();
	void fullscreen(int w, int h);

};

}

#endif /* CLIENT_HXX_ */
