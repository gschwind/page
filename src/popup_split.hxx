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

	region _damaged;
	region _visible_region;

	xcb_window_t _wid;

	void _paint_exposed();

public:

	rect const & position();
	void set_position(double pos);
	void update_layout();

	popup_split_t(tree_t * ctx, shared_ptr<split_t> split);

	virtual ~popup_split_t();

	//virtual void hide();
	virtual void show() override;
	//virtual auto get_node_name() const -> string;
	//virtual void remove(tree_p t);
	//virtual void clear();

	//virtual void update_layout(time64_t const time);
	virtual void render(cairo_t * cr, region const & area) override;
	virtual void trigger_redraw() override;
	virtual void render_finished() override;
	//virtual void reconfigure(); // used to place all windows taking in account the current tree state
	//virtual void on_workspace_enable();
	//virtual void on_workspace_disable();

	virtual auto get_opaque_region() -> region override;
	virtual auto get_visible_region() -> region override;
	virtual auto get_damaged() -> region override;

	//virtual auto button_press(xcb_button_press_event_t const * ev) -> button_action_e;
	//virtual bool button_release(xcb_button_release_event_t const * ev);
	//virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	//virtual bool leave(xcb_leave_notify_event_t const * ev);
	//virtual bool enter(xcb_enter_notify_event_t const * ev);
	virtual void expose(xcb_expose_event_t const * ev) override;

	virtual auto get_toplevel_xid() const -> xcb_window_t override;
	//virtual rect get_window_position() const;
	//virtual void queue_redraw();

};

}

#endif /* POPUP_SPLIT_HXX_ */
