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

/**
 * tree_t is the base of the hierarchy of desktop, viewports,
 * client_managed and unmanaged, etc...
 * It define the stack order of each component drawn within page.
 **/
class tree_t {
public:
	tree_t() { }

	virtual ~tree_t();

	/**
	 * Return parent within tree
	 **/
	virtual auto parent() const -> tree_t * = 0;

	/**
	 * Return the name of this tree node
	 **/
	virtual auto get_node_name() const -> std::string = 0;

	/**
	 * Remove i from direct child of this node (not recusively)
	 **/
	virtual auto remove(tree_t * t) -> void = 0;

	/**
	 * Change parent of this node to parent.
	 **/
	virtual auto set_parent(tree_t * parent) -> void = 0;

	/**
	 * Return direct children of this node (net recursive)
	 **/
	virtual auto children(std::vector<tree_t *> & out) const -> void = 0;

	/**
	 * get all children recursively
	 **/
	virtual auto get_all_children(std::vector<tree_t *> & out) const -> void = 0;

	/**
	 * get all visible children
	 **/
	virtual auto get_visible_children(std::vector<tree_t *> & out) -> void = 0;

	/**
	 * Hide this node recursively
	 **/
	virtual auto hide() -> void = 0;

	/**
	 * Show this node recursively
	 **/
	virtual auto show() -> void = 0;

	/**
	 * return the list of renderable object to draw this tree ordered and recursively
	 **/
	virtual auto prepare_render(std::vector<std::shared_ptr<renderable_t>> & out, page::time_t const & time) -> void = 0;

	/**
	 * make the component active.
	 **/
	virtual void activate(tree_t * t = nullptr) = 0;


	virtual bool button_press(xcb_button_press_event_t const * ev) {
		for(auto x: tree_t::children()) {
			if(x->button_press(ev))
				return true;
		}
		return false;
	}

	virtual bool button_release(xcb_button_release_event_t const * ev) {
		for(auto x: tree_t::children()) {
			if(x->button_release(ev))
				return true;
		}
		return false;
	}

	virtual bool button_motion(xcb_motion_notify_event_t const * ev) {
		for(auto x: tree_t::children()) {
			if(x->button_motion(ev))
				return true;
		}
		return false;
	}

	virtual xcb_window_t get_window() {
		if(parent() != nullptr)
			return parent()->get_window();
		else
			return XCB_WINDOW_NONE;
	}

	virtual i_rect get_window_postion() {
		if(parent() != nullptr)
			return parent()->get_window_postion();
		else
			return i_rect{};
	}


	/**
	 * Useful template to generate node name.
	 **/
	template<char const c>
	std::string _get_node_name() const {
		char buffer[64];
		snprintf(buffer, 64, "%c #%016lx #%016lx", c, (uintptr_t) parent(),
				(uintptr_t) this);
		return std::string(buffer);
	}

	/**
	 * Print the tree recursively using node names.
	 **/
	void print_tree(int level = 0) const {
		char space[] = "                               ";
		space[level] = 0;
		std::cout << space << get_node_name() << std::endl;
		for(auto i: children()) {
			i->print_tree(level+1);
		}

	}

	/**
	 * Short cut to children(std::vector<tree_t *> & out)
	 **/
	std::vector<tree_t *> children() const {
		std::vector<tree_t *> ret;
		children(ret);
		return ret;
	}

	/**
	 * Short cut to get_all_children(std::vector<tree_t *> & out)
	 **/
	std::vector<tree_t *> get_all_children() const {
		std::vector<tree_t *> ret;
		get_all_children(ret);
		return ret;
	}

	/**
	 * Short cut to get_visible_children(std::vector<tree_t *> & out)
	 **/
	std::vector<tree_t *> get_visible_children() {
		std::vector<tree_t *> ret;
		get_visible_children(ret);
		return ret;
	}

};


}

#endif /* TREE_HXX_ */
