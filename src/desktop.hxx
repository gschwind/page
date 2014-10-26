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

public:
	page_component_t * _parent;
	i_rect _allocation;

	/** map viewport to real outputs **/
	map<RRCrtc, viewport_t *> _viewport_outputs;

	notebook_t * _default_pop;

	desktop_t(desktop_t const & v);
	desktop_t & operator= (desktop_t const &);

public:

	page_component_t * parent() const {
		return _parent;
	}

	desktop_t() : _allocation(), _parent(nullptr), _viewport_outputs(), _default_pop(nullptr) {

	}

	virtual void replace(page_component_t * src, page_component_t * by);
	virtual void remove(tree_t * src);

	notebook_t * get_nearest_notebook();

	virtual void set_allocation(i_rect const & area);

	void set_raw_area(i_rect const & area);
	void set_effective_area(i_rect const & area);

	virtual bool is_visible() {
		return true;
	}

	void notebook_close(notebook_t * src);

	virtual void render(cairo_t * cr, i_rect const & area) const {

	}

	virtual list<tree_t *> childs() const {
		list<tree_t *> ret;

		for(auto &i: _viewport_outputs) {
			ret.push_back(i.second);
		}

		return ret;
	}

	void raise_child(tree_t * t) {
		if(_parent != nullptr) {
			_parent->raise_child(this);
		}
	}

	virtual string get_node_name() const {
		return _get_node_name<'D'>();
	}

	virtual void prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time) {
		for(auto &i: _viewport_outputs) {
			i.second->prepare_render(out, time);
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

	void get_all_children(vector<tree_t *> & out) const;

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

};

}

#endif /* DESKTOP_HXX_ */
