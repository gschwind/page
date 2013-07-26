/*
 * region.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef REGION_HXX_
#define REGION_HXX_

#include <iostream>
#include <sstream>
#include "box.hxx"

namespace page {

template<typename T>
class region_t: public std::list<box_t<T> > {
	typedef box_t<T> _box_t;
public:
	typedef std::list<_box_t> box_list_t;
private:

	inline static void copy_without(typename box_list_t::const_iterator x,
			typename box_list_t::const_iterator y, box_list_t const & list,
			box_list_t & out) {
		typename box_list_t::const_iterator i = list.begin();
		while (i != list.end()) {
			if (i != x && i != y) {
				out.push_back(*i);
			}
			++i;
		}
	}

	/* merge 2 rectangle, not efficiently */
	static bool merge_area_macro(box_list_t & list) {

		box_list_t result;
		box_list_t tmp;
		typename box_list_t::const_iterator i = list.begin();
		while (i != list.end()) {
			typename box_list_t::const_iterator j = list.begin();
			while (j != list.end()) {
				if (i != j) {
					if ((*i).x + (*i).w == (*j).x && (*i).y == (*j).y
							&& (*i).h == (*j).h) {
						copy_without(i, j, list, tmp);
						tmp.push_back(
								box_int_t((*i).x, (*i).y, (*j).w + (*i).w,
										(*i).h));
						list = tmp;
						return true;
					}

					if ((*i).x == (*j).x + (*j).w && (*i).y == (*j).y
							&& (*i).h == (*j).h) {
						copy_without(i, j, list, tmp);
						tmp.push_back(
								box_int_t((*j).x, (*j).y, (*j).w + (*i).w,
										(*j).h));
						list = tmp;
						return true;
					}

					if ((*i).y == (*j).y + (*j).h && (*i).x == (*j).x
							&& (*i).w == (*j).w) {
						copy_without(i, j, list, tmp);
						tmp.push_back(
								box_int_t((*j).x, (*j).y, (*j).w,
										(*j).h + (*i).h));
						list = tmp;
						return true;
					}

					if ((*i).y + (*i).h == (*j).y && (*i).x == (*j).x
							&& (*i).w == (*j).w) {
						copy_without(i, j, list, tmp);
						tmp.push_back(
								box_int_t((*i).x, (*i).y, (*i).w,
										(*j).h + (*i).h));
						list = tmp;
						return true;
					}

				}
				++j;
			}
			++i;
		}

		return false;

	}

	static bool _is_null(box_int_t & b) {
		return b.is_null();
	}

	static void remove_empty(box_list_t & list) {
		list.remove_if(_is_null);
	}

	bool is_null() {
		return this->empty();
	}

	/* box0 - box1 */
	static region_t::box_list_t substract_box(_box_t const & box0,
			_box_t const & box1) {
		//std::cout <<"a "<< box0.to_string() << std::endl;
		//std::cout <<"b "<< box1.to_string() << std::endl;
		box_list_t result;

		box_int_t inter_sec = box0 & box1;
		//std::cout <<"c "<< inter_sec.to_string() << std::endl;

		if (inter_sec.w > 0 && inter_sec.h > 0) {
			/* top box */
			{
				int left = inter_sec.x;
				int right = inter_sec.x + inter_sec.w;
				int top = box0.y;
				int bottom = inter_sec.y;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							box_int_t(left, top, right - left, bottom - top));
				}
			}

			/* bottom box */
			{
				int left = inter_sec.x;
				int right = inter_sec.x + inter_sec.w;
				int top = inter_sec.y + inter_sec.h;
				int bottom = box0.y + box0.h;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							box_int_t(left, top, right - left, bottom - top));
				}
			}

			/* left box */
			{
				int left = box0.x;
				int right = inter_sec.x;
				int top = inter_sec.y;
				int bottom = inter_sec.y + inter_sec.h;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							box_int_t(left, top, right - left, bottom - top));
				}
			}

			/* right box */
			{
				int left = inter_sec.x + inter_sec.w;
				int right = box0.x + box0.w;
				int top = inter_sec.y;
				int bottom = inter_sec.y + inter_sec.h;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							box_int_t(left, top, right - left, bottom - top));
				}
			}

			/* top left box */
			{
				int left = box0.x;
				int right = inter_sec.x;
				int top = box0.y;
				int bottom = inter_sec.y;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							box_int_t(left, top, right - left, bottom - top));
				}
			}

			/* top right box */
			{
				int left = inter_sec.x + inter_sec.w;
				int right = box0.x + box0.w;
				int top = box0.y;
				int bottom = inter_sec.y;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							box_int_t(left, top, right - left, bottom - top));
				}
			}

			/* bottom left box */
			{
				int left = box0.x;
				int right = inter_sec.x;
				int top = inter_sec.y + inter_sec.h;
				int bottom = box0.y + box0.h;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							box_int_t(left, top, right - left, bottom - top));
				}
			}

			/* bottom right box */
			{
				int left = inter_sec.x + inter_sec.w;
				int right = box0.x + box0.w;
				int top = inter_sec.y + inter_sec.h;
				int bottom = box0.y + box0.h;

				if (right - left > 0 && bottom - top > 0) {
					result.push_back(
							box_int_t(left, top, right - left, bottom - top));
				}
			}

		} else {
			result.push_back(box0);
		}

		while (merge_area_macro(result))
			continue;

		remove_empty(result);

		return result;

	}

public:

	region_t() {

	}

	region_t(region_t const & r) :
			box_list_t(r) {
	}

	region_t(box_t<T> const & b) {
		if (!b.is_null())
			this->push_back(b);
	}

	region_t(T x, T y, T w, T h) {
		box_t<T> b(x, y, w, h);
		if (!b.is_null())
			this->push_back(b);
	}

	region_t & operator=(box_t<T> const & b) {
		box_list_t::clear();
		if (!b.is_null())
			this->push_back(b);
		return *this;
	}

	region_t operator-(box_t<T> const & box1) const {
		region_t result;
		typename box_list_t::const_iterator i = box_list_t::begin();
		while (i != box_list_t::end()) {
			region_t::box_list_t x = substract_box(*i, box1);
			result.insert(result.end(), x.begin(), x.end());
			++i;
		}

		while (merge_area_macro(result))
			continue;

		remove_empty(result);

		return result;
	}

	region_t & operator-=(box_t<T> const & b) {
		*this = *this - b;
		return *this;
	}

	region_t operator-(region_t const & b) const {
		region_t result = *this;
		typename box_list_t::const_iterator i = b.begin();
		while (i != b.end()) {
			result -= *i;
			++i;
		}

		return result;
	}

	region_t & operator+=(box_t<T> const & b) {
		if (!b.is_null()) {
			//std::cout << "XX this =" << this->to_string() << std::endl;
			*this -= b;
			//std::cout << "X@ this =" << this->to_string() << std::endl;
			this->push_back(b);
			//std::cout << "X# this =" << this->to_string() << std::endl;
			while (merge_area_macro(*this))
				continue;

			remove_empty(*this);
		}
		//std::cout << "X%% this =" << this->to_string() << std::endl;
		return *this;
	}

	region_t operator+(box_t<T> const & b) const {
		region_t result = *this;
		result += b;
		return result;
	}

	region_t operator+(region_t const & r) const {
		region_t result = *this;
		typename box_list_t::const_iterator i = r.begin();
		while (i != r.end()) {
			result += *i;
			++i;
		}
		//std::cout << "X$ this =" << result.to_string() << std::endl;
		return result;
	}

	region_t<T> operator&(region_t<T> const & r) const {
		region_t<T> result;
		typename region_t<T>::box_list_t::const_iterator i, j;
		i = box_list_t::begin();
		while (i != box_list_t::end()) {
			j = r.begin();
			while (j != r.end()) {
				box_int_t v = (*i & *j);
				if (!v.is_null()) {
					result = result + v;
				}
				++j;
			}
			++i;
		}
		return result;
	}


	std::string to_string() const {
		std::ostringstream os;
		//os << area.size();
		typename box_list_t::const_iterator i = box_list_t::begin();
		while (i != box_list_t::end()) {
			if (i != box_list_t::begin())
				os << ",";
			os << (*i).to_string();
			++i;
		}

		return os.str();

	}

	void translate(T x, T y) {
		typename box_list_t::iterator i = box_list_t::begin();
		while (i != box_list_t::end()) {
			(*i).x += x;
			(*i).y += y;
			++i;
		}
	}

};



}

#endif /* REGION_HXX_ */
