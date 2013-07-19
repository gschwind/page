/*
 * popup_split.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_SPLIT_HXX_
#define POPUP_SPLIT_HXX_

#include "window_overlay.hxx"
#include "box.hxx"
#include "renderable.hxx"

namespace page {

struct popup_split_t: public window_overlay_t {
	Window const & wid;
	popup_split_t(xconnection_t * cnx);
	~popup_split_t();
	void move_resize(box_int_t const & a);
};

}

#endif /* POPUP_SPLIT_HXX_ */
