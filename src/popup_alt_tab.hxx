/*
 * popup_alt_tab.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef POPUP_ALT_TAB_HXX_
#define POPUP_ALT_TAB_HXX_

#include <cairo/cairo.h>

#include <memory>

#include "renderable.hxx"
#include "viewport.hxx"
#include "icon_handler.hxx"
#include "renderable_thumbnail.hxx"

#include "client_managed.hxx"


namespace page {

using namespace std;

class cycle_window_entry_t {
	client_managed_w client;
	string title;
	shared_ptr<icon64> icon;
	shared_ptr<renderable_thumbnail_t> _thumbnail;

public:
	cycle_window_entry_t() { }

	cycle_window_entry_t(cycle_window_entry_t const & x) :
		client{x.client},
		title{x.title},
		icon{x.icon},
		_thumbnail{x._thumbnail}
	{ }

	friend class popup_alt_tab_t;

};

class popup_alt_tab_t : public tree_t {
	page_context_t * _ctx;
	xcb_window_t _wid;
	rect _position_extern;
	rect _position_intern;
	viewport_w _viewport;

	list<cycle_window_entry_t> _client_list;
	list<cycle_window_entry_t>::iterator _selected;

	bool _is_durty;
	bool _exposed;
	bool _damaged;

	void _create_composite_window();
	void update_backbuffer();
	void paint_exposed();

	void _init();

	void _select_from_mouse(int x, int y);

	void _reconfigure();

	void _clear_selected();

public:

	popup_alt_tab_t(page_context_t * ctx, list<client_managed_p> client_list, viewport_p viewport);

	template<typename ... Args>
	static shared_ptr<popup_alt_tab_t> create(Args ... args) {
		auto ths = make_shared<popup_alt_tab_t>(args...);
		ths->_init();
		return ths;
	}

	virtual ~popup_alt_tab_t();

	void move(int x, int y);
	rect const & position();

	client_managed_w selected(client_managed_w c);
	client_managed_w selected();

	void destroy_client(client_managed_t * c);

	void grab_button_press(xcb_button_press_event_t const * ev);
	void grab_button_motion(xcb_motion_notify_event_t const * ev);

	/**
	 * tree_t virtual API
	 **/

	virtual void hide();
	virtual void show();
	virtual auto get_node_name() const -> string;
	// virtual void remove(shared_ptr<tree_t> t);

	virtual void append_children(vector<shared_ptr<tree_t>> & out) const;
	virtual void update_layout(time64_t const time);
	virtual void render(cairo_t * cr, region const & area);
	virtual void trigger_redraw();
	virtual void render_finished();

	virtual auto get_opaque_region() -> region;
	virtual auto get_visible_region() -> region;
	virtual auto get_damaged() -> region;

	using tree_t::activate;
	//virtual void activate();
	//virtual void activate(shared_ptr<tree_t> t);

	//virtual bool button_press(xcb_button_press_event_t const * ev);
	//virtual bool button_release(xcb_button_release_event_t const * ev);
	//virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	//virtual bool leave(xcb_leave_notify_event_t const * ev);
	//virtual bool enter(xcb_enter_notify_event_t const * ev);
	virtual void expose(xcb_expose_event_t const * ev);

	virtual auto get_xid() const -> xcb_window_t;
	virtual auto get_parent_xid() const -> xcb_window_t;
	//virtual rect get_window_position() const;
	//virtual void queue_redraw();

};

using popup_alt_tab_p = shared_ptr<popup_alt_tab_t>;
using popup_alt_tab_w = weak_ptr<popup_alt_tab_t>;

}



#endif /* POPUP_ALT_TAB_HXX_ */
