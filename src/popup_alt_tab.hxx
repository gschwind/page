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
#include "icon_handler.hxx"
#include "renderable_thumbnail.hxx"

#include "client_managed.hxx"


namespace page {

using namespace std;

class cycle_window_entry_t {
	weak_ptr<client_managed_t> client;

	string title;
	shared_ptr<icon64> icon;
	shared_ptr<renderable_thumbnail_t> _thumbnail;

	decltype(client_managed_t::on_destroy)::signal_func_t destroy_func;


public:
	cycle_window_entry_t() { }

	cycle_window_entry_t(cycle_window_entry_t const & x) :
		client{x.client},
		title{x.title},
		icon{x.icon},
		_thumbnail{x._thumbnail},
		destroy_func{x.destroy_func}
	{ }

	friend class popup_alt_tab_t;

};

class popup_alt_tab_t : public tree_t {
	page_context_t * _ctx;
	//xcb_window_t _wid;
	//shared_ptr<pixmap_t> _surf;
	rect _position;
	list<cycle_window_entry_t> _client_list;
	list<cycle_window_entry_t>::iterator _selected;

	bool _is_durty;
	bool _exposed;
	bool _damaged;

	void _create_composite_window();
	void update_backbuffer();
	void paint_exposed();
	void destroy_client(client_managed_t * c);

	void _init();

public:

	popup_alt_tab_t(page_context_t * ctx, list<shared_ptr<client_managed_t>> client_list);

	template<typename ... Args>
	static shared_ptr<popup_alt_tab_t> create(Args ... args) {
		auto ths = make_shared<popup_alt_tab_t>(args...);
		ths->_init();
		return ths;
	}

	virtual ~popup_alt_tab_t();

	void move(int x, int y);
	rect const & position();
	void select_next();
	weak_ptr<client_managed_t> get_selected();

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

}



#endif /* POPUP_ALT_TAB_HXX_ */
