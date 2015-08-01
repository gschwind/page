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

namespace page {

struct workspace_t: public page_component_t {

private:
	page_context_t * _ctx;
	page_component_t * _parent;
	rect _allocation;
	rect _workarea;

	unsigned const _id;

	/* list of viewports in creation order, to make a sane reconfiguration */
	std::vector<viewport_t *> _viewport_outputs;

	/* dock + viewport belong this layer */
	std::list<tree_t *> _viewport_layer;

	/* floating and fullscreen window belong this layer */
	std::list<client_managed_t *> _floating_layer;

	viewport_t * _primary_viewport;
	notebook_t * _default_pop;
	bool _is_hidden;
	workspace_t(workspace_t const & v);
	workspace_t & operator= (workspace_t const &);

public:

	std::list<client_managed_t *> client_focus;

	page_component_t * parent() const {
		return _parent;
	}

	workspace_t(page_context_t * ctx, unsigned id) :
		_ctx{ctx},
		_allocation{},
		_parent{nullptr},
		_default_pop{nullptr},
		_is_hidden{false},
		_workarea{},
		_primary_viewport{nullptr},
		_id{id}
	{
		client_focus.push_back(nullptr);
	}

	void render(cairo_t * cr, rect const & area) const { }


	void activate(tree_t * t = nullptr) {

		if(_parent != nullptr) {
			_parent->activate(this);
		}

		if(t == nullptr)
			return;

		client_managed_t * mw = dynamic_cast<client_managed_t *>(t);

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

	void prepare_render(std::vector<std::shared_ptr<renderable_t>> & out, page::time_t const & time) {
		if(_is_hidden)
			return;

		for(auto x: _viewport_layer) {
			x->prepare_render(out, time);
		}

		for(auto i: _floating_layer) {
			i->prepare_render(out, time);
		}
	}

	void set_parent(tree_t * t) {
		if(t == nullptr) {
			_parent = nullptr;
			return;
		}

		auto xt = dynamic_cast<page_component_t*>(t);
		if(xt == nullptr) {
			throw exception_t("page_component_t must have a page_component_t as parent");
		} else {
			_parent = xt;
		}
	}

	rect allocation() const {
		return _allocation;
	}

	void render_legacy(cairo_t * cr, rect const & area) const { }

	auto get_viewport_map() const -> std::vector<viewport_t *> const & {
		return _viewport_outputs;
	}

	auto set_layout(std::vector<viewport_t *> const & new_layout) -> void {
		_viewport_outputs = new_layout;
		_viewport_layer.remove_if([](tree_t * t) -> bool { return typeid(viewport_t) == typeid(*t); });
		_viewport_layer.insert(_viewport_layer.end(), _viewport_outputs.begin(), _viewport_outputs.end());
	}

	auto get_any_viewport() const -> viewport_t * {
		return _primary_viewport;
	}

	auto get_viewports() const -> std::vector<viewport_t *> {
		auto tmp = filter_class<viewport_t>(_viewport_layer);
		return std::vector<viewport_t *>{tmp.begin(), tmp.end()};
	}

	void set_default_pop(notebook_t * n) {
		_default_pop->set_default(false);
		_default_pop = n;
		_default_pop->set_default(true);
	}

	notebook_t * default_pop() {
		return _default_pop;
	}

	void children(std::vector<tree_t *> & out) const {
		out.insert(out.end(), _viewport_layer.begin(), _viewport_layer.end());
		out.insert(out.end(), _floating_layer.begin(), _floating_layer.end());
	}

	void update_default_pop() {
		_default_pop = nullptr;
		for(auto i: filter_class<notebook_t>(tree_t::get_all_children())) {
			if(_default_pop == nullptr) {
				_default_pop = i;
				_default_pop->set_default(true);
			} else {
				i->set_default(false);
			}
		}
	}

	void add_floating_client(client_managed_t * c) {
		if(c->is(MANAGED_DOCK)) {
			_viewport_layer.push_back(c);
		} else {
			_floating_layer.push_back(c);
		}
		c->set_parent(this);
	}

	void add_dock_client(client_not_managed_t * c) {

		c->set_parent(this);
	}

	void add_fullscreen_client(client_managed_t * c) {
		if(c->is(MANAGED_DOCK)) {
			_viewport_layer.push_back(c);
		} else {
			_floating_layer.push_back(c);
		}
		c->set_parent(this);
	}

	void replace(page_component_t * src, page_component_t * by) {
		throw std::runtime_error("desktop_t::replace implemented yet!");
	}

	void remove(tree_t * src) {

		if(has_key(_viewport_outputs, reinterpret_cast<viewport_t*>(src))) {
			throw exception_t("%s:%d invalid call of viewport::remove", __FILE__, __LINE__);
		}

		/**
		 * use reinterpret_cast because we try to remove src pointer
		 * and we don't care is type
		 **/
		_floating_layer.remove(reinterpret_cast<client_managed_t*>(src));
		_viewport_layer.remove(src);
	}

	void set_allocation(rect const & area) {
		_allocation = area;
	}

	void hide() {
		_is_hidden = true;
		for(auto i: tree_t::children()) {
			i->hide();
		}
	}

	void show() {
		_is_hidden = false;
		for(auto i: tree_t::children()) {
			i->show();
		}
	}

	bool is_hidden() {
		return _is_hidden;
	}

	void get_visible_children(std::vector<tree_t *> & out) {
		if (not _is_hidden) {
			out.push_back(this);
			for (auto i : tree_t::children()) {
				i->get_visible_children(out);
			}
		}
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

};

}

#endif /* DESKTOP_HXX_ */
