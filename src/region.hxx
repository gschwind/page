/*
 * region.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef REGION_HXX_
#define REGION_HXX_

#include <iostream>
#include <sstream>

#include "box.hxx"

namespace page {

template<typename T>
class region_t: public std::list<rectangle_t<T> > {
	/** short cut for the superior class **/
	typedef std::list<rectangle_t<T> > _superior_t;
	typedef rectangle_t<T> _box_t;

	/** this function reduce the number of boxes if possible **/
	static region_t & clean_up(region_t & lst) {
		remove_empty(lst);
		merge_area_macro(lst);
		return lst;
	}

	/** merge 2 rectangles when it is possible **/
	static void merge_area_macro(region_t & list) {
		typename region_t::iterator i = list.begin();
		while (i != list.end()) {
			bool have_removed = false;
			typename region_t::iterator j = i;
			++j;
			while (j != list.end()) {
				_box_t & bi = *i;
				_box_t & bj = *j;

				/** left/right **/
				if (bi.x + bi.w == bj.x && bi.y == bj.y && bi.h == bj.h) {
					_box_t new_box(bi.x, bi.y, bj.w + bi.w, bi.h);
					list.push_back(new_box);
					list.erase(j);
					have_removed = true;
					break;
				}

				/** right/left **/
				if (bi.x == bj.x + bj.w && bi.y == bj.y && bi.h == bj.h) {
					_box_t new_box(bj.x, bj.y, bj.w + bi.w, bj.h);
					list.push_back(new_box);
					list.erase(j);
					have_removed = true;
					break;
				}

				/** top/bottom **/
				if (bi.y == bj.y + bj.h && bi.x == bj.x && bi.w == bj.w) {
					_box_t new_box(bj.x, bj.y, bj.w, bj.h + bi.h);
					list.push_back(new_box);
					list.erase(j);
					have_removed = true;
					break;
				}

				/** bottom/top **/
				if (bi.y + bi.h == bj.y && bi.x == bj.x && bi.w == bj.w) {
					_box_t new_box(bi.x, bi.y, bi.w, bj.h + bi.h);
					list.push_back(new_box);
					list.erase(j);
					have_removed = true;
					break;
				}

				++j;
			}

			if(have_removed) {
				i = list.erase(i);
			} else {
				++i;
			}
		}
	}

	static bool _is_null(_box_t const & b) {
		return b.is_null();
	}

	/** remove empty boxes **/
	static void remove_empty(region_t & list) {
		list.remove_if(_is_null);
	}

	bool is_null() {
		return this->empty();
	}

	/* box0 - box1 */
	static region_t substract_box(_box_t const & box0,
			_box_t const & box1) {
		region_t result;

		_box_t intersection = box0 & box1;

		if (!intersection.is_null()) {
			/* top box */
			{
				T left = intersection.x;
				T right = intersection.x + intersection.w;
				T top = box0.y;
				T bottom = intersection.y;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							_box_t(left, top, right - left, bottom - top));
				}
			}

			/* bottom box */
			{
				T left = intersection.x;
				T right = intersection.x + intersection.w;
				T top = intersection.y + intersection.h;
				T bottom = box0.y + box0.h;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							_box_t(left, top, right - left, bottom - top));
				}
			}

			/* left box */
			{
				T left = box0.x;
				T right = intersection.x;
				T top = intersection.y;
				T bottom = intersection.y + intersection.h;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							_box_t(left, top, right - left, bottom - top));
				}
			}

			/* right box */
			{
				T left = intersection.x + intersection.w;
				T right = box0.x + box0.w;
				T top = intersection.y;
				T bottom = intersection.y + intersection.h;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							_box_t(left, top, right - left, bottom - top));
				}
			}

			/* top left box */
			{
				T left = box0.x;
				T right = intersection.x;
				T top = box0.y;
				T bottom = intersection.y;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							_box_t(left, top, right - left, bottom - top));
				}
			}

			/* top right box */
			{
				T left = intersection.x + intersection.w;
				T right = box0.x + box0.w;
				T top = box0.y;
				T bottom = intersection.y;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							_box_t(left, top, right - left, bottom - top));
				}
			}

			/* bottom left box */
			{
				T left = box0.x;
				T right = intersection.x;
				T top = intersection.y + intersection.h;
				T bottom = box0.y + box0.h;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							_box_t(left, top, right - left, bottom - top));
				}
			}

			/* bottom right box */
			{
				T left = intersection.x + intersection.w;
				T right = box0.x + box0.w;
				T top = intersection.y + intersection.h;
				T bottom = box0.y + box0.h;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							_box_t(left, top, right - left, bottom - top));
				}
			}

		} else {
			result.push_back(box0);
		}

		return clean_up(result);

	}

public:

	/** create an empty region **/
	region_t() : _superior_t() { }

	region_t(region_t const & r) :
			_superior_t(r) {
	}

	region_t(_box_t const & b) {
		if (!b.is_null())
			this->push_back(b);
	}

	region_t(T x, T y, T w, T h) {
		_box_t b(x, y, w, h);
		if (!b.is_null())
			this->push_back(b);
	}

	region_t & operator =(_box_t const & b) {
		this->clear();
		if (!b.is_null())
			this->push_back(b);
		return *this;
	}

	region_t & operator =(region_t const & r) {
		/** call the superior class assign operator **/
		if (this != &r) {
			this->_superior_t::operator =(r);
		}
		return *this;
	}

	region_t & operator -=(_box_t const & box1) {
		for(typename region_t::iterator i = this->begin(); i != this->end(); ) {
			if ((*i).has_intersection(box1)) {
				region_t x = substract_box(*i, box1);
				i = this->erase(i);
				this->insert(this->begin(), x.begin(), x.end());
			} else {
				++i;
			}
		}
		return clean_up(*this);
	}

	region_t operator -(_box_t const & b) {
		region_t r = *this;
		return (r -= b);
	}

	region_t & operator -=(region_t const & b) {

		if(this == &b) {
			this->clear();
			return *this;
		}

		for (typename region_t::const_iterator i = b.begin(); i != b.end(); ++i) {
			*this -= *i;
		}

		return *this;
	}

	region_t operator -(region_t const & b) const {
		region_t result = *this;
		return (result -= b);
	}

	region_t & operator +=(_box_t const & b) {
		if (!b.is_null()) {
			*this -= b;
			this->push_back(b);
			clean_up(*this);
		}
		return *this;
	}

	region_t & operator +=(region_t const & r) {
		/** sum same regions give same region **/
		if(this == &r)
			return *this;

		for(typename region_t::const_iterator i = r.begin(); i != r.end(); ++i) {
			*this += *i;
		}

		return *this;
	}

	region_t operator +(_box_t const & b) const {
		region_t result = *this;
		return result += b;
	}

	region_t operator +(region_t const & r) const {
		region_t result = *this;
		return result += r;
	}

	region_t operator &(_box_t const & b) const {
		region_t result;
		for(typename region_t::const_iterator i = this->begin(); i != this->end(); ++i) {
			_box_t x = b & *i;
			/**
			 * since this is a region, no over lap is possible, just add the
			 * intersection if not null. (do not use operator+= for optimal
			 * result)
			 **/
			if(!x.is_null()) {
				result.push_back(x);
			}
		}
		return clean_up(result);
	}

	region_t operator &(region_t const & r) const {
		region_t result;

		for(typename region_t::const_iterator i = this->begin(); i != this->end(); ++i) {
			region_t clipped = r & *i;
			result.insert(result.begin(), clipped.begin(), clipped.end());
		}

		return clean_up(result);
	}

	region_t & operator &=(_box_t const & b) {
		*this = *this & b;
		return *this;
	}

	region_t & operator &=(region_t const & r) {
		if(this == &r)
			return*this;
		*this = *this & r;
		return *this;
	}


	/** make string **/
	std::string to_string() const {
		std::ostringstream os;
		for(typename region_t::const_iterator i = this->begin(); i != this->end(); ++i) {
			if (i != this->begin())
				os << ",";
			os << i->to_string();
		}
		return os.str();
	}

	void translate(T x, T y) {
		for(typename region_t::iterator i = this->begin(); i != this->end(); ++i) {
			i->x += x;
			i->y += y;
		}
	}

};

typedef region_t<double> region;

}

#endif /* REGION_HXX_ */
