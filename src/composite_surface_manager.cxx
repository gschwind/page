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

void composite_surface_manager_t::make_surface_stats(int & size, int & count) {
	size = 0;
	count = 0;
	for (auto &i : _data) {
		if (i.second->get_pixmap() != nullptr) {
			count += 1;
			auto x = i.second;
			size += x->depth() / 8 * x->width() * x->height();
		}
	}
}

void composite_surface_manager_t::process_event(xcb_generic_event_t const * e) {

	if (e->response_type == XCB_CONFIGURE_NOTIFY) {
		xcb_configure_notify_event_t const * ev =
				reinterpret_cast<xcb_configure_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			x->second->on_resize(ev->width, ev->height);
		}
	} else if (e->response_type == XCB_UNMAP_NOTIFY) {
		xcb_unmap_notify_event_t const * ev =
				reinterpret_cast<xcb_unmap_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			x->second->on_unmap();
		}
	} else if (e->response_type == XCB_MAP_NOTIFY) {
		xcb_map_notify_event_t const * ev =
				reinterpret_cast<xcb_map_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			x->second->on_map();
		}
	} else if (e->response_type == _dpy->damage_event + XCB_DAMAGE_NOTIFY) {
		xcb_damage_notify_event_t const * ev = reinterpret_cast<xcb_damage_notify_event_t const *>(e);
		auto x = _data.find(ev->drawable);
		if (x != _data.end()) {
			x->second->on_damage();
		} else {
			std::cout << "damage received but not corresponding surface found" << std::endl;
		}
	} else if (e->response_type == XCB_DESTROY_NOTIFY) {
		xcb_destroy_notify_event_t const * ev = reinterpret_cast<xcb_destroy_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			x->second->on_destroy();
			if(x->second->keep_composite_surface_count() == 0 and x->second->is_destroyed()) {
				_data.erase(x);
			}
		}
	}
}

void composite_surface_manager_t::enable() {
	_enabled = true;
	for(auto i: _data) {
		i.second->set_composited(true);
	}
}

void composite_surface_manager_t::disable() {
	_enabled = false;
	for(auto i: _data) {
		i.second->set_composited(false);
	}
}

void composite_surface_manager_t::keep_composite_surface(xcb_window_t w) {

	/** try to find a valid composite surface **/
	auto x = _data.find(w);
	if (x != _data.end()) {
		x->second->keep_composite_surface_count_incr();
		return;
	}

	/** otherwise, create a new one **/
	auto ret = std::shared_ptr<composite_surface_t> {new composite_surface_t {_dpy, w, _enabled}};
	ret->keep_composite_surface_count_incr();
	_data[w] = ret;

}

void composite_surface_manager_t::trash_composite_surface(xcb_window_t w) {

	/** try to find a valid composite surface **/
	auto x = _data.find(w);
	if (x != _data.end()) {
		x->second->keep_composite_surface_count_decr();

		if(x->second->keep_composite_surface_count() == 0 and x->second->is_destroyed()) {
			_data.erase(x);
		}

		return;
	}

	/** otherwise, create a new one **/
	auto ret = std::shared_ptr<composite_surface_t> {new composite_surface_t {_dpy, w, _enabled}};
	ret->keep_composite_surface_count_incr();
	_data[w] = ret;

}

std::shared_ptr<pixmap_t> composite_surface_manager_t::get_last_pixmap(xcb_window_t w) {
	/** try to find a valid composite surface **/
	auto x = _data.find(w);
	if (x != _data.end()) {
		return x->second->get_pixmap();
	}

	return nullptr;

}

region composite_surface_manager_t::get_damaged(xcb_window_t w) {
	/** try to find a valid composite surface **/
	auto x = _data.find(w);
	if (x != _data.end()) {
		return x->second->get_damaged();
	}

	return region{};

}

void composite_surface_manager_t::clear_damaged(xcb_window_t w) {
	/** try to find a valid composite surface **/
	auto x = _data.find(w);
	if (x != _data.end()) {
		x->second->clear_damaged();
	}
}

}

