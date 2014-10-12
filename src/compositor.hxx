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

#include <memory>
#include <list>
#include <cassert>

#include <ctime>
#include <cairo.h>
#include <cairo-xlib.h>

#ifdef WITH_PANGO
#include <pango/pangocairo.h>
#endif


#include "smart_pointer.hxx"
#include "utils.hxx"
#include "display.hxx"
#include "region.hxx"
#include "renderable.hxx"
#include "composite_window.hxx"
#include "composite_surface_manager.hxx"

#include "time.hxx"

using namespace std;

namespace page {

class compositor_t {

private:

	display_t * _cnx;

	Window cm_window;
	Window composite_overlay;
	XdbeBackBuffer composite_back_buffer;

	XWindowAttributes root_attributes;

	/*damage event handler */
	int damage_event;
	/* xshape extension handler */
	int xshape_event;
	/* xrandr extension handler */
	int xrandr_event;

	shared_ptr<atom_handler_t> _A;

	/* throw compositor_fail_t on compositor already working */
	struct compositor_fail_t { };

	vector<ptr<renderable_t>> _graph_scene;

	static int const _FPS_WINDOWS = 80;
	int _fps_top;
	page::time_t _fps_history[_FPS_WINDOWS];
	bool _show_fps;
	bool _show_damaged;
	bool _show_opac;
	int _damaged_area[_FPS_WINDOWS];
	int _direct_render_area[_FPS_WINDOWS];
	int _debug_x;
	int _debug_y;

	bool _need_render;

	unsigned _missed_forecast;
	unsigned _forecast_count;

	map<Window, Damage> _damage_event;

	region _damaged;
	region _desktop_region;

	int back_buffer_state;


#ifdef WITH_PANGO
	PangoFontDescription * _fps_font_desc;
	PangoFontMap * _fps_font_map;
	PangoContext * _fps_context;
#endif

private:

	void repair_damaged_window(Window w, region area);
	void repair_moved_window(Window w, region from, region to);

	void render_flush();

	void repair_area(i_rect const & box);
	void repair_overlay(cairo_t * cr, i_rect const & area, cairo_surface_t * src);

	void process_event(XCreateWindowEvent const & e);
	void process_event(XReparentEvent const & e);
	void process_event(XMapEvent const & e);
	void process_event(XUnmapEvent const & e);
	void process_event(XDestroyWindowEvent const & e);
	void process_event(XConfigureEvent const & e);
	void process_event(XCirculateEvent const & e);
	void process_event(XDamageNotifyEvent const & e);

public:
	virtual void process_event(XEvent const & e);

private:
	void scan();

	void update_layout();


	void init_composite_overlay();
	void release_composite_overlay();
	bool process_check_event();

	void destroy_cairo();
	void init_cairo();

	void repair_area_region(region const & repair);

	void cleanup_internal_data();

public:
	region read_damaged_region(Damage d);
	virtual ~compositor_t();
	compositor_t(display_t * cnx, int damage_event, int xshape_event, int xrandr_event);

	void process_events();

	int fd() {
		return _cnx->fd();
	}

	void xflush() {
		XFlush(_cnx->dpy());
	}

	void render();

	void destroy_composite_surface(Window w);
	void create_damage(Window w, XWindowAttributes & wa);
	void destroy_damage(Window w);
	void set_fade_in_time(int nsec);
	void set_fade_out_time(int nsec);
	Window get_composite_overlay();

	void renderable_add(renderable_t * r);
	void renderable_remove(renderable_t * r);
	void renderable_clear();

	Atom A(atom_e a) {
		return (*_A)(a);
	}

	bool show_fps() {
		return _show_fps;
	}

	void set_show_fps(bool b) {
		if(_show_fps != b)
			_damaged += _desktop_region;
		_show_fps = b;
	}

	bool show_damaged() {
		return _show_damaged;
	}

	bool show_opac() {
		return _show_opac;
	}

	void set_show_damaged(bool b) {
		if(_show_damaged != b)
			_damaged += _desktop_region;
		_show_damaged = b;
	}

	void set_show_opac(bool b) {
		if(_show_opac != b)
			_damaged += _desktop_region;
		_show_opac = b;
	}

	void need_render() {
		_need_render = true;
	}

	void add_damaged(region const & r) {
		_damaged += r;
	}

	void clear_renderable() {
		_graph_scene.clear();
	}

	void push_back_renderable(ptr<renderable_t> r) {
		_graph_scene.push_back(r);
	}

	void push_back_renderable(vector<ptr<renderable_t>> const & r) {
		_graph_scene += r;
	}

	void pango_printf(cairo_t * cr, double x, double y, char const * fmt, ...);


};

}



#endif /* RENDER_CONTEXT_HXX_ */
