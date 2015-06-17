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

/**
 * pre-process event aim to update the pixmap of a client and damage status.
 **/
void composite_surface_manager_t::pre_process_event(xcb_generic_event_t const * e) {

	if (e->response_type == XCB_CONFIGURE_NOTIFY) {
		xcb_configure_notify_event_t const * ev =
				reinterpret_cast<xcb_configure_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			x->second->on_resize(ev->width, ev->height);
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
	} else 	if (e->response_type == XCB_DESTROY_NOTIFY) {
		xcb_destroy_notify_event_t const * ev = reinterpret_cast<xcb_destroy_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			x->second->on_destroy();
		}
	} else if (e->response_type == XCB_UNMAP_NOTIFY) {
		xcb_unmap_notify_event_t const * ev =
				reinterpret_cast<xcb_unmap_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			x->second->on_unmap();
		}
	} else if (e->response_type == 0x80|XCB_UNMAP_NOTIFY) {
		xcb_unmap_notify_event_t const * ev =
				reinterpret_cast<xcb_unmap_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			x->second->on_unmap();
		}
	}
}

void composite_surface_manager_t::apply_updates() {

	for(auto s: _data) {
		s.second->update_pixmap();

		/* send all damage request */
		s.second->start_gathering_damage();
	}

	for(auto s: _data) {

		/* get all damage reply */
		s.second->finish_gathering_damage();
	}

}


void composite_surface_manager_t::enable() {
	_enabled = true;
	for(auto x: _data) {
		x.second->enable_composited();
	}
}

void composite_surface_manager_t::disable() {
	_enabled = false;
	for(auto x: _data) {
		x.second->disable_composite();
	}
}

void composite_surface_manager_t::register_window(xcb_window_t w) {
	/** try to find a valid composite surface **/
	auto x = _data.find(w);
	if (x != _data.end()) {
		x->second->incr_ref();
	} else {
		return _create_surface(w);
	}
}

void composite_surface_manager_t::unregister_window(xcb_window_t w) {
	/** try to find a valid composite surface **/
	auto x = _data.find(w);
	if (x != _data.end()) {
		x->second->decr_ref();
		if(x->second->ref_count() == 0) {
			_data.erase(x);
		}
	}
}

std::shared_ptr<pixmap_t> composite_surface_manager_t::get_last_pixmap(xcb_window_t w) {
	if(not _enabled)
		return nullptr;

	/** try to find a valid composite surface **/
	auto x = _data.find(w);
	if (x != _data.end()) {
		return x->second->get_pixmap();
	}

	return nullptr;

}

region composite_surface_manager_t::get_damaged(xcb_window_t w) {
	if(not _enabled)
		return region{};

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

void composite_surface_manager_t::_create_surface(xcb_window_t w) {
	_data[w] = std::make_shared<composite_surface_t>(_dpy, w, _enabled);
}

void composite_surface_manager_t::freeze(xcb_window_t w, bool b) {
	auto x = _data.find(w);
	if (x != _data.end()) {
		x->second->freeze(b);
	}
}


}

