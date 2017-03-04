/*
 * theme_split.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef THEME_SPLIT_HXX_
#define THEME_SPLIT_HXX_

#include "box.hxx"

namespace page {

enum split_type_e {
	HORIZONTAL_SPLIT, VERTICAL_SPLIT,
};

struct theme_split_t {
	int root_x, root_y;
	rect allocation;
	split_type_e type;
	double split;
	bool has_mouse_over;
};

}


#endif /* SPLIT_BASE_HXX_ */
