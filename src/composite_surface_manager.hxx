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


#include <memory>
#include <map>
#include <utility>
#include <iostream>

#include "display.hxx"
#include "composite_surface.hxx"


namespace page {

class composite_surface_manager_t {

	typedef std::pair<Display *, Window> _key_t;
	typedef std::map<_key_t, std::weak_ptr<composite_surface_t>> _data_map_t;
	typedef typename _data_map_t::iterator _map_iter_t;

	_data_map_t _data;

private:
	std::shared_ptr<composite_surface_t> _get_composite_surface(Display * dpy, Window w) {
		std::weak_ptr<composite_surface_t> & wp = _data[_key_t(dpy, w)];
		/** do not put into local if () then { } **/
		std::shared_ptr<composite_surface_t> h;
		if(wp.expired()) {
			h = std::shared_ptr<composite_surface_t>(new composite_surface_t(dpy, w));
			wp = h;
		}
		return wp.lock();
	}

	bool _has_composite_surface(Display * dpy, Window w) {
		return not _data[_key_t(dpy, w)].expired();
	}

	void _onmap(Display * dpy, Window w) {
		std::weak_ptr<composite_surface_t> wp = _data[_key_t(dpy, w)];
		if(not wp.expired()) {
			wp.lock()->onmap();
		}
	}

	void _onresize(Display * dpy, Window w, unsigned width, unsigned heigth) {
		std::weak_ptr<composite_surface_t> wp = _data[_key_t(dpy, w)];
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

	std::weak_ptr<composite_surface_t> _get_weak_surface(Display * dpy, Window w) {
		return _data[_key_t(dpy, w)];
	}

	void _make_surface_stats(int & size, int & count) {
		size = 0;
		count = 0;
		for(auto &i: _data) {
			if(not i.second.expired()) {
				count += 1;
				auto x = i.second.lock();
				size += x->depth()/8 * x->width() * x->height();
			}
		}
	}

public:

	~composite_surface_manager_t() {
		/* sanity check */
		if(not _data.empty()) {
			std::cout << "WARNING: composite_surface_manager is not empty while destroying." << std::endl;
		}
	}

	static bool exist(Display * dpy, Window w);
	static void onmap(Display * dpy, Window w);
	static void onresize(Display * dpy, Window w, unsigned width, unsigned heigth);
	static void ondestroy(Display * dpy, Window w);

	static std::shared_ptr<composite_surface_t> get(Display * dpy, Window w);
	static std::weak_ptr<composite_surface_t> get_weak_surface(Display * dpy, Window w);

	static void make_surface_stats(int & size, int & count);

};

}



#endif /* COMPOSITE_SURFACE_MANAGER_HXX_ */
