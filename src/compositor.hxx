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
#include "composite_window.hxx"

using namespace std;

namespace page {

class compositor_t : public xevent_handler_t {

	/** short cut **/
	typedef region_t<int> _region_t;
	typedef box_t<int>	_box_t;


	Window cm_window;
	xconnection_t * _cnx;

	/** Performance counter **/
	unsigned int flush_count;
	unsigned int damage_count;
	struct timespec last_tic;
	struct timespec curr_tic;

	/** clip region for multi monitor setup **/
	region_t<int> _desktop_region;

	/* throw compositor_fail_t on compositor already working */
	struct compositor_fail_t { };

	/** list that track window stack order **/
	std::list<Window> window_stack;

	map<Window, region_t<int> > compositor_window_state;

	/**
	 * map do not handle properly objects, it's need copy constructor ...
	 * Use pointer instead.
	 **/
	map<Window, composite_window_t *> window_data;

	/* composite overlay surface */
	cairo_surface_t * _front_buffer;
	/** back buffer, used when composition is needed (i.e. transparency) **/
	cairo_surface_t * _bask_buffer;

	/** performance counter **/
	double fast_region_surf_monitor;
	double slow_region_surf_monitor;

	/* damaged region */
	_region_t pending_damage;

	//void add_damage_area(_region_t const & box);

	void repair_damaged_window(Window w, region_t<int> area);
	void repair_moved_window(Window w, region_t<int> from, region_t<int> to);


	//static bool z_comp(renderable_t * x, renderable_t * y);
	void render_flush();

	void repair_area(_box_t const & box);
	void repair_overlay(cairo_t * cr, _box_t const & area, cairo_surface_t * src);
	void repair_buffer(std::list<composite_window_t *> & visible, cairo_t * cr,
			_box_t const & area);

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

	void register_window(Window w);
	void unregister_window(Window w);

	void scan();

	void update_layout();

	void stack_window_update(Window window, Window above);
	void stack_window_place_on_top(Window w);
	void stack_window_place_on_bottom(Window w);
	void stack_window_remove(Window w);

	void save_state();

	region_t<int> read_damaged_region(Damage d);


	void compute_pending_damage();

public:

	virtual ~compositor_t();
	compositor_t();

	void process_events();

	inline int get_connection_fd() {
		return _cnx->fd();
	}

	inline void xflush() {
		XFlush(_cnx->dpy);
	}

};

}



#endif /* RENDER_CONTEXT_HXX_ */
