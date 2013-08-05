/*
 * window_overlay.hxx
 *
 *  Created on: 18 juil. 2013
 *      Author: bg
 */

#ifndef WINDOW_OVERLAY_HXX_
#define WINDOW_OVERLAY_HXX_

#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "xconnection.hxx"

namespace page {


class window_overlay_t {

protected:
	xconnection_t * _cnx;
	cairo_surface_t * _back_surf;
	Window _wid;

public:
	window_overlay_t(xconnection_t * cnx, int depth, box_int_t position = box_int_t(-10,-10, 1, 1)) {
		_cnx = cnx;
		_back_surf = 0;

		/**
		 * Create RGB window for back ground
		 **/

		XVisualInfo vinfo;
		if (XMatchVisualInfo(_cnx->dpy, _cnx->screen(), depth, TrueColor,
				&vinfo) == 0) {
			throw std::runtime_error(
					"Unable to find valid visual for background windows");
		}

		XSetWindowAttributes wa;
		wa.override_redirect = True;

		/**
		 * To create RGBA window, the following field MUST bet set, for unknown
		 * reason. i.e. border_pixel, background_pixel and colormap.
		 **/
		wa.border_pixel = 0;
		wa.background_pixel = 0;
		wa.colormap = XCreateColormap(_cnx->dpy, _cnx->get_root_window(),
				vinfo.visual, AllocNone);

		_wid = XCreateWindow(_cnx->dpy, _cnx->get_root_window(), position.x,
				position.y, position.w, position.h, 0, vinfo.depth, InputOutput,
				vinfo.visual,
				CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWColormap,
				&wa);

		_cnx->select_input(_wid, ExposureMask | StructureNotifyMask);


	}

	void create_back_buffer() {
		if (_back_surf != 0)
			return;
		XWindowAttributes const * wa = _cnx->get_window_attributes(_wid);
		assert(wa != 0);

		cairo_surface_t * tmp = cairo_xlib_surface_create(_cnx->dpy, _wid,
				wa->visual, wa->width, wa->height);
		_back_surf = cairo_surface_create_similar(tmp,
				CAIRO_CONTENT_COLOR_ALPHA, wa->width, wa->height);
		cairo_surface_destroy(tmp);
	}

	void destroy_back_buffer() {
		if (_back_surf != 0) {
			cairo_surface_destroy(_back_surf);
			_back_surf = 0;
		}
	}

	virtual ~window_overlay_t() {
		destroy_back_buffer();
		XDestroyWindow(_cnx->dpy, _wid);
	}

	void move_resize(box_int_t const & area) {
		_cnx->move_resize(_wid, box_int_t(area.x, area.y, area.w, area.h));
		destroy_back_buffer();
	}

	void move(int x, int y) {
		_cnx->move_window(_wid, x, y);
		expose();
	}

	void show() {
		_cnx->map_window(_wid);
	}

	void hide() {
		_cnx->unmap(_wid);
		destroy_back_buffer();
	}

	virtual void repair_back_buffer() {
		XWindowAttributes const * wa = _cnx->get_window_attributes(_wid);
		assert(wa != 0);

		cairo_t * cr = cairo_create(_back_surf);
		cairo_rectangle(cr, 0, 0, wa->width, wa->height);
		cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.5);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_fill(cr);
		cairo_destroy(cr);

	}

	void expose () {
		XWindowAttributes const * wa = _cnx->get_window_attributes(_wid);
		assert(wa != 0);

		if(_back_surf == 0) {
			create_back_buffer();
			repair_back_buffer();
		}

		cairo_surface_t * front_surf = cairo_xlib_surface_create(_cnx->dpy,
				_wid, wa->visual, wa->width, wa->height);
		cairo_t * cr = cairo_create(front_surf);

		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(cr, 0, 0, wa->width, wa->height);
		cairo_set_source_surface(cr, _back_surf, 0, 0);
		cairo_fill(cr);

		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_rectangle(cr, 0 + 0.5, 0 + 0.5, wa->width - 1, wa->height - 1);
		cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.5);
		cairo_stroke(cr);

		cairo_destroy(cr);
		cairo_surface_flush(front_surf);
		cairo_surface_destroy(front_surf);

	}

	Window id() {
		return _wid;
	}


};

}



#endif /* WINDOW_OVERLAY_HXX_ */
