/*
 * composite_surface.cxx
 *
 *  Created on: 9 août 2015
 *      Author: gschwind
 */

#include <xcb/xcb_event.h>
#include "composite_surface.hxx"

namespace page {

using namespace std;

composite_surface_view_t::composite_surface_view_t(composite_surface_t * parent) :
	_parent{parent}
{
	_damaged += region(0, 0, _parent->_width, _parent->_height);
}

auto composite_surface_view_t::get_pixmap() -> shared_ptr<pixmap_t> {
	return _parent->get_pixmap();
}

void composite_surface_view_t::clear_damaged() {
	_damaged.clear();
}

auto composite_surface_view_t::get_damaged() -> region const & {
	return _damaged;
}

bool composite_surface_view_t::has_damage() {
	return not _damaged.empty();
}

void composite_surface_t::create_damage() {
	if (_damage == XCB_NONE) {
		_damage = xcb_generate_id(_dpy->xcb());
		xcb_damage_create(_dpy->xcb(), _damage, _window_id, XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);
		add_damaged(region(0, 0, _width, _height));
	}
}

void composite_surface_t::destroy_damage() {
	if (_damage != XCB_NONE) {
		xcb_damage_destroy(_dpy->xcb(), _damage);
		_damage = XCB_NONE;
	}
}

void composite_surface_t::enable_redirect() {
	_is_redirected = true;
	xcb_composite_redirect_window(_dpy->xcb(), _window_id, XCB_COMPOSITE_REDIRECT_MANUAL);
}

void composite_surface_t::disable_redirect() {
	_is_redirected = false;
	xcb_composite_unredirect_window(_dpy->xcb(), _window_id, XCB_COMPOSITE_REDIRECT_MANUAL);
	_pixmap = nullptr;
}

void composite_surface_t::on_map() {
	_need_pixmap_update = true;
}

void composite_surface_t::on_resize(int width, int height) {
	if (width != _width or height != _height) {
		_need_pixmap_update = true;
		_width = width;
		_height = height;
	}
}

void composite_surface_t::on_damage(xcb_damage_notify_event_t const * ev) {
	for(auto x: _views) {
		x->_damaged += ev->area;
	}
}

display_t * composite_surface_t::dpy() {
	return _dpy;
}

xcb_window_t composite_surface_t::wid() {
	return _window_id;
}

void composite_surface_t::add_damaged(region const & r) {
	for(auto x: _views) {
		x->_damaged += r;
	}
}

int composite_surface_t::depth() {
	return _depth;
}

bool composite_surface_t::_safe_pixmap_update() {
	xcb_pixmap_t pixmap_id = xcb_generate_id(_dpy->xcb());
	xcb_void_cookie_t ck = xcb_composite_name_window_pixmap_checked(_dpy->xcb(), _window_id, pixmap_id);
	auto err = xcb_request_check(_dpy->xcb(), ck);
	if(err != nullptr) {
		cout << "INFO: could not get pixmap : " << xcb_event_get_error_label(err->error_code) << endl;
		xcb_discard_reply(_dpy->xcb(), ck.sequence);
		return false;
	} else {
		_pixmap = make_shared<pixmap_t>(_dpy, _vis, pixmap_id, _width, _height);
		xcb_discard_reply(_dpy->xcb(), ck.sequence);
		return true;
	}
}

void composite_surface_t::destroy_pixmap() {
	_pixmap = nullptr;
}

void composite_surface_t::freeze(bool x) {
//	_is_freezed = x;
//	if(_pixmap != nullptr and _is_freezed) {
//		xcb_pixmap_t pix = xcb_generate_id(_dpy->xcb());
//		xcb_create_pixmap(_dpy->xcb(), _depth, pix, _dpy->root(), _width, _height);
//		auto xpix = std::make_shared<pixmap_t>(_dpy, _vis, pix, _width, _height);
//
//		cairo_surface_t * s = xpix->get_cairo_surface();
//		cairo_t * cr = cairo_create(s);
//		cairo_set_source_surface(cr, _pixmap->get_cairo_surface(), 0, 0);
//		cairo_paint(cr);
//		cairo_destroy(cr);
//
//		_pixmap = xpix;
//	}
}


composite_surface_t::composite_surface_t(display_t * dpy, xcb_window_t w) :
	_dpy{dpy},
	_window_id{w},
	_pixmap{nullptr},
	_vis{nullptr},
	_width{0},
	_height{0},
	_depth{0},
	_damage{XCB_NONE}
{

	auto ck0 = xcb_get_geometry(_dpy->xcb(), _window_id);
	auto ck1 = xcb_get_window_attributes(_dpy->xcb(), _window_id);

	auto geometry = xcb_get_geometry_reply(_dpy->xcb(), ck0, 0);
	auto attrs = xcb_get_window_attributes_reply(_dpy->xcb(), ck1, 0);

	if (attrs == nullptr or geometry == nullptr) {
		return;
	}

	if(attrs->_class != XCB_WINDOW_CLASS_INPUT_OUTPUT) {
		return;
	}

	_depth = geometry->depth;
	_vis = _dpy->get_visual_type(attrs->visual);

	_need_pixmap_update= true;
	_width = geometry->width;
	_height = geometry->height;

	create_damage();

	free(geometry);
	free(attrs);
}

composite_surface_t::~composite_surface_t() {
	if(_is_redirected)
		xcb_composite_unredirect_window(_dpy->xcb(), _window_id, XCB_COMPOSITE_REDIRECT_MANUAL);
	destroy_damage();
}

shared_ptr<pixmap_t> composite_surface_t::get_pixmap() {

	if(_need_pixmap_update) {
		_need_pixmap_update = false;
		_safe_pixmap_update();
	}

	return _pixmap;
}

auto composite_surface_t::create_view() -> composite_surface_view_t * {
	auto x = new composite_surface_view_t{this};
	_views.push_back(x);
	return x;
}

void composite_surface_t::remove_view(composite_surface_view_t * v) {
	_views.remove(v);
}

bool composite_surface_t::_has_views() {
	return not _views.empty();
}


}
