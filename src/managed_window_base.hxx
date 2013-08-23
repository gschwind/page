/*
 * managed_window_base.hxx
 *
 *  Created on: 23 août 2013
 *      Author: bg
 */

#ifndef MANAGED_WINDOW_BASE_HXX_
#define MANAGED_WINDOW_BASE_HXX_

#include <list>
#include <string>

#include "box.hxx"
#include "window_icon_handler.hxx"

using namespace std;

namespace page {

struct managed_window_base_t {

	virtual ~managed_window_base_t() { }

	virtual box_t<int> const & base_position() const = 0;
	virtual window_icon_handler_t * icon() const = 0;
	virtual char const * title() const = 0;

	/** create a cairo context for top border, must be destroyed with cairo_destroy() **/
	virtual cairo_t * cairo_top() const = 0;
	/** create a cairo context for top border, must be destroyed with cairo_destroy() **/
	virtual cairo_t * cairo_bottom() const = 0;
	/** create a cairo context for top border, must be destroyed with cairo_destroy() **/
	virtual cairo_t * cairo_left() const = 0;
	/** create a cairo context for top border, must be destroyed with cairo_destroy() **/
	virtual cairo_t * cairo_right() const = 0;

	virtual bool is_focused() const = 0;
};

}

#endif /* MANAGED_WINDOW_BASE_HXX_ */
