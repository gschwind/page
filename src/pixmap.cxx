/*
 * pixmap.cxx
 *
 *  Created on: 28 août 2015
 *      Author: gschwind
 */

#include "display.hxx"

#include "pixmap.hxx"

namespace page {

pixmap_t::
pixmap_t(display_t * dpy, xcb_visualtype_t * v, xcb_pixmap_t p, unsigned w, unsigned h) {
	_dpy = dpy;
	_pixmap_id = p;
	_surf = cairo_xcb_surface_create(dpy->xcb(), p, v, w, h);
	if(cairo_surface_status(_surf) != CAIRO_STATUS_SUCCESS) {
		throw exception_t{"unable to create cairo_surface in %s", __PRETTY_FUNCTION__};
	}
	_w = w;
	_h = h;
}

pixmap_t::
pixmap_t(display_t * dpy, pixmap_format_e format, unsigned width, unsigned height) :
	_dpy{dpy}, _w{width}, _h{height}
{
	if (format == PIXMAP_RGB) {
		_pixmap_id = xcb_generate_id(_dpy->xcb());
		xcb_create_pixmap(_dpy->xcb(), _dpy->root_depth(), _pixmap_id, _dpy->root(), width, height);
		_surf = cairo_xcb_surface_create(_dpy->xcb(), _pixmap_id, _dpy->root_visual(), _w, _h);
	} else {
		_pixmap_id = xcb_generate_id(_dpy->xcb());
		xcb_create_pixmap(_dpy->xcb(), 32, _pixmap_id, _dpy->root(), width, height);
		_surf = cairo_xcb_surface_create(_dpy->xcb(), _pixmap_id, _dpy->default_visual_rgba(), _w, _h);
	}

	if(cairo_surface_status(_surf) != CAIRO_STATUS_SUCCESS) {
		throw exception_t{"Unable to create cairo_surface in %s (format=%s,width=%u,height=%u)",
			__PRETTY_FUNCTION__, format==PIXMAP_RGB?"RGB":"RGBA", width, height};
	}

}

pixmap_t::~pixmap_t() {
	cairo_surface_destroy(_surf);
	xcb_free_pixmap(_dpy->xcb(), _pixmap_id);
}

cairo_surface_t * pixmap_t::get_cairo_surface() const {
	return _surf;
}

unsigned pixmap_t::witdh() const {
	return _w;
}

unsigned pixmap_t::height() const {
	return _h;
}

}


