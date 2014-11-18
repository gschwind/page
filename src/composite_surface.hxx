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
#include <X11/extensions/Xcomposite.h>

#include <cassert>

#include <memory>

#include "exception.hxx"
#include "display.hxx"
#include "region.hxx"
#include "pixmap.hxx"

namespace page {

class composite_surface_t {
	display_t * _dpy;
	xcb_visualtype_t * _vis;
	xcb_window_t _window_id;
	xcb_damage_damage_t _damage;
	std::shared_ptr<pixmap_t> _pixmap;
	int _width, _height;
	int _depth;
	region _damaged;

	bool _is_map;

	bool _is_composited;

	void create_damage() {
		if (_damage == XCB_NONE) {
			std::cout << "create damage for " << this << std::endl;
			_damage = xcb_generate_id(_dpy->xcb());
			xcb_damage_create(_dpy->xcb(), _damage, _window_id, XCB_DAMAGE_REPORT_LEVEL_NON_EMPTY);
			_damaged += i_rect{0, 0, _width, _height};
			_damaged += _dpy->read_damaged_region(_damage);
		}
	}

	void destroy_damage() {
		if(_damage != XCB_NONE) {
			std::cout << "destroy damage for " << this << std::endl;
			xcb_damage_destroy(_dpy->xcb(), _damage);
			_damage = XCB_NONE;
		}
	}

public:

	composite_surface_t(display_t * dpy, xcb_window_t w, bool composited) : _dpy(dpy), _window_id(w), _is_composited(composited) {
		xcb_get_geometry_cookie_t ck0 = xcb_get_geometry(_dpy->xcb(), _window_id);
		xcb_get_window_attributes_cookie_t ck1 = xcb_get_window_attributes(_dpy->xcb(), _window_id);

		xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(_dpy->xcb(), ck0, 0);
		xcb_get_window_attributes_reply_t * attrs = xcb_get_window_attributes_reply(_dpy->xcb(), ck1, 0);

		if(attrs == nullptr or geometry == nullptr) {
			throw exception_t("fail to get window attribute");
		}

		_depth = geometry->depth;
		_vis = _dpy->get_visual_type(attrs->visual);
		_width = geometry->width;
		_height = geometry->height;
		_pixmap = nullptr;
		_damage = XCB_NONE;

		_is_map = (attrs->map_state != XCB_MAP_STATE_UNMAPPED);

		printf("create composite_surface (%p) %dx%d\n", this, _width, _height);

		if(_is_map and _is_composited) {
			on_map();
		}

	}

	~composite_surface_t() {
		destroy_damage();
	}

	void on_map() {
		if(_dpy->lock(_window_id)) {
			if(not _dpy->check_for_unmap_window(_window_id)) {
				_is_map = true;
				if (_is_composited) {
					xcb_pixmap_t pixmap_id = xcb_generate_id(_dpy->xcb());
					xcb_composite_name_window_pixmap(_dpy->xcb(), _window_id,
							pixmap_id);
					_pixmap = std::shared_ptr<pixmap_t>(
							new pixmap_t(_dpy, _vis, pixmap_id, _width,
									_height));
					create_damage();
				} else {
					_pixmap = nullptr;
				}
			} else {
				_is_map = false;
			}

			_dpy->unlock();
		}
	}

	void on_unmap() {
		if(_dpy->lock(_window_id)) {
			destroy_damage();
			_is_map = false;
			_dpy->unlock();
		}
	}

	void on_resize(int width, int height) {
		if (width != _width or height != _height) {
			_width = width;
			_height = height;
			if(_is_map) {
				on_map();
			}
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

	int width() {
		return _width;
	}

	int height() {
		return _height;
	}

	int depth() {
		return _depth;
	}

	void set_composited(bool composited) {
		if(composited) {
			if(_dpy->lock(_window_id)) {
				if(not _dpy->check_for_unmap_window(_window_id)) {
					xcb_pixmap_t pixmap_id = xcb_generate_id(_dpy->xcb());
					xcb_composite_name_window_pixmap(_dpy->xcb(), _window_id, pixmap_id);
					_pixmap = std::shared_ptr<pixmap_t>{new pixmap_t(_dpy, _vis, pixmap_id, _width, _height)};
					create_damage();
				}
				_dpy->unlock();
			}
		} else {
			destroy_damage();
			_pixmap = nullptr;
		}
		_is_composited = composited;
	}

};

}


#endif /* COMPOSITE_SURFACE_HXX_ */
