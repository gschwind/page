/*
 * renderable.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef RENDERABLE_HXX_
#define RENDERABLE_HXX_

#include <list>
#include <cairo.h>
#include "leak_checker.hxx"
#include "time.hxx"
#include "box.hxx"
#include "region.hxx"
#include "utils.hxx"

namespace page {

using namespace std;

/**
 * Renderable object, are used to render a static scene graph (en optimise the scene graph)
 **/
class renderable_t : public leak_checker {
public:

	/**
	 * Destroy renderable
	 **/
	virtual ~renderable_t() { }

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) = 0;

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() = 0;

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() = 0;

	/**
	 * return currently damaged area (absolute)
	 **/
	virtual region get_damaged() = 0;

};

/**
 * short cut for renderable list.
 */
typedef list<renderable_t *> renderable_list_t;

}


#endif /* RENDERABLE_HXX_ */
