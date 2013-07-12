/*
 * render_context.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef RENDER_CONTEXT_HXX_
#define RENDER_CONTEXT_HXX_


#include "ctime"
#include <cairo.h>
#include <cairo-xlib.h>
#include "xconnection.hxx"
#include "region.hxx"
#include "renderable.hxx"

namespace page {

class compositor_t {
	xconnection_t * _cnx;

	unsigned int flush_count;
	struct timespec last_tic;
	struct timespec curr_tic;

public:

	renderable_list_t list;

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
	compositor_t(xconnection_t * cnx);

	void add_damage_area(region_t<int> const & box);

	static bool z_comp(renderable_t * x, renderable_t * y);
	void render_flush();

	void repair_area(box_int_t const & box);

	void repair_overlay(box_int_t const & area, cairo_surface_t * src);

	void add(renderable_t * x);
	void remove(renderable_t * x);

	void overlay_add(renderable_t * x);
	void overlay_remove(renderable_t * x);

	void draw_box(box_int_t box, double r, double g, double b);

	void repair_buffer(renderable_list_t & visible, cairo_t * cr, box_int_t const & area);

	void move_above(renderable_t * r, renderable_t * above);
	void raise(renderable_t * r);
	void lower(renderable_t * r);

	void damage_all();

	renderable_list_t get_renderable_list();

	virtual void process_event(XEvent const & e);

};

}



#endif /* RENDER_CONTEXT_HXX_ */
