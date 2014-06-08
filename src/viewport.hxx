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
#include "unmanaged_window.hxx"


using namespace std;

namespace page {

struct viewport_t: public viewport_base_t {

private:
	theme_t * _theme;

	viewport_t(viewport_t const & v);
	viewport_t & operator= (viewport_t const &);

public:
	//page_base_t & page;
	rectangle raw_aera;
	rectangle effective_aera;
	tree_t * _subtree;

	bool _is_visible;

	virtual tree_t * parent() const {
		throw std::runtime_error("viewport has no parent");
	}

	viewport_t(theme_t * theme, rectangle const & area);

	void reconfigure();

	virtual void replace(tree_t * src, tree_t * by);
	virtual void remove(tree_t * src);
	virtual void close(tree_t * src);

	notebook_t * get_nearest_notebook();

	virtual rectangle get_absolute_extend();
	virtual void set_allocation(rectangle const & area);

	void set_raw_area(rectangle const & area);
	void set_effective_area(rectangle const & area);

	virtual bool is_visible() {
		return true;
	}

	void split(notebook_t * nbk, split_type_e type);
	void split_left(notebook_t * nbk, managed_window_t * c);
	void split_right(notebook_t * nbk, managed_window_t * c);
	void split_top(notebook_t * nbk, managed_window_t * c);
	void split_bottom(notebook_t * nbk, managed_window_t * c);
	void notebook_close(notebook_t * src);

	virtual void render(cairo_t * cr, rectangle const & area) const {

	}

	virtual list<tree_t *> childs() const {
		list<tree_t *> ret;

		if (_subtree != nullptr) {
			ret.push_back(_subtree);
		}

		return ret;
	}

	void raise_child(tree_t * t) {
		if(_parent != nullptr) {
			_parent->raise_child(this);
		}
	}

	virtual string get_node_name() const {
		return _get_node_name<'V'>();
	}

	virtual void render(cairo_t * cr, time_t time) {
		for(auto i: childs()) {
			i->render(cr, time);
		}
	}


};

}

#endif /* VIEWPORT_HXX_ */
