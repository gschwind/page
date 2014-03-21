/*
 * box.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef BOX_HXX_
#define BOX_HXX_

#include <X11/Xlib.h>

#include <algorithm>
#include <list>
#include <sstream>
#include <iostream>

namespace page {

template<typename T>
struct box_t {
	T x, y;
	T w, h;

	bool is_inside(T _x, T _y) const {
		return (x <= _x && _x < x + w && y <= _y && _y < y + h);
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

	box_t(XRectangle const & rec) :
			x(rec.x), y(rec.y), w(rec.width), h(rec.height) {
	}

	bool is_null() const {
		return (w <= 0 || h <= 0);
	}

	std::string to_string() const {
		std::ostringstream os;
		os << "(" << this->x << "," << this->y << "," << this->w << ","
				<< this->h << ")";
		return os.str();
	}

	bool operator==(box_t const & b) const {
		return (b.x == x and b.y == y and b.w == w and b.h == h);
	}

	bool operator!=(box_t const & b) const {
		return (b.x != x or b.y != y or b.w != w or b.h != h);
	}

	/* compute the smallest area that include 2 boxes */
	box_t get_max_extand(box_t const & b) const {
		box_t<T> result;
		result.x = std::min(x, b.x);
		result.y = std::min(y, b.y);
		result.w = std::max(x + w, b.x + b.w) - result.x;
		result.h = std::max(y + h, b.y + b.h) - result.y;
		return result;
	}

	bool has_intersection(box_t const & b) const {
		T left = std::max(x, b.x);
		T right = std::min(x + w, b.x + b.w);

		if(right <= left)
			return false;

		T top = std::max(y, b.y);
		T bottom = std::min(y + h, b.y + b.h);

		if(bottom <= top)
			return false;

		return true;

	}

	box_t & operator&=(box_t const & b) {
		if(this == &b) {
			*this = box_t<T>(0, 0, 0, 0);
			return *this;
		}

		T left = std::max(x, b.x);
		T right = std::min(x + w, b.x + b.w);
		T top = std::max(y, b.y);
		T bottom = std::min(y + h, b.y + b.h);

		if (right <= left || bottom <= top) {
			*this = box_t<T>(0, 0, 0, 0);
		} else {
			*this = box_t<T>(left, top, right - left, bottom - top);
		}

		return *this;
	}

	box_t operator&(box_t const & b) const {
		box_t<T> x = *this;
		return (x &= b);
	}
};

typedef box_t<int> box_int_t;

}

#endif /* BOX_HXX_ */
