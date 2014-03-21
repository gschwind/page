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
#include "tree_renderable.hxx"
#include "notebook.hxx"
#include "split.hxx"
#include "theme.hxx"

using namespace std;

namespace page {

struct viewport_t: public viewport_base_t, public tree_renderable_t {

private:
	theme_t * _theme;

	viewport_t(viewport_t const & v);
	viewport_t & operator= (viewport_t const &);

public:
	//page_base_t & page;
	box_t<int> raw_aera;
	box_t<int> effective_aera;
	tree_t * _subtree;
	managed_window_t * fullscreen_client;

	bool _is_visible;

	virtual tree_t * parent() const {
		throw std::runtime_error("viewport has no parent");
	}

	viewport_t(theme_t * theme, box_t<int> const & area);

	void reconfigure();

	virtual void replace(tree_t * src, tree_t * by);
	virtual void remove(tree_t * src);
	virtual void close(tree_t * src);

	notebook_t * get_nearest_notebook();

	virtual box_int_t get_absolute_extend();
	virtual region_t<int> get_area();
	virtual void set_allocation(box_int_t const & area);

	void set_raw_area(box_int_t const & area);
	void set_effective_area(box_int_t const & area);

	virtual bool is_visible() {
		return true;
	}

	void split(notebook_t * nbk, split_type_e type);
	void split_left(notebook_t * nbk, managed_window_t * c);
	void split_right(notebook_t * nbk, managed_window_t * c);
	void split_top(notebook_t * nbk, managed_window_t * c);
	void split_bottom(notebook_t * nbk, managed_window_t * c);
	void notebook_close(notebook_t * src);

	virtual void get_childs(vector<tree_t *> & lst);

	virtual void render(cairo_t * cr, box_t<int> const & area) const {

	}

	virtual vector<tree_t *> get_direct_childs() const {
		vector<tree_t *> ret;
		ret.push_back(_subtree);
		return ret;
	}

};

}

#endif /* VIEWPORT_HXX_ */
