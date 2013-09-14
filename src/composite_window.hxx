/*
 * composite_window.hxx
 *
 *  copyright (2012) Benoit Gschwind
 *
 */

#ifndef COMPOSITE_WINDOW_HXX_
#define COMPOSITE_WINDOW_HXX_

#include <X11/Xlib.h>
#include <set>
#include <map>

#include "xconnection.hxx"
#include "region.hxx"
#include "icon.hxx"
#include "composite_window.hxx"

namespace page {

/**
 * This class is an handler of window, it just store some data cache about
 * an X window, and provide some macro.
 **/

class composite_window_t {
	/** short cut **/
	typedef region_t<int> _region_t;
	typedef box_t<int> _box_t;

	Display * dpy;

	Window _wid;
	Damage _damage;

	box_int_t _position;
	int _depth;
	Visual * _visual;
	int _c_class;
	unsigned int _map_state;

	Window _above;

	cairo_surface_t * _surf;

	bool _has_alpha;

	_region_t _region;


	unsigned int _old_map_state;
	_region_t _old_region;


	_region_t damaged_region;

	bool _has_moved;


	/* avoid copy */
	composite_window_t(composite_window_t const &);
	composite_window_t & operator=(composite_window_t const &);
public:


	timespec fade_start;
	double fade_in_step;

	static long int const ClientEventMask = (StructureNotifyMask
			| PropertyChangeMask);

	composite_window_t(Display * dpy, Window w,
			XWindowAttributes const * wa, Window above) {
		page_assert(dpy != 0);

		this->dpy = dpy;
		_wid = w;

		/** copy usefull window attributes **/
		_position = box_int_t(wa->x, wa->y, wa->width, wa->height);
		_depth = wa->depth;
		_visual = wa->visual;
		_c_class = wa->c_class;
		_map_state = wa->map_state;

		_above = above;

		_damage = None;
		_surf = 0;
		_has_moved = true;

		_old_region = _region_t();
		_old_map_state = IsUnmapped;

		damaged_region.clear();

		/** guess if window has alpha channel **/
		_has_alpha = false;
		if (_c_class == InputOutput) {
			XRenderPictFormat * format = XRenderFindVisualFormat(dpy,
					_visual);
			if (format != 0) {
				_has_alpha = (format->type == PictTypeDirect
						&& format->direct.alphaMask);
			}
		}

		/** read shape date **/
		XShapeInputSelected(dpy, _wid);
		update_shape();

		/** if window is mapped, create cairo surface and damage **/
		if (_map_state != IsUnmapped and _c_class == InputOutput) {
			create_damage();
			create_cairo();
		}

	}

	void create_cairo() {
		if (_surf == 0 and _c_class == InputOutput) {
			_surf = cairo_xlib_surface_create(dpy, _wid, _visual,
					_position.w, _position.h);
		}
	}

	void update_cairo() {
		if (_surf != 0) {
			cairo_xlib_surface_set_size(_surf, _position.w, _position.h);
		}
	}

	void destroy_cairo() {
		if (_surf != 0) {
			cairo_surface_destroy(_surf);
			_surf = 0;
		}
	}

	~composite_window_t() {
		destroy_cairo();
		destroy_damage();
	}

	void draw_to(cairo_t * cr, box_int_t const & area) {
		if (_surf == 0)
			return;

		box_int_t clip = area
				& box_int_t(_position.x, _position.y, _position.w, _position.h);

		if (clip.w > 0 && clip.h > 0) {
			cairo_save(cr);
			cairo_reset_clip(cr);
			cairo_identity_matrix(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_set_source_surface(cr, _surf, _position.x, _position.y);
			cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
			cairo_clip(cr);
			cairo_paint(cr);
			cairo_restore(cr);
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
				_region += box_int_t(recs[i]);
			}

			_region.translate(_position.x, _position.y);

			/* In doubt */
			XFree(recs);
		} else {
			_region = box_int_t(_position.x, _position.y, _position.w,
					_position.h);
		}
	}

	region_t<int> const & get_region() {
		return _region;
	}

	void create_damage() {
		destroy_damage();

		_damage = XDamageCreate(dpy, _wid, XDamageReportNonEmpty);
		if (_damage != None) {
			XserverRegion region = XFixesCreateRegion(dpy, 0, 0);
			XDamageSubtract(dpy, _damage, None, region);
			XFixesDestroyRegion(dpy, region);
		}
	}

	void destroy_damage() {
		if (_damage != None) {
			XDamageDestroy(dpy, _damage);
			_damage = None;
		}
	}

	void add_damaged_region(region_t<int> const & r) {
		damaged_region += r;
	}

	region_t<int> get_damaged_region() {
		region_t<int> r = damaged_region;
		r.translate(_position.x, _position.y);
		r &= _region;
		return r;
	}



	void moved() {
		_has_moved = true;
	}

	void clear_state() {
		damaged_region.clear();
		_has_moved = false;

		_old_region = _region;
		_old_map_state = _map_state;

	}

	bool has_moved() {
		return _has_moved;
	}

	Window get_w() {
		return _wid;
	}

	_region_t const & old_region() {
		return _old_region;
	}

	unsigned int old_map_state() {
		return _old_map_state;
	}

	box_int_t const & position() {
		return _position;
	}

	unsigned int map_state() {
		return _map_state;
	}

	void update_position(XConfigureEvent const & ev) {
		if (_position != box_int_t(ev.x, ev.y, ev.width, ev.height)) {
			_has_moved = true;
			_position = box_int_t(ev.x, ev.y, ev.width, ev.height);
			update_shape();
		}

		if(ev.above != _above) {
			_has_moved = true;
			_above = ev.above;
		}

	}

	void update_map_state(int state) {
		_map_state = state;
		if(state != IsUnmapped) {
			create_cairo();
			create_damage();
		} else {
			destroy_cairo();
			destroy_damage();
		}
	}

	int c_class() {
		return _c_class;
	}


};

}

#endif /* COMPOSITE_WINDOW_HXX_ */
