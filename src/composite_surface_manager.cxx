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
		if (i->get_pixmap() != nullptr) {
			count += 1;
			size += (i->depth() / 8) * i->width() * i->height();
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
		auto x = _index.find(ev->window);
		if (x != _index.end()) {
			x->second.lock()->on_resize(ev->width, ev->height);
		}
	} else if (e->response_type == XCB_MAP_NOTIFY) {
		xcb_map_notify_event_t const * ev =
				reinterpret_cast<xcb_map_notify_event_t const *>(e);
		auto x = _index.find(ev->window);
		if (x != _index.end()) {
			x->second.lock()->on_map();
		}
	} else if (e->response_type == _dpy->damage_event + XCB_DAMAGE_NOTIFY) {
		xcb_damage_notify_event_t const * ev = reinterpret_cast<xcb_damage_notify_event_t const *>(e);
		auto x = _index.find(ev->drawable);
		if (x != _index.end()) {
			x->second.lock()->on_damage();
		} else {
			std::cout << "damage received but not corresponding surface found" << std::endl;
		}
	} else 	if (e->response_type == XCB_DESTROY_NOTIFY) {
		xcb_destroy_notify_event_t const * ev = reinterpret_cast<xcb_destroy_notify_event_t const *>(e);
		auto x = _index.find(ev->window);
		if (x != _index.end()) {
			x->second.lock()->on_destroy();
			/**
			 * Immediately remove it from list of valid surface
			 * to avoid miss application of some events
			 **/
			_index.erase(x);
		}
	} else if (e->response_type == XCB_UNMAP_NOTIFY) {
		xcb_unmap_notify_event_t const * ev =
				reinterpret_cast<xcb_unmap_notify_event_t const *>(e);
		auto x = _index.find(ev->window);
		if (x != _index.end()) {
			x->second.lock()->on_unmap();
		}
	} else if (e->response_type == 0x80|XCB_UNMAP_NOTIFY) {
		xcb_unmap_notify_event_t const * ev =
				reinterpret_cast<xcb_unmap_notify_event_t const *>(e);
		auto x = _index.find(ev->window);
		if (x != _index.end()) {
			x->second.lock()->on_unmap();
		}
	}
}

void composite_surface_manager_t::apply_updates() {

	{
		/* remove obsolete references */
		auto i = _data.begin();
		while(i != _data.end()) {
			if((*i)->ref_count() <= 0) {
				i = _data.erase(i);
			} else {
				(*i)->apply_change();
				++i;
			}
		}
	}

	{
		/** cleanup index **/
		auto i = _index.begin();
		while(i != _index.end()) {
			if(i->second.expired()) {
				i = _index.erase(i);
			} else {
				++i;
			}
		}
	}

	/**
	 * for optimization we choose to gather damage here
	 **/
	for(auto const & s: _data) {
		/* send all damage request */
		s->start_gathering_damage();
	}

	for(auto const & s: _data) {
		/* get all damage reply */
		s->finish_gathering_damage();
	}

}


void composite_surface_manager_t::enable() {
	if (not _enabled) {
		_enabled = true;
		for (auto const & x : _data) {
			x->enable_redirect();
		}
	}
}

void composite_surface_manager_t::disable() {
	if (_enabled) {
		_enabled = false;
		for (auto const & x : _data) {
			x->disable_redirect();
		}
	}
}

auto composite_surface_manager_t::register_window(xcb_window_t w) -> weak_ptr<composite_surface_t> {
	/** try to find a valid composite surface **/
	auto x = _index.find(w);
	if (x != _index.end()) {
		x->second.lock()->incr_ref();
		return x->second;
	} else {
		return _create_surface(w);
	}
}

void composite_surface_manager_t::register_window(weak_ptr<composite_surface_t> w) {
	assert(not w.expired());
	w.lock()->incr_ref();
}

void composite_surface_manager_t::unregister_window(weak_ptr<composite_surface_t> w) {
	assert(not w.expired());
	assert(w.lock()->ref_count() > 0);
	/**
	 * We just decrement reference here. We will cleanup on apply_updates()
	 * This make register/unregister more versatile.
	 **/

	w.lock()->decr_ref();

}

auto composite_surface_manager_t::_create_surface(xcb_window_t w) -> weak_ptr<composite_surface_t> {
	auto x = make_shared<composite_surface_t>(_dpy, w);
	if(_enabled)
		x->enable_redirect();
	_data.push_back(x);
	_index[w] = x;
	return x;
}

}

