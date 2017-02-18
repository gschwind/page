/*
 * page_context.hxx
 *
 *  Created on: 13 juin 2015
 *      Author: gschwind
 */

#ifndef SRC_PAGE_TYPES_HXX_
#define SRC_PAGE_TYPES_HXX_

#include <typeinfo>

#include "tree.hxx"
#include "display.hxx"
#include "compositor.hxx"
#include "theme_split.hxx"
#include "keymap.hxx"

namespace page {

using namespace std;

class page_t;
class theme_t;
class notebook_t;
class viewport_t;
class client_base_t;
class client_managed_t;
class client_proxy_t;
class client_view_t;
class workspace_t;
class mainloop_t;


struct grab_handler_t {
	virtual ~grab_handler_t() { }
	virtual void button_press(xcb_button_press_event_t const *) = 0;
	virtual void button_motion(xcb_motion_notify_event_t const *) = 0;
	virtual void button_release(xcb_button_release_event_t const *) = 0;
	virtual void key_press(xcb_key_press_event_t const * ev) = 0;
	virtual void key_release(xcb_key_release_event_t const * ev) = 0;
};

struct page_configuration_t {
	bool _replace_wm;
	bool _menu_drop_down_shadow;
	bool _auto_refocus;
	bool _mouse_focus;
	bool _enable_shade_windows;
	int64_t _fade_in_time;
};

}

#endif /* SRC_PAGE_TYPES_HXX_ */
