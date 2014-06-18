/*
 * unmanaged_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef UNMANAGED_WINDOW_HXX_
#define UNMANAGED_WINDOW_HXX_

#include <X11/X.h>

#include "composite_surface_manager.hxx"
#include "client_base.hxx"
#include "display.hxx"

namespace page {

class unmanaged_window_t : public client_base_t {
private:

	static unsigned long const UNMANAGED_ORIG_WINDOW_EVENT_MASK =
	StructureNotifyMask | PropertyChangeMask;

	Atom _net_wm_type;

	composite_surface_handler_t surf;

	/* avoid copy */
	unmanaged_window_t(unmanaged_window_t const &);
	unmanaged_window_t & operator=(unmanaged_window_t const &);

public:

	unmanaged_window_t(Atom type, client_base_t * c) : client_base_t(*c),
			 _net_wm_type(type) {
		_cnx->grab();
		XSelectInput(_cnx->dpy(), _id, UNMANAGED_ORIG_WINDOW_EVENT_MASK);
		_cnx->ungrab();

		surf = composite_surface_manager_t::get(_cnx->dpy(), base());

	}

	~unmanaged_window_t() {
		XSelectInput(_cnx->dpy(), _id, NoEventMask);
	}

	Atom net_wm_type() {
		return _net_wm_type;
	}

	virtual bool has_window(Window w) {
		return w == _id;
	}

	void map() {
		_cnx->map_window(_id);
	}

	virtual string get_node_name() const {
		string s = _get_node_name<'U'>();
		ostringstream oss;
		oss << s << " " << orig();
		return oss.str();
	}

	virtual void render(cairo_t * cr, time_t time) {
		if (surf != nullptr) {

			if (type() == A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)) {
				cairo_save(cr);

				unsigned const int _shadow_width = 4;

				cairo_reset_clip(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_rectangle(cr, wa.x - _shadow_width, wa.y, _shadow_width, wa.height);
				cairo_clip(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
				cairo_pattern_t * grad0 = cairo_pattern_create_linear(wa.x - _shadow_width, 0.0, wa.x, 0.0);
				cairo_pattern_add_color_stop_rgba(grad0, 0.0, 0.0, 0.0, 0.0, 0.0);
				cairo_pattern_add_color_stop_rgba(grad0, 1.0, 0.0, 0.0, 0.0, 0.2);
				cairo_mask(cr, grad0);
				cairo_pattern_destroy(grad0);

				cairo_reset_clip(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_rectangle(cr, wa.x + wa.width, wa.y, _shadow_width, wa.height);
				cairo_clip(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
				cairo_pattern_t * grad1 = cairo_pattern_create_linear(wa.x + wa.width, 0.0, wa.x + wa.width + _shadow_width, 0.0);
				cairo_pattern_add_color_stop_rgba(grad1, 1.0, 0.0, 0.0, 0.0, 0.0);
				cairo_pattern_add_color_stop_rgba(grad1, 0.0, 0.0, 0.0, 0.0, 0.2);
				cairo_mask(cr, grad1);
				cairo_pattern_destroy(grad1);

				cairo_reset_clip(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_rectangle(cr, wa.x, wa.y - _shadow_width, wa.width, _shadow_width);
				cairo_clip(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
				cairo_pattern_t * grad2 = cairo_pattern_create_linear(0.0, wa.y - _shadow_width, 0.0, wa.y);
				cairo_pattern_add_color_stop_rgba(grad2, 0.0, 0.0, 0.0, 0.0, 0.0);
				cairo_pattern_add_color_stop_rgba(grad2, 1.0, 0.0, 0.0, 0.0, 0.2);
				cairo_mask(cr, grad2);
				cairo_pattern_destroy(grad2);

				cairo_reset_clip(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_rectangle(cr, wa.x, wa.y + wa.height, wa.width, _shadow_width);
				cairo_clip(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
				cairo_pattern_t * grad3 = cairo_pattern_create_linear(0.0, wa.y + wa.height, 0.0, wa.y + wa.height + _shadow_width);
				cairo_pattern_add_color_stop_rgba(grad3, 1.0, 0.0, 0.0, 0.0, 0.0);
				cairo_pattern_add_color_stop_rgba(grad3, 0.0, 0.0, 0.0, 0.0, 0.2);
				cairo_mask(cr, grad3);
				cairo_pattern_destroy(grad3);


				cairo_reset_clip(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_rectangle(cr, wa.x - _shadow_width, wa.y - _shadow_width, _shadow_width, _shadow_width);
				cairo_clip(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
				cairo_pattern_t * r0grad = cairo_pattern_create_radial(wa.x, wa.y, 0.0, wa.x, wa.y, _shadow_width);
				cairo_pattern_add_color_stop_rgba(r0grad, 0.0, 0.0, 0.0, 0.0, 0.2);
				cairo_pattern_add_color_stop_rgba(r0grad, 1.0, 0.0, 0.0, 0.0, 0.0);
				cairo_mask(cr, r0grad);
				cairo_pattern_destroy(r0grad);

				cairo_reset_clip(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_rectangle(cr, wa.x + wa.width, wa.y - _shadow_width, _shadow_width, _shadow_width);
				cairo_clip(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
				cairo_pattern_t * r1grad = cairo_pattern_create_radial(wa.x + wa.width, wa.y, 0.0, wa.x + wa.width, wa.y, _shadow_width);
				cairo_pattern_add_color_stop_rgba(r1grad, 0.0, 0.0, 0.0, 0.0, 0.2);
				cairo_pattern_add_color_stop_rgba(r1grad, 1.0, 0.0, 0.0, 0.0, 0.0);
				cairo_mask(cr, r1grad);
				cairo_pattern_destroy(r1grad);

				cairo_reset_clip(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_rectangle(cr, wa.x - _shadow_width, wa.y + wa.height, _shadow_width, _shadow_width);
				cairo_clip(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
				cairo_pattern_t * r2grad = cairo_pattern_create_radial(wa.x, wa.y + wa.height, 0.0, wa.x, wa.y + wa.height, _shadow_width);
				cairo_pattern_add_color_stop_rgba(r2grad, 0.0, 0.0, 0.0, 0.0, 0.2);
				cairo_pattern_add_color_stop_rgba(r2grad, 1.0, 0.0, 0.0, 0.0, 0.0);
				cairo_mask(cr, r2grad);
				cairo_pattern_destroy(r2grad);

				cairo_reset_clip(cr);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_rectangle(cr, wa.x + wa.width, wa.y + wa.height, _shadow_width, _shadow_width);
				cairo_clip(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
				cairo_pattern_t * r3grad = cairo_pattern_create_radial(wa.x + wa.width, wa.y + wa.height, 0.0, wa.x + wa.width, wa.y + wa.height, _shadow_width);
				cairo_pattern_add_color_stop_rgba(r3grad, 0.0, 0.0, 0.0, 0.0, 0.2);
				cairo_pattern_add_color_stop_rgba(r3grad, 1.0, 0.0, 0.0, 0.0, 0.0);
				cairo_mask(cr, r3grad);
				cairo_pattern_destroy(r3grad);

				cairo_restore(cr);
			}

			shared_ptr<pixmap_t>  p = surf->get_pixmap();
			if (p != nullptr) {
				cairo_surface_t * s = p->get_cairo_surface();
				display_t::create_context(__FILE__, __LINE__);
				cairo_save(cr);
				cairo_set_source_surface(cr, s, wa.x, wa.y);
				cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
				cairo_rectangle(cr, wa.x, wa.y, wa.width, wa.height);
				cairo_fill(cr);
				display_t::destroy_context(__FILE__, __LINE__);
				cairo_restore(cr);
			}
		}

		for(auto i: childs()) {
			i->render(cr, time);
		}

	}


};

}

#endif /* UNMANAGED_WINDOW_HXX_ */
