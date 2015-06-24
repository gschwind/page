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
#include "composite_surface_manager.hxx"
#include "compositor.hxx"
#include "theme_split.hxx"

namespace page {

class theme_t;
class notebook_t;
class viewport_t;
class client_base_t;
class client_managed_t;
class workspace_t;

struct overlay_t : public renderable_t {
	overlay_t() { }
	virtual ~overlay_t() { }
	virtual xcb_window_t id() const = 0;
	virtual void expose() = 0;
};


struct grab_handler_t {
	virtual ~grab_handler_t() { }
	virtual void button_press(xcb_button_press_event_t const *) = 0;
	virtual void button_motion(xcb_motion_notify_event_t const *) = 0;
	virtual void button_release(xcb_button_release_event_t const *) = 0;
};

class page_context_t {

public:
	page_context_t() { }

	virtual ~page_context_t() { }
	virtual theme_t const * theme() const = 0;
	virtual composite_surface_manager_t * csm() const = 0;
	virtual display_t * dpy() const = 0;
	virtual compositor_t * cmp() const = 0;

	virtual void overlay_add(std::shared_ptr<overlay_t> x) = 0;
	virtual void overlay_remove(std::shared_ptr<overlay_t> x) = 0;

	virtual void add_global_damage(region const & r) = 0;

	virtual void safe_raise_window(client_base_t * c) = 0;

	virtual viewport_t * find_mouse_viewport(int x, int y) const = 0;
	virtual workspace_t * get_current_workspace() const = 0;
	virtual workspace_t * get_workspace(int id) const = 0;
	virtual int get_workspace_count() const = 0;


	virtual void grab_start(grab_handler_t * handler) = 0;
	virtual void grab_stop() = 0;

	virtual void detach(tree_t * t) = 0;
	virtual void insert_window_in_notebook(client_managed_t * x, notebook_t * n, bool prefer_activate) = 0;

	virtual void fullscreen_client_to_viewport(client_managed_t * c, viewport_t * v) = 0;
	virtual void split_left(notebook_t * nbk, client_managed_t * c) = 0;
	virtual void split_right(notebook_t * nbk, client_managed_t * c) = 0;
	virtual void split_top(notebook_t * nbk, client_managed_t * c) = 0;
	virtual void split_bottom(notebook_t * nbk, client_managed_t * c) = 0;
	virtual void set_focus(client_managed_t * w, xcb_timestamp_t tfocus) = 0;
	virtual void mark_durty(tree_t * t) = 0;

	virtual void notebook_close(notebook_t * nbk) = 0;
	virtual void split(notebook_t * nbk, split_type_e type) = 0;
	virtual void unbind_window(client_managed_t * mw) = 0;


};


}

#endif /* SRC_PAGE_CONTEXT_HXX_ */
