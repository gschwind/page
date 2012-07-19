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

namespace page {

class renderable_t {
public:
	int z;
	renderable_t() : z(0) { }
	virtual ~renderable_t() { }
	virtual void repair1(cairo_t * cr, box_int_t const & area) = 0;
	virtual box_int_t get_absolute_extend() = 0;
	virtual void reconfigure(box_int_t const & area) = 0;
	virtual bool is_visible()= 0;

};

typedef std::list<renderable_t *> renderable_list_t;

}


#endif /* RENDERABLE_HXX_ */
