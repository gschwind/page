/*
 * workspace.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <algorithm>
#include <typeinfo>

#include "page.hxx"
#include "viewport.hxx"
#include "workspace.hxx"
#include "view.hxx"
#include "view_popup.hxx"
#include "view_fullscreen.hxx"
#include "view_floating.hxx"
#include "view_notebook.hxx"
#include "view_dock.hxx"

namespace page {

using namespace std;

time64_t const workspace_t::_switch_duration{0.5};

void workspace_t::_fix_view_floating_position()
{
	region new_layout_region;
	for(auto x: _viewport_outputs) {
		new_layout_region += x->allocation();
	}

	int default_x = _viewport_outputs[0]->allocation().x
			+ _ctx->_theme->floating.margin.left;
	int default_y = _viewport_outputs[0]->allocation().y
			+ _ctx->_theme->floating.margin.top;

	// update position of floating managed clients to avoid offscreen
	// floating window
	for(auto x: gather_children_root_first<view_floating_t>()) {
		auto r = x->_base_position;
		/**
		 * if the current window do not overlap any desktop move to
		 * the center of an existing monitor
		 **/
		if((new_layout_region & r).empty()) {
			r.x = default_x;
			r.y = default_y;
			x->_client->set_floating_wished_position(r);
			x->reconfigure();
		}
	}
}

xcb_atom_t workspace_t::A(atom_e atom)
{
	return _ctx->_dpy->A(atom);
}

workspace_t::workspace_t(page_t * ctx, unsigned id) :
	tree_t{this},
	_ctx{ctx},
	//_allocation{},
	_default_pop{},
	_workarea{},
	_primary_viewport{},
	_id{id},
	_switch_renderable{nullptr},
	_switch_direction{WORKSPACE_SWITCH_LEFT},
	_is_enable{false}
{

	_stack_is_locked = true;

	_viewport_layer = make_shared<tree_t>(this);
	_floating_layer = make_shared<tree_t>(this);
	_dock_layer = make_shared<tree_t>(this);
	_fullscreen_layer = make_shared<tree_t>(this);
	_tooltips_layer = make_shared<tree_t>(this);
	_notification_layer = make_shared<tree_t>(this);
	_overlays_layer = make_shared<tree_t>(this);
	_unknown_layer = make_shared<tree_t>(this);

	push_back(_viewport_layer);
	push_back(_floating_layer);
	push_back(_dock_layer);
	push_back(_fullscreen_layer);
	push_back(_tooltips_layer);
	push_back(_notification_layer);
	push_back(_overlays_layer);

	push_back(_unknown_layer);

	set_to_default_name();

}

workspace_t::~workspace_t()
{
	_viewport_layer.reset();
	_floating_layer.reset();
	_dock_layer.reset();
	_fullscreen_layer.reset();
	_tooltips_layer.reset();
	_notification_layer.reset();
	_overlays_layer.reset();
	_unknown_layer.reset();
}

auto workspace_t::shared_from_this() -> workspace_p
{
	return static_pointer_cast<workspace_t>(tree_t::shared_from_this());
}

string workspace_t::get_node_name() const {
	return _get_node_name<'D'>();
}

void workspace_t::update_layout(time64_t const time) {
	if(not _is_visible)
		return;

}

auto workspace_t::get_viewport_map() const -> vector<viewport_p> {
	return _viewport_outputs;
}

void workspace_t::update_viewports_layout(vector<rect> const & layout)
{
	_viewport_layer->clear();

	/** get old layout to recycle old viewport, and keep unchanged outputs **/
	auto old_layout = _viewport_outputs;
	/** store the newer layout, to be able to cleanup obsolete viewports **/
	_viewport_outputs.clear();
	/** for each not overlaped rectangle **/
	for (unsigned i = 0; i < layout.size(); ++i) {
//		printf("%d: found viewport (%d,%d,%d,%d)\n", id(),
//				layout[i].x, layout[i].y,
//				layout[i].w, layout[i].h);
		viewport_p vp;
		if (i < old_layout.size()) {
			vp = old_layout[i];
			vp->set_raw_area(layout[i]);
		} else {
			vp = make_shared<viewport_t>(this, layout[i]);
		}
		_viewport_outputs.push_back(vp);
		_viewport_layer->push_back(vp);
	}

	/** clean up obsolete layout **/
	for (unsigned i = _viewport_outputs.size(); i < old_layout.size(); ++i) {
		/** destroy this viewport **/
		remove_viewport(old_layout[i]);
		old_layout[i].reset();
	}

	_primary_viewport = _viewport_outputs[0];

	for (auto x: _viewport_outputs) {
		_ctx->compute_viewport_allocation(shared_from_this(), x);
	}

	_fix_view_floating_position();

	// update visibility
	if (_is_visible)
		show();
	else
		hide();

}

void workspace_t::remove_viewport(viewport_p v)
{
	/* remove fullscreened clients if needed */
	for (auto &x : gather_children_root_first<view_fullscreen_t>()) {
		if (x->_viewport.lock() == v) {
			switch_fullscreen_to_prefered_view_mode(x, XCB_CURRENT_TIME);
			break;
		}
	}

	/* Transfer clients to a valid notebook */
	for (auto x : v->gather_children_root_first<view_notebook_t>()) {
		ensure_default_notebook()->add_client_from_view(x, XCB_CURRENT_TIME);
	}

	for (auto x : v->gather_children_root_first<view_floating_t>()) {
		insert_as_floating(x->_client, XCB_CURRENT_TIME);
	}

}

auto workspace_t::get_any_viewport() const -> viewport_p {
	return _primary_viewport.lock();
}

auto workspace_t::get_viewports() const -> vector<viewport_p> {
	return _viewport_layer->gather_children_root_first<viewport_t>();
}

void workspace_t::set_default_pop(notebook_p n) {
	assert(n->_root == this);

	if (not _default_pop.expired()) {
		_default_pop.lock()->set_default(false);
	}

	if (n != nullptr) {
		_default_pop = n;
		_default_pop.lock()->set_default(true);
	}
}

auto workspace_t::ensure_default_notebook() -> notebook_p {
	auto notebooks = gather_children_root_first<notebook_t>();
	assert(notebooks.size() > 0); // workspace must have at less one notebook.


	if(_default_pop.expired()) {
		_default_pop = notebooks[0];
		notebooks[0]->set_default(true);
		return notebooks[0];
	}

	if(not has_key(notebooks, _default_pop.lock())) {
		_default_pop = notebooks[0];
		notebooks[0]->set_default(true);
		return notebooks[0];
	}

	return _default_pop.lock();

}

void workspace_t::attach(shared_ptr<client_managed_t> c) {
	assert(c != nullptr);

//	if(c->is(MANAGED_FULLSCREEN)) {
//		_fullscreen_layer->push_back(c);
//	} else {
//		_floating_layer->push_back(c);
//	}
//
//	if(_is_visible) {
//		c->show();
//	} else {
//		c->hide();
//	}
}

void workspace_t::enable(xcb_timestamp_t time)
{
	_is_enable = true;
	broadcast_on_workspace_enable();

	view_p focus;
	if(client_focus_history_front(focus)) {
		set_focus(focus, time);
	} else {
		set_focus(nullptr, time);
	}

}

void workspace_t::disable()
{
	_is_enable = false;
	broadcast_on_workspace_disable();
}

bool workspace_t::is_enable()
{
	return _is_enable;
}

void workspace_t::insert_as_popup(client_managed_p c, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	c->set_managed_type(MANAGED_POPUP);

	auto wid = c->ensure_workspace();
	if (wid != ALL_DESKTOP) {
		c->set_net_wm_desktop(_id);
	}

	auto fv = make_shared<view_popup_t>(this, c);
	if (is_enable())
		fv->acquire_client();

	view_p v;
	auto transient_for = dynamic_pointer_cast<client_managed_t>(_ctx->get_transient_for(c));
	if (transient_for != nullptr)
		v = lookup_view_for(transient_for);

	if (v != nullptr) {
		v->add_popup(fv);
	} else {
		if (c->wm_type() == A(_NET_WM_WINDOW_TYPE_TOOLTIP)) {
			add_tooltips(fv);
		} else if (c->wm_type() == A(_NET_WM_WINDOW_TYPE_NOTIFICATION)) {
			add_notification(fv);
		} else {
			add_unknown(fv);
		}
	}

	_ctx->_need_restack = true;
	fv->raise();
	fv->show();

	_ctx->add_global_damage(fv->get_visible_region());
	_ctx->schedule_repaint(0L);

}

void workspace_t::insert_as_dock(client_managed_p c, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	c->set_managed_type(MANAGED_DOCK);

	auto wid = c->ensure_workspace();
	if(wid != ALL_DESKTOP) {
		c->set_net_wm_desktop(_id);
	}

	auto fv = make_shared<view_dock_t>(this, c);
	if(is_enable())
		fv->acquire_client();

	add_dock(fv);

	fv->raise();
	fv->show();

	if(is_enable())
		set_focus(fv, XCB_CURRENT_TIME);

	_ctx->update_workarea();
	_ctx->_need_restack = true;
}

void workspace_t::insert_as_floating(client_managed_p c, xcb_timestamp_t time)
{
	auto fv = make_shared<view_floating_t>(this, c);
	_insert_view_floating(fv, time);
}

void workspace_t::insert_as_fullscreen(client_managed_p mw, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	auto v = get_any_viewport();
	insert_as_fullscreen(mw, v);
}

void workspace_t::insert_as_notebook(client_managed_p mw, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);

	/** select if the client want to appear mapped or iconic **/
	bool activate = true;

	/**
	 * first try if previous vm has put this window in IconicState, then
	 * Check if the client have a preferred initial state.
	 **/
	if (mw->get<p_wm_state>() != nullptr) {
		if (mw->get<p_wm_state>()->state == IconicState) {
			activate = false;
		}
	} else {
		if (mw->_wm_hints != nullptr) {
			if ((mw->_wm_hints->flags & StateHint)
					and (mw->_wm_hints->initial_state == IconicState)) {
				activate = false;
			}
		}
	}

	if(activate and time == XCB_CURRENT_TIME) {
		_ctx->get_safe_net_wm_user_time(mw, time);
	}

	ensure_default_notebook()->add_client(mw, time);

	_ctx->_need_update_client_list = true;
	_ctx->_need_restack = true;
}

void workspace_t::insert_as_fullscreen(shared_ptr<client_managed_t> mw, shared_ptr<viewport_t> v) {
	//printf("call %s\n", __PRETTY_FUNCTION__);
	assert(v != nullptr);

	if(mw->is(MANAGED_FULLSCREEN))
		return;
	mw->set_managed_type(MANAGED_FULLSCREEN);

	auto fv = make_shared<view_fullscreen_t>(mw, v);
	if(is_enable())
		fv->acquire_client();

	/* default: revert to default pop */
	fv->revert_type = MANAGED_NOTEBOOK;
	fv->revert_notebook.reset();

	// unfullscreen client that already use this screen
	for (auto & x : gather_children_root_first<view_fullscreen_t>()) {
		switch_fullscreen_to_prefered_view_mode(x, XCB_CURRENT_TIME);
	}

	add_fullscreen(fv);
	fv->show();

	/* hide the viewport because he is covered by the fullscreen client */
	v->hide();
	_ctx->_need_restack = true;
}

void workspace_t::switch_view_to_fullscreen(view_p v, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	auto vx = dynamic_pointer_cast<view_floating_t>(v);
	if(vx) {
		switch_floating_to_fullscreen(vx, time);
		return;
	}

	auto vn = dynamic_pointer_cast<view_notebook_t>(v);
	if(vn) {
		switch_notebook_to_fullscreen(vn, time);
		return;
	}
}

void workspace_t::switch_view_to_floating(view_p v, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	auto vn = dynamic_pointer_cast<view_notebook_t>(v);
	if(vn) {
		switch_notebook_to_floating(vn, time);
		return;
	}

	auto vf = dynamic_pointer_cast<view_fullscreen_t>(v);
	if(vf) {
		switch_fullscreen_to_floating(vf, time);
		return;
	}
}

void workspace_t::switch_view_to_notebook(view_p v, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	auto vx = dynamic_pointer_cast<view_floating_t>(v);
	if(vx) {
		switch_floating_to_notebook(vx, time);
		return;
	}

	auto vf = dynamic_pointer_cast<view_fullscreen_t>(v);
	if(vf) {
		switch_fullscreen_to_notebook(vf, time);
		return;
	}
}

void workspace_t::switch_notebook_to_floating(view_notebook_p vn, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	vn->remove_this_view();
	auto vf = make_shared<view_floating_t>(vn.get());
	_insert_view_floating(vf, time);
}

void workspace_t::switch_notebook_to_fullscreen(view_notebook_p vn, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	auto v = _find_viewport_of(vn);
	auto nbk = vn->parent_notebook();
	assert(nbk != nullptr);
	nbk->remove_view_notebook(vn);
	auto vf = make_shared<view_fullscreen_t>(vn.get(), v);
	vf->revert_type = MANAGED_NOTEBOOK;
	vf->revert_notebook = nbk;
	if(v->_root->is_enable())
		vf->acquire_client();
	_insert_view_fullscreen(vf, time);
}

void workspace_t::switch_floating_to_fullscreen(view_floating_p vx, xcb_timestamp_t time)
{
	//printf("call %s\n", __PRETTY_FUNCTION__);
	auto viewport = get_any_viewport();
	vx->remove_this_view();
	auto vf = make_shared<view_fullscreen_t>(vx.get(), viewport);
	if(is_enable())
		vf->acquire_client();
	vf->revert_type = MANAGED_FLOATING;
	_insert_view_fullscreen(vf, time);
}

void workspace_t::switch_floating_to_notebook(view_floating_p vf, xcb_timestamp_t time)
{
	vf->remove_this_view();
	ensure_default_notebook()->add_client_from_view(vf, time);
}

void workspace_t::switch_fullscreen_to_floating(view_fullscreen_p view, xcb_timestamp_t time)
{
	view->remove_this_view();

	if(is_visible() and not view->_viewport.expired()) {
		view->_viewport.lock()->show();
	}

	view->_client->net_wm_state_remove(_NET_WM_STATE_FULLSCREEN);
	auto fv = make_shared<view_floating_t>(view.get());
	_insert_view_floating(fv, time);
}

void workspace_t::switch_fullscreen_to_notebook(view_fullscreen_p view, xcb_timestamp_t time)
{
	view->remove_this_view();

	if(is_visible() and not view->_viewport.expired()) {
		view->_viewport.lock()->show();
	}

	auto n = ensure_default_notebook();
	if(not view->revert_notebook.expired()) {
		n = view->revert_notebook.lock();
	}

	view->_client->net_wm_state_remove(_NET_WM_STATE_FULLSCREEN);
	view->_client->set_managed_type(MANAGED_NOTEBOOK);
	n->add_client_from_view(view, time);
	_ctx->_need_restack = true;
}

/* switch a fullscreened and managed window into floating or notebook window */
void workspace_t::switch_fullscreen_to_prefered_view_mode(view_p c, xcb_timestamp_t time)
{
	auto vf = dynamic_pointer_cast<view_fullscreen_t>(c);
	if(vf)
		switch_fullscreen_to_prefered_view_mode(vf, time);
}

void workspace_t::switch_fullscreen_to_prefered_view_mode(view_fullscreen_p view, xcb_timestamp_t time)
{
	view->remove_this_view();

	if (view->revert_type == MANAGED_NOTEBOOK) {
		auto n = ensure_default_notebook();
		if(not view->revert_notebook.expired()) {
			n = view->revert_notebook.lock();
		}
		view->_client->net_wm_state_remove(_NET_WM_STATE_FULLSCREEN);
		view->_client->set_managed_type(MANAGED_NOTEBOOK);
		n->add_client_from_view(view, time);
	} else {
		view->_client->net_wm_state_remove(_NET_WM_STATE_FULLSCREEN);
		auto vf = make_shared<view_floating_t>(view.get());
		_insert_view_floating(vf, time);
	}

	_ctx->_need_restack = true;
}


void workspace_t::add_dock(shared_ptr<tree_t> c)
{
	_dock_layer->push_back(c);
}

void workspace_t::add_floating(shared_ptr<tree_t> c)
{
	_floating_layer->push_back(c);
}

void workspace_t::add_fullscreen(shared_ptr<tree_t> c)
{
	_fullscreen_layer->push_back(c);
}

void workspace_t::add_overlay(shared_ptr<tree_t> t)
{
	_overlays_layer->push_back(t);
}

void workspace_t::add_unknown(shared_ptr<tree_t> c)
{
	_unknown_layer->push_back(c);
}

void workspace_t::add_tooltips(shared_ptr<tree_t> t)
{
	_tooltips_layer->push_back(t);
}

void workspace_t::add_notification(shared_ptr<tree_t> t)
{
	_notification_layer->push_back(t);
}

void workspace_t::set_name(string const & s) {
	_name = s;
}

auto workspace_t::name() -> string const & {
	return _name;
}

void workspace_t::set_to_default_name() {
	std::ostringstream os;
	os << "Workspace #" << _id;
	_name = os.str();
}

void workspace_t::set_workarea(rect const & r) {
	_workarea = r;
}

rect const & workspace_t::workarea() {
	return _workarea;
}

void workspace_t::set_primary_viewport(shared_ptr<viewport_t> v) {
	if(not has_key(_viewport_outputs, v))
		throw exception_t("invalid primary viewport");
	_primary_viewport = v;
}

shared_ptr<viewport_t> workspace_t::primary_viewport() const {
	return _primary_viewport.lock();
}

int workspace_t::id() {
	return _id;
}

void workspace_t::start_switch(workspace_switch_direction_e direction) {
	if(_ctx->cmp() == nullptr)
		return;

	if(_switch_renderable != nullptr) {
		remove(_switch_renderable);
		_switch_renderable = nullptr;
	}

	_switch_direction = direction;
	_switch_start_time.update_to_current_time();
	//_switch_screenshot = _ctx->cmp()->create_screenshot();
	_switch_screenshot = _ctx->theme()->workspace_switch_popup(_name);
	_switch_renderable = make_shared<renderable_pixmap_t>(this,
			_switch_screenshot,
			(_ctx->left_most_border() - _switch_screenshot->witdh()) / 2.0,
			_ctx->top_most_border());
	push_back(_switch_renderable);
	_switch_renderable->show();
}

auto workspace_t::get_visible_region() -> region {
	/** workspace do not render any thing **/
	return region{};
}

auto workspace_t::get_opaque_region() -> region {
	/** workspace do not render any thing **/
	return region{};
}

auto workspace_t::get_damaged() -> region {
	/** workspace do not render any thing **/
	return region{};
}

void workspace_t::render(cairo_t * cr, region const & area) {

}

auto workspace_t::client_focus_history() -> list<view_w>
{
	return _client_focus_history;
}

bool workspace_t::client_focus_history_front(view_p & out) {
	if(not client_focus_history_is_empty()) {
		out = _client_focus_history.front().lock();
		return true;
	}
	return false;
}

void workspace_t::client_focus_history_remove(view_p in) {
	_client_focus_history.remove_if([in](view_w w) { return w.expired() or w.lock() == in; });
}

void workspace_t::client_focus_history_move_front(view_p in) {
	move_front(_client_focus_history, in);
}

bool workspace_t::client_focus_history_is_empty() {
	_client_focus_history.remove_if([](view_w const & w) { return w.expired(); });
	return _client_focus_history.empty();
}

auto workspace_t::lookup_view_for(client_managed_p c) const -> view_p
{
	for (auto & x: gather_children_root_first<view_t>()) {
		if (x->_client == c)
			return x;
	}
	return nullptr;
}

void workspace_t::set_focus(view_p new_focus, xcb_timestamp_t time) {
	if(new_focus) {
		client_focus_history_move_front(new_focus);
	}
	_net_active_window = new_focus;
	_ctx->apply_focus(time);
}

void workspace_t::unmanage(client_managed_p mw)
{
	auto v = lookup_view_for(mw);
	if (v == nullptr)
		return;

	bool has_focus = false;
	if (_net_active_window.lock() == v) {
		has_focus = true;
	}

	/* if managed window have active clients */
	log(LOG_MANAGE, "unmanaging : 0x%x '%s'\n", mw->_client_proxy->id(), mw->title().c_str());

	for (auto c : v->get_transients()) {
		c->remove_this_view();
		_ctx->insert_as_floating(c->_client);
	}

	client_focus_history_remove(v);
	v->remove_this_view();

	if (dynamic_pointer_cast<view_rebased_t>(v) != nullptr) {
		if (_ctx->configuration._auto_refocus and has_focus) {
			view_p focus;
			if (client_focus_history_front(focus)) {
				set_focus(focus, XCB_CURRENT_TIME);
				focus->xxactivate(XCB_CURRENT_TIME);
			}
		}
	}

}

auto workspace_t::_find_viewport_of(tree_p t) -> viewport_p {
	while(t != nullptr) {
		auto ret = dynamic_pointer_cast<viewport_t>(t);
		if(ret != nullptr)
			return ret;
		t = t->parent()->shared_from_this();
	}

	return nullptr;
}

void workspace_t::_insert_view_fullscreen(view_fullscreen_p vf, xcb_timestamp_t time)
{
	auto viewport = vf->_viewport.lock();
	auto workspace = viewport->workspace();

	// unfullscreen client that already use this viewport
	for (auto &x : workspace->gather_children_root_first<view_fullscreen_t>()) {
		if(x->_viewport.lock() == viewport)
			switch_fullscreen_to_prefered_view_mode(x, XCB_CURRENT_TIME);
	}

	vf->_client->set_managed_type(MANAGED_FULLSCREEN);
	workspace->add_fullscreen(vf);
	vf->show();

	/* hide the viewport because he is covered by the fullscreen client */
	viewport->hide();

	workspace->set_focus(vf, time);
	_ctx->_need_restack = true;
}

void workspace_t::_insert_view_floating(view_floating_p fv, xcb_timestamp_t time)
{
	auto c = fv->_client;
	c->set_managed_type(MANAGED_FLOATING);

	auto wid = c->ensure_workspace();
	if (wid != ALL_DESKTOP) {
		c->set_net_wm_desktop(_id);
	}

	if (is_enable())
		fv->acquire_client();

	view_p v;
	auto transient_for = dynamic_pointer_cast<client_managed_t>(_ctx->get_transient_for(c));
	if (transient_for != nullptr)
		v = lookup_view_for(transient_for);

	if (v != nullptr) {
		v->add_transient(fv);
	} else {
		add_floating(fv);
	}

	fv->raise();
	fv->show();
	set_focus(fv, time);
	_ctx->_need_restack = true;
}

}
