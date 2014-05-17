/*
 * split_base.hxx
 *
 *  Created on: 10 mai 2014
 *      Author: gschwind
 */

#ifndef SPLIT_BASE_HXX_
#define SPLIT_BASE_HXX_

#include "tree.hxx"

namespace page {

enum split_type_e {
	HORIZONTAL_SPLIT, VERTICAL_SPLIT,
};

struct split_base_t : public tree_t {
	virtual ~split_base_t() { }
	virtual split_type_e type() const = 0;
	virtual double split() const = 0;

	virtual string get_node_name() const {
		return _get_node_name<'s'>();
	}

};

}


#endif /* SPLIT_BASE_HXX_ */
