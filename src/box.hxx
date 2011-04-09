/*
 * box.hxx
 *
 *  Created on: 26 févr. 2011
 *      Author: gschwind
 */

#ifndef BOX_HXX_
#define BOX_HXX_

namespace page_next {

template<typename T>
struct box_t {
	T x, y;
	T w, h;
	inline bool is_inside(T _x, T _y) {
		return (x <= _x && _x <= x + w && y <= _y && _y <= y + h);
	}
	box_t(box_t const & b) :
		x(b.x), y(b.y), w(b.w), h(b.h) {
	}

	box_t() :
		x(), y(), w(), h() {
	}

	box_t(T x, T y, T w, T h) :
		x(x), y(y), w(w), h(h) {
	}
};

}

#endif /* BOX_HXX_ */
