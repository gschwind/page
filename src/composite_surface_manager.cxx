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
		if (i.second->_pixmap != nullptr) {
			count += 1;
			size += (i.second->depth() / 8) * i.second->_width * i.second->_height;
		}
	}
}

/**
 * pre-process event aim to update the pixmap of a client and damage status.
 **/
void composite_surface_manager_t::pre_process_event(xcb_generic_event_t const * e) {
	if (e->response_type == XCB_CONFIGURE_NOTIFY) {
		auto ev = reinterpret_cast<xcb_configure_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			x->second->on_resize(ev->width, ev->height);
		}
	} else if (e->response_type == XCB_MAP_NOTIFY) {
		auto ev = reinterpret_cast<xcb_map_notify_event_t const *>(e);
		auto x = _data.find(ev->window);
		if (x != _data.end()) {
			x->second->on_map();
		}
	} else if (e->response_type == _dpy->damage_event + XCB_DAMAGE_NOTIFY) {
		auto ev = reinterpret_cast<xcb_damage_notify_event_t const *>(e);
		auto x = _data.find(ev->drawable);
		if (x != _data.end()) {
			x->second->on_damage(ev);
		}
	}
}

void composite_surface_manager_t::enable() {
	if (not _enabled) {
		_enabled = true;
		for (auto const & x : _data) {
			x.second->enable_redirect();
		}
	}
}

void composite_surface_manager_t::disable() {
	if (_enabled) {
		_enabled = false;
		for (auto const & x : _data) {
			x.second->disable_redirect();
		}
	}
}

auto composite_surface_manager_t::create_view(xcb_window_t w) -> composite_surface_view_t * {
	/** try to find a valid composite surface **/
	auto x = _data.find(w);
	if (x == _data.end()) {
		_create_surface(w);
	}

	return _data[w]->create_view();

}

void composite_surface_manager_t::destroy_view(composite_surface_view_t * v) {
	/* same behavior than delete */
	if(v == nullptr)
		return;

	auto p = v->_parent;
	p->remove_view(v);
	delete v;

	if(p->_views.empty()) {
		_data.erase(p->_window_id);
		on_visibility_change.signal(p->_window_id, false);
		delete p;
	}

}

auto composite_surface_manager_t::_create_surface(xcb_window_t w) -> composite_surface_t * {
	auto x = new composite_surface_t{_dpy, w};
	if(_enabled)
		x->enable_redirect();
	_data[w] = x;

	on_visibility_change.signal(x->wid(), true);
	return x;
}

}

