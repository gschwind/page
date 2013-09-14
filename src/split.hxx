/*
 * split.hxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#ifndef SPLIT_HXX_
#define SPLIT_HXX_

#include <cmath>

#include "tree.hxx"
#include "tree_renderable.hxx"
#include "theme.hxx"

#include <set>

using namespace std;

namespace page {



class split_t : public split_base_t, public tree_renderable_t {

	theme_t const * _theme;

	box_int_t _split_bar_area;
	split_type_e _split_type;
	double _split;
	tree_t * _pack0;
	tree_t * _pack1;

	box_int_t bpack0;
	box_int_t bpack1;

	split_t(split_t const &);
	split_t & operator=(split_t const &);

	void update_allocation_pack0();
	void update_allocation_pack1();
	void update_allocation();

public:
	split_t(split_type_e type, theme_t const * theme, tree_t * p0 = 0, tree_t * p1 = 0);
	~split_t();

	void replace(tree_t * src, tree_t * by);

	void compute_split_bar_area(box_int_t & area, double split) const;

	box_int_t get_absolute_extend();
	void set_allocation(box_int_t const & area);

	void set_split(double split);
	box_int_t const & get_split_bar_area() const;

	tree_t * get_pack0();
	tree_t * get_pack1();
	split_type_e get_split_type();

	double get_split_ratio();

	void set_theme(theme_t const * theme);

	virtual void get_childs(vector<tree_t *> & lst);

	void set_pack0(tree_t * x);
	void set_pack1(tree_t * x);


	/* compute the slider area */
	void compute_split_location(double split, int & x, int & y) const {

		box_int_t const & alloc = allocation();

		if (_split_type == VERTICAL_SPLIT) {


			int w = alloc.w - 2 * _theme->split_margin.left
					- 2 * _theme->split_margin.right - _theme->split_width;
			int w0 = floor(w * split + 0.5);

			x = alloc.x + _theme->split_margin.left + w0
					+ _theme->split_margin.right;
			y = alloc.y;

		} else {

			int h = alloc.h - 2 * _theme->split_margin.top
					- 2 * _theme->split_margin.bottom - _theme->split_width;
			int h0 = floor(h * split + 0.5);

			x = alloc.x;
			y = alloc.y + _theme->split_margin.top + h0
					+ _theme->split_margin.bottom;
		}
	}

	/* compute the slider area */
	void compute_split_size(double split, int & w, int & h) const {
		box_int_t const & alloc = allocation();
		if (_split_type == VERTICAL_SPLIT) {
			w = _theme->split_width;
			h = alloc.h;
		} else {
			w = alloc.w;
			h = _theme->split_width;
		}
	}

	double split() const {
		return _split;
	}

	split_type_e type() const {
		return _split_type;
	}

	virtual void render(cairo_t * cr, box_t<int> const & area) const {
		_theme->render_split(cr, this, area);
	}

	virtual vector<tree_t *> get_direct_childs() const {
		vector<tree_t *> ret;
		ret.push_back(_pack0);
		ret.push_back(_pack1);
		return ret;
	}

};

}

#endif /* SPLIT_HXX_ */
