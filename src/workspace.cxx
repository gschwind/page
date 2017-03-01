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

namespace page {

using namespace std;

time64_t const workspace_t::_switch_duration{0.5};

workspace_t::workspace_t(page_t * ctx, unsigned id) :
	_ctx{ctx},
	//_allocation{},
	_default_pop{},
	_workarea{},
	_primary_viewport{},
	_id{id},
	_switch_renderable{nullptr},
	_switch_direction{WORKSPACE_SWITCH_LEFT}
{
	_viewport_layer = make_shared<tree_t>();
	_floating_layer = make_shared<tree_t>();
	_fullscreen_layer = make_shared<tree_t>();

	push_back(_viewport_layer);
	push_back(_floating_layer);
	push_back(_fullscreen_layer);

	set_to_default_name();

}

static bool is_dock(shared_ptr<tree_t> const & x) {
	auto c = dynamic_pointer_cast<client_managed_t>(x);
	if(c != nullptr) {
		return c->is(MANAGED_DOCK);
	}
	return false;
}

string workspace_t::get_node_name() const {
	return _get_node_name<'D'>();
}

void workspace_t::update_layout(time64_t const time) {
	if(not _is_visible)
		return;

	if (_switch_renderable != nullptr and time < (_switch_start_time + _switch_duration)) {
		double ratio = (static_cast<double>(time - _switch_start_time) / static_cast<double const>(_switch_duration));
		ratio = ratio*1.05 - 0.025;
		ratio = min(1.0, max(0.0, ratio));
		int new_x = _ctx->left_most_border();
		if(_switch_direction == WORKSPACE_SWITCH_LEFT) {
			new_x += ratio*_switch_screenshot->witdh();
		} else {
			new_x -= ratio*_switch_screenshot->witdh();
		}
		//_switch_renderable->move(new_x, _ctx->top_most_border());
	} else if (_switch_renderable != nullptr) {
		for(auto x: get_viewports()) {
			_ctx->add_global_damage(x->raw_area());
		}

		remove(_switch_renderable);
		_switch_screenshot = nullptr;
		_switch_renderable = nullptr;
	}

}

void workspace_t::activate() {
	_ctx->switch_to_workspace(id(), XCB_CURRENT_TIME);
}

void workspace_t::activate(shared_ptr<tree_t> t) {
	activate();
	/* do no reorder layers */
}

//rect workspace_t::allocation() const {
//	return _allocation;
//}

auto workspace_t::get_viewport_map() const -> vector<shared_ptr<viewport_t>> {
	return _viewport_outputs;
}

auto workspace_t::set_layout(vector<shared_ptr<viewport_t>> const & new_layout) -> void {
	_viewport_outputs = new_layout;
	_viewport_layer->clear();
	for(auto x: _viewport_outputs) {
		_viewport_layer->push_back(x);
		x->show();
	}

	if(_viewport_outputs.size() > 0) {
		_primary_viewport = _viewport_outputs[0];
	} else {
		_primary_viewport.reset();
	}

}

auto workspace_t::get_any_viewport() const -> shared_ptr<viewport_t> {
	return _primary_viewport.lock();
}

auto workspace_t::get_viewports() const -> vector<shared_ptr<viewport_t>> {
	return filter_class<viewport_t>(_viewport_layer->children());
}

void workspace_t::set_default_pop(shared_ptr<notebook_t> n) {
	if (not _default_pop.expired()) {
		_default_pop.lock()->set_default(false);
	}

	if (n != nullptr) {
		_default_pop = n;
		_default_pop.lock()->set_default(true);
	}
}

shared_ptr<notebook_t> workspace_t::default_pop() {
	return _default_pop.lock();
}

void workspace_t::update_default_pop() {
	auto notebooks = filter_class<notebook_t>(get_all_children());
	assert(notebooks.size() > 0); // workspace must have at less one notebook.

	if(_default_pop.expired()) {
		set_default_pop(notebooks[0]);
	}

	/* check if the current default_pop is valid, if yes do nothing */
	for(auto const & x: notebooks) {
		if(_default_pop.lock() == x)
			return;
	}

	/* else */
	set_default_pop(notebooks[0]);

}

void workspace_t::attach(shared_ptr<client_managed_t> c) {
	assert(c != nullptr);

	if(c->is(MANAGED_FULLSCREEN)) {
		_fullscreen_layer->push_back(c);
	} else {
		_floating_layer->push_back(c);
	}

	if(_is_visible) {
		c->show();
	} else {
		c->hide();
	}
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
	_switch_renderable = make_shared<renderable_pixmap_t>(_ctx,
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

void workspace_t::hide() {
	for(auto x: children()) {
		x->hide();
	}

	_is_visible = false;
}

void workspace_t::show() {
	_is_visible = true;

	for(auto x: children()) {
		x->show();
	}
}

bool workspace_t::client_focus_history_front(shared_ptr<client_managed_t> & out) {
	if(not client_focus_history_is_empty()) {
		out = _client_focus_history.front().lock();
		return true;
	}
	return false;
}

void workspace_t::client_focus_history_remove(shared_ptr<client_managed_t> in) {
	_client_focus_history.remove_if([in](weak_ptr<tree_t> w) { return w.expired() or w.lock() == in; });
}

void workspace_t::client_focus_history_move_front(shared_ptr<client_managed_t> in) {
	move_front(_client_focus_history, in);
}

bool workspace_t::client_focus_history_is_empty() {
	_client_focus_history.remove_if([](weak_ptr<tree_t> const & w) { return w.expired(); });
	return _client_focus_history.empty();
}

}
