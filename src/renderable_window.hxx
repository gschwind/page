/*
 * renderable_window.hxx
 *
 *  Created on: 15 févr. 2013
 *      Author: gschwind
 */

#ifndef RENDERABLE_WINDOW_HXX_
#define RENDERABLE_WINDOW_HXX_

#include <X11/Xlib.h>
#include <set>
#include <map>

#include "xconnection.hxx"
#include "region.hxx"
#include "icon.hxx"
#include "renderable_window.hxx"
#include "window.hxx"

namespace page {

class renderable_window_t {

public:

	Display * _dpy;
	Window wid;
	Visual * visual;

	Damage damage;

	bool _has_alpha;

	int c_class;

	box_int_t const & position;

private:

	bool has_shape;
	region_t<int> _region;

	bool _is_map;
	cairo_surface_t * _surf;
	box_int_t _position;


	/* avoid copy */
	renderable_window_t(renderable_window_t const &);
	renderable_window_t & operator=(renderable_window_t const &);
public:
	renderable_window_t();

	virtual ~renderable_window_t();

	void update(Display * dpy, Window w, XWindowAttributes & wa);

	void repair1(cairo_t * cr, box_int_t const & area);
	bool has_alpha();

	void init_cairo();
	void destroy_cairo();

	void update_map_state(bool is_map);

	void update_position(box_int_t const & position);

	bool is_visible() {
		return _is_map && c_class == InputOutput;
	}

	void read_shape();
	region_t<int> get_region();

};

typedef std::list<window_t *> window_list_t;
typedef std::set<window_t *> window_set_t;

}



#endif /* RENDERABLE_WINDOW_HXX_ */
