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
	typedef map<_key_t, weak_ptr<composite_surface_t> > _data_map_t;
	typedef typename _data_map_t::iterator _map_iter_t;

	_data_map_t _data;

private:
	shared_ptr<composite_surface_t> _get_composite_surface(Display * dpy, Window w) {
		weak_ptr<composite_surface_t> & wp = _data[_key_t(dpy, w)];
		/** do not put into local if () then { } **/
		shared_ptr<composite_surface_t> h;
		if(wp.expired()) {
			h = shared_ptr<composite_surface_t>(new composite_surface_t(dpy, w));
			wp = h;
		}
		return wp.lock();
	}

	bool _has_composite_surface(Display * dpy, Window w) {
		return not _data[_key_t(dpy, w)].expired();
	}

	void _onmap(Display * dpy, Window w) {
		weak_ptr<composite_surface_t> wp = _data[_key_t(dpy, w)];
		if(not wp.expired()) {
			wp.lock()->onmap();
		}
	}

	void _onresize(Display * dpy, Window w, unsigned width, unsigned heigth) {
		weak_ptr<composite_surface_t> wp = _data[_key_t(dpy, w)];
		if(not wp.expired()) {
			wp.lock()->onresize(width, heigth);
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
	static void ondestroy(Display * dpy, Window w);

	static shared_ptr<composite_surface_t> get(Display * dpy, Window w);


};

}



#endif /* COMPOSITE_SURFACE_MANAGER_HXX_ */
