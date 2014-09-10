/*
 * window_overlay.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef WINDOW_OVERLAY_HXX_
#define WINDOW_OVERLAY_HXX_

#include <X11/Xlib.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "cairo_surface_type_name.hxx"
#include "display.hxx"

namespace page {


class window_overlay_t {

	static long const OVERLAY_EVENT_MASK = ExposureMask | StructureNotifyMask;

protected:
	rectangle _position;

	bool _has_alpha;
	bool _is_durty;
	bool _is_visible;

public:
	window_overlay_t(rectangle position = rectangle(-10,-10, 1, 1)) {
		_position = position;
		_has_alpha = true;
		_is_durty = true;
		_is_visible = false;
	}

	void mark_durty() {
		_is_durty = true;
	}

	virtual ~window_overlay_t() {

	}

	void move_resize(rectangle const & area) {
		_position = area;
	}

	void move(int x, int y) {
		_position.x = x;
		_position.y = y;
	}

	void show() {
		_is_visible = true;
	}

	void hide() {
		_is_visible = false;
	}

	bool is_visible() {
		return _is_visible;
	}

	rectangle const & position() {
		return _position;
	}

};

}



#endif /* WINDOW_OVERLAY_HXX_ */
