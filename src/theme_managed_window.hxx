/*
 * theme_managed_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef THEME_MANAGED_WINDOW_HXX_
#define THEME_MANAGED_WINDOW_HXX_

#include <list>
#include <string>

#include "icon_handler.hxx"

namespace page {

using namespace std;

class client_managed_t;

struct theme_managed_window_t {
	i_rect position;
	string title;
	ptr<icon16> icon;
	cairo_t * cairo_top;
	cairo_t * cairo_bottom;
	cairo_t * cairo_left;
	cairo_t * cairo_right;
	bool focuced;
	bool demand_attention;
};

}

#endif /* MANAGED_WINDOW_BASE_HXX_ */
