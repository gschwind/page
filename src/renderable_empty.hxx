/*
 * renderable_empty.hxx
 *
 *  Created on: 11 oct. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_EMPTY_HXX_
#define RENDERABLE_EMPTY_HXX_

#include "tree.hxx"

namespace page {


/**
 * Transparent and empty renderable that will force render of renderable under
 * this on. i.e. is a fake damaged area.
 */
class renderable_empty_t : public tree_t {
	region damaged;
public:

	renderable_empty_t(region damaged) : damaged(damaged) { }

	virtual ~renderable_empty_t() { }

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {
		/** do nothing **/
	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return region{};
	}

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() {
		return damaged;
	}

	virtual region get_damaged() {
		return damaged;
	}

};



}


#endif /* RENDERABLE_EMPTY_HXX_ */
