/*
 * theme_tab.hxx
 *
 *  Created on: 1 oct. 2014
 *      Author: gschwind
 */

#ifndef THEME_TAB_HXX_
#define THEME_TAB_HXX_

namespace page {

struct theme_tab_t {
	i_rect position;
	std::string title;
	std::shared_ptr<icon16> icon;
	bool selected;
	bool focuced;
	bool demand_attention;
	bool is_iconic;

	theme_tab_t() :
		position{},
		title{},
		icon{},
		selected{},
		focuced{},
		demand_attention{},
		is_iconic{}
	{ }

	theme_tab_t(theme_tab_t const & x) :
		position{x.position},
		title{x.title},
		icon{x.icon},
		selected{x.selected},
		focuced{x.focuced},
		demand_attention{x.demand_attention},
		is_iconic{x.is_iconic}
	{ }

};

}



#endif /* THEME_TAB_HXX_ */
