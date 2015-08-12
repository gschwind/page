/*
 * composite_surface.cxx
 *
 *  Created on: 9 août 2015
 *      Author: gschwind
 */


#include "composite_surface.hxx"

namespace page {

using namespace std;

composite_surface_view_t::composite_surface_view_t(shared_ptr<composite_surface_t> parent) :
	_parent{parent}
{

}

auto composite_surface_view_t::get_pixmap() -> shared_ptr<pixmap_t> {
	return _parent.lock()->get_pixmap();
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

auto composite_surface_view_t::width() -> unsigned{
	return _parent.lock()->width();
}

auto composite_surface_view_t::height() -> unsigned {
	return _parent.lock()->height();
}

void composite_surface_t::create_damage() {
	if (_damage == XCB_NONE) {
		_damage = xcb_generate_id(_dpy->xcb());
		xcb_damage_create(_dpy->xcb(), _damage, _window_id, XCB_DAMAGE_REPORT_LEVEL_NON_EMPTY);
		_damaged += rect(0, 0, _width, _height);
		_pending_damage = true;
	}
}

void composite_surface_t::destroy_damage() {
	if (_damage != XCB_NONE) {
		xcb_damage_destroy(_dpy->xcb(), _damage);
		_damage = XCB_NONE;
	}
}

void composite_surface_t::composite_surface_t::enable_redirect() {
	assert(not _is_composited);
	_is_composited = true;
	_is_freezed = false;
	if(not _pending_unredirect)
		_pending_redirect = true;
	_pending_unredirect = false;
}

void composite_surface_t::disable_redirect() {
	assert(_is_composited);
	_is_composited = false;
	if(not _pending_redirect)
		_pending_unredirect = true;
	_pending_redirect = false;
	_pixmap = nullptr;
}

void composite_surface_t::on_map() {
	_is_map = true;
	_is_freezed = false;
	if(not _pending_resize) { // Do not override resize
		_pending_resize = true;
		_pending_state.width = _width;
		_pending_state.height = _height;
	}
}

void composite_surface_t::on_unmap() {
	_is_map = false;
}

void composite_surface_t::on_resize(int width, int height) {
	if(_is_freezed)
		return;
	_pending_resize = true;
	_pending_state.width = width;
	_pending_state.height = height;
}

void composite_surface_t::on_damage() {
	_pending_damage = true;
}

display_t * composite_surface_t::dpy() {
	return _dpy;
}

xcb_window_t composite_surface_t::wid() {
	return _window_id;
}

void composite_surface_t::add_damaged(region const & r) {
	_damaged += r;
}

int composite_surface_t::depth() {
	return _depth;
}

void composite_surface_t::apply_change() {
	if(_is_destroyed)
		return;

	if(_pending_redirect) {
		_pending_redirect = false;
		xcb_composite_redirect_window(_dpy->xcb(), _window_id, XCB_COMPOSITE_REDIRECT_MANUAL);
	}

	if(_pending_unredirect) {
		_pending_unredirect = false;
		xcb_composite_unredirect_window(_dpy->xcb(), _window_id, XCB_COMPOSITE_REDIRECT_MANUAL);
	}

	/**
	 * lazy update to avoid destroyed windows.
	 **/
	if(_pending_initialize) {
		_pending_initialize = false;
		/** initialize internals **/

		auto ck0 = xcb_get_geometry(_dpy->xcb(), _window_id);
		auto ck1 = xcb_get_window_attributes(_dpy->xcb(), _window_id);

		auto geometry = xcb_get_geometry_reply(_dpy->xcb(), ck0, 0);
		auto attrs = xcb_get_window_attributes_reply(_dpy->xcb(), ck1, 0);

		if (attrs == nullptr or geometry == nullptr) {
			_is_destroyed = true;
			return;
		}

		if(attrs->_class != XCB_WINDOW_CLASS_INPUT_OUTPUT) {
			_is_destroyed = true;
			return;
		}

		_depth = geometry->depth;
		_vis = _dpy->get_visual_type(attrs->visual);

		_pending_resize = true;
		_pending_state.width = geometry->width;
		_pending_state.height = geometry->height;

		_damage = XCB_NONE;
		create_damage();

		_is_map = (attrs->map_state != XCB_MAP_STATE_UNMAPPED);

		free(geometry);
		free(attrs);
	}


	/**
	 * We update the pixmap only if we know that pixmap is valid.
	 *
	 * pixmap is invalid if the window is destroyed or unmapped or
	 * we are not in composited mode.
	 *
	 **/
	if (_pending_resize) {
		if (_is_map and _is_composited and not _is_freezed) {
			_pending_resize = false;
			_width = _pending_state.width;
			_height = _pending_state.height;
			xcb_pixmap_t pixmap_id = xcb_generate_id(_dpy->xcb());
			xcb_composite_name_window_pixmap(_dpy->xcb(), _window_id, pixmap_id);
			_pixmap = make_shared<pixmap_t>(_dpy, _vis, pixmap_id, _width, _height);
			_damaged += rect(0, 0, _width, _height);
		}
	}

}

void composite_surface_t::on_destroy() {
	_is_destroyed = true;
	_damage = XCB_NONE;
}

bool composite_surface_t::is_destroyed() const {
	return _is_destroyed;
}

bool composite_surface_t::is_map() const {
	return _is_map;
}

void composite_surface_t::destroy_pixmap() {
	_pixmap = nullptr;
}

void composite_surface_t::freeze(bool x) {
	_is_freezed = x;
	if(_pixmap != nullptr and _is_freezed) {
		xcb_pixmap_t pix = xcb_generate_id(_dpy->xcb());
		xcb_create_pixmap(_dpy->xcb(), _depth, pix, _dpy->root(), _width, _height);
		auto xpix = std::make_shared<pixmap_t>(_dpy, _vis, pix, _width, _height);

		cairo_surface_t * s = xpix->get_cairo_surface();
		cairo_t * cr = cairo_create(s);
		cairo_set_source_surface(cr, _pixmap->get_cairo_surface(), 0, 0);
		cairo_paint(cr);
		cairo_destroy(cr);

		_pixmap = xpix;
	}
}



void composite_surface_t::start_gathering_damage() {
	if(_pending_damage) {
		/* clear the region */
		xcb_xfixes_set_region(_dpy->xcb(), _region_proxy, 0, 0);
		/* get damaged region and remove them from damaged status */
		xcb_damage_subtract(_dpy->xcb(), _damage, XCB_XFIXES_REGION_NONE, _region_proxy);
		/* get all i_rects for the damaged region */
		ck = xcb_xfixes_fetch_region(_dpy->xcb(), _region_proxy);
	}
}

void composite_surface_t::finish_gathering_damage() {
	if(_pending_damage) {
		_pending_damage = false;
		xcb_generic_error_t * err;
		xcb_xfixes_fetch_region_reply_t * r = xcb_xfixes_fetch_region_reply(_dpy->xcb(), ck, &err);
		if (err == nullptr and r != nullptr) {
			auto iter = xcb_xfixes_fetch_region_rectangles_iterator(r);
			while(iter.rem > 0) {
				//printf("rect %dx%d+%d+%d\n", i.data->width, i.data->height, i.data->x, i.data->y);
				_damaged += rect{iter.data->x, iter.data->y, iter.data->width, iter.data->height};
				xcb_rectangle_next(&iter);
			}
			free(r);
		} else {
			throw exception_t{"Could not fetch region"};
		}
	}

	for(auto & x: _views) {
		x.lock()->_damaged += _damaged;
	}

	_damaged.clear();
}

composite_surface_t::composite_surface_t(display_t * dpy, xcb_window_t w) :
	_dpy{dpy},
	_window_id{w},
	_is_destroyed{false},
	_ref_count{1U},
	_is_composited{false},
	_is_freezed{false},
	_pixmap{nullptr},
	_vis{nullptr},
	_width{0},
	_height{0},
	_depth{0},
	_damage{XCB_NONE},
	_is_map{false},
	_pending_damage{false},
	_pending_others{false},
	_pending_redirect{false},
	_pending_unredirect{false},
	_pending_initialize{true},
	_pending_resize{false}
{
	_region_proxy = xcb_generate_id(_dpy->xcb());
	xcb_xfixes_create_region(_dpy->xcb(), _region_proxy, 0, 0);
}

composite_surface_t::~composite_surface_t() {
	if(_is_composited and not _pending_redirect and not _is_destroyed)
		xcb_composite_unredirect_window(_dpy->xcb(), _window_id, XCB_COMPOSITE_REDIRECT_MANUAL);

	xcb_xfixes_destroy_region(_dpy->xcb(), _region_proxy);
	destroy_damage();
}

shared_ptr<pixmap_t> composite_surface_t::get_pixmap() {
	return _pixmap;
}

void composite_surface_t::clear_damaged() {
	_damaged.clear();
}

region const & composite_surface_t::get_damaged() {
	return _damaged;
}

bool composite_surface_t::has_damage() {
	return not _damaged.empty();
}

unsigned composite_surface_t::width() {
	return _width;
}

unsigned composite_surface_t::height() {
	return _height;
}

auto composite_surface_t::create_view() -> shared_ptr<composite_surface_view_t> {
	auto x = make_shared<composite_surface_view_t>(shared_from_this());
	_views.push_back(x);
	return x;
}

void composite_surface_t::_cleanup_views() {
	_views.remove_if([](weak_ptr<composite_surface_view_t> const & x) { return x.expired(); });
}

bool composite_surface_t::_has_views() {
	_cleanup_views();
	return not _views.empty();
}


}
