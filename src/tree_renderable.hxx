/*
 * page_renderable.hxx
 *
 *  Created on: 23 août 2013
 *      Author: bg
 */

#ifndef TREE_RENDERABLE_HXX_
#define TREE_RENDERABLE_HXX_

#include <cairo/cairo.h>

#include "box.hxx"

namespace page {

struct tree_renderable_t {
	virtual ~tree_renderable_t() { }
	virtual void render(cairo_t * cr, box_t<int> const & area) const = 0;
};

}


#endif /* PAGE_RENDERABLE_HXX_ */
