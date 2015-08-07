/*
 * desktop.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <algorithm>
#include <typeinfo>
#include "viewport.hxx"
#include "desktop.hxx"

namespace page {

using namespace std;

time64_t const workspace_t::_switch_duration{0.5};

workspace_t::workspace_t(page_context_t * ctx, unsigned id) :
	_ctx{ctx},
	_allocation{},
	_default_pop{},
	_workarea{},
	_primary_viewport{},
	_id{id},
	_switch_renderable{nullptr},
	_switch_direction{WORKSPACE_SWITCH_LEFT}
{

}

void workspace_t::activate(shared_ptr<tree_t> t) {

	if(not _parent.expired()) {
		_parent.lock()->activate(shared_from_this());
	}

	if(t == nullptr)
		return;

	auto mw = dynamic_pointer_cast<client_managed_t>(t);

	if(has_key(_floating_layer, mw)) {
		_floating_layer.remove(mw);
		_floating_layer.push_back(mw);
	}

	if(has_key(_viewport_layer, t)) {
		_viewport_layer.remove(t);
		_viewport_layer.push_back(t);
	}

}

std::string workspace_t::get_node_name() const {
	return _get_node_name<'D'>();
}

void workspace_t::update_layout(time64_t const time) {
	if(not _is_visible)
		return;

	if (_switch_screenshot != nullptr and time < (_switch_start_time + _switch_duration)) {
		_ctx->add_global_damage(_allocation);
		double ratio = (static_cast<double>(time - _switch_start_time) / static_cast<double const>(_switch_duration));
		ratio = ratio*1.05 - 0.025;
		ratio = min(1.0, max(0.0, ratio));
		rect pos{_allocation};
		if(_switch_direction == WORKSPACE_SWITCH_LEFT) {
			pos.x += ratio*_switch_screenshot->witdh();
		} else {
			pos.x -= ratio*_switch_screenshot->witdh();
		}

		delete _switch_renderable;
		_switch_renderable = new renderable_pixmap_t{_switch_screenshot, pos, pos};
	} else if (_switch_screenshot != nullptr) {
		_ctx->add_global_damage(_allocation);
		_switch_screenshot = nullptr;
		delete _switch_renderable;
		_switch_renderable = nullptr;
	}

}

rect workspace_t::allocation() const {
	return _allocation;
}

auto workspace_t::get_viewport_map() const -> vector<shared_ptr<viewport_t>> {
	return _viewport_outputs;
}

auto workspace_t::set_layout(vector<shared_ptr<viewport_t>> const & new_layout) -> void {
	_viewport_outputs = new_layout;
	_viewport_layer.remove_if([](shared_ptr<tree_t> const & t) -> bool { return typeid(viewport_t) == typeid(*t.get()); });
	_viewport_layer.insert(_viewport_layer.end(), _viewport_outputs.begin(), _viewport_outputs.end());
}

auto workspace_t::get_any_viewport() const -> shared_ptr<viewport_t> {
	return _primary_viewport.lock();
}

auto workspace_t::get_viewports() const -> vector<shared_ptr<viewport_t>> {
	auto tmp = filter_class<viewport_t>(_viewport_layer);
	return vector<shared_ptr<viewport_t>>{tmp.begin(), tmp.end()};
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

void workspace_t::children(vector<shared_ptr<tree_t>> & out) const {
	out.insert(out.end(), _viewport_layer.begin(), _viewport_layer.end());
	out.insert(out.end(), _floating_layer.begin(), _floating_layer.end());
	//if(_switch_renderable != nullptr)
	//	out.push_back(_switch_renderable);
}

void workspace_t::update_default_pop() {
	_default_pop.reset();
	for(auto i: filter_class<notebook_t>(get_all_children())) {
		i->set_default(true);
		_default_pop = i;
		return;
	}
}

void workspace_t::add_floating_client(shared_ptr<client_managed_t> c) {
	if(c != nullptr)
		return;

	c->set_parent(shared_from_this());

	if(c->is(MANAGED_DOCK)) {
		_viewport_layer.push_back(c);
	} else {
		_floating_layer.push_back(c);
	}
}

void workspace_t::replace(shared_ptr<page_component_t> src, shared_ptr<page_component_t> by) {
	throw std::runtime_error("desktop_t::replace implemented yet!");
}

void workspace_t::remove(shared_ptr<tree_t> src) {

	if(has_key(_viewport_outputs, dynamic_pointer_cast<viewport_t>(src))) {
		throw exception_t("%s:%d invalid call of viewport::remove", __FILE__, __LINE__);
	}

	/**
	 * use reinterpret_cast because we try to remove src pointer
	 * and we don't care is type
	 **/
	_floating_layer.remove(dynamic_pointer_cast<client_managed_t>(src));
	_viewport_layer.remove(src);
}

void workspace_t::set_allocation(rect const & area) {
	_allocation = area;
}

bool workspace_t::is_hidden() {
	return not _is_visible;
}

void workspace_t::set_workarea(rect const & r) {
	_workarea = r;
}

rect const & workspace_t::workarea() {
	return _workarea;
}

void workspace_t::set_primary_viewport(shared_ptr<viewport_t> v) {
	if(not has_key(_viewport_layer, dynamic_pointer_cast<tree_t>(v)))
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

	_switch_direction = direction;
	_switch_start_time.update_to_current_time();
	_switch_screenshot = _ctx->cmp()->create_screenshot();

}


}
