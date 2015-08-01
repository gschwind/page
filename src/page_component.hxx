/*
 * split.cxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef PAGE_COMPONENT_HXX_
#define PAGE_COMPONENT_HXX_

#include "box.hxx"
#include "tree.hxx"

namespace page {

class page_component_t : public tree_t {
public:

	page_component_t() { }
	virtual ~page_component_t() { }
	virtual void set_allocation(i_rect const & area) = 0;
	virtual i_rect allocation() const = 0;
	virtual void replace(page_component_t * src, page_component_t * by) = 0;

};

}



#endif /* PAGE_COMPONENT_HXX_ */
