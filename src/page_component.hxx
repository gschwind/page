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
	virtual void set_allocation(rect const & area) = 0;
	virtual rect allocation() const = 0;
	virtual void replace(shared_ptr<page_component_t> src, shared_ptr<page_component_t> by) = 0;
	virtual void get_min_allocation(int & width, int & height) const = 0;

};

}



#endif /* PAGE_COMPONENT_HXX_ */
