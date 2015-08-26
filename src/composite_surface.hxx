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
#include <X11/extensions/Xcomposite.h>

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
	friend class composite_surface_manager_t;
	friend class composite_surface_t;

	composite_surface_t * _parent;
	region _damaged;

public:

	composite_surface_view_t(composite_surface_t * parent);

	auto get_pixmap() -> shared_ptr<pixmap_t>;
	void clear_damaged();
	auto get_damaged() -> region const &;
	bool has_damage();

};


class composite_surface_t {
	friend class composite_surface_manager_t;
	friend class composite_surface_view_t;

	display_t * _dpy;
	xcb_visualtype_t * _vis;
	xcb_window_t _window_id;
	xcb_damage_damage_t _damage;
	shared_ptr<pixmap_t> _pixmap;
	unsigned _width;
	unsigned _height;
	int _depth;

	list<composite_surface_view_t *> _views;

	bool _need_pixmap_update;
	bool _is_redirected;

	void create_damage();
	void destroy_damage();
	void enable_redirect();
	void disable_redirect();
	void on_map();
	void on_resize(int width, int height);
	void on_damage(xcb_damage_notify_event_t const * ev);
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

	bool _safe_pixmap_update();

	composite_surface_t(display_t * dpy, xcb_window_t w);
	~composite_surface_t();

	auto get_pixmap() -> shared_ptr<pixmap_t>;
	auto create_view() -> composite_surface_view_t *;
	void remove_view(composite_surface_view_t *);

};

}


#endif /* COMPOSITE_SURFACE_HXX_ */
