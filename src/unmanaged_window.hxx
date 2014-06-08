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
		return _get_node_name<'U'>();
	}

	virtual void render(cairo_t * cr, time_t time) {
		if (surf != nullptr) {
			cairo_surface_t * s = surf->get_surf();

			cairo_save(cr);
			cairo_set_source_surface(cr, s, wa.x, wa.y);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_rectangle(cr, wa.x, wa.y, wa.width, wa.height);
			cairo_fill(cr);
			cairo_restore(cr);

		}

		for(auto i: childs()) {
			i->render(cr, time);
		}

	}


};

}

#endif /* UNMANAGED_WINDOW_HXX_ */
