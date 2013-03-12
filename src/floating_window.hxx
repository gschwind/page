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

	box_int_t _wished_position;

	/* avoid copy */
	floating_window_t(floating_window_t const &);
	floating_window_t & operator=(floating_window_t const &);

public:
	window_t * w;
	window_t * border;

	cairo_t * cr;
	cairo_surface_t * win_surf;



	floating_window_t(window_t * w, window_t * border);
	virtual ~floating_window_t();

	void normalize();
	void iconify();

	void reconfigure();
	void fake_configure();

	void set_wished_position(box_int_t const & position);
	box_int_t const & get_wished_position() const;

};

}

#endif /* FLOATING_WINDOW_HXX_ */
