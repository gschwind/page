/*
 * window_overlay.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef WINDOW_OVERLAY_HXX_
#define WINDOW_OVERLAY_HXX_

#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "cairo_surface_type_name.hxx"
#include "display.hxx"

namespace page {


class window_overlay_t {

	static long const OVERLAY_EVENT_MASK = ExposureMask | StructureNotifyMask;

protected:
	display_t * _cnx;
	cairo_surface_t * _front_surf;
	cairo_surface_t * _back_surf;
	Window _wid;

	rectangle _position;

	Visual * _visual;
	int _depth;

	bool _has_alpha;
	bool _is_durty;
	bool _is_visible;

public:
	window_overlay_t(display_t * cnx, int depth, rectangle position = rectangle(-10,-10, 1, 1)) {
		_cnx = cnx;
		_back_surf = 0;

		_position = position;

		/**
		 * Create RGB window for back ground
		 **/

		XVisualInfo vinfo;
		if (XMatchVisualInfo(_cnx->dpy(), _cnx->screen(), depth, TrueColor, &vinfo)
				== 0) {
			throw std::runtime_error(
					"Unable to find valid visual for background windows");
		}

		_depth = depth;
		_visual = vinfo.visual;

		XSetWindowAttributes wa;
		wa.override_redirect = True;

		/**
		 * To create RGBA window, the following field MUST bet set, for unknown
		 * reason. i.e. border_pixel, background_pixel and colormap.
		 **/
		wa.border_pixel = 0;
		wa.background_pixel = 0;
		wa.colormap = XCreateColormap(_cnx->dpy(), _cnx->root(), vinfo.visual,
				AllocNone);

		_wid = XCreateWindow(_cnx->dpy(), _cnx->root(), position.x,
				position.y, position.w, position.h, 0, vinfo.depth, InputOutput,
				vinfo.visual,
				CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWColormap,
				&wa);

		XSelectInput(_cnx->dpy(), _wid, OVERLAY_EVENT_MASK);

		_front_surf = cairo_xlib_surface_create(_cnx->dpy(), _wid, vinfo.visual,
				position.w, position.h);

		_back_surf = 0;

		if(vinfo.depth == 32) {
			_has_alpha = true;
		} else {
			_has_alpha = false;
		}

		_is_durty = true;
		_is_visible = false;

	}

	void mark_durty() {
		_is_durty = true;
	}

	void create_back_buffer() {
		if (_back_surf == 0) {
			if (_has_alpha) {
				_back_surf = cairo_surface_create_similar(_front_surf,
						CAIRO_CONTENT_COLOR_ALPHA, _position.w, _position.h);
			} else {
				_back_surf = cairo_surface_create_similar(_front_surf,
						CAIRO_CONTENT_COLOR, _position.w, _position.h);
			}
			//printf("CAIRO SURF TYPE = %s\n", cairo_surface_type_name[cairo_surface_get_type(_back_surf)]);

		}
	}

	void destroy_back_buffer() {
		if (_back_surf != nullptr) {
			cairo_surface_destroy(_back_surf);
			_back_surf = nullptr;
		}
	}

	virtual ~window_overlay_t() {
		destroy_back_buffer();
		cairo_surface_destroy(_front_surf);
		XDestroyWindow(_cnx->dpy(), _wid);
	}

	void move_resize(rectangle const & area) {
		_position = area;
		_cnx->move_resize(_wid, _position);
		if (_is_visible) {
			destroy_back_buffer();
			create_back_buffer();
		}
		mark_durty();
	}

	void move(int x, int y) {
		_cnx->move_window(_wid, x, y);
		expose();
	}

	void show() {
		_cnx->map_window(_wid);
		_is_visible = true;
		create_back_buffer();
		mark_durty();
	}

	void hide() {
		_cnx->unmap(_wid);
		_is_visible = false;
		destroy_back_buffer();
	}

	virtual void repair_back_buffer() {

		cairo_t * cr = cairo_create(_back_surf);
		cairo_rectangle(cr, 0, 0, _position.w, _position.h);
		cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.5);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_fill(cr);
		cairo_destroy(cr);

	}

	void expose () {

		if(!_is_durty || !_is_visible)
			return;
		_is_durty = false;

		cairo_xlib_surface_set_size(_front_surf, _position.w, _position.h);

		repair_back_buffer();

		cairo_t * cr = cairo_create(_front_surf);

		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(cr, 0, 0, _position.w, _position.h);
		cairo_set_source_surface(cr, _back_surf, 0, 0);
		cairo_fill(cr);

//		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
//		cairo_rectangle(cr, 0 + 0.5, 0 + 0.5, wa->width - 1, wa->height - 1);
//		cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.5);
//		cairo_stroke(cr);

		cairo_destroy(cr);

	}

	Window id() {
		return _wid;
	}


};

}



#endif /* WINDOW_OVERLAY_HXX_ */
