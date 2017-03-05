/*
 * page_context.hxx
 *
 *  Created on: 13 juin 2015
 *      Author: gschwind
 */

#ifndef SRC_PAGE_TYPES_HXX_
#define SRC_PAGE_TYPES_HXX_

#include <typeinfo>
#include <xcb/xcb.h>
#include <memory>

namespace page {

using namespace std;

class tree_t;
using tree_p = shared_ptr<tree_t>;
using tree_w = weak_ptr<tree_t>;

class page_root_t;
using page_root_p = shared_ptr<page_root_t>;
using page_root_w = weak_ptr<page_root_t>;

class workspace_t;
using workspace_p = shared_ptr<workspace_t>;
using workspace_w = weak_ptr<workspace_t>;

class viewport_t;
using viewport_p = shared_ptr<viewport_t>;
using viewport_w = weak_ptr<viewport_t>;

class split_t;
using split_p = shared_ptr<split_t>;
using split_w = weak_ptr<split_t>;

class notebook_t;
using notebook_p = shared_ptr<notebook_t>;
using notebook_w = weak_ptr<notebook_t>;

class view_t;
using view_p = shared_ptr<view_t>;
using view_w = weak_ptr<view_t>;

class view_fullscreen_t;
using view_fullscreen_p = shared_ptr<view_fullscreen_t>;
using view_fullscreen_w = weak_ptr<view_fullscreen_t>;

class view_notebook_t;
using view_notebook_p = shared_ptr<view_notebook_t>;
using view_notebook_w = weak_ptr<view_notebook_t>;

class view_floating_t;
using view_floating_p = shared_ptr<view_floating_t>;
using view_floating_w = weak_ptr<view_floating_t>;

class view_rebased_t;
using view_rebased_p = shared_ptr<view_rebased_t>;
using view_rebased_w = weak_ptr<view_rebased_t>;

class view_dock_t;
using view_dock_p = shared_ptr<view_dock_t>;
using view_dock_w = weak_ptr<view_dock_t>;

class view_popup_t;
using view_popup_p = shared_ptr<view_popup_t>;
using view_popup_w = weak_ptr<view_popup_t>;

class xdg_shell_client_t;
using xdg_shell_client_p = shared_ptr<xdg_shell_client_t>;
using xdg_shell_client_w = weak_ptr<xdg_shell_client_t>;

class client_proxy_t;
using client_proxy_p = shared_ptr<client_proxy_t>;
using client_proxy_w = weak_ptr<client_proxy_t>;

class client_managed_t;
using client_managed_p = shared_ptr<client_managed_t>;
using client_managed_w = weak_ptr<client_managed_t>;

class client_view_t;
using client_view_p = shared_ptr<client_view_t>;
using client_view_w = weak_ptr<client_view_t>;

class page_t;
class theme_t;
class notebook_t;
class viewport_t;
class client_base_t;

class client_proxy_t;

class workspace_t;
class mainloop_t;

class pixmap_t;

class view_fullscreen_t;


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
