/*
 * split.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef SPLIT_HXX_
#define SPLIT_HXX_

#include "theme.hxx"
#include "page_context.hxx"
#include "page_component.hxx"

namespace page {

class split_t : public page_component_t {
	page_context_t * _ctx;

	rect _allocation;
	rect _split_bar_area;
	split_type_e _type;
	double _ratio;

	bool _has_mouse_over;

	shared_ptr<page_component_t> _pack0;
	shared_ptr<page_component_t> _pack1;

	rect _bpack0;
	rect _bpack1;

	list<shared_ptr<tree_t>> _children;

	/** window used for cursor **/
	xcb_window_t _wid;

	split_t(split_t const &);
	split_t & operator=(split_t const &);

	rect compute_split_bar_location(rect const & bpack0, rect const & bpack1) const;
	rect compute_split_bar_location() const;

	void update_allocation();
	shared_ptr<split_t> shared_from_this();

public:
	split_t(page_context_t * ctx, split_type_e type);
	~split_t();

	/* access to stuff */
	auto get_split_bar_area() const -> rect const & { return _split_bar_area; }
	auto get_pack0() const -> shared_ptr<page_component_t> { return _pack0; }
	auto get_pack1() const -> shared_ptr<page_component_t> { return _pack1; }
	auto ratio() const -> double { return _ratio; }
	auto type() const -> split_type_e { return _type; }
	void render_legacy(cairo_t * cr) const;

	void set_split(double split);
	void set_pack0(shared_ptr<page_component_t> x);
	void set_pack1(shared_ptr<page_component_t> x);

	double compute_split_constaint(double split);
	rect root_location();

	void compute_children_allocation(double split, rect & bpack0, rect & bpack1);
	void compute_children_root_allocation(double split, rect & bpack0, rect & bpack1);


	/**
	 * tree_t virtual API
	 **/

	virtual void hide();
	virtual void show();
	virtual auto get_node_name() const -> string;
	virtual void remove(shared_ptr<tree_t> t);

	virtual void append_children(vector<shared_ptr<tree_t>> & out) const;
	virtual void update_layout(time64_t const time);
	virtual void render(cairo_t * cr, region const & area);

	virtual auto get_opaque_region() -> region;
	virtual auto get_visible_region() -> region;
	virtual auto get_damaged() -> region;

	virtual void activate();
	virtual void activate(shared_ptr<tree_t> t);
	virtual bool button_press(xcb_button_press_event_t const * ev);
	//virtual bool button_release(xcb_button_release_event_t const * ev);
	virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	virtual bool leave(xcb_leave_notify_event_t const * ev);
	//virtual bool enter(xcb_enter_notify_event_t const * ev);
	//virtual void expose(xcb_expose_event_t const * ev);
	//virtual void trigger_redraw();

	//virtual auto get_xid() const -> xcb_window_t;
	//virtual auto get_parent_xid() const -> xcb_window_t;
	//virtual rect get_window_position() const;
	//virtual void queue_redraw();

	/**
	 * page_component_t virtual API
	 **/

	virtual void set_allocation(rect const & area);
	virtual rect allocation() const;
	virtual void replace(shared_ptr<page_component_t> src, shared_ptr<page_component_t> by);
	virtual void get_min_allocation(int & width, int & height) const;

};

}

#endif /* SPLIT_HXX_ */
