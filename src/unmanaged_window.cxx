/*
 * unmanaged_window.cxx
 *
 *  Created on: 3 août 2013
 *      Author: bg
 */

#include "xconnection.hxx"
#include "unmanaged_window.hxx"

namespace page {

unmanaged_window_t::unmanaged_window_t(xconnection_t * cnx, Window orig, Atom type) :
		orig(orig), _net_wm_type(type), cnx(cnx) {

	cnx->grab();
	cnx->select_input(orig, UNMANAGED_ORIG_WINDOW_EVENT_MASK);
	cnx->ungrab();

}

}
