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
#include "keymap.hxx"

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
	virtual void key_press(xcb_key_press_event_t const * ev) = 0;
	virtual void key_release(xcb_key_release_event_t const * ev) = 0;
};

/**
 * Page context is a pseudo global context for page.
 * This interface allow any page component to perform action on page context.
 **/
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

	/**
	 * Get the current workspace.
	 *
	 * @return: a pointer to the current workspace
	 **/
	virtual workspace_t * get_current_workspace() const = 0;

	/**
	 * Get a given workspace from id. Desktop id start from 0 to workspace count(excluding).
	 *
	 * @input id: the id of the desktop.
	 * @return: the workscape with the id.
	 */
	virtual workspace_t * get_workspace(int id) const = 0;

	/**
	 * Give how much workspace are available workspace.
	 *
	 * @return: the count of avalaible workspace.
	 **/
	virtual int get_workspace_count() const = 0;

	/**
	 * Start a grab process
	 *
	 * Note: only one grab can be done at time.
	 * Note: The grab handler is owned by page after this call and destroy is done by page.
	 *       The grab can be canceled at any time by destroying the handler.
	 *
	 * @input handler: the implementation of grab handler.
	 */
	virtual void grab_start(grab_handler_t * handler) = 0;

	/**
	 * Terminate current grab.
	 **/
	virtual void grab_stop() = 0;

	/**
	 * remove t from the page tree.
	 * @input t: the component to remove.
	 **/
	virtual void detach(tree_t * t) = 0;

	/**
	 * add a client to a notebook.
	 *
	 * Note: the client to insert MUST be detached.
	 *
	 * @input x: client to insert
	 * @input n: the target notebook
	 * @input prefer_activate: set to true to ask to show the client after inserting it.
	 **/
	virtual void insert_window_in_notebook(client_managed_t * x, notebook_t * n, bool prefer_activate) = 0;

	/**
	 * Take a client and put it into fullscreen at given viewport.
	 *
	 * Note: the client to insert MUST be detached.
	 *
	 * @input c: client to set fullscreen.
	 * @input v: the target viewport, if nullptr select the default one.
	 **/
	virtual void fullscreen_client_to_viewport(client_managed_t * c, viewport_t * v) = 0;

	/**
	 * Remove a client from a note book and turn it to floating mode
	 *
	 * @input mw: the client to make floating.
	 **/
	virtual void unbind_window(client_managed_t * mw) = 0;

	/**
	 * Split a notebook in two notebooks, inserting a client to the new one
	 *
	 * @input nbk: notebook to split
	 * @input c: the client to add to the new notebook. if nullptr no client is inserted.
	 **/
	virtual void split_left(notebook_t * nbk, client_managed_t * c = nullptr) = 0;
	virtual void split_right(notebook_t * nbk, client_managed_t * c = nullptr) = 0;
	virtual void split_top(notebook_t * nbk, client_managed_t * c = nullptr) = 0;
	virtual void split_bottom(notebook_t * nbk, client_managed_t * c = nullptr) = 0;

	/**
	 * Focus the client w at time stamp tfocus
	 *
	 * @input w: the client to focus.
	 * @input tfocus: timestamp of focus.
	 */
	virtual void set_focus(client_managed_t * w, xcb_timestamp_t tfocus) = 0;

	/**
	 * Close a given notebook. i.e. unsplit it.
	 * Client that belong this notebook will be transfered to the default notebook.
	 *
	 * @input nbk: the notebook to close.
	 **/
	virtual void notebook_close(notebook_t * nbk) = 0;

	/**
	 * This function enable the placement of client outside the screen without unmap them.
	 **/
	virtual int left_most_border() = 0;
	virtual int top_most_border() = 0;

	virtual std::list<client_managed_t *> global_client_focus_history() = 0;
	virtual std::list<client_managed_t *> clients_list() = 0;

	virtual keymap_t const * keymap() const = 0;

};


}

#endif /* SRC_PAGE_CONTEXT_HXX_ */
