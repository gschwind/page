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

#include <memory>
#include <list>
#include <cassert>

#include <ctime>
#include <cairo.h>
#include <cairo-xlib.h>

#include "smart_pointer.hxx"
#include "utils.hxx"
#include "display.hxx"
#include "region.hxx"
#include "renderable.hxx"
#include "composite_window.hxx"
#include "composite_surface.hxx"

#include "time.hxx"

using namespace std;

namespace page {

class compositor_t {

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

	/** Performance counter **/
	unsigned int flush_count;
	unsigned int damage_count;

	time_t fade_in_length;
	time_t fade_out_length;

	double cur_t;

	/** clip region for multi monitor setup **/
	region _desktop_region;

	region _pending_damage;

	/* throw compositor_fail_t on compositor already working */
	struct compositor_fail_t { };

	/** list that track window stack order **/
	std::list<Window> window_stack;

	/**
	 * map do not handle properly objects, it's need copy constructor ...
	 * Use pointer instead.
	 **/
	map<Window, composite_window_t *> window_data;

	map<Window, p_composite_surface_t> window_to_composite_surface;

	map<Window, Damage> damage_map;

	/** back buffer, used when composition is needed (i.e. transparency) **/
	cairo_surface_t * _back_buffer;

	/** performance counter **/
	double fast_region_surf_monitor;
	double slow_region_surf_monitor;

	GLXContext glx_ctx;

	renderable_list_t _graph_scene;

public:

	enum render_mode_e {
		COMPOSITOR_MODE_AUTO,
		COMPOSITOR_MODE_MANAGED
	};

private:
	render_mode_e render_mode;

	void repair_damaged_window(Window w, region area);
	void repair_moved_window(Window w, region from, region to);


	//static bool z_comp(renderable_t * x, renderable_t * y);
	void render_flush();

	void repair_area(rectangle const & box);
	void repair_overlay(cairo_t * cr, rectangle const & area, cairo_surface_t * src);
	void repair_buffer(std::list<composite_window_t *> & visible, cairo_t * cr,
			rectangle const & area);

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

	void stack_window_update(Window window, Window above);
	void stack_window_place_on_top(Window w);
	void stack_window_place_on_bottom(Window w);
	void stack_window_remove(Window w);

	region read_damaged_region(Damage d);
	void init_composite_overlay();
	void release_composite_overlay();
	bool process_check_event();

	bool check_glx_context();

	void destroy_cairo();
	void init_cairo();

	void repair_area_region(region const & repair);

	bool check_glx_for_extensions(char const * const * ext);



public:

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
	void render_auto();
	void render_managed();

	void set_render_mode(render_mode_e mode) {
		render_mode = mode;
	}

	render_mode_e get_render_mode() {
		return render_mode;
	}

	p_composite_surface_t get_composite_surface(Window w,
			XWindowAttributes const & wa) {
		assert(w != None);
		assert(wa.c_class == InputOutput);

		auto i = window_to_composite_surface.find(w);
		if (i != window_to_composite_surface.end()) {
			return i->second;
		} else {
			//printf("number of surfaces = %lu\n", window_to_composite_surface.size());
			p_composite_surface_t x(new composite_surface_t(_cnx->dpy(), w, wa));
			window_to_composite_surface[w] = x;
			return x;
		}
	}

	void destroy_composite_surface(Window w) {
		auto i = window_to_composite_surface.find(w);
		if (i != window_to_composite_surface.end()) {
			printf("number of surfaces = %lu\n",
					window_to_composite_surface.size());
			cout << "try to destroy " << i->first << " with "
					<< i->second.use_count() << endl;
			window_to_composite_surface.erase(i);
		}
	}

	void create_damage(Window w, XWindowAttributes & wa) {
		assert(wa.c_class == InputOutput);
		assert(w != None);

		Damage damage = XDamageCreate(_cnx->dpy(), w, XDamageReportNonEmpty);
		if (damage != None) {
			damage_map[w] = damage;
			XserverRegion region = XFixesCreateRegion(_cnx->dpy(), 0, 0);
			XDamageSubtract(_cnx->dpy(), damage, None, region);
			XFixesDestroyRegion(_cnx->dpy(), region);
		}
	}

	void destroy_damage(Window w) {
		map<Window, Damage>::iterator x = damage_map.find(w);
		if (x != damage_map.end()) {
			XDamageDestroy(_cnx->dpy(), x->second);
			damage_map.erase(x);
		}
	}


	void set_fade_in_time(int nsec) {
		fade_in_length = nsec;
	}

	void set_fade_out_time(int nsec) {
		fade_out_length = nsec;
	}

	Window get_composite_overlay() {
		return composite_overlay;
	}

	void add_render(renderable_t * r) {
		_graph_scene.push_back(r);
	}

	void clear() {
		for(auto &i : window_data) {
			delete i.second;
		}

		window_to_composite_surface.clear();

	}

	Atom A(atom_e a) {
		return (*_A)(a);
	}


};

}



#endif /* RENDER_CONTEXT_HXX_ */
