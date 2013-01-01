/*
 * popup_notebook0.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_NOTEBOOK0_HXX_
#define POPUP_NOTEBOOK0_HXX_

#include "renderable.hxx"

namespace page {

struct popup_notebook0_t: public renderable_t {
	box_t<int> area;
	popup_notebook0_t(int x, int y, int width, int height);
	~popup_notebook0_t();
	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual box_int_t get_absolute_extend();
	virtual region_t<int> get_area();
	virtual void reconfigure(box_int_t const & area);
	virtual bool is_visible();
};

}


#endif /* POPUP_NOTEBOOK0_HXX_ */
