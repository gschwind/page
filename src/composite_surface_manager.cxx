/*
 * composite_surface_manager.cxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <memory>

#include "composite_surface_manager.hxx"

namespace page {

std::shared_ptr<composite_surface_t> composite_surface_manager_t::get_managed_composite_surface(xcb_window_t w) {

	/** try to find a valid composite surface **/
	auto x = _data.find(w);
	if(x != _data.end()) {
		if(not x->second.expired()) {
			return x->second.lock();
		}
	}

	/** otherwise, create a new one **/
	auto callback = [this](composite_surface_t * p) { this->remove(p); delete p; };
	auto ret = std::shared_ptr<composite_surface_t>{new composite_surface_t{_dpy, w}, callback };
	_data[w] = ret;
	return ret;
}

void composite_surface_manager_t::make_surface_stats(int & size, int & count) {
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

void composite_surface_manager_t::process_event(xcb_generic_event_t const * e) {

	if (e->response_type == XCB_CONFIGURE_NOTIFY) {
		xcb_configure_notify_event_t const * ev =
				reinterpret_cast<xcb_configure_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			std::weak_ptr<composite_surface_t> wp = x->second;
			if (not wp.expired()) {
				wp.lock()->on_resize(ev->width, ev->height);
			}
		}
	} else if (e->response_type == XCB_UNMAP_NOTIFY) {
		xcb_unmap_notify_event_t const * ev =
				reinterpret_cast<xcb_unmap_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			std::weak_ptr<composite_surface_t> wp = x->second;
			if (not wp.expired()) {
				wp.lock()->on_unmap();
			}
		}
	} else if (e->response_type == XCB_MAP_NOTIFY) {
		xcb_map_notify_event_t const * ev =
				reinterpret_cast<xcb_map_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			std::weak_ptr<composite_surface_t> wp = x->second;
			if (not wp.expired()) {
				wp.lock()->on_map();
			}
		}
	} else if (e->response_type == _dpy->damage_event + XCB_DAMAGE_NOTIFY) {
		xcb_damage_notify_event_t const * ev = reinterpret_cast<xcb_damage_notify_event_t const *>(e);
		auto x = _data.find(ev->drawable);
		if (x != _data.end()) {
			std::weak_ptr<composite_surface_t> wp = x->second;
			if (not wp.expired()) {
				wp.lock()->on_damage();
			} else {
				std::cout << "damage received and found expired surface" << std::endl;
			}
		} else {
			std::cout << "damage received but not corresponding surface found" << std::endl;
		}
	}
}



}

