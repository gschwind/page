/*
 * unmanaged_window.hxx
 *
 *  Created on: 3 août 2013
 *      Author: bg
 */

#ifndef UNMANAGED_WINDOW_HXX_
#define UNMANAGED_WINDOW_HXX_

#include <X11/X.h>
#include "window.hxx"

namespace page {

class unmanaged_window_t {
private:

	static unsigned long const UNMANAGED_ORIG_WINDOW_EVENT_MASK =
			StructureNotifyMask | PropertyChangeMask;

	xconnection_t * cnx;

	Atom _net_wm_type;

	/* avoid copy */
	unmanaged_window_t(unmanaged_window_t const &);
	unmanaged_window_t & operator=(unmanaged_window_t const &);

public:
	window_t * const orig;

	unmanaged_window_t(window_t * orig, Atom type);
	~unmanaged_window_t() { }

	Atom net_wm_type() {
		return _net_wm_type;
	}

};

}

#endif /* UNMANAGED_WINDOW_HXX_ */
