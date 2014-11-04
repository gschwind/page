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

#ifdef WITH_PANGO
#include <pango/pango.h>
#include <pango/pangocairo.h>
#endif

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo-xcb.h>

#include <memory>
#include <vector>

#include "display.hxx"
#include "time.hxx"


#include "box.hxx"
#include "region.hxx"
#include "renderable.hxx"
#include "composite_surface_manager.hxx"

namespace page {

class compositor_t {

private:

	display_t * _cnx;
	composite_surface_manager_t * _cmgr;

	xcb_window_t cm_window;
	xcb_window_t composite_overlay;
	xcb_pixmap_t composite_back_buffer;

	int width;
	int height;

	std::shared_ptr<atom_handler_t> _A;

	/* throw compositor_fail_t on compositor already working */
	struct compositor_fail_t { };

	std::vector<std::shared_ptr<renderable_t>> _graph_scene;

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

	region _damaged;
	region _desktop_region;


#ifdef WITH_PANGO
	PangoFontDescription * _fps_font_desc;
	PangoFontMap * _fps_font_map;
	PangoContext * _fps_context;
#endif

private:

	void repair_damaged_window(xcb_window_t w, region area);
	void repair_moved_window(xcb_window_t w, region from, region to);

	void render_flush();

	void repair_area(i_rect const & box);
	void repair_overlay(cairo_t * cr, i_rect const & area, cairo_surface_t * src);

	void init_composite_overlay();
	void release_composite_overlay();
	bool process_check_event();

	void destroy_cairo();
	void init_cairo();

	void repair_area_region(region const & repair);

public:
	//region read_damaged_region(xcb_damage_damage_t d);
	~compositor_t();
	compositor_t(display_t * cnx, composite_surface_manager_t * cmgr);

	void update_layout();
	void render();

	void destroy_composite_surface(xcb_window_t w);
	void set_fade_in_time(int nsec);
	void set_fade_out_time(int nsec);
	xcb_window_t get_composite_overlay();

	void renderable_add(renderable_t * r);
	void renderable_remove(renderable_t * r);
	void renderable_clear();

	xcb_atom_t A(atom_e a) {
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

	void push_back_renderable(std::shared_ptr<renderable_t> r) {
		_graph_scene.push_back(r);
	}

	void push_back_renderable(std::vector<std::shared_ptr<renderable_t>> const & r) {
		_graph_scene.insert(_graph_scene.end(), r.begin(), r.end());
	}

	void pango_printf(cairo_t * cr, double x, double y, char const * fmt, ...);


};

}



#endif /* RENDER_CONTEXT_HXX_ */
