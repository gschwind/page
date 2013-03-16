/*
 * managed_window.hxx
 *
 *  Created on: 16 mars 2013
 *      Author: gschwind
 */

#ifndef MANAGED_WINDOW_HXX_
#define MANAGED_WINDOW_HXX_

#include "xconnection.hxx"
#include "window.hxx"

namespace page {

enum managed_window_type_e {
	MANAGED_FLOATING,
	MANAGED_NOTEBOOK,
	MANAGED_FULLSCREEN
};

class managed_window_t {
private:

	managed_window_type_e _type;

	unsigned _margin_top;
	unsigned _margin_bottom;
	unsigned _margin_left;
	unsigned _margin_right;

	window_t * _orig;
	window_t * _base;

	box_int_t _wished_position;
	box_int_t _orig_position;
	box_int_t _base_position;

	cairo_t * _cr;
	cairo_surface_t * _surf;

	/* avoid copy */
	managed_window_t(managed_window_t const &);
	managed_window_t & operator=(managed_window_t const &);

public:


	managed_window_t(managed_window_type_e initial_type, window_t * w, window_t * border);
	virtual ~managed_window_t();

	void normalize();
	void iconify();

	void reconfigure();
	void fake_configure();

	void set_wished_position(box_int_t const & position);
	box_int_t const & get_wished_position() const;

	void delete_window(Time t);

	window_t * get_orig();
	window_t * get_base();

	bool check_orig_position(box_int_t const & position);
	bool check_base_position(box_int_t const & position);

	void set_managed_type(managed_window_type_e type);

	string get_title();

	cairo_t * get_cairo_context();

	void focus();

};

}


#endif /* MANAGED_WINDOW_HXX_ */
