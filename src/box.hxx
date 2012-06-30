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
inline T min(T x, T y) {
	return (((x)<(y))?(x):(y));
}

template<typename T>
inline T max(T x, T y) {
	return (((x)>(y))?(x):(y));
}


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

	box_t<T> operator&(box_t<T> const & box) const {

		T left = max(this->x, box.x);
		T right = min(this->x+this->w, box.x+box.w);
		T top = max(this->y, box.y);
		T bottom = min(this->y+this->h, box.y+box.h);

		if(right - left < 0 || bottom - top < 0) {
			return box_t<T>(0,0,0,0);
		} else {
			return box_t<T>(left, top, right-left, bottom-top);
		}

	}

};

}

#endif /* BOX_HXX_ */
