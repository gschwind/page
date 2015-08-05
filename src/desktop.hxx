/*
 * desktop.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef DESKTOP_HXX_
#define DESKTOP_HXX_

#include <X11/extensions/Xrandr.h>

#include <map>
#include <vector>

#include "utils.hxx"
#include "page_context.hxx"
#include "viewport.hxx"
#include "client_managed.hxx"
#include "client_not_managed.hxx"
#include "renderable_pixmap.hxx"

namespace page {

using namespace std;

enum workspace_switch_direction_e {
	WORKSPACE_SWITCH_LEFT,
	WORKSPACE_SWITCH_RIGHT
};

struct workspace_t: public page_component_t {

private:
	page_context_t * _ctx;

	rect _allocation;
	rect _workarea;

	unsigned const _id;

	/* list of viewports in creation order, to make a sane reconfiguration */
	vector<shared_ptr<viewport_t>> _viewport_outputs;

	/* dock + viewport belong this layer */
	list<shared_ptr<tree_t>> _viewport_layer;

	/* floating and fullscreen window belong this layer */
	list<shared_ptr<client_managed_t>> _floating_layer;

	viewport_t * _primary_viewport;
	weak_ptr<notebook_t> _default_pop;

	workspace_t(workspace_t const & v);
	workspace_t & operator= (workspace_t const &);

	static time64_t const _switch_duration;

	time64_t _switch_start_time;
	shared_ptr<pixmap_t> _switch_screenshot;
	renderable_pixmap_t * _switch_renderable;

	workspace_switch_direction_e _switch_direction;

public:

	list<weak_ptr<client_managed_t>> client_focus;

	workspace_t(page_context_t * ctx, unsigned id) :
		_ctx{ctx},
		_allocation{},
		_default_pop{},
		_workarea{},
		_primary_viewport{nullptr},
		_id{id},
		_switch_renderable{nullptr},
		_switch_direction{WORKSPACE_SWITCH_LEFT}
	{

	}

	void render(cairo_t * cr, rect const & area) const { }


	void activate(shared_ptr<tree_t> t) {

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

	std::string get_node_name() const {
		return _get_node_name<'D'>();
	}

	virtual void update_layout(time64_t const time) {
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

	rect allocation() const {
		return _allocation;
	}

	void render_legacy(cairo_t * cr, rect const & area) const { }

	auto get_viewport_map() const -> vector<shared_ptr<viewport_t>> const & {
		return _viewport_outputs;
	}

	auto set_layout(vector<shared_ptr<viewport_t>> const & new_layout) -> void {
		_viewport_outputs = new_layout;
		_viewport_layer.remove_if([](shared_ptr<tree_t> const & t) -> bool { return typeid(viewport_t) == typeid(*t.get()); });
		_viewport_layer.insert(_viewport_layer.end(), _viewport_outputs.begin(), _viewport_outputs.end());
	}

	auto get_any_viewport() const -> viewport_t * {
		return _primary_viewport;
	}

	auto get_viewports() const -> vector<shared_ptr<viewport_t>> {
		auto tmp = filter_class<viewport_t>(_viewport_layer);
		return vector<shared_ptr<viewport_t>>{tmp.begin(), tmp.end()};
	}

	void set_default_pop(shared_ptr<notebook_t> n) {
		if (not _default_pop.expired()) {
			_default_pop.lock()->set_default(false);
		}

		if (n != nullptr) {
			_default_pop = n;
			_default_pop.lock()->set_default(true);
		}
	}

	shared_ptr<notebook_t> default_pop() {
		return _default_pop.lock();
	}

	void children(vector<shared_ptr<tree_t>> & out) const {
		out.insert(out.end(), _viewport_layer.begin(), _viewport_layer.end());
		out.insert(out.end(), _floating_layer.begin(), _floating_layer.end());
		//if(_switch_renderable != nullptr)
		//	out.push_back(_switch_renderable);
	}

	void update_default_pop() {
		_default_pop.reset();
		for(auto i: filter_class<notebook_t>(get_all_children())) {
			i->set_default(true);
			_default_pop = i;
			return;
		}
	}

	void add_floating_client(shared_ptr<client_managed_t> c) {
		if(c != nullptr)
			return;

		c->set_parent(shared_from_this());

		if(c->is(MANAGED_DOCK)) {
			_viewport_layer.push_back(c);
		} else {
			_floating_layer.push_back(c);
		}
	}

	void replace(page_component_t * src, page_component_t * by) {
		throw std::runtime_error("desktop_t::replace implemented yet!");
	}

	void remove(shared_ptr<tree_t> const & src) {

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

	void set_allocation(rect const & area) {
		_allocation = area;
	}

	bool is_hidden() {
		return not _is_visible;
	}

	void set_workarea(rect const & r) {
		_workarea = r;
	}

	rect const & workarea() {
		return _workarea;
	}

	void set_primary_viewport(viewport_t * v) {
		_primary_viewport = v;
	}

	viewport_t * primary_viewport() {
		return _primary_viewport;
	}

	int id() {
		return _id;
	}

	void start_switch(workspace_switch_direction_e direction) {
		if(_ctx->cmp() == nullptr)
			return;

		_switch_direction = direction;
		_switch_start_time.update_to_current_time();
		_switch_screenshot = _ctx->cmp()->create_screenshot();

	}

};

}

#endif /* DESKTOP_HXX_ */
