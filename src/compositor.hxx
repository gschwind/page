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
#include "xconnection.hxx"
#include "region.hxx"
#include "renderable.hxx"
#include "renderable_window.hxx"

using namespace std;

namespace page {

class compositor_t : public xevent_handler_t {

	Window cm_window;
	xconnection_t _cnx;

	unsigned int flush_count;
	struct timespec last_tic;
	struct timespec curr_tic;

	region_t<int> _desktop_region;

public:

	/* throw compositor_fail_t on compositor already working */
	struct compositor_fail_t { };
	std::list<Window> window_stack;


	/**
	 * map do not handle properly objects, it's need copy constructor ...
	 * Use pointer instead.
	 **/
	map<Window, renderable_window_t *> window_data;

	/* composite overlay surface (front buffer) */
	cairo_surface_t * composite_overlay_s;
	/* composite overlay cairo context */
	cairo_t * composite_overlay_cr;

	cairo_t * pre_back_buffer_cr;
	cairo_surface_t * pre_back_buffer_s;

	double fast_region_surf;
	double slow_region_surf;

	/* damaged region */
	region_t<int> pending_damage;

	virtual ~compositor_t() { };
	compositor_t();

	void add_damage_area(region_t<int> const & box);

	//static bool z_comp(renderable_t * x, renderable_t * y);
	void render_flush();

	void repair_area(box_int_t const & box);

	void repair_overlay(box_int_t const & area, cairo_surface_t * src);

	void draw_box(box_int_t box, double r, double g, double b);

	void repair_buffer(std::list<renderable_window_t *> & visible, cairo_t * cr, box_int_t const & area);

	void damage_all();

	void process_event(XCreateWindowEvent const & e);
	void process_event(XReparentEvent const & e);
	void process_event(XMapEvent const & e);
	void process_event(XUnmapEvent const & e);
	void process_event(XDestroyWindowEvent const & e);
	void process_event(XConfigureEvent const & e);
	void process_event(XCirculateEvent const & e);
	void process_event(XDamageNotifyEvent const & e);

	virtual void process_event(XEvent const & e);

	renderable_window_t * find_window(Window w);

	void try_add_window(Window w);
	void try_remove_window(Window w);

	void scan();

	inline int get_connection_fd() {
		return _cnx.connection_fd;
	}

	inline void process_events() {
		while(_cnx.process_check_event())
			continue;
		render_flush();
	}

	inline void xflush() {
		XFlush(_cnx.dpy);
	}

	void update_layout();

};

}



#endif /* RENDER_CONTEXT_HXX_ */
