/*
 * tiny_theme.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <cairo/cairo-xcb.h>

#include "config.hxx"
#include "renderable_solid.hxx"
#include "renderable_pixmap.hxx"
#include "tiny_theme.hxx"
#include "box.hxx"
#include "color.hxx"
#include <string>
#include <algorithm>
#include <iostream>

namespace page {

tiny_theme_t::tiny_theme_t(display_t * cnx, config_handler_t & conf) :
	simple2_theme_t(cnx, conf)
		{

}


tiny_theme_t::~tiny_theme_t() {

}

}

