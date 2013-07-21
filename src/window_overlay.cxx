/*
 * window_overlay.cxx
 *
 *  Created on: 18 juil. 2013
 *      Author: bg
 */

#include "window_overlay.hxx"

namespace page {

window_overlay_t::window_overlay_t(xconnection_t * cnx, int depth,
		box_int_t position) {

	_cnx = cnx;

	_surf = 0;
	_cr = 0;

	_wid_cr = 0;
	_wid_surf = 0;

	_area = position;

	/**
	 * Create RGB window for back ground
	 **/

	XVisualInfo vinfo;
	if (XMatchVisualInfo(_cnx->dpy, _cnx->screen, depth, TrueColor, &vinfo)
			== 0) {
		throw std::runtime_error(
				"Unable to find valid visual for background windows");
	}

	_visual = vinfo.visual;

	XSetWindowAttributes wa;
	wa.override_redirect = True;

	/**
	 * To create RGBA window, the following field MUST bet set, for unknown
	 * reason. i.e. border_pixel, background_pixel and colormap.
	 **/
	wa.border_pixel = 0;
	wa.background_pixel = 0;
	wa.colormap = XCreateColormap(_cnx->dpy, _cnx->xroot, _visual, AllocNone);

	_wid = XCreateWindow(_cnx->dpy, _cnx->xroot, _area.x, _area.y, _area.w,
			_area.h, 0, vinfo.depth, InputOutput, _visual, CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWColormap, &wa);

	XSelectInput(_cnx->dpy, _wid, ExposureMask | StructureNotifyMask);

	//_cnx->allow_input_passthrough(_wid);

	rebuild_cairo();

}

window_overlay_t::~window_overlay_t() {
	cleanup();
}


void window_overlay_t::rebuild_cairo() {

	cleanup();

	_wid_surf = cairo_xlib_surface_create(_cnx->dpy, _wid, _visual, _area.w, _area.h);
	_wid_cr = cairo_create(_wid_surf);

	_surf = cairo_surface_create_similar(_wid_surf, CAIRO_CONTENT_COLOR_ALPHA, _area.w, _area.h);
	_cr = cairo_create(_surf);

}

void window_overlay_t::move_resize(box_int_t const & area) {
	_area = area;
	XMoveResizeWindow(_cnx->dpy, _wid, area.x, area.y, area.w, area.h);
	cairo_xlib_surface_set_size(_wid_surf, _area.w, _area.h);

	/**
	 * the way to resize cairo non XLIB surface is to destroy it first and
	 * create new one.
	 **/
	if(_cr != 0)
		cairo_destroy(_cr);
	if(_surf != 0)
		cairo_surface_destroy(_surf);

	_surf = cairo_surface_create_similar(_wid_surf, CAIRO_CONTENT_COLOR_ALPHA, _area.w, _area.h);
	_cr = cairo_create(_surf);

}

void window_overlay_t::move(int x, int y) {
	_area.x = x;
	_area.y = y;
	XMoveWindow(_cnx->dpy, _wid, _area.x, _area.y);
}


void window_overlay_t::cleanup() {
	if (_cr != 0)
		cairo_destroy(_cr);
	if (_surf != 0)
		cairo_surface_destroy(_surf);
	if(_wid_cr != 0)
		cairo_destroy(_wid_cr);
	if(_wid_surf != 0)
		cairo_surface_destroy(_wid_surf);

	_surf = 0;
	_cr = 0;
	_wid_cr = 0;
	_wid_surf = 0;

}

void window_overlay_t::show() {
	_cnx->map_window(_wid);
}

void window_overlay_t::hide() {
	_cnx->unmap(_wid);
}

void window_overlay_t::repair(box_int_t const & area) {
	//region_t<int> r = get_area();
	//std::cout << r.to_string() << std::endl;
	//region_t<int>::box_list_t::const_iterator i = r.begin();
	//while(i != r.end()) {
		box_int_t clip = _area & area;
		if (!clip.is_null()) {
			//std::cout << "yyy: " << clip.to_string() << std::endl;
			cairo_set_operator(_wid_cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(_wid_cr, clip.x, clip.y, clip.w, clip.h);
			cairo_set_source_surface(_wid_cr, _surf, 0, 0);
			cairo_paint(_wid_cr);
			cairo_surface_flush(_wid_surf);
		}
//		++i;
//	}
}


}

