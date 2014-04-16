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
#include "box.hxx"
#include "region.hxx"
#include "utils.hxx"

namespace page {

/**
 * Renderable class are object that can be draw on screen, mainly Window
 */
class renderable_t {
public:

	/* level to draw, lower value mean draw at bottom */

	double z;
	/**
	 * renderable default constructor.
	 */
	renderable_t() { }

	/**
	 * Destroy renderable
	 */
	virtual ~renderable_t() { }

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 */
	virtual void render(cairo_t * cr, time_t time) = 0;

};

/**
 * short cut for renderable list.
 */
typedef std::list<renderable_t *> renderable_list_t;

}


#endif /* RENDERABLE_HXX_ */
