/*
 * page_context.hxx
 *
 *  Created on: 13 juin 2015
 *      Author: gschwind
 */

#ifndef SRC_PAGE_CONTEXT_HXX_
#define SRC_PAGE_CONTEXT_HXX_

#include "display.hxx"
#include "composite_surface_manager.hxx"
#include "compositor.hxx"

namespace page {

class theme_t;

class page_context_t {

public:
	page_context_t() { }

	virtual ~page_context_t() { }
	virtual theme_t const * theme() const = 0;
	virtual composite_surface_manager_t * csm() const = 0;
	virtual display_t * dpy() const = 0;
	virtual compositor_t * cmp() const = 0;


};


}

#endif /* SRC_PAGE_CONTEXT_HXX_ */
