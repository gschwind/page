/*
 * split.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef SPLIT_HXX_
#define SPLIT_HXX_

#include <cmath>

#include "tree.hxx"
#include "theme.hxx"

#include <set>

using namespace std;

namespace page {

class split_t : public split_base_t {

	theme_t const * _theme;

	i_rect _split_bar_area;
	split_type_e _split_type;
	double _split;
	tree_t * _pack0;
	tree_t * _pack1;

	i_rect bpack0;
	i_rect bpack1;

	list<tree_t *> _children;

	split_t(split_t const &);
	split_t & operator=(split_t const &);

	void update_allocation_pack0();
	void update_allocation_pack1();
	void update_allocation();

public:
	split_t(split_type_e type, theme_t const * theme, tree_t * p0 = nullptr,
			tree_t * p1 = nullptr);
	~split_t();
	void replace(tree_t * src, tree_t * by);
	void compute_split_bar_area(i_rect & area, double split) const;
	i_rect get_absolute_extend();
	void set_allocation(i_rect const & area);
	void set_split(double split);
	i_rect const & get_split_bar_area() const;
	tree_t * get_pack0();
	tree_t * get_pack1();
	split_type_e get_split_type();
	double get_split_ratio();
	void set_theme(theme_t const * theme);
	void set_pack0(tree_t * x);
	void set_pack1(tree_t * x);
	/* compute the slider area */
	void compute_split_location(double split, int & x, int & y) const;
	/* compute the slider area */
	void compute_split_size(double split, int & w, int & h) const;
	double split() const;
	split_type_e type() const;
	void render_legacy(cairo_t * cr, i_rect const & area) const;
	list<tree_t *> childs() const;
	void raise_child(tree_t * t);

	virtual string get_node_name() const {
		return _get_node_name<'S'>();
	}

	void remove(tree_t * t);

	virtual void render(cairo_t * cr, time_t time) {
//		for(auto i: childs()) {
//			i->render(cr, time);
//		}
	}


	bool need_render(time_t time) {

		for(auto i: childs()) {
			if(i->need_render(time)) {
				return true;
			}
		}
		return false;
	}

	virtual void prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time);

	i_rect compute_split_bar_location() const;

};

}

#endif /* SPLIT_HXX_ */
