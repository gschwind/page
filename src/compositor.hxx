/*
 * render_context.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef RENDER_CONTEXT_HXX_
#define RENDER_CONTEXT_HXX_

#include <list>

#include <ctime>
#include <cairo.h>
#include <cairo-xlib.h>

#include "utils.hxx"
#include "xconnection.hxx"
#include "region.hxx"
#include "renderable.hxx"
#include "composite_window.hxx"

#include "time.hxx"

using namespace std;

namespace page {

class compositor_t : public xevent_handler_t {

	static char const * const require_glx_extensions[];

	Display * _dpy;

	Window cm_window;
	Window composite_overlay;
	XdbeBackBuffer composite_back_buffer;

	XWindowAttributes root_attributes;

	/*damage event handler */
	int damage_opcode, damage_event, damage_error;

	/* composite event handler */
	int composite_opcode, composite_event, composite_error;

	/* fixes event handler */
	int fixes_opcode, fixes_event, fixes_error;

	/* xshape extension handler */
	int xshape_opcode, xshape_event, xshape_error;

	/* xrandr extension handler */
	int xrandr_opcode, xrandr_event, xrandr_error;

	/* GLX extension handler */
	int glx_opcode, glx_event, glx_error;

	int (*old_error_handler)(Display * _dpy, XErrorEvent * ev);

	atom_handler_t A;

	/** Performance counter **/
	unsigned int flush_count;
	unsigned int damage_count;
	time_t last_tic;
	time_t curr_tic;
	time_t last_render;

	time_t fade_in_length;
	time_t fade_out_length;

	time_t fade_framerate_limit;

	double cur_t;

	/** clip region for multi monitor setup **/
	region_t<int> _desktop_region;

	region_t<int> _pending_damage;

	/* throw compositor_fail_t on compositor already working */
	struct compositor_fail_t { };

	/** list that track window stack order **/
	std::list<Window> window_stack;

	/**
	 * map do not handle properly objects, it's need copy constructor ...
	 * Use pointer instead.
	 **/
	map<Window, composite_window_t *> window_data;

	/* composite overlay surface */
	cairo_surface_t * _front_buffer;
	/** back buffer, used when composition is needed (i.e. transparency) **/
	cairo_surface_t * _back_buffer;

	/** performance counter **/
	double fast_region_surf_monitor;
	double slow_region_surf_monitor;

	GLXContext glx_ctx;

public:

	enum render_mode_e {
		COMPOSITOR_MODE_AUTO,
		COMPOSITOR_MODE_MANAGED
	};

private:
	render_mode_e render_mode;

	void repair_damaged_window(Window w, region_t<int> area);
	void repair_moved_window(Window w, region_t<int> from, region_t<int> to);


	//static bool z_comp(renderable_t * x, renderable_t * y);
	void render_flush();

	void repair_area(box_t<int> const & box);
	void repair_overlay(cairo_t * cr, box_t<int> const & area, cairo_surface_t * src);
	void repair_buffer(std::list<composite_window_t *> & visible, cairo_t * cr,
			box_t<int> const & area);

	void process_event(XCreateWindowEvent const & e);
	void process_event(XReparentEvent const & e);
	void process_event(XMapEvent const & e);
	void process_event(XUnmapEvent const & e);
	void process_event(XDestroyWindowEvent const & e);
	void process_event(XConfigureEvent const & e);
	void process_event(XCirculateEvent const & e);
	void process_event(XDamageNotifyEvent const & e);

	virtual void process_event(XEvent const & e);

	void scan();

	void update_layout();

	void stack_window_update(Window window, Window above);
	void stack_window_place_on_top(Window w);
	void stack_window_place_on_bottom(Window w);
	void stack_window_remove(Window w);

	region_t<int> read_damaged_region(Damage d);

	bool register_cm(Window w);
	void init_composite_overlay();
	bool process_check_event();

	bool check_glx_context();

	void destroy_cairo();
	void init_cairo();

	void repair_area_region(region_t<int> const & repair);

	bool check_glx_for_extensions(char const * const * ext);



public:

	virtual ~compositor_t();
	compositor_t();

	void process_events();

	int fd() {
		return ConnectionNumber(_dpy);
	}

	void xflush() {
		XFlush(_dpy);
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

};

}



#endif /* RENDER_CONTEXT_HXX_ */
