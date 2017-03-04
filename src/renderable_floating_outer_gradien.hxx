/*
 * renderable_floating_outer_gradien.hxx
 *
 *  Created on: 25 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_FLOATING_OUTER_GRADIEN_HXX_
#define RENDERABLE_FLOATING_OUTER_GRADIEN_HXX_

#include <cairo.h>
#include <cairo-xlib.h>
#include <cairo-xcb.h>

#include "tree.hxx"
#include "utils.hxx"
#include "renderable.hxx"
#include "region.hxx"


namespace page {

class renderable_floating_outer_gradien_t : public tree_t {
	rect _r;
	double _shadow_width;
	double _radius;

public:

	renderable_floating_outer_gradien_t(tree_t * ref, rect r, double shadow_width,
			double radius);

	virtual ~renderable_floating_outer_gradien_t();

	virtual void render(cairo_t * cr, region const & area);
	virtual region get_opaque_region();
	virtual region get_visible_region();
	virtual region get_damaged();

};

}



#endif /* RENDERABLE_FLOATING_OUTER_GRADIEN_HXX_ */
