/*
 * render_context.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef RENDER_CONTEXT_HXX_
#define RENDER_CONTEXT_HXX_

#include <cairo.h>
#include <cairo-xlib.h>
#include "xconnection.hxx"
#include "region.hxx"
#include "renderable.hxx"

namespace page {

class render_context_t {

	class xevent_handler_t : public ::page::xevent_handler_t {
		render_context_t & rnd;
		xevent_handler_t(render_context_t & rnd) : rnd(rnd) { }
		virtual ~xevent_handler_t() { }
		virtual void process_event(XEvent const & e);
	};

	xconnection_t & _cnx;


public:
	renderable_list_t list;

	/* composite overlay surface (front buffer) */
	cairo_surface_t * composite_overlay_s;
	/* composite overlay cairo context */
	cairo_t * composite_overlay_cr;

	cairo_t * pre_back_buffer_cr;
	cairo_surface_t * pre_back_buffer_s;

	/* damaged region */
	region_t<int> pending_damage;

	render_context_t(xconnection_t & cnx);
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

	renderable_list_t get_renderable_list();

};

}



#endif /* RENDER_CONTEXT_HXX_ */
