/*
 * page_renderable.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef TREE_RENDERABLE_HXX_
#define TREE_RENDERABLE_HXX_

#include <cairo/cairo.h>

#include "box.hxx"

namespace page {

struct tree_renderable_t {
	virtual ~tree_renderable_t() { }
	virtual void render(cairo_t * cr, rectangle const & area) const = 0;
};

}


#endif /* PAGE_RENDERABLE_HXX_ */
