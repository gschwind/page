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

static composite_surface_manager_t _mngr;

std::shared_ptr<composite_surface_t> composite_surface_manager_t::get(Display * dpy, Window w) {
	return _mngr._get_composite_surface(dpy, w);
}

bool composite_surface_manager_t::exist(Display * dpy, Window w) {
	return _mngr._has_composite_surface(dpy, w);
}

void composite_surface_manager_t::onmap(Display * dpy, Window w) {
	_mngr._onmap(dpy, w);
}

void composite_surface_manager_t::onresize(Display * dpy, Window w, unsigned width, unsigned heigth) {
	_mngr._onresize(dpy, w, width, heigth);
}

void composite_surface_manager_t::ondestroy(Display * dpy, Window w) {
	_mngr._ondestroy(dpy, w);
}

std::weak_ptr<composite_surface_t> composite_surface_manager_t::get_weak_surface(Display * dpy, Window w) {
	return _mngr._get_weak_surface(dpy, w);
}

void composite_surface_manager_t::make_surface_stats(int & size, int & count) {
	_mngr._make_surface_stats(size, count);
}


}

