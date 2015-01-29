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
	}
}

void composite_surface_manager_t::cleanup() {

	/** remove destroyed windows **/
	for(auto i = _data.begin(); i != _data.end(); ) {
		if(i->second->is_destroyed()) {
			i = _data.erase(i);
		} else {
			++i;
		}
	}

	/** destroy obsolete pixmap **/
	for(auto & i : _data) {
		if(not i.second->is_map()) {
			i.second->destroy_pixmap();
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

std::shared_ptr<pixmap_t> composite_surface_manager_t::get_last_pixmap(xcb_window_t w) {
	/** try to find a valid composite surface **/
	auto x = _data.find(w);
	if (x != _data.end()) {
		return x->second->get_pixmap();
	} else {
		return _create_surface(w);
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

std::shared_ptr<pixmap_t>  composite_surface_manager_t::_create_surface(xcb_window_t w) {
	if (_dpy->lock(w)) {

		xcb_get_geometry_cookie_t ck0 = xcb_get_geometry(_dpy->xcb(),
				w);
		xcb_get_window_attributes_cookie_t ck1 = xcb_get_window_attributes(
				_dpy->xcb(), w);

		xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(
				_dpy->xcb(), ck0, 0);
		xcb_get_window_attributes_reply_t * attrs =
				xcb_get_window_attributes_reply(_dpy->xcb(), ck1, 0);

		if (attrs == nullptr or geometry == nullptr) {
			return nullptr;
		}

		if(attrs->_class != XCB_WINDOW_CLASS_INPUT_OUTPUT)
			return nullptr;

		/** otherwise, create a new one **/
		auto ret = std::shared_ptr<composite_surface_t> {
				new composite_surface_t { _dpy, w, geometry, attrs, _enabled } };
		_data[w] = ret;
		_dpy->unlock();
		return ret->get_pixmap();
	}

	return nullptr;
}



}

