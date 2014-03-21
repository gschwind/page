/*
 * window_icon_handler.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef WINDOW_ICON_HANDLER_HXX_
#define WINDOW_ICON_HANDLER_HXX_

#include <cairo.h>
#include "icon.hxx"
#include "xconnection.hxx"

namespace page {

class window_icon_handler_t {
	/* icon surface */
	cairo_surface_t * icon_surf;

	xconnection_t * _cnx;

public:
	window_icon_handler_t(xconnection_t * cnx, Window w, unsigned int xsize, unsigned int ysize);
	~window_icon_handler_t();
	cairo_surface_t * get_cairo_surface();

};

}



#endif /* WINDOW_ICON_HANDLER_HXX_ */
