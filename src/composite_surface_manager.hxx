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
	using _data_map_t = std::map<xcb_window_t, std::shared_ptr<composite_surface_t>>;
	using _map_iter_t = _data_map_t::iterator;

	display_t * _dpy;
	_data_map_t _data;

	bool _enabled;

private:

	void  _create_surface(xcb_window_t w);

public:

	~composite_surface_manager_t() {
		/* sanity check */
		if(not _data.empty()) {
			std::cout << "WARNING: composite_surface_manager is not empty while destroying." << std::endl;
		}
	}

	composite_surface_manager_t(display_t * dpy) : _dpy{dpy}, _enabled{true} { }

	void pre_process_event(xcb_generic_event_t const * e);

	/** cleanup unmaped or destroyed window **/
	void apply_updates();

	void register_window(xcb_window_t w);
	void unregister_window(xcb_window_t w);
	void freeze(xcb_window_t w, bool x);

	void make_surface_stats(int & size, int & count);

	void enable();
	void disable();

	std::shared_ptr<pixmap_t> get_last_pixmap(xcb_window_t w);
	region get_damaged(xcb_window_t w);
	void clear_damaged(xcb_window_t w);
};

}



#endif /* COMPOSITE_SURFACE_MANAGER_HXX_ */
