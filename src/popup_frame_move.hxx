/*
 * popup_frame_move.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_FRAME_MOVE_HXX_
#define POPUP_FRAME_MOVE_HXX_

#include "xconnection.hxx"
#include "window_overlay.hxx"
#include "renderable.hxx"

namespace page {

struct popup_frame_move_t: public window_overlay_t {

	Window const & wid;

	popup_frame_move_t(xconnection_t * cnx);
	~popup_frame_move_t();

	void reconfigure(box_int_t const & a);
};

}


#endif /* POPUP_FRAME_MOVE_HXX_ */
