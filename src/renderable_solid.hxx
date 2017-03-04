/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_SOLID_HXX_
#define RENDERABLE_SOLID_HXX_

#include "tree.hxx"
#include "utils.hxx"
#include "renderable.hxx"
#include "color.hxx"

namespace page {

class renderable_solid_t : public tree_t {
	rect location;
	color_t color;
	region damaged;
	region opaque_region;
	region visible_region;

public:

	renderable_solid_t(tree_t * ref, color_t color, rect loc, region damaged);
	virtual ~renderable_solid_t();
	virtual void render(cairo_t * cr, region const & area);
	virtual region get_opaque_region();
	virtual region get_visible_region();
	virtual region get_damaged();
	void set_opaque_region(region const & r);
	void set_visible_region(region const & r);

};



}



#endif /* RENDERABLE_SURFACE_HXX_ */
