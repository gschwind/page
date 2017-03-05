/*
 * icon_handler.hxx
 *
 *  Created on: 20 sept. 2014
 *      Author: gschwind
 */

#ifndef ICON_HANDLER_HXX_
#define ICON_HANDLER_HXX_

#include <cairo.h>

#include <limits>

#include "page-types.hxx"

namespace page {

using namespace std;

template<unsigned const WIDTH, unsigned const HEIGHT>
class icon_handler_t {

	cairo_surface_t * icon_surf;

	struct _icon_ref_t {
		int width;
		int height;
		uint32_t const * data;
	};

public:
	icon_handler_t(client_managed_t * c);
	~icon_handler_t();
	cairo_surface_t * get_cairo_surface() const;

};

using icon16 = icon_handler_t<16,16>;
using icon64 = icon_handler_t<64,64>;


}



#endif /* ICON_HANDLER_HXX_ */
