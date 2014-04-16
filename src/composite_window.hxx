/*
 * composite_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef COMPOSITE_WINDOW_HXX_
#define COMPOSITE_WINDOW_HXX_

#include <X11/Xlib.h>
#include <set>
#include <map>

#include "time.hxx"

#include "xconnection.hxx"
#include "region.hxx"
#include "icon.hxx"
#include "composite_window.hxx"
#include "composite_surface.hxx"



namespace page {

inline static void print_cairo_status(cairo_t * cr, char const * file, int line) {
	cairo_status_t s = cairo_status(cr);
	if (s != CAIRO_STATUS_SUCCESS) {
		printf("Cairo status %s:%d = %s\n", file, line,
				cairo_status_to_string(s));
	}
}


#define CHECK_CAIRO(x) do { \
	x;\
	print_cairo_status(cr, __FILE__, __LINE__); \
} while(false)


/**
 * This class is an handler of window, it just store some data cache about
 * an X window, and provide some macro.
 **/

class composite_window_t {
	composite_surface_t * _surf;

	Display * dpy;
	Window _wid;
	rectangle _position;
	bool _has_alpha;

	region _region;
	region _opaque_region;

	atom_handler_t A;

	/* avoid copy */
	composite_window_t(composite_window_t const &);
	composite_window_t & operator=(composite_window_t const &);
public:

	enum fade_mode_e {
		FADE_NONE,
		FADE_OUT,
		FADE_IN,
		FADE_DESTROY
	};

	time_t fade_start;
	double fade_step;
	fade_mode_e fade_mode;

	static long int const ClientEventMask = (StructureNotifyMask
			| PropertyChangeMask);

	composite_window_t(Display * dpy, Window w,
			XWindowAttributes const * wa, composite_surface_t * surf) : A(dpy) {
		page_assert(dpy != 0);

		_surf = surf;

		this->dpy = dpy;
		_wid = w;

		/** copy usefull window attributes **/
		_position = rectangle(wa->x, wa->y, wa->width, wa->height);

		/** guess if window has alpha channel **/
		_has_alpha = false;
		XRenderPictFormat * format = XRenderFindVisualFormat(dpy, wa->visual);
		if (format != 0) {
			_has_alpha = (format->type == PictTypeDirect
					&& format->direct.alphaMask);
		}

		/** read shape date **/
		XShapeInputSelected(dpy, _wid);
		update_shape();
		update_opaque_region();

		fade_step = 0.0;
		fade_mode = FADE_NONE;

	}

	~composite_window_t() {

	}

	void draw_to(cairo_t * cr, rectangle const & area) {
		if (_surf == 0)
			return;

		rectangle clip = area
				& rectangle(_position.x, _position.y, _position.w, _position.h);

		if (clip.w > 0 && clip.h > 0) {
			CHECK_CAIRO(cairo_save(cr));
			CHECK_CAIRO(cairo_reset_clip(cr));
			CHECK_CAIRO(cairo_identity_matrix(cr));
			CHECK_CAIRO(cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h));
			CHECK_CAIRO(cairo_clip(cr));

			CHECK_CAIRO(cairo_set_source_surface(cr, _surf->get_surf(), _position.x, _position.y));
			if (fade_mode != FADE_NONE) {
				cairo_pattern_t * pat = cairo_pattern_create_rgba(0.0, 0.0, 0.0,
						fade_step);
				CHECK_CAIRO(cairo_mask(cr, pat));
				CHECK_CAIRO(cairo_pattern_destroy(pat));
			} else {
				CHECK_CAIRO(cairo_paint(cr));
			}

			CHECK_CAIRO(cairo_restore(cr));

		}

	}

	bool has_alpha() {
		return _has_alpha;
	}

	void update_shape() {

		int count = 0, ordering = 0;
		XRectangle * recs = XShapeGetRectangles(dpy, _wid, ShapeBounding,
				&count, &ordering);

		_region.clear();

		if (count > 0) {
			for (int i = 0; i < count; ++i) {
				_region += rectangle(recs[i]);
			}

			_region.translate(_position.x, _position.y);

			/* In doubt */
			XFree(recs);
		} else {
			_region = rectangle(_position.x, _position.y, _position.w,
					_position.h);
		}
	}

	region read_opaque_region(Window w) {
		region ret;
		std::vector<long> * data = get_window_property<long>(dpy, w, A(_NET_WM_OPAQUE_REGION), A(CARDINAL));
		if(data != 0) {
			for(int i = 0; i < data->size() / 4; ++i) {
				ret += rectangle((*data)[i*4+0],(*data)[i*4+1],(*data)[i*4+2],(*data)[i*4+3]);
			}
			delete data;
		}
		return ret;
	}

	void update_opaque_region() {
		if (not has_alpha()) {
			_opaque_region.clear();
			_opaque_region += _position;

		} else {
			_opaque_region.clear();
			_opaque_region = read_opaque_region(_wid);
			_opaque_region.translate(_position.x, _position.y);
		}

		_opaque_region = _opaque_region & _region;
	}

	region const & get_region() {
		return _region;
	}

	region const & get_opaque_region() {
		return _opaque_region;
	}

	Window get_w() {
		return _wid;
	}

	rectangle const & position() {
		return _position;
	}

	void update_position(XConfigureEvent const & ev) {
		if (_position != rectangle(ev.x, ev.y, ev.width, ev.height)) {
			_position = rectangle(ev.x, ev.y, ev.width, ev.height);
			update_shape();
			update_opaque_region();
		}

	}

	composite_surface_t * get_surf() {
		return _surf;
	}

};

}

#endif /* COMPOSITE_WINDOW_HXX_ */
