/*
 * popup_notebook0.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef POPUP_NOTEBOOK0_HXX_
#define POPUP_NOTEBOOK0_HXX_

#include "theme.hxx"
#include "renderable.hxx"
#include "icon_handler.hxx"
#include "page-types.hxx"
#include "tree.hxx"

namespace page {

using namespace std;

struct popup_notebook0_t : public tree_t {
	static int const border_width = 6;
	page_t * _ctx;

protected:
	rect _position;
	bool _exposed;
	region _damaged;

	region _visible_cache;

	xcb_window_t _wid;

public:
	popup_notebook0_t(tree_t * ref);

	void _create_window();
	void move_resize(rect const & area);
	void move(int x, int y);
	rect const & position();
	virtual region get_opaque_region();
	virtual region get_visible_region();
	virtual region get_damaged();
	~popup_notebook0_t();
	void show();
	void hide();
	virtual void render(cairo_t * cr, region const & area);
	void _paint_exposed();
	xcb_window_t get_toplevel_xid() const;
	void expose(xcb_expose_event_t const * ev);
	void trigger_redraw();
	void render_finished();

};

}


#endif /* POPUP_NOTEBOOK0_HXX_ */
