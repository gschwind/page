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

namespace page {

class split_t;

enum split_type_e {
	HORIZONTAL_SPLIT, VERTICAL_SPLIT,
};

struct theme_split_t {
	i_rect allocation;
	split_type_e type;
	double split;
};

}


#endif /* SPLIT_BASE_HXX_ */
