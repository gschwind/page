/*
 * viewport.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef VIEWPORT_HXX_
#define VIEWPORT_HXX_

#include "page_base.hxx"
#include "tree.hxx"
#include "box.hxx"
#include "notebook.hxx"
#include "split.hxx"

using namespace std;

namespace page {

struct viewport_t: public tree_t {
public:
	//page_base_t & page;
	box_t<int> raw_aera;
	box_t<int> effective_aera;
	tree_t * _subtree;
	managed_window_t * fullscreen_client;

	set<notebook_t *> _notebook_set;
	set<split_t *> _split_set;

	bool _is_visible;

	viewport_t();

	viewport_t(box_t<int> const & area);

	void add_notebook(notebook_t * n) {
		_subtree = n;
		_subtree->set_parent(this);
		_subtree->set_allocation(effective_aera);
	}

	void reconfigure();

	virtual void replace(tree_t * src, tree_t * by);
	virtual void remove(tree_t * src);
	virtual void close(tree_t * src);
	//virtual tab_window_set_t get_windows();

	notebook_t * get_nearest_notebook();

	virtual box_int_t get_absolute_extend();
	virtual region_t<int> get_area();
	virtual void set_allocation(box_int_t const & area);

	void fix_allocation();

	virtual bool is_visible() {
		return true;
	}

	void split(notebook_t * nbk, split_type_e type);
	void split_left(notebook_t * nbk, managed_window_t * c);
	void split_right(notebook_t * nbk, managed_window_t * c);
	void split_top(notebook_t * nbk, managed_window_t * c);
	void split_bottom(notebook_t * nbk, managed_window_t * c);
	void notebook_close(notebook_t * src);

	virtual void get_childs(list<tree_t *> & lst);

	void get_notebooks(list<notebook_t *> & l);
	void get_splits(list<split_t *> & l);

};

}

#endif /* VIEWPORT_HXX_ */
