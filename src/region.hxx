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

#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include "box.hxx"

namespace page {

using namespace std;

/**
 * region are immuable, any operation create a new
 */
class region_t {

	/**
	 * Data is immuable formed list of int.
	 *
	 * the first int is the size of data in bytes used for creating
	 * new region, from 2 existing region.
	 *
	 * following directly is the list of bands, each band start with the
	 * number of wall in this band and the Y coordinate of the start (inclusive)
	 * of the band followed by the list of wall. note odd walls are inclusive,
	 * even walls are exclusive.
	 *
	 * the last band is terminated by a -1 as size with the last Y coordinate.
	 *
	 * band are always sorted by ascending Y
	 *
	 *
	 * pseudo layout:
	 *
	 * struct region_header_t {
	 *   int band_count;
	 *   int wall_count;
	 *   int first_band_offset; (relative to _data in integer)
	 * };
	 *
	 * struct band_layout_t {
	 *   int next_band_offset; (relative to _data in integer)
	 *   int band_wall_count;
	 *   int band_position_start; (inclusive)
	 *   int bans_position_end; (exclusive)
	 *   int walls[N];
	 * };
	 *
	 *
	 *
	 **/
	int * _data;

	int _data_int_count() const {
		return
		 /* the header */
		  3
		/* band sizes, each band have 2 int overhead */
		+ 4 * _band_count()
		/* the walls size */
		+ _wall_count();
	}


	static bool _operator_union(bool a, bool b) {
		return a or b;
	}

	static bool _operator_substract(bool a, bool b) {
		return a and not b;
	}

	static bool _operator_intersec(bool a, bool b) {
		return a and b;
	}

	inline int & _band_count() {
		return _data[0];
	}

	inline int & _wall_count() {
		return _data[1];
	}

	inline int & _first_band_offset() {
		return _data[2];
	}

	inline int * _first_band() {
		if(_first_band_offset() <= 0)
			return nullptr;
		else
			return &_data[_first_band_offset()];
	}

	inline int const & _band_count() const {
		return _data[0];
	}

	inline int const & _wall_count() const {
		return _data[1];
	}

	inline int const & _first_band_offset() const {
		return _data[2];
	}

	inline int const * _first_band() const {
		if(_first_band_offset() <= 0)
			return nullptr;
		else
			return &_data[_first_band_offset()];
	}

	inline int * _next_band(int * band) {
		if(_band_next_offset(band) <= 0)
			return nullptr;
		else
			return &_data[_band_next_offset(band)];
	}

	inline int const * _next_band(int const * band) const {
		if(_band_next_offset(band) <= 0)
			return nullptr;
		else
			return &_data[_band_next_offset(band)];
	}

	inline int _rects_count() const {
		return _wall_count() / 2;
	}

	inline static int & _band_next_offset(int * band) {
		return band[0];
	}

	/* return the number of wall in this band (always even) */
	inline static int & _band_wall_count(int * band) {
		return band[1];
	}

	/* return the Y start offset of the band, including */
	inline static int & _band_position_start(int * band) {
		return band[2];
	}

	/* return the Y end offset of the band, excluding */
	inline static int & _band_position_end(int * band) {
		return band[3];
	}

	/* get the n th. wall within the band */
	inline static int & _band_get_wall(int * band, int n) {
		return band[4+n];
	}

	inline static int _band_next_offset(int const * band) {
		return band[0];
	}

	/* return the number of wall in this band (always even) */
	inline static int const & _band_wall_count(int const * band) {
		return band[1];
	}

	/* return the Y start offset of the band, including */
	inline static int const & _band_position_start(int const * band) {
		return band[2];
	}

	/* return the Y end offset of the band, excluding */
	inline static int const & _band_position_end(int const * band) {
		return band[3];
	}

	/* get the n th. wall within the band */
	inline static int const & _band_get_wall(int const * band, int n) {
		return band[4+n];
	}

	static bool _equals_band(int const * prev_band, int const * next_band) {
		if(prev_band == nullptr and next_band == nullptr)
			return true;

		if(prev_band == nullptr or next_band == nullptr)
			return false;

		if(_band_wall_count(prev_band) != _band_wall_count(next_band))
			return false;

		for(unsigned k = 0; k < _band_wall_count(prev_band); ++k) {
			if(_band_get_wall(prev_band, k) != _band_get_wall(next_band, k))
				return false;
		}

		return true;
	}


	template<typename F>
	static void _merge_band(F f, int const * band_a, int const * band_b, int * band_r) {
		/* a fake empty band */
		static int const fake_band[] = {0, 0, 0, 0};

		if(band_a == nullptr)
			band_a = &fake_band[0];

		if(band_b == nullptr)
			band_b = &fake_band[0];

		int wall_a = 0;
		int wall_b = 0;

		bool inside_a = false;
		bool inside_b = false;
		bool inside_r = false;

		_band_wall_count(band_r) = 0;

		while(wall_a < _band_wall_count(band_a)
				or wall_b < _band_wall_count(band_b)) {

			int next_wall_a = wall_a;
			int next_wall_b = wall_b;

			if(wall_a < _band_wall_count(band_a)
					and wall_b < _band_wall_count(band_b)) {

				if(_band_get_wall(band_a, wall_a) <= _band_get_wall(band_b, wall_b)) {
					inside_a = not inside_a;
					_band_get_wall(band_r, _band_wall_count(band_r))
						= _band_get_wall(band_a, wall_a);
					++next_wall_a;
				}

				if (_band_get_wall(band_b, wall_b) <= _band_get_wall(band_a, wall_a)) {
					inside_b = not inside_b;
					_band_get_wall(band_r, _band_wall_count(band_r))
						= _band_get_wall(band_b, wall_b);
					++next_wall_b;
				}

			} else if (wall_a < _band_wall_count(band_a)) {
				inside_a = not inside_a;
				_band_get_wall(band_r, _band_wall_count(band_r))
					= _band_get_wall(band_a, wall_a);
				++next_wall_a;
			} else {
				inside_b = not inside_b;
				_band_get_wall(band_r, _band_wall_count(band_r))
					= _band_get_wall(band_b, wall_b);
				++next_wall_b;
			}

			if(inside_r xor f(inside_a, inside_b)) {
				inside_r = not inside_r;
				/* keep the last written wall */
				//cout << "wall = " << _band_get_wall(band_r, _band_wall_count(band_r)) << endl;
				++_band_wall_count(band_r);
			}

			wall_a = next_wall_a;
			wall_b = next_wall_b;

		}

	}

	/**
	 * handler compressed region, i.e. region have empty bands that
	 * aren't stored, this handler fill gaps with empty bands
	 **/
	struct _band_uncompress_handler_t {
		region_t const & r;

		/**
		 * current band or nullptr for empty band.
		 **/
		int const * cur;

		/**
		 * next non-empty band
		 **/
		int const * nxt;

		/**
		 * the start of the current band, whether it is empty or not.
		 **/
		int start;

		/**
		 * the end of the current band, whether it is empty or not.
		 **/
		int end;

		/**
		 * initialise to the first band
		 **/
		_band_uncompress_handler_t(region_t const & r) : r{r} {
			int const * first_band = r._first_band();

			if(first_band == nullptr) {
				cur = nullptr;
				nxt = nullptr;
				start = std::numeric_limits<int>::min();
				end = std::numeric_limits<int>::max();
				return;
			}

			if(_band_position_start(first_band) == std::numeric_limits<int>::min()) {
				cur = first_band;
				start = _band_position_start(cur);
				end = _band_position_end(cur);
				nxt = r._next_band(cur);
			} else {

				cur = nullptr;
				nxt = first_band;
				start = std::numeric_limits<int>::min();

				if(nxt != nullptr)
					end = _band_position_start(nxt);
				else
					end = std::numeric_limits<int>::max();

			}
		}

		/**
		 * advance to the next band.
		 **/
		void next() {
			if(nxt == nullptr and cur == nullptr)
				return;

			/* advance band_a */
			if(cur == nullptr) {
				/** an empty band is followed by an existing band **/

				cur = nxt;
				nxt = r._next_band(cur);
				start = _band_position_start(cur);
				end = _band_position_end(cur);
			} else {
				/* no more bands */
				if(nxt == nullptr) {
					cur = nullptr;
					start = end;
					end = std::numeric_limits<int>::max();
				} else {
					if(_band_position_start(nxt) == end) {
						cur = nxt;
						nxt = r._next_band(cur);
						start = _band_position_start(cur);
						end = _band_position_end(cur);
					} else {
						cur = nullptr;
						start = end;
						end = _band_position_start(nxt);
					}
				}
			}
		}

	};




	template<typename F>
	static region_t _merge(F f, region_t const & a, region_t const & b) {
		static int buffer_size = 0;
		static int * buffer = nullptr;

		region_t r;

		/**
		 * Calculate the size of the biguest region that is possible to
		 * generate, knowing some parameters of a and b. I didn't thought
		 * carefully at the question but I try to use a large margin.
		 **/
		int maxsize = 3 + 4*4*(a._band_count()+b._band_count())
				+ 4*(a._band_count()+b._band_count())*(a._wall_count()+b._wall_count());

		if(r._data != nullptr)
			std::free(r._data);

		r._data = reinterpret_cast<int*>(std::malloc(sizeof(int)*maxsize));

		int band_r = 0;
		int wall_r_count = 0;


		/** uncompress empty band **/
		_band_uncompress_handler_t band_a{a};

		/** uncompress empty band **/
		_band_uncompress_handler_t band_b{b};


		/** keep this ref to remove last band if needed **/
		r._first_band_offset() = 3;
		int * current_band_r_ref = &r._first_band_offset();
		int * current_band_r = r._first_band();
		int * prev_band = nullptr;

		while(band_a.end != std::numeric_limits<int>::max()
				or band_b.end != std::numeric_limits<int>::max()) {

			//cout << "banda = " << band_a.start << " " << band_a.end << endl;
			//cout << "bandb = " << band_b.start << " " << band_b.end << endl;

			/* by definition they must overlap i.e. start <= end */
			int start = std::max(band_a.start, band_b.start);
			int end = std::min(band_a.end, band_b.end);
			_band_position_start(current_band_r) = start;
			_band_position_end(current_band_r) = end;
			_band_next_offset(current_band_r) = 0;

			_merge_band(f, band_a.cur, band_b.cur, current_band_r);

			if(band_a.end == band_b.end) {
				band_a.next();
				band_b.next();
			} else if(band_a.end < band_b.end) {
				/* advance band_a */
				band_a.next();
			} else {
				/* advance band_b */
				band_b.next();
			}

			if(prev_band == nullptr) {
				if (_band_wall_count(current_band_r) > 0) {
					/** keep the current band **/
					prev_band = current_band_r;
					wall_r_count += _band_wall_count(current_band_r);
					_band_next_offset(current_band_r) = *current_band_r_ref + 4
							+ _band_wall_count(current_band_r);
					current_band_r_ref = &_band_next_offset(current_band_r);
					current_band_r = &r._data[*current_band_r_ref];
					_band_next_offset(current_band_r) = 0;
					++band_r;
				}
			} else {
				/* validate the fact the the current band is not the same of the previous one */
				if(_equals_band(prev_band, current_band_r)
						and _band_position_end(prev_band) == _band_position_start(current_band_r)) {
					/** if band are the same, merge current band with the previous one **/
					_band_position_end(prev_band) = _band_position_end(current_band_r);
				} else if (_band_wall_count(current_band_r) <= 0) {
					/** ignore empty band **/
				} else {
					/** keep the current band **/
					prev_band = current_band_r;
					wall_r_count += _band_wall_count(current_band_r);
					_band_next_offset(current_band_r) =
							*current_band_r_ref + 4 + _band_wall_count(current_band_r);
					current_band_r_ref = &_band_next_offset(current_band_r);
					current_band_r = &r._data[*current_band_r_ref];
					_band_next_offset(current_band_r) = 0;
					++band_r;
				}
			}
		}

		/* remove last band */
		*current_band_r_ref = 0;

		r._band_count() = band_r;
		r._wall_count() = wall_r_count;

		//cout << "xxx " << r.dump_data() << endl;
		r._data = reinterpret_cast<int*>(std::realloc(r._data,
				sizeof(int)*r._data_int_count()));

		return r;
	}





public:

	region_t() : _data{nullptr} {
		clear();
	}

	region_t(int x, int y, int w, int h) : region_t(i_rect_t<int>(x,y,w,h)){

	}

	region_t(i_rect_t<int> const & b) : _data{nullptr} {
		if (not b.is_null()) {
			/**
			 * a box is a single band thus size is:
			 *  size header;
			 *  the first band with 2 wall;
			 *  the terminating band.
			 **/
			_data = reinterpret_cast<int*>(std::malloc(sizeof(int)*(3 + 4 + 2)));

			/* the size header */
			_band_count() = 1; /* band count */
			_wall_count() = 2; /* wall count */
			_first_band_offset() = 3;

			int * first_band = _first_band();

			_band_next_offset(first_band) = 0;
			_band_wall_count(first_band) = 2;
			_band_position_start(first_band) = b.y;
			_band_position_end(first_band) = b.y+b.h;
			_band_get_wall(first_band, 0) = b.x;
			_band_get_wall(first_band, 1) = b.x+b.w;

		} else {
			clear();
		}
	}
	region_t(xcb_rectangle_t const * r) : region_t(r->x, r->y, r->width, r->height) {

	}

	region_t(vector<int> const & l) : _data{nullptr} {
		clear();
		for(int k = 0; k < l.size(); k += 4) {
			(*this) += region_t(l[k], l[k+1], l[k+2], l[k+3]);
		}
	}

	region_t(region_t const & b) {
		_data = reinterpret_cast<int*>(std::malloc(sizeof(int)*b._data_int_count()));
		std::copy(b._data, &b._data[b._data_int_count()], _data);
	}

	region_t(region_t && b) : _data{nullptr} {
		std::swap(_data, b._data);
	}

	~region_t() {
		if(_data != nullptr) {
			std::free(_data);
		}
	}

	region_t const & operator =(region_t const & b) {
		if(this != &b) {
			if(_data != nullptr)
				std::free(_data);
			_data = reinterpret_cast<int*>(std::malloc(sizeof(int)*b._data_int_count()));
			std::copy(b._data, &b._data[b._data_int_count()], _data);
		}
		return *this;
	}

	region_t operator +(region_t const & b) const {
		return _merge(&_operator_union, *this, b);
	}

	region_t operator -(region_t const & b) const {
		return _merge(&_operator_substract, *this, b);
	}

	region_t operator &(region_t const & b) const {
		return _merge(&_operator_intersec, *this, b);
	}

	region_t const & operator +=(region_t const & b) {
		(*this) = (*this) + b;
		return *this;
	}

	region_t const & operator -=(region_t const & b) {
		(*this) = (*this) - b;
		return *this;
	}

	region_t const & operator &=(region_t const & b) {
		(*this) = (*this) & b;
		return *this;
	}

	vector<i_rect_t<int>> rects() const {
		vector<i_rect_t<int>> ret(_rects_count());
		int nr = 0;
		int const * band = _first_band();
		while(band != nullptr) {
			for(int k = 0; k < _band_wall_count(band); k += 2) {
				ret[nr] = i_rect_t<int>{
					_band_get_wall(band, k),
					_band_position_start(band),
					_band_get_wall(band, k + 1)-_band_get_wall(band, k),
					_band_position_end(band)-_band_position_start(band)
				};
				++nr;
			}
			band = _next_band(band);
		}
		return ret;
	}

	void translate(int x, int y) {
		int * band = _first_band();
		while(band != nullptr) {
			_band_position_start(band) += y;
			_band_position_end(band) += y;
			for(int k = 0; k < _band_wall_count(band); ++k) {
				_band_get_wall(band, k) += x;
			}
			band = _next_band(band);
		}
	}


	void clear() {

		if(_data != nullptr)
			std::free(_data);

		_data = reinterpret_cast<int*>(std::malloc(sizeof(int) * 3));

		/* the size header */
		_band_count() = 0;
		_wall_count() = 0;
		_first_band_offset() = 0;

	}

	bool empty() const {
		return _band_count() == 0;
	}

	int area() {
		int ret = 0;
		for(auto & r : rects()) {
			ret += r.h*r.w;
		}
		return ret;
	}

	std::string to_string() {
		if(empty())
			return std::string{"[]"};

		std::ostringstream os;

		for(auto & r : rects()) {
			os << "[" << r.x << "," << r.y << "," << r.w << "," << r.h << "]";
		}

		return os.str();
	}


	std::string dump_data() {
		std::ostringstream os;

		if(0 < _data_int_count())
			os << _data[0];

		for(int i = 1; i < _data_int_count(); ++i)
			os << " " << _data[i];

		return os.str();
	}

	bool is_inside(int x, int y) {
		int const * band = _first_band();
		while (band != nullptr) {

			if (y >= _band_position_start(band)
					and y < _band_position_end(band)) {

				for (int k = 0; k < _band_wall_count(band); k += 2) {
					if (x >= _band_get_wall(band, k)
							and x < _band_get_wall(band, k + 1))
						return true;
				}

				return false;

			}
			band = _next_band(band);
		}
		return false;
	}

};


typedef region_t region;

}

#endif /* REGION_HXX_ */
