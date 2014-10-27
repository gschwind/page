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

#include "tree.hxx"
#include "page_component.hxx"
#include "notebook.hxx"
#include "split.hxx"
#include "theme.hxx"
#include "client_not_managed.hxx"
#include "viewport.hxx"


namespace page {

using namespace std;

struct desktop_t: public page_component_t {

private:

	page_component_t * _parent;
	i_rect _allocation;

	/** map viewport to real outputs **/
	map<RRCrtc, viewport_t *> _viewport_outputs;
	list<client_not_managed_t *> _dock_clients;
	list<client_managed_t *> _floating_clients;
	list<client_managed_t *> _fullscreen_clients;


	notebook_t * _default_pop;



	bool _is_hidden;

	desktop_t(desktop_t const & v);
	desktop_t & operator= (desktop_t const &);

public:

	page_component_t * parent() const {
		return _parent;
	}

	desktop_t() :
		_allocation(),
		_parent(nullptr),
		_viewport_outputs(),
		_default_pop(nullptr),
		_is_hidden(false)
	{

	}

	notebook_t * get_nearest_notebook();

	void set_raw_area(i_rect const & area);
	void set_effective_area(i_rect const & area);

	void notebook_close(notebook_t * src);

	void render(cairo_t * cr, i_rect const & area) const {

	}

	void raise_child(tree_t * t) {
		if(_parent != nullptr) {
			_parent->raise_child(this);
		}
	}

	string get_node_name() const {
		return _get_node_name<'D'>();
	}

	void prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time) {
		if(_is_hidden)
			return;

		for(auto i: _viewport_outputs) {
			i.second->prepare_render(out, time);
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

	i_rect const & raw_area() const;

	auto get_viewport_map() const -> map<RRCrtc, viewport_t *> const & {
		return _viewport_outputs;
	}

	auto set_layout(map<RRCrtc, viewport_t *> new_layout) -> void {
		_viewport_outputs = new_layout;
	}

	auto get_any_viewport() const -> viewport_t * {
		if(_viewport_outputs.size() > 0) {
			return _viewport_outputs.begin()->second;
		} else {
			return nullptr;
		}
	}

	auto get_viewports() const -> vector< viewport_t * > {
		vector<viewport_t *> ret;
		for(auto i: _viewport_outputs) {
				ret.push_back(i.second);
		}
		return ret;
	}

	void set_default_pop(notebook_t * n) {
		_default_pop = n;
	}

	notebook_t * default_pop() {
		return _default_pop;
	}

	void children(vector<tree_t *> & out) const {
		for(auto i: _viewport_outputs) {
				out.push_back(i.second);
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

	notebook_t * get_default_pop() {
		vector<tree_t *> all_children;
		get_all_children(all_children);
		for(auto i: all_children) {
			if(typeid(*i) == typeid(notebook_t)) {
				return dynamic_cast<notebook_t*>(i);
			}
		}

		return nullptr;
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
		for(auto i : _viewport_outputs) {
			if(i.second == src) {
				_viewport_outputs.erase(i.first);
				return;
			}
		}

		_dock_clients.remove(reinterpret_cast<client_not_managed_t*>(src));
		_floating_clients.remove(reinterpret_cast<client_managed_t*>(src));
		_fullscreen_clients.remove(reinterpret_cast<client_managed_t*>(src));

	}

	void set_allocation(i_rect const & area) {
		_allocation = area;
	}

	void get_all_children(vector<tree_t *> & out) const {
		for(auto i: _viewport_outputs) {
			out.push_back(i.second);
			i.second->get_all_children(out);
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

	void get_visible_children(vector<tree_t *> & out) {
		if (not _is_hidden) {
			out.push_back(this);
			for (auto i : tree_t::children()) {
				i->get_visible_children(out);
			}
		}
	}

};

}

#endif /* DESKTOP_HXX_ */
