/*
 * box.hxx
 *
 *  Created on: 26 févr. 2011
 *      Author: gschwind
 */

#ifndef BOX_HXX_
#define BOX_HXX_

#include <X11/Xlib.h>

#include <algorithm>

namespace page {

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

	box_t(XRectangle const & rec) : x(rec.x), y(rec.y), w(rec.width), h(rec.height) {

	}

	/* compute intersection */
	box_t<T> operator&(box_t<T> const & box) const {

		T left = std::max(x, box.x);
		T right = std::min(x + w, box.x + box.w);
		T top = std::max(y, box.y);
		T bottom = std::min(y + h, box.y + box.h);

		if (right - left < 0 || bottom - top < 0) {
			return box_t<T>(0, 0, 0, 0);
		} else {
			return box_t<T>(left, top, right - left, bottom - top);
		}

	}

	template<typename T1>
	friend box_t<T1> get_max_extand(box_t<T1> const & box0,
			box_t<T1> const & box1);


};

/* compute the smallest area that include 2 boxes */
template<typename T>
box_t<T> get_max_extand(box_t<T> const & box0, box_t<T> const & box1) {
	box_t<T> result;
	result.x = std::min(box0.x, box1.x);
	result.y = std::min(box0.y, box1.y);
	result.w = std::max(box0.x + box0.w, box1.x + box1.w) - result.x;
	result.h = std::max(box0.y + box0.h, box1.y + box1.h) - result.y;
	return result;
}

typedef box_t<int> box_int_t;

}

#endif /* BOX_HXX_ */
