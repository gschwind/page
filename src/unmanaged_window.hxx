/*
 * unmanaged_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef UNMANAGED_WINDOW_HXX_
#define UNMANAGED_WINDOW_HXX_

#include <X11/X.h>

#include "client_base.hxx"
#include "xconnection.hxx"

namespace page {

class unmanaged_window_t : public client_base_t {
private:

	static unsigned long const UNMANAGED_ORIG_WINDOW_EVENT_MASK =
	StructureNotifyMask | PropertyChangeMask;

	Atom _net_wm_type;

	/* avoid copy */
	unmanaged_window_t(unmanaged_window_t const &);
	unmanaged_window_t & operator=(unmanaged_window_t const &);

public:

	unmanaged_window_t(Atom type, client_base_t * c) : client_base_t(*c),
			 _net_wm_type(type) {
		_cnx->grab();
		_cnx->select_input(_id, UNMANAGED_ORIG_WINDOW_EVENT_MASK);
		_cnx->ungrab();
	}

	~unmanaged_window_t() {

	}

	Atom net_wm_type() {
		return _net_wm_type;
	}

	virtual bool has_window(Window w) {
		return w == _id;
	}

	void map() {
		_cnx->map_window(_id);
	}

	virtual string get_node_name() const {
		return _get_node_name<'U'>();
	}


};

}

#endif /* UNMANAGED_WINDOW_HXX_ */
