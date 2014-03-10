/*
 * window_handler.hxx
 *
 *  Created on: 8 mars 2014
 *      Author: gschwind
 */

#ifndef WINDOW_HANDLER_HXX_
#define WINDOW_HANDLER_HXX_

#include <list>
#include "managed_window.hxx"

namespace page {

struct window_handler_t {

	xconnection_t * cnx;
	Window id;

	XWindowAttributes wa;

	bool has_net_wm_type;
	std::list<Atom> net_wm_type;

	bool has_transient_for;
	Window transient_for;

	managed_window_t * mw;
	unmanaged_window_t * uw;



	window_handler_t(xconnection_t * c, Window w) : net_wm_type() {
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
