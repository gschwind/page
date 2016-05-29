/*
 * page_context.hxx
 *
 *  Created on: 13 juin 2015
 *      Author: gschwind
 */

#ifndef SRC_PAGE_CONTEXT_HXX_
#define SRC_PAGE_CONTEXT_HXX_

#include <typeinfo>

#include "tree.hxx"
#include "display.hxx"
#include "compositor.hxx"
#include "theme_split.hxx"
#include "keymap.hxx"

namespace page {

using namespace std;

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

/**
 * Page context is a pseudo global context for page.
 * This interface allow any page component to perform action on page context.
 **/
class page_context_t {

public:
	page_context_t() { }

	virtual ~page_context_t() { }

	/**
	 * page_context_t virtual API
	 **/

	virtual auto conf() const -> page_configuration_t const & = 0;
	virtual auto theme() const -> theme_t const * = 0;
	virtual auto dpy() const -> display_t * = 0;
	virtual auto cmp() const -> compositor_t * = 0;
	virtual void overlay_add(shared_ptr<tree_t> x) = 0;
	virtual void add_global_damage(region const & r) = 0;
	virtual auto find_mouse_viewport(int x, int y) const -> shared_ptr<viewport_t> = 0;
	virtual auto get_current_workspace() const -> shared_ptr<workspace_t> const & = 0;
	virtual auto get_workspace(int id) const -> shared_ptr<workspace_t> const & = 0;
	virtual int  get_workspace_count() const = 0;
	virtual int  create_workspace() = 0;
	virtual void grab_start(grab_handler_t * handler) = 0;
	virtual void grab_stop() = 0;
	virtual void detach(shared_ptr<tree_t> t) = 0;
	virtual void insert_window_in_notebook(shared_ptr<client_managed_t> x, shared_ptr<notebook_t> n, bool prefer_activate) = 0;
	virtual void fullscreen_client_to_viewport(shared_ptr<client_managed_t> c, shared_ptr<viewport_t> v) = 0;
	virtual void unbind_window(shared_ptr<client_managed_t> mw) = 0;
	virtual void split_left(shared_ptr<notebook_t> nbk, shared_ptr<client_managed_t> c) = 0;
	virtual void split_right(shared_ptr<notebook_t> nbk, shared_ptr<client_managed_t> c) = 0;
	virtual void split_top(shared_ptr<notebook_t> nbk, shared_ptr<client_managed_t> c) = 0;
	virtual void split_bottom(shared_ptr<notebook_t> nbk, shared_ptr<client_managed_t> c) = 0;
	virtual void set_focus(shared_ptr<client_managed_t> w, xcb_timestamp_t tfocus) = 0;
	virtual void notebook_close(shared_ptr<notebook_t> nbk) = 0;
	virtual int  left_most_border() = 0;
	virtual int  top_most_border() = 0;
	virtual auto global_client_focus_history() -> list<weak_ptr<client_managed_t>> = 0;
	virtual auto net_client_list() -> list<shared_ptr<client_managed_t>> = 0;
	virtual auto keymap() const -> keymap_t const * = 0;
	virtual void switch_to_desktop(unsigned int desktop) = 0;
	virtual auto create_view(xcb_window_t w) -> shared_ptr<client_view_t> = 0;
	virtual void make_surface_stats(int & size, int & count) = 0;
	virtual auto mainloop() -> mainloop_t * = 0;

};


}

#endif /* SRC_PAGE_CONTEXT_HXX_ */
