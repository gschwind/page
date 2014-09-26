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

using namespace std;

template<typename T>
class region_t : public vector<i_rect_t<T>> {
	/** short cut for the superior class **/
	using super = vector<i_rect_t<T> >;
	using _box_t = i_rect_t<T>;

	/** this function reduce the number of boxes if possible **/
	static region_t & clean_up(region_t & lst) {
		merge_area_macro(lst);
		remove_empty(lst);
		return lst;
	}

	/** merge 2 i_rects when it is possible **/
	static void merge_area_macro(region_t & list) {
		bool end = false;
		while (not end) {
			end = true;
			for (auto i = list.begin(); i != list.end(); ++i) {
				_box_t & bi = *i;
				if (bi.is_null()) {
					continue;
				}

				for (auto j = i + 1; j != list.end(); ++j) {
					_box_t & bj = *j;
					if (bj.is_null()) {
						continue;
					}

					/** left/right **/
					if (bi.x + bi.w == bj.x and bi.y == bj.y and bi.h == bj.h) {
						bi = _box_t(bi.x, bi.y, bj.w + bi.w, bi.h);
						bj = _box_t();
						end = false;
						continue;
					}

					/** right/left **/
					if (bi.x == bj.x + bj.w and bi.y == bj.y and bi.h == bj.h) {
						bi = _box_t(bj.x, bj.y, bj.w + bi.w, bj.h);
						bj = _box_t();
						end = false;
						continue;
					}

					/** top/bottom **/
					if (bi.y == bj.y + bj.h and bi.x == bj.x and bi.w == bj.w) {
						bi = _box_t(bj.x, bj.y, bj.w, bj.h + bi.h);
						bj = _box_t();
						end = false;
						continue;
					}

					/** bottom/top **/
					if (bi.y + bi.h == bj.y and bi.x == bj.x and bi.w == bj.w) {
						bi = _box_t(bi.x, bi.y, bi.w, bj.h + bi.h);
						bj = _box_t();
						end = false;
						continue;
					}
				}
			}
		}
	}

	/** remove empty boxes **/
	static void remove_empty(region_t & list) {
		auto i = list.begin();
		auto j = list.begin();
		while (j != list.end()) {
			if(not j->is_null()) {
				*i = *j;
				++i;
			}
			++j;
		}

		/* reduce the list size */
		list.resize(distance(list.begin(), i));
	}

	bool is_null() {
		return this->empty();
	}

	/* box0 - box1 */
	static region_t substract_box(_box_t const & box0,
			_box_t const & box1) {
		region_t result;

		_box_t intersection = box0 & box1;

		if (not intersection.is_null()) {
			/* top box */
			{
				T left = intersection.x;
				T right = intersection.x + intersection.w;
				T top = box0.y;
				T bottom = intersection.y;

				if (right - left > 0 and bottom - top > 0) {
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

				if (right - left > 0 and bottom - top > 0) {
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

				if (right - left > 0 and bottom - top > 0) {
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

				if (right - left > 0 and bottom - top > 0) {
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

				if (right - left > 0 and bottom - top > 0) {
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

				if (right - left > 0 and bottom - top > 0) {
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

				if (right - left > 0 and bottom - top > 0) {
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

				if (right - left > 0 and bottom - top > 0) {
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
	region_t() : super() {
		//printf("capacity = %lu\n", this->capacity());
	}

	region_t(region_t const & r) : super(r) {
		//printf("capacity = %lu\n", this->capacity());
	}

	region_t(region_t const && r) : super(r) { }

	template<typename U>
	region_t(vector<U> const & v) {
		for (int i = 0; i + 3 < v.size(); i += 4) {
			super::push_back(_box_t ( v[i + 0], v[i + 1], v[i + 2], v[i + 3] ));
		}
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

	~region_t() {
		//printf("capacity = %lu\n", this->capacity());
	}

	region_t & operator =(_box_t const & b) {
		this->clear();
		if (!b.is_null())
			this->push_back(b);
		return *this;
	}

	region_t & operator =(region_t const & r) {
		/** call the superior class assign operator **/
		super::operator =(r);
		return *this;
	}

	region_t & operator =(region_t const && r) {
		/** call the superior class assign operator **/
		super::operator =(r);
		return *this;
	}

	region_t & operator -=(_box_t const & box1) {
		region_t tmp;
		for (auto & i : *this) {
			region_t x = substract_box(i, box1);
			tmp.insert(tmp.end(), x.begin(), x.end());
		}
		return (*this = tmp);
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

		for (auto & i : b) {
			*this -= i;
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

		for(auto & i : r) {
			*this += i;
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
		for(auto & i : *this) {
			_box_t x = b & i;
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
		for(auto const & i : *this) {
			region_t clipped = r & i;
			result.insert(result.end(), clipped.begin(), clipped.end());
		}
		return clean_up(result);
	}

	region_t & operator &=(_box_t const & b) {
		*this = *this & b;
		return *this;
	}

	region_t & operator &=(region_t const & r) {
		if(this != &r)
			*this = *this & r;
		return *this;
	}


	/** make string **/
	string to_string() const {
		std::ostringstream os;
		auto i = this->begin();
		if(i == this->end())
			return os.str();
		os << i->to_string();
		++i;
		while(i != this->end()) {
			os << "," << i->to_string();
			++i;
		}

		return os.str();
	}

	void translate(T x, T y) {
		for(auto & i : *this) {
			i.x += x;
			i.y += y;
		}
	}


	T area() {
		T ret{T()};
		for(auto &i: *this) {
			ret += i.w * i.h;
		}
		return ret;
	}

};

typedef region_t<int> region;

}

#endif /* REGION_HXX_ */
