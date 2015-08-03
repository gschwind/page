/*
 * tree.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef TREE_HXX_
#define TREE_HXX_

#include <memory>
#include <iostream>

#include "renderable.hxx"
#include "time.hxx"

namespace page {

using namespace std;

/**
 * tree_t is the base of the hierarchy of desktop, viewports,
 * client_managed and unmanaged, etc...
 * It define the stack order of each component drawn within page.
 **/
class tree_t {

	template<typename ... T>
	bool _broadcast_root_first(bool (tree_t::* f)(T ... args), T ... args) {
		if((this->*f)(args...))
			return true;
		for(auto x: tree_t::get_all_children_root_first()) {
			if((x->*f)(args...))
				return true;
		}
		return false;
	}

	template<typename ... T>
	bool _broadcast_root_first(void (tree_t::* f)(T ... args), T ... args) {
		(this->*f)(args...);
		for(auto x: tree_t::get_all_children_root_first()) {
			(x->*f)(args...);
		}
	}

	template<typename ... T>
	bool _broadcast_deep_first(bool (tree_t::* f)(T ... args), T ... args) {
		for(auto x: tree_t::get_all_children_deep_first()) {
			if((x->*f)(args...))
				return true;
		}
		if((this->*f)(args...))
			return true;
		return false;
	}

	template<typename ... T>
	void _broadcast_deep_first(void (tree_t::* f)(T ... args), T ... args) {
		for(auto x: tree_t::get_all_children_deep_first()) {
			(x->*f)(args...);
		}
		(this->*f)(args...);
	}

	bool _trigger_redraw_handler() { trigger_redraw(); return false; }

public:
	tree_t() { }

	virtual ~tree_t() { }

	/**
	 * Return parent within tree
	 **/
	virtual auto parent() const -> tree_t * = 0;

	/**
	 * Return the name of this tree node
	 **/
	virtual auto get_node_name() const -> string {
		return string{"ANONYMOUS NODE"};
	}

	/**
	 * Remove i from direct child of this node (not recusively)
	 **/
	virtual auto remove(tree_t * t) -> void {
		/* because by default we have no child, by default remove do nothing */
	}

	/**
	 * Change parent of this node to parent.
	 **/
	virtual auto set_parent(tree_t * parent) -> void = 0;

	/**
	 * Return direct children of this node (net recursive)
	 **/
	virtual auto children(std::vector<tree_t *> & out) const -> void {
		/* by default we have no child */
	}


	/**
	 * get all visible children
	 **/
	virtual auto get_visible_children(vector<tree_t *> & out) -> void {
		/* by default we have no child, thus we do nothing here */
	}

	/**
	 * Hide this node recursively
	 **/
	virtual auto hide() -> void { }

	/**
	 * Show this node recursively
	 **/
	virtual auto show() -> void { }

	/**
	 * return the list of renderable object to draw this tree ordered and recursively
	 **/
	virtual auto update_layout(time64_t const time) -> void {
		/* by default do not update anything */
	}

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {
		/* by default tree_t do not render any thing */
	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		/* by default tree_t is invisible */
		return region{};
	}

	/**
	 * Derived class must return visible region,
	 * If unknown the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() {
		/* by default tree_t is invisible */
		return region{};
	}

	/**
	 * return currently damaged area (absolute)
	 **/
	virtual region get_damaged() {
		/* by default tree_t has no damage */
		return region{};
	}

	/**
	 * make the component active.
	 **/
	virtual void activate(tree_t * t = nullptr) {

	}

	virtual bool button_press(xcb_button_press_event_t const * ev) { return false; }
	virtual bool button_release(xcb_button_release_event_t const * ev) { return false; }
	virtual bool button_motion(xcb_motion_notify_event_t const * ev) { return false; }
	virtual bool leave(xcb_leave_notify_event_t const * ev) { return false; }
	virtual bool enter(xcb_enter_notify_event_t const * ev) { return false; }
	virtual void expose(xcb_expose_event_t const * ev) { }

	virtual void trigger_redraw() { }

	/**
	 * return the root top level xid or XCB_WINDOW_NONE if not applicable.
	 **/
	virtual xcb_window_t get_xid() const { return XCB_WINDOW_NONE; }

	virtual xcb_window_t get_window() const {
		if(parent() != nullptr)
			return parent()->get_window();
		else
			return XCB_NONE;
	}

	virtual rect get_window_position() const {
		if(parent() != nullptr)
			return parent()->get_window_position();
		else
			return rect{};
	}

	virtual void queue_redraw() {
		if (parent() != nullptr)
			parent()->queue_redraw();
	}


	/**
	 * Useful template to generate node name.
	 **/
	template<char const c>
	string _get_node_name() const {
		char buffer[64];
		snprintf(buffer, 64, "%c #%016lx #%016lx", c, (uintptr_t) parent(),
				(uintptr_t) this);
		return string(buffer);
	}

	/**
	 * Print the tree recursively using node names.
	 **/
	void print_tree(int level = 0) const {
		char space[] = "                               ";
		space[level] = 0;
		cout << space << get_node_name() << endl;
		for(auto i: children()) {
			i->print_tree(level+1);
		}

	}

	/**
	 * Short cut to children(std::vector<tree_t *> & out)
	 **/
	vector<tree_t *> children() const {
		vector<tree_t *> ret;
		children(ret);
		return ret;
	}

	/**
	 * get all children recursively
	 **/
	void get_all_children_deep_first(vector<tree_t *> & out) const {
		for(auto x: children()) {
			x->get_all_children_deep_first(out);
			out.push_back(x);
		}
	}

	void get_all_children_root_first(vector<tree_t *> & out) const {
		for(auto x: children()) {
			out.push_back(x);
			x->get_all_children_root_first(out);
		}
	}

	/**
	 * Short cut to get_all_children(std::vector<tree_t *> & out)
	 **/
	vector<tree_t *> get_all_children() const {
		vector<tree_t *> ret;
		get_all_children_root_first(ret);
		return ret;
	}

	vector<tree_t *> get_all_children_deep_first() const {
		vector<tree_t *> ret;
		get_all_children_deep_first(ret);
		return ret;
	}

	vector<tree_t *> get_all_children_root_first() const {
		vector<tree_t *> ret;
		get_all_children_root_first(ret);
		return ret;
	}


	/**
	 * Short cut to get_visible_children(std::vector<tree_t *> & out)
	 **/
	vector<tree_t *> get_visible_children() {
		vector<tree_t *> ret;
		get_visible_children(ret);
		return ret;
	}

	void broadcast_trigger_redraw() {
		_broadcast_deep_first(&tree_t::_trigger_redraw_handler);
	}

	bool broadcast_button_press(xcb_button_press_event_t const * ev) {
		return _broadcast_deep_first(&tree_t::button_press, ev);
	}

	bool broadcast_button_release(xcb_button_release_event_t const * ev) {
		return _broadcast_deep_first(&tree_t::button_release, ev);
	}

	bool broadcast_button_motion(xcb_motion_notify_event_t const * ev) {
		return _broadcast_deep_first(&tree_t::button_motion, ev);
	}

	bool broadcast_leave(xcb_leave_notify_event_t const * ev) {
		return _broadcast_deep_first(&tree_t::leave, ev);
	}

	bool broadcast_enter(xcb_enter_notify_event_t const * ev) {
		return _broadcast_deep_first(&tree_t::enter, ev);
	}

	void broadcast_expose(xcb_expose_event_t const * ev) {
		_broadcast_deep_first(&tree_t::expose, ev);
	}

	void broadcast_update_layout(time64_t const time) {
		_broadcast_root_first(&tree_t::update_layout, time);
	}

	rect to_root_position(rect const & r) const {
		return rect{r.x+get_window_position().x, r.y+get_window_position().y, r.w, r.h};
	}

};


}

#endif /* TREE_HXX_ */
