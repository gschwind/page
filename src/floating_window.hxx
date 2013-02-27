/*
 * floating_window.hxx
 *
 *  Created on: 15 févr. 2013
 *      Author: gschwind
 */

#ifndef FLOATING_WINDOW_HXX_
#define FLOATING_WINDOW_HXX_

#include "xconnection.hxx"
#include "window.hxx"

namespace page {

class floating_window_t {
private:
	/* avoid copy */
	floating_window_t(floating_window_t const &);
	floating_window_t & operator=(floating_window_t const &);

public:
	window_t * w;
	window_t * border;

	cairo_t * cr;
	cairo_surface_t * win_surf;

	floating_window_t(window_t * w, Window parent);
	virtual ~floating_window_t();

	void map();
	void unmap();

	void paint();

	void reconfigure(box_int_t const & area);

	void move_resize(box_int_t const & area);

	box_int_t get_position();

};

}

#endif /* FLOATING_WINDOW_HXX_ */
