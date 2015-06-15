/*
 * composite_surface.hxx
 *
 *  Created on: 13 sept. 2014
 *      Author: gschwind
 */

#ifndef COMPOSITE_SURFACE_HXX_
#define COMPOSITE_SURFACE_HXX_

#include <X11/Xlib.h>
#include <X11/extensions/Xdamage.h>
#ifdef WITH_COMPOSITOR
#include <X11/extensions/Xcomposite.h>
#endif

#include <cassert>

#include <memory>

#include "exception.hxx"
#include "display.hxx"
#include "region.hxx"
#include "pixmap.hxx"

namespace page {

class composite_surface_t {
	unsigned _ref_count;
	display_t * _dpy;
	xcb_visualtype_t * _vis;
	xcb_window_t _window_id;
	xcb_damage_damage_t _damage;
	std::shared_ptr<pixmap_t> _pixmap;
	unsigned _px_width, _px_height;
	unsigned _width, _height;
	int _depth;
	region _damaged;

	bool _is_map;
	bool _is_destroyed;
	bool _is_composited;
	bool _is_freezed;

	void create_damage() {
		if (_damage == XCB_NONE) {
			_damage = xcb_generate_id(_dpy->xcb());
			xcb_damage_create(_dpy->xcb(), _damage, _window_id, XCB_DAMAGE_REPORT_LEVEL_NON_EMPTY);
			_damaged += i_rect(0, 0, _width, _height);
			_damaged += _dpy->read_damaged_region(_damage);
		}
	}

	void destroy_damage() {
		if (_dpy->lock(_window_id)) {
			if (_damage != XCB_NONE) {
				xcb_damage_destroy(_dpy->xcb(), _damage);
				_damage = XCB_NONE;
			}
			_dpy->unlock();
		}
	}

public:

	composite_surface_t(
			display_t * dpy,
			xcb_window_t w,
			xcb_get_geometry_reply_t * geometry,
			xcb_get_window_attributes_reply_t * attrs,
			bool composited) :
		_dpy(dpy),
		_window_id(w),
		_is_destroyed(false),
		_ref_count{1U},
		_is_composited{composited},
		_is_freezed{false}
	{
		if (_dpy->lock(_window_id)) {

			_depth = geometry->depth;
			_vis = _dpy->get_visual_type(attrs->visual);
			_width = geometry->width;
			_height = geometry->height;
			_pixmap = nullptr;
			_damage = XCB_NONE;
			create_damage();

			_is_map = (attrs->map_state != XCB_MAP_STATE_UNMAPPED);

			if (_is_map) {
				on_map();
			}

			_dpy->unlock();

		}
	}

	~composite_surface_t() {
		destroy_damage();
	}

	void on_map() {
		_is_map = true;
		_is_freezed = false;
		update_pixmap();
	}

	void on_unmap() {
		_is_map = false;
	}

	void on_resize(int width, int height) {
		if (width != _width or height != _height) {
			_width = width;
			_height = height;
			update_pixmap();
		}
	}

	void on_damage() {
		if(_dpy->lock(_window_id)) {
			_damaged += _dpy->read_damaged_region(_damage);
			_dpy->unlock();
		}
	}

	display_t * dpy() {
		return _dpy;
	}

	xcb_window_t wid() {
		return _window_id;
	}

	std::shared_ptr<pixmap_t> get_pixmap() {
		return _pixmap;
	}

	void clear_damaged() {
		_damaged.clear();
	}

	region const & get_damaged() {
		return _damaged;
	}

	void add_damaged(region const & r) {
		_damaged += r;
	}

	unsigned width() {
		return _px_width;
	}

	unsigned height() {
		return _px_height;
	}

	int depth() {
		return _depth;
	}

	void update_pixmap() {
		/**
		 * if we already know that the window is destroyed or
		 * unmapped, return immediately.
		 **/
		if (_is_destroyed or not _is_map or not _is_composited or _is_freezed)
			return;
		/**
		 * lock the window and check if it will be destroyed soon
		 **/
		if (_dpy->lock(_window_id)) {
			/** check if the window will be unmapped soon **/
			if (not _dpy->check_for_unmap_window(_window_id)) {
				_px_width = _width;
				_px_height = _height;
				xcb_pixmap_t pixmap_id = xcb_generate_id(_dpy->xcb());
				xcb_composite_name_window_pixmap(_dpy->xcb(), _window_id,
						pixmap_id);
				_pixmap = std::shared_ptr<pixmap_t>(
						new pixmap_t(_dpy, _vis, pixmap_id, _width,
								_height));
				_damaged += i_rect ( 0, 0, _width, _height );
				_dpy->read_damaged_region(_damage);
			}
			_dpy->unlock();
		}
	}

	void on_destroy() {
		_is_destroyed = true;
	}

	bool is_destroyed() const {
		return _is_destroyed;
	}

	bool is_map() const {
		return _is_map;
	}

	void destroy_pixmap() {
		_pixmap = nullptr;
	}

	void incr_ref() {
		++_ref_count;
	}

	void decr_ref() {
		--_ref_count;
	}

	unsigned ref_count() {
		return _ref_count;
	}

	void disable_composite() {
		_is_composited = false;
		destroy_pixmap();
	}

	void enable_composited() {
		_is_composited = true;
		_is_freezed = false;
		update_pixmap();
	}

	void freeze(bool x) {
		_is_freezed = x;
		if(_pixmap != nullptr) {
			xcb_pixmap_t pix = xcb_generate_id(_dpy->xcb());
			xcb_create_pixmap(_dpy->xcb(), _depth, pix, _dpy->root(), _px_width, _px_height);
			auto xpix = std::make_shared<pixmap_t>(_dpy, _vis, pix, _px_width, _px_height);

			cairo_surface_t * s = xpix->get_cairo_surface();
			cairo_t * cr = cairo_create(s);
			cairo_set_source_surface(cr, _pixmap->get_cairo_surface(), 0, 0);
			cairo_paint(cr);
			cairo_destroy(cr);

			_pixmap = xpix;
		}
	}

};

}


#endif /* COMPOSITE_SURFACE_HXX_ */
