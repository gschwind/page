/*
 * viewport.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef VIEWPORT_HXX_
#define VIEWPORT_HXX_

#include <memory>
#include <vector>

#include "renderable.hxx"
#include "theme.hxx"
#include "page_component.hxx"
#include "notebook.hxx"

namespace page {

struct viewport_t: public page_component_t {

private:
	page_component_t * _parent;
	theme_t * _theme;
	bool _is_hidden;

	/** area without considering dock windows **/
	i_rect _raw_aera;

	/** area considering dock windows **/
	i_rect _effective_aera;
	page_component_t * _subtree;

	viewport_t(viewport_t const & v);
	viewport_t & operator= (viewport_t const &);

public:

	page_component_t * parent() const {
		return _parent;
	}

	viewport_t(theme_t * theme, i_rect const & area);

	virtual void replace(page_component_t * src, page_component_t * by);
	virtual void remove(tree_t * src);

	notebook_t * get_nearest_notebook();

	virtual void set_allocation(i_rect const & area);

	void set_raw_area(i_rect const & area);
	void set_effective_area(i_rect const & area);

	virtual bool is_visible() {
		return true;
	}

	void split(notebook_t * nbk, split_type_e type);
	void split_left(notebook_t * nbk, client_managed_t * c);
	void split_right(notebook_t * nbk, client_managed_t * c);
	void split_top(notebook_t * nbk, client_managed_t * c);
	void split_bottom(notebook_t * nbk, client_managed_t * c);
	void notebook_close(notebook_t * src);

	virtual void render(cairo_t * cr, i_rect const & area) const {

	}

	virtual std::list<tree_t *> childs() const {
		std::list<tree_t *> ret;

		if (_subtree != nullptr) {
			ret.push_back(_subtree);
		}

		return ret;
	}

	void raise_child(tree_t * t) {

		if(t != _subtree and t != nullptr) {
			throw exception_t("viewport::raise_child trying to raise a non child tree");
		}

		if(_parent != nullptr and (t == _subtree or t == nullptr)) {
			_parent->raise_child(this);
		}
	}

	virtual std::string get_node_name() const {
		return _get_node_name<'V'>();
	}

	virtual void prepare_render(std::vector<std::shared_ptr<renderable_t>> & out, page::time_t const & time) {
		if(_is_hidden)
			return;
		if(_subtree != nullptr) {
			_subtree->prepare_render(out, time);
		}
	}

	void set_parent(tree_t * t) {
		throw exception_t("viewport cannot have tree_t as parent");
	}

	void set_parent(page_component_t * t) {
		_parent = t;
	}

	i_rect allocation() const {
		return _effective_aera;
	}

	void render_legacy(cairo_t * cr, i_rect const & area) const { }

	i_rect const & raw_area() const;

	void get_all_children(std::vector<tree_t *> & out) const;

	void children(std::vector<tree_t *> & out) const {
		if(_subtree != nullptr) {
			out.push_back(_subtree);
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

	i_rect const & raw_area() {
		return _raw_aera;
	}

	void get_visible_children(std::vector<tree_t *> & out) {
		if (not _is_hidden) {
			out.push_back(this);
			for (auto i : tree_t::children()) {
				i->get_visible_children(out);
			}
		}
	}

};

}

#endif /* VIEWPORT_HXX_ */
