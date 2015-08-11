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

#include "utils.hxx"
#include "display.hxx"
#include "composite_surface.hxx"


namespace page {

using namespace std;

class composite_surface_manager_t {
	using _data_map_t = map<xcb_window_t, weak_ptr<composite_surface_t>>;
	using _data_list_t = list<shared_ptr<composite_surface_t>>;
	using _map_iter_t = _data_map_t::iterator;

	display_t * _dpy;


	/**
	 * list all available surfaces.
	 * Note: xid can be reused after being freed, this mean that some
	 * surface may be in this list but do not have a valid xid anymore.
	 * The following index store the up-to-date xid to surface.
	 **/
	_data_list_t _data;

	/**
	 * map current valid xid to current composite_surface_t
	 **/
	_data_map_t _index;


	bool _enabled;

private:

	auto  _create_surface(xcb_window_t w) -> weak_ptr<composite_surface_t>;

public:

	signal_t<xcb_window_t, bool> on_visibility_change;

	~composite_surface_manager_t() {
		/* sanity check */
		if(not _data.empty()) {
			cout << "WARNING: composite_surface_manager is not empty while destroying." << endl;
		}
	}

	composite_surface_manager_t(display_t * dpy) : _dpy{dpy}, _enabled{true} { }

	void pre_process_event(xcb_generic_event_t const * e);

	/** cleanup unmaped or destroyed window **/
	void apply_updates();

	auto register_window(xcb_window_t w) -> shared_ptr<composite_surface_view_t>;
	void make_surface_stats(int & size, int & count);
	void enable();
	void disable();

};

}



#endif /* COMPOSITE_SURFACE_MANAGER_HXX_ */
