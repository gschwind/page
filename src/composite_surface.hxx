/*
 * composite_surface.hxx
 *
 *  Created on: 22 févr. 2014
 *      Author: gschwind
 */

#ifndef COMPOSITE_SURFACE_HXX_
#define COMPOSITE_SURFACE_HXX_

#include <X11/Xlib.h>
#include <set>
#include <map>

#include "time.hxx"

#include "xconnection.hxx"
#include "region.hxx"
#include "icon.hxx"
#include "composite_window.hxx"

namespace page {

class composite_surface_t {

	int _nref;
	Display * _dpy;
	Visual * _vis;
	Window _window_id;
	Pixmap _pixmap_id;
	cairo_surface_t * _surf;
	int _width, _height;

public:

	composite_surface_t(Display * dpy, Window w, XWindowAttributes & wa) {
		_window_id = w;
		_dpy = dpy;
		_vis = wa.visual;
		_nref = 1;
		_width = wa.width;
		_height = wa.height;
		_surf = 0;
		_pixmap_id = None;

		printf("create %lu\n", _window_id);
	}

	~composite_surface_t() {
		printf("destroy %lu\n", _window_id);
		destroy_cache();
	}

	void destroy_cache() {
		if (_surf != 0) {
			cairo_surface_destroy(_surf);
			_surf = 0;
		}

		if(_pixmap_id != None) {
			XFreePixmap(_dpy, _pixmap_id);
			_pixmap_id = None;
		}
	}

	void onmap() {
		if(_pixmap_id != None) {
			XFreePixmap(_dpy, _pixmap_id);
			_pixmap_id = None;
		}
		_pixmap_id = XCompositeNameWindowPixmap(_dpy, _window_id);

	}

	void onresize(int width, int height) {
		if (width != _width or height != _height) {
			_width = width;
			_height = height;
			onmap();
		}
	}

	void incr_ref() {
		++_nref;
	}

	void decr_ref() {
		--_nref;
	}

	int nref() {
		return _nref;
	}

	Window wid() {
		return _window_id;
	}

	cairo_surface_t * get_surf() {
		if (_surf != 0) {
			cairo_surface_destroy(_surf);
			_surf = 0;
		}
		_surf = cairo_xlib_surface_create(_dpy, _pixmap_id, _vis, _width, _height);
		return _surf;
	}

};



}


#endif /* COMPOSITE_SURFACE_HXX_ */
