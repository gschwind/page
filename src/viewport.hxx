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

namespace page {

struct viewport_t: public tree_t {
	page_base_t & page;
	box_t<int> raw_aera;
	box_t<int> effective_aera;
	tree_t * _subtree;
	window_t * fullscreen_client;

	viewport_t(page_base_t & page, box_t<int> const & area);

	virtual void replace(tree_t * src, tree_t * by);
	virtual void remove(tree_t * src);
	virtual void close(tree_t * src);
	virtual window_list_t get_windows();

	virtual bool add_client(window_t * x);
	virtual box_int_t get_new_client_size();
	virtual void remove_client(window_t * x);
	virtual void activate_client(window_t * x);
	virtual void iconify_client(window_t * x);
	virtual void delete_all();

	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual box_int_t get_absolute_extend();
	virtual void reconfigure(box_int_t const & area);

	void fix_allocation();

	virtual bool is_visible() {
		return true;
	}

};

}

#endif /* VIEWPORT_HXX_ */
