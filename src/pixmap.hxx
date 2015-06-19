/*
 * shared_pixmap.hxx
 *
 *  Created on: 14 juin 2014
 *      Author: gschwind
 */

#ifndef PIXMAP_HXX_
#define PIXMAP_HXX_

#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

#include "display.hxx"

namespace page {

/**
 * Self managed pixmap and cairo.
 **/
class pixmap_t {

	display_t * _dpy;
	xcb_pixmap_t _pixmap_id;
	cairo_surface_t * _surf;
	unsigned _w, _h;

public:

	pixmap_t(display_t * dpy, xcb_visualtype_t * v, xcb_pixmap_t p, unsigned w, unsigned h) {
		_dpy = dpy;
		_pixmap_id = p;
		_surf = cairo_xcb_surface_create(dpy->xcb(), p, v, w, h);
		_w = w;
		_h = h;
	}

	~pixmap_t() {
		cairo_surface_destroy(_surf);
		xcb_free_pixmap(_dpy->xcb(), _pixmap_id);
	}

	cairo_surface_t * get_cairo_surface() const {
		return _surf;
	}

	unsigned witdh() const {
		return _w;
	}

	unsigned height() const {
		return _h;
	}

};

}

#endif /* SHARED_PIXMAP_HXX_ */
