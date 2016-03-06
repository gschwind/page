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
#include <map>

#include "utils.hxx"
#include "renderable.hxx"
#include "time.hxx"
#include "transition.hxx"

namespace page {

using namespace std;

/**
 * tree_t is the base of the hierarchy of desktop, viewports,
 * client_managed and unmanaged, etc...
 * It define the stack order of each component drawn within page.
 **/
class tree_t : public enable_shared_from_this<tree_t> {

protected:
	template<typename ... T>
	bool _broadcast_root_first(bool (tree_t::* f)(T ... args), T ... args) {
		if((this->*f)(args...))
			return true;
		for(auto x: weak(get_all_children_root_first())) {
			if((x->*f)(args...))
				return true;
		}
		return false;
	}

	template<typename ... T>
	bool _broadcast_root_first(void (tree_t::* f)(T ... args), T ... args) {
		(this->*f)(args...);
		for(auto x: weak(get_all_children_root_first())) {
			if(not x.expired())
				(x.lock().get()->*f)(args...);
		}
		return true;
	}

	template<typename ... T>
	bool _broadcast_deep_first(bool (tree_t::* f)(T ... args), T ... args) {
		for(auto x: weak(get_all_children_deep_first())) {
			if(not x.expired()) {
				if((x.lock().get()->*f)(args...))
					return true;
			}
		}
		if((this->*f)(args...))
			return true;
		return false;
	}

	template<typename ... T>
	void _broadcast_deep_first(void (tree_t::* f)(T ... args), T ... args) {
		for(auto x: weak(get_all_children_deep_first())) {
			if(not x.expired())
				(x.lock().get()->*f)(args...);
		}
		(this->*f)(args...);
	}

	template<char const c>
	string _get_node_name() const {
		return xformat("%c(%ld) #%016lx #%016lx", c, shared_from_this().use_count(), _parent, (uintptr_t) this);
	}

	/**
	 * Parent must exist or beeing NULL, when a node is destroyed, he must
	 * clear children _parent.
	 **/
	tree_t * _parent;

	list<shared_ptr<tree_t>> _children;

	bool _is_visible;

	map<void *, shared_ptr<transition_t>> _transition;

private:
	tree_t(tree_t const &);
	tree_t & operator=(tree_t const &);

public:
	void set_parent(tree_t * parent);
	void clear_parent();

public:
	tree_t();

	auto parent() const -> shared_ptr<tree_t>;

	bool is_visible() const;

	void print_tree(int level = 0) const;

	vector<shared_ptr<tree_t>> children() const;
	vector<shared_ptr<tree_t>> get_all_children() const;
	vector<shared_ptr<tree_t>> get_all_children_deep_first() const;
	vector<shared_ptr<tree_t>> get_all_children_root_first() const;

	void get_all_children_deep_first(vector<shared_ptr<tree_t>> & out) const;
	void get_all_children_root_first(vector<shared_ptr<tree_t>> & out) const;

	void broadcast_trigger_redraw();

	bool broadcast_button_press(xcb_button_press_event_t const * ev);
	bool broadcast_button_release(xcb_button_release_event_t const * ev);
	bool broadcast_button_motion(xcb_motion_notify_event_t const * ev);
	bool broadcast_leave(xcb_leave_notify_event_t const * ev);
	bool broadcast_enter(xcb_enter_notify_event_t const * ev);
	void broadcast_expose(xcb_expose_event_t const * ev);
	void broadcast_update_layout(time64_t const time);
	void broadcast_render_finished();

	void add_transition(shared_ptr<transition_t> t);

	rect to_root_position(rect const & r) const;

	/**
	 * tree_t virtual API
	 **/

	virtual ~tree_t();
	virtual void hide();
	virtual void show();
	virtual auto get_node_name() const -> string;
	virtual void remove(shared_ptr<tree_t> t);
	virtual void clear();

	virtual void push_back(shared_ptr<tree_t> t);
	virtual void push_front(shared_ptr<tree_t> t);

	virtual void append_children(vector<shared_ptr<tree_t>> & out) const;
	virtual void update_layout(time64_t const time);
	virtual void render(cairo_t * cr, region const & area);
	virtual void trigger_redraw();
	virtual void render_finished();

	virtual auto get_opaque_region() -> region;
	virtual auto get_visible_region() -> region;
	virtual auto get_damaged() -> region;

	virtual void activate();
	virtual void activate(shared_ptr<tree_t> t);

	virtual bool button_press(xcb_button_press_event_t const * ev);
	virtual bool button_release(xcb_button_release_event_t const * ev);
	virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	virtual bool leave(xcb_leave_notify_event_t const * ev);
	virtual bool enter(xcb_enter_notify_event_t const * ev);
	virtual void expose(xcb_expose_event_t const * ev);

	virtual auto get_xid() const -> xcb_window_t;
	virtual auto get_parent_xid() const -> xcb_window_t;
	virtual rect get_window_position() const;
	virtual void queue_redraw();

};


}

#endif /* TREE_HXX_ */
