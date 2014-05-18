/*
 * composite_surface.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef COMPOSITE_SURFACE_HXX_
#define COMPOSITE_SURFACE_HXX_

#include <X11/Xlib.h>

#include <memory>
#include <set>
#include <map>

#include "time.hxx"

#include "xconnection.hxx"
#include "region.hxx"
#include "icon.hxx"
#include "composite_window.hxx"

namespace page {


class composite_surface_t {

	Display * _dpy;
	Visual * _vis;
	Window _window_id;
	Pixmap _pixmap_id;
	cairo_surface_t * _surf;
	int _width, _height;

public:

	composite_surface_t(Display * dpy, Window w, XWindowAttributes const & wa) {
		_window_id = w;
		_dpy = dpy;
		_vis = wa.visual;
		_width = wa.width;
		_height = wa.height;
		_surf = 0;
		_pixmap_id = None;

		//printf("create %lu\n", _window_id);
	}

	~composite_surface_t() {
		//printf("destroy %lu\n", _window_id);
		destroy_cache();
	}

	void destroy_cache() {
		if (_surf != nullptr) {
			cairo_surface_destroy(_surf);
			_surf = nullptr;
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

	Window wid() {
		return _window_id;
	}

	cairo_surface_t * get_surf() {
		if (_surf != nullptr) {
			cairo_surface_destroy(_surf);
			_surf = nullptr;
		}
		_surf = cairo_xlib_surface_create(_dpy, _pixmap_id, _vis, _width, _height);
		return _surf;
	}

};

typedef std::shared_ptr<composite_surface_t> p_composite_surface_t;


}


#endif /* COMPOSITE_SURFACE_HXX_ */
