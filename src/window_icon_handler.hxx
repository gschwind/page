/*
 * window_icon_handler.hxx
 *
 *  Created on: 15 févr. 2013
 *      Author: gschwind
 */

#ifndef WINDOW_ICON_HANDLER_HXX_
#define WINDOW_ICON_HANDLER_HXX_

#include <cairo.h>
#include "icon.hxx"
#include "xconnection.hxx"

namespace page {

class window_icon_handler_t {
	/* icon data */
	icon_t icon;
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
