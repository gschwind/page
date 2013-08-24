/*
 * floating_event.hxx
 *
 *  Created on: 24 août 2013
 *      Author: bg
 */

#ifndef FLOATING_EVENT_HXX_
#define FLOATING_EVENT_HXX_

#include "box.hxx"

namespace page {

enum floating_event_type_e {
	FLOATING_EVENT_NONE,
	FLOATING_EVENT_CLOSE,
	FLOATING_EVENT_BIND,
	FLOATING_EVENT_TITLE,
	FLOATING_EVENT_GRIP_TOP,
	FLOATING_EVENT_GRIP_BOTTOM,
	FLOATING_EVENT_GRIP_LEFT,
	FLOATING_EVENT_GRIP_RIGHT,
	FLOATING_EVENT_GRIP_TOP_LEFT,
	FLOATING_EVENT_GRIP_TOP_RIGHT,
	FLOATING_EVENT_GRIP_BOTTOM_LEFT,
	FLOATING_EVENT_GRIP_BOTTOM_RIGHT
};

struct floating_event_t {
	floating_event_type_e type;
	box_t<int> position;

	floating_event_t(floating_event_type_e type = FLOATING_EVENT_NONE) :
			type(type), position() {
	}

	floating_event_t(floating_event_t const & x) :
			type(x.type), position(x.position) {
	}

	floating_event_t & operator=(floating_event_t const & x) {
		if (this != &x) {
			type = x.type;
			position = x.position;
		}
		return *this;
	}

};

}

#endif /* FLOATING_EVENT_HXX_ */
