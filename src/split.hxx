/*
 * split.hxx
 *
 *  Created on: 27 févr. 2011
 *      Author: gschwind
 */

#ifndef SPLIT_HXX_
#define SPLIT_HXX_



#include "box.hxx"
#include "tree.hxx"

#include <set>

using namespace std;

namespace page {

enum split_type_e {
	HORIZONTAL_SPLIT, VERTICAL_SPLIT,
};

class split_t: public tree_t {
	static int const GRIP_SIZE = 3;

	box_int_t _split_bar_area;
	split_type_e _split_type;
	double _split;
	tree_t * _pack0;
	tree_t * _pack1;

	split_t(split_t const &);
	split_t & operator=(split_t const &);

	void update_allocation_pack0();
	void update_allocation_pack1();
	void update_allocation();

public:
	split_t(split_type_e type);
	~split_t();

	void replace(tree_t * src, tree_t * by);

	void compute_split_bar_area(box_int_t & area, double split) const;

	virtual box_int_t get_absolute_extend();
	virtual void set_allocation(box_int_t const & area);
	virtual region_t<int> get_area();

	void set_split(double split);
	box_int_t const & get_split_bar_area() const;

	tree_t * get_pack0();
	tree_t * get_pack1();
	split_type_e get_split_type();

	double get_split_ratio();

};

}

#endif /* SPLIT_HXX_ */
