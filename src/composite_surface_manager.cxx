/*
 * composite_surface_manager.cxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "composite_surface_manager.hxx"

namespace page {

static composite_surface_manager_t _mngr;

composite_surface_handler_t composite_surface_manager_t::get(Display * dpy, Window w) {
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


}

