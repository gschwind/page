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
#include "viewport.hxx"
#include "client_managed.hxx"
#include "client_not_managed.hxx"

namespace page {

struct desktop_t: public page_component_t {

private:

	page_component_t * _parent;
	i_rect _allocation;
	i_rect _workarea;

	unsigned const _id;

	/** map viewport to real outputs **/
	std::vector<viewport_t *> _viewport_outputs;
	std::list<viewport_t *> _viewport_stack;
	std::list<client_not_managed_t *> _dock_clients;
	std::list<client_managed_t *> _floating_clients;
	std::list<client_managed_t *> _fullscreen_clients;


	viewport_t * _primary_viewport;
	notebook_t * _default_pop;
	bool _is_hidden;
	desktop_t(desktop_t const & v);
	desktop_t & operator= (desktop_t const &);

public:

	std::list<client_managed_t *> client_focus;

	page_component_t * parent() const {
		return _parent;
	}

	desktop_t(unsigned id) :
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

	void render(cairo_t * cr, i_rect const & area) const { }

	void raise_child(tree_t * t) {

		client_managed_t * mw = dynamic_cast<client_managed_t *>(t);
		if(has_key(_fullscreen_clients, mw)) {
			_fullscreen_clients.remove(mw);
			_fullscreen_clients.push_back(mw);
		}

		if(has_key(_floating_clients, mw)) {
			_floating_clients.remove(mw);
			_floating_clients.push_back(mw);
		}

		client_not_managed_t * uw = dynamic_cast<client_not_managed_t *>(t);
		if(has_key(_dock_clients, uw)) {
			_dock_clients.remove(uw);
			_dock_clients.push_back(uw);
		}

		viewport_t * v = dynamic_cast<viewport_t *>(t);
		if(has_key(_viewport_stack, v)) {
			_viewport_stack.remove(v);
			_viewport_stack.push_back(v);
		}

		if(_parent != nullptr) {
			_parent->raise_child(this);
		}
	}

	std::string get_node_name() const {
		return _get_node_name<'D'>();
	}

	void prepare_render(std::vector<std::shared_ptr<renderable_t>> & out, page::time_t const & time) {
		if(_is_hidden)
			return;

		for(auto i: _viewport_outputs) {
			i->prepare_render(out, time);
		}

		for(auto i: _dock_clients) {
			i->prepare_render(out, time);
		}

		for(auto i: _floating_clients) {
			i->prepare_render(out, time);
		}

		for(auto i: _fullscreen_clients) {
			i->prepare_render(out, time);
		}
	}

	void set_parent(tree_t * t) {
		throw exception_t("viewport cannot have tree_t as parent");
	}

	void set_parent(page_component_t * t) {
		_parent = t;
	}

	i_rect allocation() const {
		return _allocation;
	}

	void render_legacy(cairo_t * cr, i_rect const & area) const { }

	auto get_viewport_map() const -> std::vector<viewport_t *> const & {
		return _viewport_outputs;
	}

	auto set_layout(std::vector<viewport_t *> const & new_layout) -> void {
		_viewport_outputs = new_layout;
		_viewport_stack.clear();
		_viewport_stack.insert(_viewport_stack.end(), _viewport_outputs.begin(), _viewport_outputs.end());
	}

	auto get_any_viewport() const -> viewport_t * {
		if(_viewport_stack.size() > 0) {
			return _viewport_stack.front();
		} else {
			return nullptr;
		}
	}

	auto get_viewports() const -> std::vector< viewport_t * > {
		std::vector<viewport_t *> ret;
		for(auto i: _viewport_stack) {
				ret.push_back(i);
		}
		return ret;
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
		for(auto i: _viewport_stack) {
				out.push_back(i);
		}

		for(auto i: _dock_clients) {
				out.push_back(i);
		}

		for(auto i: _floating_clients) {
				out.push_back(i);
		}

		for(auto i: _fullscreen_clients) {
				out.push_back(i);
		}

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
		_floating_clients.push_back(c);
		c->set_parent(this);
	}

	void add_dock_client(client_not_managed_t * c) {
		_dock_clients.push_back(c);
		c->set_parent(this);
	}

	void add_fullscreen_client(client_managed_t * c) {
		_fullscreen_clients.push_back(c);
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
		_dock_clients.remove(reinterpret_cast<client_not_managed_t*>(src));
		_floating_clients.remove(reinterpret_cast<client_managed_t*>(src));
		_fullscreen_clients.remove(reinterpret_cast<client_managed_t*>(src));

	}

	void set_allocation(i_rect const & area) {
		_allocation = area;
	}

	void get_all_children(std::vector<tree_t *> & out) const {
		for(auto i: _viewport_stack) {
			out.push_back(i);
			i->get_all_children(out);
		}

		for(auto i: _dock_clients) {
			out.push_back(i);
			i->get_all_children(out);
		}

		for(auto i: _floating_clients) {
			out.push_back(i);
			i->get_all_children(out);
		}

		for(auto i: _fullscreen_clients) {
			out.push_back(i);
			i->get_all_children(out);
		}
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

	void set_workarea(i_rect const & r) {
		_workarea = r;
	}

	i_rect const & workarea() {
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
