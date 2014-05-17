/*
 * notebook_base.hxx
 *
 *  Created on: 10 mai 2014
 *      Author: gschwind
 */

#ifndef NOTEBOOK_BASE_HXX_
#define NOTEBOOK_BASE_HXX_

#include "tree.hxx"
#include "managed_window_base.hxx"

namespace page {

struct notebook_base_t : public tree_t {
	virtual ~notebook_base_t() { }
	virtual list<managed_window_base_t const *> clients() const = 0;
	virtual managed_window_base_t const * selected() const = 0;
	virtual bool is_default() const = 0;

	virtual string get_node_name() const {
		return _get_node_name<'n'>();
	}

};

}


#endif /* NOTEBOOK_BASE_HXX_ */
