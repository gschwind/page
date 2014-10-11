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

#include "tree.hxx"
#include "viewport_base.hxx"
#include "notebook.hxx"
#include "split.hxx"
#include "theme.hxx"
#include "client_not_managed.hxx"


using namespace std;

namespace page {

struct viewport_t: public viewport_base_t {

private:

	page_component_t * _parent;

	theme_t * _theme;

	viewport_t(viewport_t const & v);
	viewport_t & operator= (viewport_t const &);

public:

	/** area without considering dock windows **/
	i_rect _raw_aera;

	/** area considering dock windows **/
	i_rect _effective_aera;
	page_component_t * _subtree;

	bool _is_visible;

	page_component_t * parent() const {
		throw std::runtime_error("viewport has no parent");
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

	virtual list<tree_t *> childs() const {
		list<tree_t *> ret;

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

	virtual string get_node_name() const {
		return _get_node_name<'V'>();
	}

	virtual void prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time) {
		tree_t::_prepare_render(out, time);
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

};

}

#endif /* VIEWPORT_HXX_ */
