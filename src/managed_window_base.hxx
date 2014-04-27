/*
 * managed_window_base.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef MANAGED_WINDOW_BASE_HXX_
#define MANAGED_WINDOW_BASE_HXX_

#include <list>
#include <string>

#include "client_base.hxx"
#include "box.hxx"
#include "window_icon_handler.hxx"

using namespace std;

namespace page {

struct managed_window_base_t : public client_base_t {

	managed_window_base_t(client_base_t const & c) : client_base_t(c) {

	}

	virtual ~managed_window_base_t() { }

	virtual rectangle const & base_position() const = 0;
	virtual window_icon_handler_t * icon() const = 0;

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
