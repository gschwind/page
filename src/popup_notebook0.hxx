/*
 * popup_notebook0.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef POPUP_NOTEBOOK0_HXX_
#define POPUP_NOTEBOOK0_HXX_

#include "window_overlay.hxx"

namespace page {

struct popup_notebook0_t : public window_overlay_t {

	Window const & wid;

	bool _show;

	popup_notebook0_t(xconnection_t * cnx);
	~popup_notebook0_t();

	void reconfigure(box_int_t const & area);

	bool is_visible();

	void show();
	void hide();

};

}


#endif /* POPUP_NOTEBOOK0_HXX_ */
