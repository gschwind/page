/*
 * region.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef REGION_HXX_
#define REGION_HXX_

#include "box.hxx"

namespace page {

template<typename T>
class region_t {
	typedef box_t<T> _box_t;
	typedef std::list<_box_t> box_list_t;
	box_list_t area;

	/* merge 2 reactangle, not efficiently */
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

	/* box0 - box1 */
	static region_t::box_list_t substract_box(_box_t const & box0, _box_t const & box1) {
		box_list_t result;

		box_int_t inter_sec = box0 & box1;

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

		return result;

	}

public:

	region_t() {

	}

	region_t(region_t const & r) {
		area = r.area;
	}

	region_t(box_t<T> const & b) {
		area.push_back(b);
	}

	region_t & operator=(region_t const & r) {
		if (this != &r)
			area = r.area;
		return *this;
	}

	region_t & operator=(box_t<T> const & b) {
		area.clear();
		area.push_back(b);
		return *this;
	}

	region_t operator-(box_t<T> const & box1) const {
		region_t result;
		typename box_list_t::const_iterator i = area.begin();
		while (i != area.end()) {
			region_t::box_list_t x = substract_box(*i, box1);
			result.area.insert(result.area.end(), x.begin(), x.end());
			++i;
		}

		while (merge_area_macro(result.area))
			continue;

		return result;
	}

	region_t & operator-=(box_t<T> const & b) {
		*this = *this - b;
		return *this;
	}

	region_t operator-(region_t const & b) const {
		region_t result = *this;
		typename box_list_t::const_iterator i = b.area.begin();
		while (i != b.area.end()) {
			result -= *i;
			++i;
		}

		return result;
	}

	region_t & operator+=(box_t<T> const & b) {
		*this -= b;
		area.push_back(b);

		while (merge_area_macro(area))
			continue;

		return *this;
	}

	region_t operator+(box_t<T> const & b) const {
		region_t result = *this;
		result += b;
		return result;
	}

	region_t operator+(region_t const & r) const {
		region_t result = *this;
		typename box_list_t::const_iterator i = r.area.begin();
		while(i != r.area.end()) {
			result += *i;
			++i;
		}
		return result;
	}

	void clear() {
		area.clear();
	}

	std::list<box_t<T> > get_area() {
		return area;
	}

	typename std::list<box_t<T> >::const_iterator begin() const {
		return area.begin();
	}

	typename std::list<box_t<T> >::const_iterator end() const {
		return area.end();
	}


};

}

#endif /* REGION_HXX_ */
