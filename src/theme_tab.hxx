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
};

}



#endif /* THEME_TAB_HXX_ */
