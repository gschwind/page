/*
 * window_handler.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef WINDOW_HANDLER_HXX_
#define WINDOW_HANDLER_HXX_

#include <list>
#include "managed_window.hxx"

namespace page {

struct window_handler_t {

	display_t * cnx;
	Window id;

	XWindowAttributes wa;

	bool has_net_wm_type;
	std::list<Atom> net_wm_type;

	bool has_transient_for;
	Window transient_for;

	managed_window_t * mw;
	unmanaged_window_t * uw;



	window_handler_t(display_t * c, Window w) : net_wm_type() {
		cnx = c;
		id = w;
		has_net_wm_type = false;
		transient_for = None;
		has_transient_for = false;
		mw = 0;
		uw = 0;
		bzero(&wa, sizeof(wa));
	}




};


}



#endif /* WINDOW_HANDLER_HXX_ */
