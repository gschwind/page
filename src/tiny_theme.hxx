/*
 * tiny_theme.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef TINY_THEME_HXX_
#define TINY_THEME_HXX_

#include <pango/pangocairo.h>

#include <memory>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>


#include "theme.hxx"
#include "simple2_theme.hxx"
#include "color.hxx"
#include "config_handler.hxx"
#include "renderable.hxx"
#include "pixmap.hxx"

namespace page {

using namespace std;

class tiny_theme_t : public simple2_theme_t {
public:
	tiny_theme_t(display_t * cnx, config_handler_t & conf);
	virtual ~tiny_theme_t();
};

}

#endif /* SIMPLE_THEME_HXX_ */
