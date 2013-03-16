/*
 * tab_window.hxx
 *
 *  Created on: 9 mars 2013
 *      Author: gschwind
 */

#ifndef TAB_WINDOW_HXX_
#define TAB_WINDOW_HXX_


#include "xconnection.hxx"
#include "window.hxx"
#include "managed_window.hxx"

namespace page {

/*
 * Some client want to be reparented ...
 *
 * So reparent all windows.
 *
 */

class tab_window_t {
private:

	box_int_t _wished_position;

	/* avoid copy */
	tab_window_t(tab_window_t const &);
	tab_window_t & operator=(tab_window_t const &);

public:
	window_t * w;
	window_t * border;

	cairo_t * cr;
	cairo_surface_t * win_surf;

	tab_window_t(window_t * w, window_t * border);
	virtual ~tab_window_t();

	void reconfigure();
	void fake_configure();

	box_int_t const & get_wished_position();
	void set_wished_position(box_int_t const & position);

	void iconify();
	void normalize();

};

typedef std::list<managed_window_t *> tab_window_list_t;
typedef std::set<managed_window_t *> tab_window_set_t;


}

#endif /* TAB_WINDOW_HXX_ */
