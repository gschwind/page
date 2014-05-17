/*
 * viewport_base.hxx
 *
 *  Created on: 10 mai 2014
 *      Author: gschwind
 */

#ifndef VIEWPORT_BASE_HXX_
#define VIEWPORT_BASE_HXX_

#include "tree.hxx"

namespace page {

struct viewport_base_t : public tree_t {
	virtual ~viewport_base_t() { }


	virtual string get_node_name() const {
		return _get_node_name<'v'>();
	}

};

}



#endif /* VIEWPORT_BASE_HXX_ */
