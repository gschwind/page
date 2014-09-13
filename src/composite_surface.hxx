/*
 * composite_surface.hxx
 *
 *  Created on: 13 sept. 2014
 *      Author: gschwind
 */

#ifndef COMPOSITE_SURFACE_HXX_
#define COMPOSITE_SURFACE_HXX_

#include "pixmap.hxx"

namespace page {

class composite_surface_t {

	Display * _dpy;
	Visual * _vis;
	Window _window_id;
	Damage _damage;
	shared_ptr<pixmap_t> _pixmap;
	int _width, _height;

	void create_damage(Window w) {
		assert(w != None);

		if (_damage == None) {
			_damage = XDamageCreate(_dpy, w, XDamageReportNonEmpty);
			if (_damage != None) {
				XserverRegion region = XFixesCreateRegion(_dpy, 0, 0);
				XDamageSubtract(_dpy, _damage, None, region);
				XFixesDestroyRegion(_dpy, region);
			}
		}
	}

	void destroy_damage(Window w) {
		if(_damage != None) {
			XDamageDestroy(_dpy, _damage);
			_damage = None;
		}
	}

public:

	composite_surface_t(Display * dpy, Window w) {
		XWindowAttributes wa;
		if(XGetWindowAttributes(dpy, w, &wa) == 0) {
			throw 0;
		}

		assert(wa.c_class != InputOnly);

		_window_id = w;
		_dpy = dpy;
		_vis = wa.visual;
		_width = wa.width;
		_height = wa.height;
		_pixmap = nullptr;

		create_damage(w);

	}

	~composite_surface_t() {
		destroy_damage(_window_id);
	}

	void onmap() {
		Pixmap pixmap_id = XCompositeNameWindowPixmap(_dpy, _window_id);
		if(pixmap_id == None) {
			printf("invalid pixmap creation\n");
		} else {
			_pixmap = shared_ptr<pixmap_t>(new pixmap_t(_dpy, _vis, pixmap_id, _width, _height));
		}
		destroy_damage(_window_id);
		create_damage(_window_id);
	}

	void onresize(int width, int height) {
		if (width != _width or height != _height) {
			_width = width;
			_height = height;
			onmap();
		}
	}

	Display * dpy() {
		return _dpy;
	}

	Window wid() {
		return _window_id;
	}

	shared_ptr<pixmap_t> get_pixmap() {
		return _pixmap;
	}



};

}


#endif /* COMPOSITE_SURFACE_HXX_ */
