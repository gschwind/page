/*
 * popup_split.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_SPLIT_HXX_
#define POPUP_SPLIT_HXX_

#include "box.hxx"
#include "renderable.hxx"

namespace page {

struct popup_split_t: public renderable_t {
	box_t<int> area;
	popup_split_t(box_t<int> const & area);
	~popup_split_t();
	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual box_int_t get_absolute_extend();
	virtual region_t<int> get_area();
	virtual void reconfigure(box_int_t const & area);
	virtual bool is_visible();
	virtual bool has_alpha();
};

}

#endif /* POPUP_SPLIT_HXX_ */
