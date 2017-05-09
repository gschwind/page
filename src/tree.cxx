/*
 * tree.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "tree.hxx"

#include <cassert>

#include "utils.hxx"
#include "workspace.hxx"

namespace page {

tree_t::tree_t(workspace_t * root) :
	_root{root},
	_parent{nullptr},
	_is_visible{false},
	_stack_is_locked{false}
{

}

tree_t::~tree_t() {
	for(auto t: children())
		t->clear_parent();
	_children.clear();
}

/**
 * Return parent within tree
 **/
auto tree_t::parent() const -> shared_ptr<tree_t> {
	if(_parent) {
		return _parent->shared_from_this();
	} else {
		return shared_ptr<tree_t>{};
	}
}

/**
 * Change parent of this node to parent.
 **/
void tree_t::set_parent(tree_t * parent) {
	assert(parent != nullptr);
	assert(_parent == nullptr);
	_parent = parent;
}

void tree_t::clear_parent() {
	_parent = nullptr;
}

bool tree_t::is_visible() const {
	return _is_visible;
}

/**
 * Hide this node recursively
 **/
void tree_t::hide() {
	_is_visible = false;
	for(auto x: _children) {
		x->hide();
	}
}

/**
 * Show this node recursively
 **/
void tree_t::show() {
	_is_visible = true;
	for(auto x: _children) {
		x->show();
	}
}

/**
 * Return the name of this tree node
 **/
auto tree_t::get_node_name() const -> string {
	return string { "RAW" };
}

/**
 * Remove i from _direct_ child of this node *not recusively*
 **/
void tree_t::remove(shared_ptr<tree_t> t) {
	assert(has_key(_children, t));
	_children.remove(t);
	t->clear_parent();
}

void tree_t::clear()
{
	for(auto x: _children)
		x->clear_parent();
	_children.clear();
}

void tree_t::detach_myself()
{
	if(_parent != nullptr) {
		_parent->remove(shared_from_this());
	}
}

void tree_t::push_back(tree_p t)
{
	assert(t->parent() == nullptr);
	_children.push_back(t);
	t->set_parent(this);

	// TODO: remove this HACK.
	t->_root = _root;
}

void tree_t::push_front(tree_p t)
{
	assert(t->parent() == nullptr);
	_children.push_front(t);
	t->set_parent(this);

	// TODO: remove this HACK.
	t->_root = _root;
}


/**
 * Return direct children of this node (not recursive)
 **/
auto tree_t::gather_children(vector<tree_p> & out) const -> void
{
	out.insert(out.end(), _children.begin(), _children.end());
}

/**
 * return the list of renderable object to draw this tree ordered and recursively
 **/
auto tree_t::update_layout(time64_t const time) -> void
{
	auto x = _transition.begin();
	while(x != _transition.end()) {
		x->second->update(time);
		if(time > x->second->end()) {
			x = _transition.erase(x);
		} else {
			++x;
		}
	}
}

/**
 * draw the area of a renderable to the destination surface
 * @param cr the destination surface context
 * @param area the area to redraw
 **/
void tree_t::render(cairo_t * cr, region const & area) {
	/* by default tree_t do not render any thing */
}

void tree_t::render_finished() {

}

void tree_t::reconfigure()
{

}

void tree_t::on_workspace_enable()
{

}

void tree_t::on_workspace_disable()
{

}

/**
 * Derived class must return opaque region for this object,
 * If unknown it's safe to leave this empty.
 **/
//region tree_t::get_opaque_region() {
//	/* by default tree_t is invisible */
//	return region { };
//}

/**
 * Derived class must return visible region,
 * If unknown the whole screen can be returned, but draw will be called each time.
 **/
//region tree_t::get_visible_region() {
//	/* by default tree_t is invisible */
//	return region { };
//}

/**
 * return currently damaged area (absolute)
 **/
//region tree_t::get_damaged() {
//	/* by default tree_t has no damage */
//	return region { };
//}

/**
 * make the component active.
 **/

void tree_t::raise(shared_ptr<tree_t> t) {

	if(_parent != nullptr) {
		_parent->raise(shared_from_this());
	}

	if(t != nullptr and not _stack_is_locked) {
		assert(has_key(_children, t));
		move_back(_children, t);
	}

}

auto tree_t::workspace() const -> shared_ptr<workspace_t>
{
	if(_root) {
		return dynamic_pointer_cast<workspace_t>(_root->shared_from_this());
	} else {
		return nullptr;
	}
}

auto tree_t::lookup_view_for(client_managed_p c) const -> view_p
{
	return _root->workspace_t::lookup_view_for(c);
}

auto tree_t::button_press(xcb_button_press_event_t const * ev) -> button_action_e
{
	return BUTTON_ACTION_CONTINUE;
}

bool tree_t::button_release(xcb_button_release_event_t const * ev)
{
	return false;
}

bool tree_t::button_motion(xcb_motion_notify_event_t const * ev)
{
	return false;
}

bool tree_t::leave(xcb_leave_notify_event_t const * ev)
{
	return false;
}

bool tree_t::enter(xcb_enter_notify_event_t const * ev)
{
	return false;
}

void tree_t::expose(xcb_expose_event_t const * ev)
{
}

void tree_t::trigger_redraw()
{
}

/**
 * return the root top level xid or XCB_WINDOW_NONE if not applicable.
 **/
xcb_window_t tree_t::get_toplevel_xid() const {
	return XCB_WINDOW_NONE;
}

rect tree_t::get_window_position() const {
	if (_parent != nullptr)
		return _parent->get_window_position();
	else
		return rect { };
}

void tree_t::queue_redraw() {
	if (_parent != nullptr)
		_parent->queue_redraw();
}

/**
 * Print the tree recursively using node names.
 **/
void tree_t::print_tree(int level) const {
	char space[] = "                               ";
	space[level] = 0;
	cout << space << get_node_name() << endl;
	for (auto i : children()) {
		i->print_tree(level + 1);
	}
}

/**
 * Short cut to children(std::vector<tree_t *> & out)
 **/
auto tree_t::children() const -> vector<tree_p>
{
	return vector<tree_p>{_children.begin(), _children.end()};
}

/**
 * get all children recursively
 **/
void tree_t::get_all_children_deep_first(vector<tree_p> & out) const
{
	for (auto const & x : reversed(_children)) {
		x->get_all_children_deep_first(out);
		out.push_back(x);
	}
}

void tree_t::get_all_children_root_first(vector<tree_p> & out) const
{
	for (auto const & x : _children) {
		out.push_back(x);
		x->get_all_children_root_first(out);
	}
}

auto tree_t::get_all_children() const  -> vector<tree_p>
{
	vector<tree_p> ret;
	get_all_children_root_first(ret);
	return ret;
}

auto tree_t::get_all_children_deep_first() const  -> vector<tree_p>
{
	vector<tree_p> ret;
	get_all_children_deep_first(ret);
	return ret;
}

auto tree_t::get_all_children_root_first() const  -> vector<tree_p>
{
	vector<tree_p> ret;
	get_all_children_root_first(ret);
	return ret;
}

void tree_t::broadcast_trigger_redraw() {
	_broadcast_deep_first(&tree_t::trigger_redraw);
}

auto tree_t::broadcast_button_press(xcb_button_press_event_t const * ev) -> button_action_e {
	return _broadcast_deep_first(&tree_t::button_press, ev);
}

bool tree_t::broadcast_button_release(xcb_button_release_event_t const * ev) {
	return _broadcast_deep_first(&tree_t::button_release, ev);
}

bool tree_t::broadcast_button_motion(xcb_motion_notify_event_t const * ev) {
	return _broadcast_deep_first(&tree_t::button_motion, ev);
}

bool tree_t::broadcast_leave(xcb_leave_notify_event_t const * ev) {
	return _broadcast_deep_first(&tree_t::leave, ev);
}

bool tree_t::broadcast_enter(xcb_enter_notify_event_t const * ev) {
	return _broadcast_deep_first(&tree_t::enter, ev);
}

void tree_t::broadcast_expose(xcb_expose_event_t const * ev) {
	_broadcast_deep_first(&tree_t::expose, ev);
}

void tree_t::broadcast_update_layout(time64_t const time) {
	_broadcast_root_first(&tree_t::update_layout, time);
}

void tree_t::broadcast_render_finished() {
	_broadcast_root_first(&tree_t::render_finished);
}

void tree_t::broadcast_on_workspace_enable() {
	_broadcast_root_first(&tree_t::on_workspace_enable);
}

void tree_t::broadcast_on_workspace_disable() {
	_broadcast_root_first(&tree_t::on_workspace_disable);
}

void tree_t::add_transition(shared_ptr<transition_t> t) {
	_transition[t->target()] = t;
}

rect tree_t::to_root_position(rect const & r) const {
	return rect { r.x + get_window_position().x, r.y + get_window_position().y,
			r.w, r.h };
}

__attribute__((deprecated)) auto tree_t::get_opaque_region() -> region
{
	return region{};
}

__attribute__((deprecated)) auto tree_t::get_visible_region() -> region
{
	return region{};
}

__attribute__((deprecated)) auto tree_t::get_damaged() -> region
{
	return region{};
}

}
