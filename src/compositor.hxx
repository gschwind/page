/*
 * render_context.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef RENDER_CONTEXT_HXX_
#define RENDER_CONTEXT_HXX_

#include "config.hxx"

#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <cairo-xcb.h>

#include <memory>
#include <vector>
#include <deque>

#include "display.hxx"
#include "time.hxx"


#include "box.hxx"
#include "region.hxx"
#include "tree.hxx"
#include "pixmap.hxx"

namespace page {

using namespace std;

class compositor_t {

private:

	display_t * _dpy;

	xcb_window_t cm_window;
	xcb_window_t composite_overlay;
	xcb_pixmap_t composite_back_buffer;

	int width;
	int height;

	shared_ptr<atom_handler_t> _A;

	static size_t const _FPS_WINDOWS = 80;
	bool _show_damaged;
	bool _show_opac;
	deque<time64_t> _fps_history;
	deque<double> _damaged_area;
	deque<double> _direct_render_area;

	region _damaged;
	region _workspace_region;
	double _workspace_region_area;

private:

	void repair_damaged_window(xcb_window_t w, region area);
	void repair_moved_window(xcb_window_t w, region from, region to);

	void render_flush();

	void repair_area(rect const & box);
	void repair_overlay(cairo_t * cr, rect const & area, cairo_surface_t * src);

	void init_composite_overlay();
	void release_composite_overlay();
	bool process_check_event();

	void destroy_cairo();
	void init_cairo();

	void repair_area_region(region const & repair);

public:
	//region read_damaged_region(xcb_damage_damage_t d);
	~compositor_t();
	compositor_t(display_t * cnx);

	void update_layout();


	/**
	 * render a tree on screen
	 **/
	void render(tree_t * t);

	void destroy_composite_surface(xcb_window_t w);
	void set_fade_in_time(int nsec);
	void set_fade_out_time(int nsec);
	xcb_window_t get_composite_overlay();

	shared_ptr<pixmap_t> create_screenshot();


	cairo_surface_t * get_front_surface() const;

	xcb_atom_t A(atom_e a) {
		return (*_A)(a);
	}

	bool show_damaged() {
		return _show_damaged;
	}

	bool show_opac() {
		return _show_opac;
	}

	void set_show_damaged(bool b) {
		if(_show_damaged != b)
			_damaged += _workspace_region;
		_show_damaged = b;
	}

	void set_show_opac(bool b) {
		if(_show_opac != b)
			_damaged += _workspace_region;
		_show_opac = b;
	}

	void add_damaged(region const & r) {
		_damaged += r;
	}

	void pango_printf(cairo_t * cr, double x, double y, char const * fmt, ...);

	double get_fps();
	deque<double> const & get_direct_area_history();
	deque<double> const & get_damaged_area_history();

};

}

#endif /* RENDER_CONTEXT_HXX_ */
