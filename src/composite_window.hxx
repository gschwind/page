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

	xconnection_t * _cnx;

	Window _wid;
	Damage _damage;
	_region_t _region;

	cairo_surface_t * _surf;

	bool _has_alpha;
	bool _has_shape;

	p_window_attribute_t _wa;

	_region_t damaged_region;

	bool _has_moved;

	/* avoid copy */
	composite_window_t(composite_window_t const &);
	composite_window_t & operator=(composite_window_t const &);
public:

	static long int const ClientEventMask = (StructureNotifyMask
			| PropertyChangeMask);

	composite_window_t(xconnection_t * cnx, Window w,
			p_window_attribute_t const & wa) {
		assert(cnx != 0);
		assert(wa->is_valid);
		assert(wa->c_class == InputOutput);

		_cnx = cnx;
		_wid = w;
		_damage = None;
		_has_alpha = false;
		_surf = 0;
		_has_shape = false;
		_has_moved = true;
		_wa = wa;

		damaged_region.clear();

		XRenderPictFormat * format =
				XRenderFindVisualFormat(cnx->dpy, _wa->visual);

		_has_alpha =
				(format->type == PictTypeDirect && format->direct.alphaMask);

		XShapeInputSelected(cnx->dpy, _wid);
		update_shape();

		create_damage();

		_surf = cairo_xlib_surface_create(_cnx->dpy, _wid, _wa->visual,
				_wa->width, _wa->height);

	}

	void init_cairo() {
		cairo_xlib_surface_set_size(_surf, _wa->width, _wa->height);
	}

	void destroy_cairo() {
	}

	~composite_window_t() {
		cairo_surface_destroy(_surf);
		destroy_damage();
	}

	void draw_to(cairo_t * cr, box_int_t const & area) {

		box_int_t clip = area & box_int_t(_wa->x, _wa->y, _wa->width, _wa->height);

		if (clip.w > 0 && clip.h > 0) {
			cairo_save(cr);
			cairo_reset_clip(cr);
			cairo_identity_matrix(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_set_source_surface(cr, _surf, _wa->x, _wa->y);
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
		XRectangle * recs = XShapeGetRectangles(_cnx->dpy, _wid, ShapeBounding,
				&count, &ordering);

		_region.clear();

		if (count > 0) {
			for (int i = 0; i < count; ++i) {
				_region += box_int_t(recs[i]);
			}

			_region.translate(_wa->x, _wa->y);

			/* In doubt */
			XFree(recs);
		} else {
			_region = box_int_t(_wa->x, _wa->y, _wa->width, _wa->height);
		}
	}

	region_t<int> const & get_region() {
		return _region;
	}

	void create_damage() {
		destroy_damage();

		_damage = XDamageCreate(_cnx->dpy, _wid, XDamageReportNonEmpty);
		if (_damage != None) {
			XserverRegion region = XFixesCreateRegion(_cnx->dpy, 0, 0);
			XDamageSubtract(_cnx->dpy, _damage, None, region);
			XFixesDestroyRegion(_cnx->dpy, region);
		}
	}

	void destroy_damage() {
		if (_damage != None) {
			XDamageDestroy(_cnx->dpy, _damage);
			_damage = None;
		}
	}

	void add_damaged_region(region_t<int> const & r) {
		damaged_region += r;
	}

	region_t<int> get_damaged_region() {
		region_t<int> r = damaged_region;
		r.translate(_wa->x, _wa->y);
		r &= _region;
		return r;
	}



	void moved() {
		_has_moved = true;
	}

	void clear_state() {
		damaged_region.clear();
		_has_moved = false;
	}

	bool has_moved() {
		return _has_moved;
	}

	Window get_w() {
		return _wid;
	}

};

}

#endif /* COMPOSITE_WINDOW_HXX_ */
