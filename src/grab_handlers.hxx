/*
 * grab_handlers.hxx
 *
 *  Created on: 24 juin 2015
 *      Author: gschwind
 */

#ifndef SRC_GRAB_HANDLERS_HXX_
#define SRC_GRAB_HANDLERS_HXX_

#include "split.hxx"
#include "workspace.hxx"
#include "popup_split.hxx"
#include "popup_notebook0.hxx"
#include "popup_alt_tab.hxx"


namespace page {

using namespace std;

enum notebook_area_e {
	NOTEBOOK_AREA_NONE,
	NOTEBOOK_AREA_TAB,
	NOTEBOOK_AREA_TOP,
	NOTEBOOK_AREA_BOTTOM,
	NOTEBOOK_AREA_LEFT,
	NOTEBOOK_AREA_RIGHT,
	NOTEBOOK_AREA_CENTER
};

class grab_split_t : public grab_handler_t {
	page_context_t * _ctx;
	weak_ptr<split_t> _split;
	rect _slider_area;
	rect _split_root_allocation;
	double _split_ratio;
	shared_ptr<popup_split_t> _ps;

public:
	grab_split_t(page_context_t * ctx, shared_ptr<split_t> s);

	virtual ~grab_split_t();
	virtual void button_press(xcb_button_press_event_t const * e);
	virtual void button_motion(xcb_motion_notify_event_t const * e);
	virtual void button_release(xcb_button_release_event_t const * e);
	virtual void key_press(xcb_key_press_event_t const * ev) { }
	virtual void key_release(xcb_key_release_event_t const * ev) { }

};

class grab_bind_client_t : public grab_handler_t {
	page_context_t * ctx;
	weak_ptr<client_managed_t> c;

	rect start_position;
	xcb_button_t _button;
	notebook_area_e zone;
	weak_ptr<notebook_t> target_notebook;
	shared_ptr<popup_notebook0_t> pn0;

	void _find_target_notebook(int x, int y, shared_ptr<notebook_t> & target, notebook_area_e & zone);

public:

	grab_bind_client_t(page_context_t * ctx, shared_ptr<client_managed_t> c, xcb_button_t button, rect const & pos);

	virtual ~grab_bind_client_t();
	virtual void button_press(xcb_button_press_event_t const * e);
	virtual void button_motion(xcb_motion_notify_event_t const * e);
	virtual void button_release(xcb_button_release_event_t const * e);
	virtual void key_press(xcb_key_press_event_t const * ev) { }
	virtual void key_release(xcb_key_release_event_t const * ev) { }
};

struct mode_data_notebook_client_menu_t  : public grab_handler_t {
	weak_ptr<notebook_t> from;
	weak_ptr<client_managed_t> client;
	bool active_grab;
	rect b;

	mode_data_notebook_client_menu_t() {
		reset();
	}

	void reset() {
		from.reset();
		client.reset();
		active_grab = false;
	}

};

enum resize_mode_e {
	RESIZE_NONE,
	RESIZE_TOP_LEFT,
	RESIZE_TOP,
	RESIZE_TOP_RIGHT,
	RESIZE_LEFT,
	RESIZE_RIGHT,
	RESIZE_BOTTOM_LEFT,
	RESIZE_BOTTOM,
	RESIZE_BOTTOM_RIGHT
};


struct grab_floating_move_t : public grab_handler_t {
	page_context_t * _ctx;
	int x_root;
	int y_root;
	rect original_position;
	rect popup_original_position;
	weak_ptr<client_managed_t> f;
	rect final_position;
	unsigned int button;

	shared_ptr<popup_notebook0_t> pfm;

	grab_floating_move_t(page_context_t * ctx, shared_ptr<client_managed_t> f, unsigned int button, int x, int y);

	virtual ~grab_floating_move_t();
	virtual void button_press(xcb_button_press_event_t const * e);
	virtual void button_motion(xcb_motion_notify_event_t const * e);
	virtual void button_release(xcb_button_release_event_t const * e);
	virtual void key_press(xcb_key_press_event_t const * ev) { }
	virtual void key_release(xcb_key_release_event_t const * ev) { }
};

struct grab_floating_resize_t : public grab_handler_t {
	page_context_t * _ctx;
	weak_ptr<client_managed_t> f;

	resize_mode_e mode;
	int x_root;
	int y_root;
	rect original_position;
	rect final_position;
	xcb_button_t button;

	shared_ptr<popup_notebook0_t> pfm;

	xcb_cursor_t _get_cursor();

public:

	grab_floating_resize_t(page_context_t * _ctx, shared_ptr<client_managed_t> f, xcb_button_t button, int x, int y, resize_mode_e mode);

	virtual ~grab_floating_resize_t();
	virtual void button_press(xcb_button_press_event_t const * e);
	virtual void button_motion(xcb_motion_notify_event_t const * e);
	virtual void button_release(xcb_button_release_event_t const * e);
	virtual void key_press(xcb_key_press_event_t const * ev) { }
	virtual void key_release(xcb_key_release_event_t const * ev) { }

};

struct grab_fullscreen_client_t : public grab_handler_t {
	page_context_t * _ctx;
	weak_ptr<client_managed_t> mw;
	weak_ptr<viewport_t> v;
	shared_ptr<popup_notebook0_t> pn0;
	xcb_button_t button;

public:

	grab_fullscreen_client_t(page_context_t * ctx, shared_ptr<client_managed_t> mw, xcb_button_t button, int x, int y);

	virtual ~grab_fullscreen_client_t();
	virtual void button_press(xcb_button_press_event_t const * e);
	virtual void button_motion(xcb_motion_notify_event_t const * e);
	virtual void button_release(xcb_button_release_event_t const * e);
	virtual void key_press(xcb_key_press_event_t const * ev) { }
	virtual void key_release(xcb_key_release_event_t const * ev) { }
};

struct grab_alt_tab_t : public grab_handler_t {
	page_context_t * _ctx;
	list<client_managed_w> _client_list;
	list<popup_alt_tab_p> _popup_list;

	map<client_managed_t *, signal_handler_t> _destroy_func_map;

	client_managed_w _selected;

	void _destroy_client(client_managed_t * c);

public:

	grab_alt_tab_t(page_context_t * ctx, list<client_managed_p> managed_window, xcb_timestamp_t time);

	virtual ~grab_alt_tab_t();
	virtual void button_press(xcb_button_press_event_t const * e);
	virtual void button_motion(xcb_motion_notify_event_t const * e);
	virtual void button_release(xcb_button_release_event_t const * e) { }
	virtual void key_press(xcb_key_press_event_t const * ev);
	virtual void key_release(xcb_key_release_event_t const * ev);
};

}


#endif /* SRC_GRAB_HANDLERS_HXX_ */
