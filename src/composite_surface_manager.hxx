/*
 * composite_surface_manager.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 *
 * Composite manager manage composite surfaces. Composite surface may be handled after the destruction of
 * the relate window. This manager keep those surfaces up-to-date and enable the access after the destruction
 * of the related window.
 *
 * This manager track the number of use of a given surface using a smart pointer like structure, when
 * all client release the handle the surface is destroyed.
 *
 * Why not simply use smart pointer ?
 *
 * Because you may want recover a released handler while it hasn't been released yet.
 *
 * For example:
 *
 * Client 1 handle (display, window N)
 * Client 2 handle (display, window N)
 * Client 2 release (display, window N)
 * Client 2 request new handle to (display, window N)
 *
 * The manager will give back (display, window N) that Client 1 also handle.
 *
 */

#ifndef COMPOSITE_SURFACE_MANAGER_HXX_
#define COMPOSITE_SURFACE_MANAGER_HXX_

#include <cassert>
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <cairo/cairo-xlib.h>
#include <map>
#include <iostream>
#include <memory>

#include "display.hxx"
#include "pixmap.hxx"
#include "composite_surface.hxx"

using namespace std;

namespace page {

class composite_surface_manager_t {

	typedef pair<Display *, Window> _key_t;
	typedef map<_key_t, shared_ptr<composite_surface_t> > _data_map_t;
	typedef typename _data_map_t::iterator _map_iter_t;

	_data_map_t _data;

private:
	shared_ptr<composite_surface_t> _get_composite_surface(Display * dpy, Window w) {
		_key_t key(dpy, w);
		_map_iter_t x = _data.find(key);
		if(x != _data.end()) {
			return x->second;
		} else {
			shared_ptr<composite_surface_t> h(new composite_surface_t(dpy, w));
			_data[key] = h;
			return _data[key];
		}
	}

	bool _has_composite_surface(Display * dpy, Window w) {
		_key_t key(dpy, w);
		_map_iter_t x = _data.find(key);
		return x != _data.end();
	}

	void _onmap(Display * dpy, Window w) {
		_key_t key(dpy, w);
		auto x = _data.find(key);
		if(x != _data.end()) {
			x->second->onmap();
		}
	}

	void _onresize(Display * dpy, Window w, unsigned width, unsigned heigth) {
		_key_t key(dpy, w);
		auto x = _data.find(key);
		if(x != _data.end()) {
			x->second->onresize(width, heigth);
		}
	}

	void _ondestroy(Display * dpy, Window w) {
		auto x = _data.find(_key_t(dpy, w));
		if(x != _data.end()) {
			_data.erase(x);
		}
	}

public:

	~composite_surface_manager_t() {
		/* sanity check */
		if(not _data.empty()) {
			cout << "WARNING: composite_surface_manager is not empty while destroying." << endl;
		}
	}

	static bool exist(Display * dpy, Window w);
	static void onmap(Display * dpy, Window w);
	static void onresize(Display * dpy, Window w, unsigned width, unsigned heigth);

	static shared_ptr<composite_surface_t> get(Display * dpy, Window w);

	static void ondestroy(Display * dpy, Window w);

};

}



#endif /* COMPOSITE_SURFACE_MANAGER_HXX_ */
