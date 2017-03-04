/*
 * popup_split.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef POPUP_SPLIT_HXX_
#define POPUP_SPLIT_HXX_

#include "page-types.hxx"
#include "split.hxx"

namespace page {

struct popup_split_t : public tree_t {
	static int const border_width = 6;

	tree_t * _parent;

	page_t * _ctx;
	double _current_split;

	weak_ptr<split_t> _s_base;

	rect _position;
	bool _exposed;
	bool _damaged;

	xcb_window_t _wid;

public:
	void show() override;

	rect const & position();

	virtual region get_opaque_region();
	virtual region get_visible_region();
	virtual region get_damaged();

	popup_split_t(tree_t * ctx, shared_ptr<split_t> split);
	~popup_split_t();

	void update_layout();
	void set_position(double pos);
	virtual void render(cairo_t * cr, region const & area);
	void _paint_exposed();
	xcb_window_t get_xid() const;
	xcb_window_t get_parent_xid() const;
	void expose(xcb_expose_event_t const * ev);
	void trigger_redraw();
	void render_finished();

};

}

#endif /* POPUP_SPLIT_HXX_ */
