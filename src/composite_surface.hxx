/*
 * composite_surface.hxx
 *
 *  Created on: 13 sept. 2014
 *      Author: gschwind
 */

#ifndef COMPOSITE_SURFACE_HXX_
#define COMPOSITE_SURFACE_HXX_

#include <X11/Xlib.h>
#include <X11/extensions/Xdamage.h>
#ifdef WITH_COMPOSITOR
#include <X11/extensions/Xcomposite.h>
#endif

#include <cassert>

#include <memory>

#include "exception.hxx"
#include "display.hxx"
#include "region.hxx"
#include "pixmap.hxx"

namespace page {

using namespace std;

class composite_surface_t;

/**
 * composite_surface_view_t is handled by client if they keep a view of the
 * composite surface.
 **/
class composite_surface_view_t {
	weak_ptr<composite_surface_t> _parent;
	region _damaged;

	friend class composite_surface_t;

public:

	composite_surface_view_t(shared_ptr<composite_surface_t> parent);

	auto get_pixmap() -> shared_ptr<pixmap_t>;
	void clear_damaged();
	auto get_damaged() -> region const &;
	bool has_damage();
	auto width() -> unsigned;
	auto height() -> unsigned;

};


class composite_surface_t : public enable_shared_from_this<composite_surface_t> {

	friend class composite_surface_manager_t;

	unsigned _ref_count;
	display_t * _dpy;
	xcb_visualtype_t * _vis;
	xcb_window_t _window_id;
	xcb_damage_damage_t _damage;
	shared_ptr<pixmap_t> _pixmap;
	unsigned _width, _height;
	int _depth;
	region _damaged;

	xcb_xfixes_fetch_region_cookie_t ck;
	xcb_xfixes_region_t _region_proxy;

	list<weak_ptr<composite_surface_view_t>> _views;

	struct {
		unsigned width;
		unsigned height;
	} _pending_state;

	bool _pending_others;
	bool _pending_resize;
	bool _pending_unredirect;
	bool _pending_redirect;
	bool _pending_initialize;
	bool _pending_damage;

	bool _is_map;
	bool _is_destroyed;
	bool _is_composited;
	bool _is_freezed;

	void create_damage();
	void destroy_damage();
	void enable_redirect();
	void disable_redirect();
	void on_map();
	void on_unmap();
	void on_resize(int width, int height);
	void on_damage();
	display_t * dpy();
	xcb_window_t wid();
	void add_damaged(region const & r);
	int depth();
	void apply_change();
	void on_destroy();
	bool is_destroyed() const;
	bool is_map() const;
	void destroy_pixmap();
	void freeze(bool x);
	void start_gathering_damage();
	void finish_gathering_damage();

	void _cleanup_views();
	bool _has_views();

public:

	composite_surface_t(display_t * dpy, xcb_window_t w);
	~composite_surface_t();
	auto get_pixmap() -> shared_ptr<pixmap_t>;
	void clear_damaged();
	auto get_damaged() -> region const &;
	bool has_damage();
	auto width() -> unsigned;
	auto height() -> unsigned;

	auto create_view() -> shared_ptr<composite_surface_view_t>;

};

}


#endif /* COMPOSITE_SURFACE_HXX_ */
