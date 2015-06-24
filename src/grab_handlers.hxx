/*
 * grab_handlers.hxx
 *
 *  Created on: 24 juin 2015
 *      Author: gschwind
 */

#ifndef SRC_GRAB_HANDLERS_HXX_
#define SRC_GRAB_HANDLERS_HXX_

#include "split.hxx"
#include "desktop.hxx"
#include "popup_split.hxx"
#include "popup_notebook0.hxx"


namespace page {

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
	split_t * _split;
	i_rect _slider_area;
	double _split_ratio;
	std::shared_ptr<popup_split_t> _ps;

public:
	grab_split_t(page_context_t * ctx, split_t * s);

	virtual ~grab_split_t();
	virtual void button_press(xcb_button_press_event_t const * e);
	virtual void button_motion(xcb_motion_notify_event_t const * e);
	virtual void button_release(xcb_button_release_event_t const * e);

};

class grab_bind_client_t : public grab_handler_t {
	page_context_t * ctx;
	client_managed_t * c;

	i_rect start_position;
	xcb_button_t _button;
	notebook_area_e zone;
	notebook_t * target_notebook;
	workspace_t * current_workspace;
	std::shared_ptr<popup_notebook0_t> pn0;

	void _find_target_notebook(int x, int y, notebook_t * & target, notebook_area_e & zone);

public:

	grab_bind_client_t(page_context_t * ctx, client_managed_t * c, workspace_t * current, xcb_button_t button, i_rect const & pos);

	virtual ~grab_bind_client_t();
	virtual void button_press(xcb_button_press_event_t const * e);
	virtual void button_motion(xcb_motion_notify_event_t const * e);
	virtual void button_release(xcb_button_release_event_t const * e);

};

struct grab_notebook_menu_t  : public grab_handler_t {
	notebook_t * from;
	bool active_grab;
	i_rect b;

	grab_notebook_menu_t(notebook_t);
	virtual ~grab_notebook_menu_t();
	virtual void button_press(xcb_button_press_event_t const * e);
	virtual void button_motion(xcb_motion_notify_event_t const * e);
	virtual void button_release(xcb_button_release_event_t const * e);

};

struct mode_data_notebook_client_menu_t  : public grab_handler_t {
	notebook_t * from;
	client_managed_t * client;
	bool active_grab;
	i_rect b;

	mode_data_notebook_client_menu_t() {
		reset();
	}

	void reset() {
		from = nullptr;
		client = nullptr;
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
	i_rect original_position;
	i_rect popup_original_position;
	client_managed_t * f;
	i_rect final_position;
	unsigned int button;

	std::shared_ptr<popup_notebook0_t> pfm;

	grab_floating_move_t(page_context_t * ctx, client_managed_t * f, unsigned int button, int x, int y);

	virtual ~grab_floating_move_t();
	virtual void button_press(xcb_button_press_event_t const * e);
	virtual void button_motion(xcb_motion_notify_event_t const * e);
	virtual void button_release(xcb_button_release_event_t const * e);

};

struct grab_floating_resize_t : public grab_handler_t {
	page_context_t * _ctx;
	client_managed_t * f;

	resize_mode_e mode;
	int x_root;
	int y_root;
	i_rect original_position;
	i_rect final_position;
	xcb_button_t button;

	std::shared_ptr<popup_notebook0_t> pfm;

	xcb_cursor_t _get_cursor();

public:

	grab_floating_resize_t(page_context_t * _ctx, client_managed_t * f, xcb_button_t button, int x, int y, resize_mode_e mode);

	virtual ~grab_floating_resize_t();
	virtual void button_press(xcb_button_press_event_t const * e);
	virtual void button_motion(xcb_motion_notify_event_t const * e);
	virtual void button_release(xcb_button_release_event_t const * e);


};

struct grab_fullscreen_client_t : public grab_handler_t {
	page_context_t * _ctx;
	client_managed_t * mw;
	viewport_t * v;
	std::shared_ptr<popup_notebook0_t> pn0;
	xcb_button_t button;

public:

	grab_fullscreen_client_t(page_context_t * ctx, client_managed_t * mw, xcb_button_t button, int x, int y);

	virtual ~grab_fullscreen_client_t();
	virtual void button_press(xcb_button_press_event_t const * e);
	virtual void button_motion(xcb_motion_notify_event_t const * e);
	virtual void button_release(xcb_button_release_event_t const * e);

};

}


#endif /* SRC_GRAB_HANDLERS_HXX_ */
