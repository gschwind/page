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

namespace page {

tree_t::tree_t() :
		_parent { }, _is_visible { false } {
}

tree_t::~tree_t() {
}

/**
 * Return parent within tree
 **/
auto tree_t::parent() const -> weak_ptr<tree_t> {
	return _parent;
}

/**
 * Change parent of this node to parent.
 **/
void tree_t::set_parent(shared_ptr<tree_t> parent) {
	_parent = parent;
}

void tree_t::clear_parent() {
	_parent.reset();
}

/**
 * Hide this node recursively
 **/
void tree_t::hide() {
	_is_visible = false;
}

/**
 * Show this node recursively
 **/
void tree_t::show() {
	_is_visible = true;
}

/**
 * Return the name of this tree node
 **/
auto tree_t::get_node_name() const -> string {
	return string { "ANONYMOUS NODE" };
}

/**
 * Remove i from _direct_ child of this node *not recusively*
 **/
void tree_t::remove(shared_ptr<tree_t> t) {
	/* because by default we have no child, by default remove do nothing */
}

/**
 * Return direct children of this node (net recursive)
 **/
auto tree_t::children(vector<shared_ptr<tree_t>> & out) const -> void {
	/* by default we have no child */
}

/**
 * return the list of renderable object to draw this tree ordered and recursively
 **/
auto tree_t::update_layout(time64_t const time) -> void {
	/* by default do not update anything */
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

/**
 * Derived class must return opaque region for this object,
 * If unknown it's safe to leave this empty.
 **/
region tree_t::get_opaque_region() {
	/* by default tree_t is invisible */
	return region { };
}

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
region tree_t::get_damaged() {
	/* by default tree_t has no damage */
	return region { };
}

/**
 * make the component active.
 **/
void tree_t::activate(shared_ptr<tree_t> t) {
}

bool tree_t::button_press(xcb_button_press_event_t const * ev) {
	return false;
}
bool tree_t::button_release(xcb_button_release_event_t const * ev) {
	return false;
}
bool tree_t::button_motion(xcb_motion_notify_event_t const * ev) {
	return false;
}
bool tree_t::leave(xcb_leave_notify_event_t const * ev) {
	return false;
}
bool tree_t::enter(xcb_enter_notify_event_t const * ev) {
	return false;
}
void tree_t::expose(xcb_expose_event_t const * ev) {
}

void tree_t::trigger_redraw() {
}

/**
 * return the root top level xid or XCB_WINDOW_NONE if not applicable.
 **/
xcb_window_t tree_t::get_xid() const {
	return XCB_WINDOW_NONE;
}

xcb_window_t tree_t::get_parent_xid() const {
	if (not _parent.expired())
		return _parent.lock()->get_parent_xid();
	else
		return XCB_WINDOW_NONE;
}

rect tree_t::get_window_position() const {
	if (not _parent.expired())
		return _parent.lock()->get_window_position();
	else
		return rect { };
}

void tree_t::queue_redraw() {
	if (not _parent.expired())
		_parent.lock()->queue_redraw();
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
vector<shared_ptr<tree_t>> tree_t::children() const {
	vector<shared_ptr<tree_t>> ret;
	children(ret);
	return ret;
}

/**
 * get all children recursively
 **/
void tree_t::get_all_children_deep_first(
		vector<shared_ptr<tree_t>> & out) const {
	for (auto x : children()) {
		x->get_all_children_deep_first(out);
		out.push_back(x);
	}
}

void tree_t::get_all_children_root_first(
		vector<shared_ptr<tree_t>> & out) const {
	for (auto x : children()) {
		out.push_back(x);
		x->get_all_children_root_first(out);
	}
}

vector<shared_ptr<tree_t>> tree_t::get_all_children() const {
	vector<shared_ptr<tree_t>> ret;
	get_all_children_root_first(ret);
	return ret;
}

vector<shared_ptr<tree_t>> tree_t::get_all_children_deep_first() const {
	vector<shared_ptr<tree_t>> ret;
	get_all_children_deep_first(ret);
	return ret;
}

vector<shared_ptr<tree_t>> tree_t::get_all_children_root_first() const {
	vector<shared_ptr<tree_t>> ret;
	get_all_children_root_first(ret);
	return ret;
}

void tree_t::broadcast_trigger_redraw() {
	_broadcast_deep_first(&tree_t::trigger_redraw);
}

bool tree_t::broadcast_button_press(xcb_button_press_event_t const * ev) {
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

rect tree_t::to_root_position(rect const & r) const {
	return rect { r.x + get_window_position().x, r.y + get_window_position().y,
			r.w, r.h };
}

}
