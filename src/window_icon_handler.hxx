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
#include "client_base.hxx"

namespace page {

class window_icon_handler_t {
	/* icon surface */
	cairo_surface_t * icon_surf;

	client_base_t const * _c;

public:
	window_icon_handler_t(client_base_t const * c, unsigned int xsize, unsigned int ysize);
	~window_icon_handler_t();
	cairo_surface_t * get_cairo_surface();

};

}



#endif /* WINDOW_ICON_HANDLER_HXX_ */
