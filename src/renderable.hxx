/*
 * renderable.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef RENDERABLE_HXX_
#define RENDERABLE_HXX_

#include <list>
#include <cairo.h>
#include "box.hxx"
#include "region.hxx"

//namespace page {
//
///**
// * Renderable class are object that can be draw on screen, mainly Window
// */
//class renderable_t {
//public:
//	/**
//	 * renderable default constructor.
//	 */
//	renderable_t() { }
//
//	/**
//	 * Destroy renderable
//	 */
//	virtual ~renderable_t() { }
//
//	/**
//	 * draw the area of a renderable to the destination surface
//	 * @param cr the destination surface context
//	 * @param area the area to redraw
//	 */
//	virtual void repair1(cairo_t * cr, box_int_t const & area) = 0;
//
//	/**
//	 * Get the area on screen.
//	 */
//	virtual region_t<int> get_area() = 0;
//
//	/**
//	 * Move the renderable on screen.
//	 * @param area the new location of object
//	 */
//	virtual void reconfigure(box_int_t const & area) = 0;
//
//	/**
//	 * Return the visibility status of an object.
//	 * @return true if the object are visible and should be draw.
//	 */
//	virtual bool is_visible() = 0;
//
//	virtual bool has_alpha() = 0;
//
//};
//
///**
// * short cut for renderable list.
// */
//typedef std::list<renderable_t *> renderable_list_t;
//
//}


#endif /* RENDERABLE_HXX_ */
