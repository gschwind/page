/*
 * unmanaged_window.cxx
 *
 *  Created on: 3 août 2013
 *      Author: bg
 */

#include "xconnection.hxx"
#include "unmanaged_window.hxx"

namespace page {

unmanaged_window_t::unmanaged_window_t(window_t * orig, Atom type) :
		orig(orig), _net_wm_type(type) {

	cnx = &orig->cnx();

	cnx->grab();
	orig->mark_durty_all();
	cnx->select_input(orig->id, UNMANAGED_ORIG_WINDOW_EVENT_MASK);
	cnx->ungrab();

}

}
