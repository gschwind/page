/*
 * shared_pixmap.hxx
 *
 *  Created on: 14 juin 2014
 *      Author: gschwind
 */

#ifndef PIXMAP_HXX_
#define PIXMAP_HXX_

#include <cassert>

#include "display.hxx"

namespace page {

class pixmap_t {
	Display * _dpy;
	Pixmap _pixmap_id;
	cairo_surface_t * _surf;

public:

	pixmap_t(Display * dpy, Visual * v, Pixmap p, unsigned w, unsigned h) {
		_dpy = dpy;
		_pixmap_id = p;
		_surf = cairo_xlib_surface_create(dpy, p, v, w, h);
	}

	~pixmap_t() {
		if (_surf != nullptr) {
			//display_t::destroy_surf(__FILE__, __LINE__);
			//assert(cairo_surface_get_reference_count(_surf) == 1);
			cairo_surface_destroy(_surf);
			_surf = nullptr;
		}
		if(_pixmap_id != None) {
			XFreePixmap(_dpy, _pixmap_id);
			_pixmap_id = None;
		}
	}

	cairo_surface_t * get_cairo_surface() {
		return _surf;
	}

};

}

#endif /* SHARED_PIXMAP_HXX_ */
