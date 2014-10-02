/*
 * theme_tab.hxx
 *
 *  Created on: 1 oct. 2014
 *      Author: gschwind
 */

#ifndef THEME_TAB_HXX_
#define THEME_TAB_HXX_

#include <list>
#include <string>

#include "icon_handler.hxx"

namespace page {

using namespace std;

struct theme_tab_t {
	i_rect position;
	string title;
	icon_handler_t<16,16> * icon;
	bool selected;
	bool focuced;
	bool demand_attention;
};

}



#endif /* THEME_TAB_HXX_ */
