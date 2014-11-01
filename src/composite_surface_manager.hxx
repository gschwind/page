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
	typedef std::map<xcb_window_t, std::weak_ptr<composite_surface_t>> _data_map_t;
	typedef typename _data_map_t::iterator _map_iter_t;

	display_t * _dpy;
	_data_map_t _data;

private:

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

	composite_surface_manager_t(display_t * dpy) : _dpy(dpy) { }

	void process_event(xcb_generic_event_t const * e);

	std::shared_ptr<composite_surface_t> get_managed_composite_surface(xcb_window_t w);
	std::weak_ptr<composite_surface_t> get_weak_surface(xcb_window_t w);

	void make_surface_stats(int & size, int & count);




};

}



#endif /* COMPOSITE_SURFACE_MANAGER_HXX_ */
