/*
 * popup_frame_move.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_FRAME_MOVE_HXX_
#define POPUP_FRAME_MOVE_HXX_

#include "renderable.hxx"

namespace page {

struct popup_frame_move_t: public renderable_t {
	box_t<int> area;
	bool _show;
	popup_frame_move_t(int x, int y, int width, int height);
	popup_frame_move_t(box_int_t x = box_int_t());
	~popup_frame_move_t();

	void show();
	void hide();

	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual box_int_t get_absolute_extend();
	virtual region_t<int> get_area();
	virtual void reconfigure(box_int_t const & area);
	virtual bool is_visible();
	virtual bool has_alpha();
};

}


#endif /* POPUP_FRAME_MOVE_HXX_ */
