/*
 * window_overlay.hxx
 *
 *  Created on: 18 juil. 2013
 *      Author: bg
 */

#ifndef WINDOW_OVERLAY_HXX_
#define WINDOW_OVERLAY_HXX_

#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "xconnection.hxx"

namespace page {


class window_overlay_t {

protected:

	box_int_t _area;
	xconnection_t * _cnx;

	/* off screen buffer */
	cairo_surface_t * _surf;
	cairo_t * _cr;

	/* on screen window */
	cairo_surface_t * _wid_surf;
	cairo_t * _wid_cr;

	Visual * _visual;

	Window _wid;

	void cleanup();

public:
	window_overlay_t(xconnection_t * cnx, int depth, box_int_t position = box_int_t(0, 0, 1, 1));

	~window_overlay_t();

	void move_resize(box_int_t const & area);
	void move(int x, int y);
	void rebuild_cairo();

	void show();
	void hide();

};

}



#endif /* WINDOW_OVERLAY_HXX_ */
