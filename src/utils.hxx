/*
 * utils.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef UTILS_HXX_
#define UTILS_HXX_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

#include <xcb/xcb_util.h>

#include <cairo/cairo.h>

#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <memory>
#include <sstream>
#include <iostream>

#include "color.hxx"
#include "box.hxx"
#include "x11_func_name.hxx"
#include "exception.hxx"

namespace page {

using namespace std;

#define warn(test) \
	do { \
		if(not (test)) { \
			printf("WARN %s:%d (%s) fail!\n", __FILE__, __LINE__, #test); \
		} \
	} while(false)

/* clients limites for Xorg */
static int const LIMITCLIENTS = 256;

//constexpr unsigned int ilog2(unsigned int val)
//{
//    int bits;
//
//    if (val <= 0)
//    	return 0;
//    for (bits = 0; val != 0; bits++)
//    	val >>= 1;
//    return bits - 1;
//}

constexpr unsigned ilog2_a(int val) {
	return val<=0? 0 : ilog2_a(val>>1) + 1;
}

constexpr unsigned int ilog2(int val) {
	return ilog2_a(val) - 1;
}

/** deduced from Xorg server **/
inline unsigned int client_id(unsigned int resource_id) {
	return ((((resource_id) & (((1 << ilog2(LIMITCLIENTS)) - 1) << (29 - ilog2(LIMITCLIENTS)))) >> (29 - ilog2(LIMITCLIENTS))));
}

/**
 * TRICK to compile time checking.
 **/
// Use of ctassert<E>, where E is a constant expression,
// will cause a compile-time error unless E evaulates to
// a nonzero integral value.

template <bool t>
struct ctassert {
  enum { N = 1 - 2 * int(!t) };
    // 1 if t is true, -1 if t is false.
  static char A[N];
};

template <bool t>
char ctassert<t>::A[N];

template<typename T>
struct dimention_t {
	T width;
	T height;

	dimention_t(T w, T h) : width{w}, height{h} { }
	dimention_t(dimention_t const & d) : width{d.width}, height{d.height} { }

};


template<typename T, typename _>
bool has_key(std::map<T, _> const & x, T const & key) {
	auto i = x.find(key);
	return i != x.end();
}

template<typename T>
bool has_key(std::set<T> const & x, T const & key) {
	auto i = x.find(key);
	return i != x.end();
}

template<typename T>
bool has_key(std::list<T> const & x, T const & key) {
	auto i = std::find(x.begin(), x.end(), key);
	return i != x.end();
}

template<typename T>
bool has_key(std::vector<T> const & x, T const & key) {
	typename std::vector<T>::const_iterator i = find(x.begin(), x.end(), key);
	return i != x.end();
}

template<typename K, typename V>
std::list<V> list_values(std::map<K, V> const & x) {
	std::list<V> ret;
	auto i = x.begin();
	while(i != x.end()) {
		ret.push_back(i->second);
		++i;
	}

	return ret;
}

template<typename _0, typename _1>
_1 get_safe_value(std::map<_0, _1> & x, _0 key, _1 def) {
	typename std::map<_0, _1>::iterator i = x.find(key);
	if (i != x.end())
		return i->second;
	else
		return def;
}

template<typename T>
void safe_delete(T & p) {
	delete p;
	p = nullptr;
}

template<typename T> T * safe_copy(T * p) {
	if (p != nullptr) {
		return new T(*p);
	} else {
		return nullptr;
	}
}

/* C++11 reverse iterators */
template <typename T>
class reverse_range {
	T& _x;
public:
	reverse_range(T & x) : _x(x) { }

	auto begin() const -> decltype(_x.rbegin()) {
		return _x.rbegin();
	}

	auto end() const -> decltype(_x.rend()) {
		return _x.rend();
	}
};

template <typename T>
reverse_range<T> reverse_iterate (T& x)
{
  return reverse_range<T> (x);
}

template<typename T0, typename T1>
std::list<std::shared_ptr<T0>> filter_class_lock(std::list<std::weak_ptr<T1>> const & x) {
	std::list<std::shared_ptr<T0>> ret;
	for (auto i : x) {
		if(i.expired())
			continue;

		auto n = dynamic_pointer_cast<T0>(i.lock());
		if (n != nullptr) {
			ret.push_back(n);
		}
	}
	return ret;
}

template<typename T0, typename T1>
std::vector<std::shared_ptr<T0>> filter_class_lock(std::vector<std::weak_ptr<T1>> const & x) {
	std::vector<std::shared_ptr<T0>> ret;
	for (auto i : x) {
		if(i.expired())
			continue;

		auto n = dynamic_pointer_cast<T0>(i.lock());
		if (n != nullptr) {
			ret.push_back(n);
		}
	}
	return ret;
}

template<typename T0, typename T1>
std::list<std::weak_ptr<T0>> filter_class(std::list<std::weak_ptr<T1>> const & x) {
	std::list<std::weak_ptr<T0>> ret;
	for (auto i : x) {
		if(i.expired())
			continue;

		auto n = dynamic_pointer_cast<T0>(i.lock());
		if (n != nullptr) {
			ret.push_back(n);
		}
	}
	return ret;
}

template<typename T0, typename T1>
std::vector<std::shared_ptr<T0>> filter_class(std::vector<std::shared_ptr<T1>> const & x) {
	std::vector<std::shared_ptr<T0>> ret;
	for (auto i : x) {
		auto n = dynamic_pointer_cast<T0>(i);
		if (n != nullptr) {
			ret.push_back(n);
		}
	}
	return ret;
}

template<typename T0, typename T1>
std::list<std::shared_ptr<T0>> filter_class(std::list<std::shared_ptr<T1>> const & x) {
	std::list<std::shared_ptr<T0>> ret;
	for (auto i : x) {
		auto n = dynamic_pointer_cast<T0>(i);
		if (n != nullptr) {
			ret.push_back(n);
		}
	}
	return ret;
}

template<typename T0>
std::list<std::weak_ptr<T0>> weak(std::list<std::shared_ptr<T0>> const & x) {
	return std::list<std::weak_ptr<T0>>{x.begin(), x.end()};
}

template<typename T0>
std::vector<std::weak_ptr<T0>> weak(std::vector<std::shared_ptr<T0>> const & x) {
	return std::vector<std::weak_ptr<T0>>{x.begin(), x.end()};
}

template <typename T>
bool is_expired(weak_ptr<T> & x) {
	return x.expired();
}

template<typename T0>
std::list<std::shared_ptr<T0>> lock(std::list<std::weak_ptr<T0>> & x) {
	x.remove_if(is_expired<T0>);
	std::list<std::shared_ptr<T0>> ret;
	for(auto i: x) ret.push_back(i.lock());
	return ret;
}

static void draw_outer_graddien(cairo_t * cr, rect r, color_t const & iner_color, double _shadow_width) {

	cairo_save(cr);

	/** draw left shawdow **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr,r.x - _shadow_width, r.y, _shadow_width, r.h);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * grad0 = cairo_pattern_create_linear(r.x - _shadow_width, 0.0, r.x, 0.0);
	cairo_pattern_add_color_stop_rgba(grad0, 0.0, iner_color.r, iner_color.g, iner_color.b, 0.0);
	cairo_pattern_add_color_stop_rgba(grad0, 1.0, iner_color.r, iner_color.g, iner_color.b, iner_color.a);
	cairo_mask(cr, grad0);
	cairo_pattern_destroy(grad0);

	/** draw right shadow **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x + r.w, r.y, _shadow_width, r.h);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * grad1 = cairo_pattern_create_linear(r.x + r.w, 0.0, r.x + r.w + _shadow_width, 0.0);
	cairo_pattern_add_color_stop_rgba(grad1, 1.0, iner_color.r, iner_color.g, iner_color.b, 0.0);
	cairo_pattern_add_color_stop_rgba(grad1, 0.0, iner_color.r, iner_color.g, iner_color.b, iner_color.a);
	cairo_mask(cr, grad1);
	cairo_pattern_destroy(grad1);

	/** draw top shadow **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x, r.y - _shadow_width, r.w, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * grad2 = cairo_pattern_create_linear(0.0, r.y - _shadow_width, 0.0, r.y);
	cairo_pattern_add_color_stop_rgba(grad2, 0.0, iner_color.r, iner_color.g, iner_color.b, 0.0);
	cairo_pattern_add_color_stop_rgba(grad2, 1.0, iner_color.r, iner_color.g, iner_color.b, iner_color.a);
	cairo_mask(cr, grad2);
	cairo_pattern_destroy(grad2);

	/** draw bottom shadow **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x, r.y + r.h, r.w, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * grad3 = cairo_pattern_create_linear(0.0, r.y + r.h, 0.0, r.y + r.h + _shadow_width);
	cairo_pattern_add_color_stop_rgba(grad3, 1.0, iner_color.r, iner_color.g, iner_color.b, 0.0);
	cairo_pattern_add_color_stop_rgba(grad3, 0.0, iner_color.r, iner_color.g, iner_color.b, iner_color.a);
	cairo_mask(cr, grad3);
	cairo_pattern_destroy(grad3);


	/** draw top-left corner **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x - _shadow_width, r.y - _shadow_width, _shadow_width, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * r0grad = cairo_pattern_create_radial(r.x, r.y, 0.0, r.x, r.y, _shadow_width);
	cairo_pattern_add_color_stop_rgba(r0grad, 0.0, iner_color.r, iner_color.g, iner_color.b, iner_color.a);
	cairo_pattern_add_color_stop_rgba(r0grad, 1.0, iner_color.r, iner_color.g, iner_color.b, 0.0);
	cairo_mask(cr, r0grad);
	cairo_pattern_destroy(r0grad);

	/** draw top-right corner **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x + r.w, r.y - _shadow_width, _shadow_width, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * r1grad = cairo_pattern_create_radial(r.x + r.w, r.y, 0.0, r.x + r.w, r.y, _shadow_width);
	cairo_pattern_add_color_stop_rgba(r1grad, 0.0, iner_color.r, iner_color.g, iner_color.b, iner_color.a);
	cairo_pattern_add_color_stop_rgba(r1grad, 1.0, iner_color.r, iner_color.g, iner_color.b, 0.0);
	cairo_mask(cr, r1grad);
	cairo_pattern_destroy(r1grad);

	/** bottom-left corner **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x - _shadow_width, r.y + r.h, _shadow_width, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * r2grad = cairo_pattern_create_radial(r.x, r.y + r.h, 0.0, r.x, r.y + r.h, _shadow_width);
	cairo_pattern_add_color_stop_rgba(r2grad, 0.0, iner_color.r, iner_color.g, iner_color.b, iner_color.a);
	cairo_pattern_add_color_stop_rgba(r2grad, 1.0, iner_color.r, iner_color.g, iner_color.b, 0.0);
	cairo_mask(cr, r2grad);
	cairo_pattern_destroy(r2grad);

	/** draw bottom-right corner **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x + r.w, r.y + r.h, _shadow_width, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * r3grad = cairo_pattern_create_radial(r.x + r.w, r.y + r.h, 0.0, r.x + r.w, r.y + r.h, _shadow_width);
	cairo_pattern_add_color_stop_rgba(r3grad, 0.0, iner_color.r, iner_color.g, iner_color.b, iner_color.a);
	cairo_pattern_add_color_stop_rgba(r3grad, 1.0, iner_color.r, iner_color.g, iner_color.b, 0.0);
	cairo_mask(cr, r3grad);
	cairo_pattern_destroy(r3grad);

	cairo_restore(cr);

}

static void draw_outer_graddien2(cairo_t * cr, rect r, double _shadow_width, double radius) {

	cairo_save(cr);

	/** draw left shawdow **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr,r.x - _shadow_width, r.y + radius, _shadow_width, r.h - radius);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * grad0 = cairo_pattern_create_linear(r.x - _shadow_width, 0.0, r.x, 0.0);
	cairo_pattern_add_color_stop_rgba(grad0, 0.0, 0.0, 0.0, 0.0, 0.00);
	cairo_pattern_add_color_stop_rgba(grad0, 0.5, 0.0, 0.0, 0.0, 0.05);
	cairo_pattern_add_color_stop_rgba(grad0, 1.0, 0.0, 0.0, 0.0, 0.20);
	cairo_mask(cr, grad0);
	cairo_pattern_destroy(grad0);

	/** draw right shadow **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x + r.w, r.y + radius, _shadow_width, r.h - radius);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * grad1 = cairo_pattern_create_linear(r.x + r.w, 0.0, r.x + r.w + _shadow_width, 0.0);
	cairo_pattern_add_color_stop_rgba(grad1, 1.0, 0.0, 0.0, 0.0, 0.00);
	cairo_pattern_add_color_stop_rgba(grad1, 0.5, 0.0, 0.0, 0.0, 0.05);
	cairo_pattern_add_color_stop_rgba(grad1, 0.0, 0.0, 0.0, 0.0, 0.20);
	cairo_mask(cr, grad1);
	cairo_pattern_destroy(grad1);

	/** draw top shadow **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x + radius, r.y - _shadow_width, r.w - 2*radius, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * grad2 = cairo_pattern_create_linear(0.0, r.y - _shadow_width, 0.0, r.y);
	cairo_pattern_add_color_stop_rgba(grad2, 0.0, 0.0, 0.0, 0.0, 0.00);
	cairo_pattern_add_color_stop_rgba(grad2, 0.5, 0.0, 0.0, 0.0, 0.05);
	cairo_pattern_add_color_stop_rgba(grad2, 1.0, 0.0, 0.0, 0.0, 0.20);
	cairo_mask(cr, grad2);
	cairo_pattern_destroy(grad2);

	/** draw bottom shadow **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x, r.y + r.h, r.w, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * grad3 = cairo_pattern_create_linear(0.0, r.y + r.h, 0.0, r.y + r.h + _shadow_width);
	cairo_pattern_add_color_stop_rgba(grad3, 1.0, 0.0, 0.0, 0.0, 0.00);
	cairo_pattern_add_color_stop_rgba(grad3, 0.5, 0.0, 0.0, 0.0, 0.05);
	cairo_pattern_add_color_stop_rgba(grad3, 0.0, 0.0, 0.0, 0.0, 0.20);
	cairo_mask(cr, grad3);
	cairo_pattern_destroy(grad3);


	/** draw top-left corner **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x - _shadow_width, r.y - _shadow_width, _shadow_width + radius, _shadow_width + radius);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * r0grad = cairo_pattern_create_radial(r.x + radius, r.y + radius, radius, r.x + radius, r.y + radius, _shadow_width + radius);
	cairo_pattern_add_color_stop_rgba(r0grad, 0.0, 0.0, 0.0, 0.0, 0.20);
	cairo_pattern_add_color_stop_rgba(r0grad, 0.5, 0.0, 0.0, 0.0, 0.05);
	cairo_pattern_add_color_stop_rgba(r0grad, 1.0, 0.0, 0.0, 0.0, 0.00);
	cairo_mask(cr, r0grad);
	cairo_pattern_destroy(r0grad);

	/** draw top-right corner **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x + r.w - radius, r.y - _shadow_width, _shadow_width + radius, _shadow_width + radius);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * r1grad = cairo_pattern_create_radial(r.x + r.w - radius, r.y + radius, radius, r.x + r.w - radius, r.y + radius, _shadow_width + radius);
	cairo_pattern_add_color_stop_rgba(r1grad, 0.0, 0.0, 0.0, 0.0, 0.20);
	cairo_pattern_add_color_stop_rgba(r1grad, 0.5, 0.0, 0.0, 0.0, 0.05);
	cairo_pattern_add_color_stop_rgba(r1grad, 1.0, 0.0, 0.0, 0.0, 0.00);
	cairo_mask(cr, r1grad);
	cairo_pattern_destroy(r1grad);

	/** bottom-left corner **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x - _shadow_width, r.y + r.h, _shadow_width, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * r2grad = cairo_pattern_create_radial(r.x, r.y + r.h, 0.0, r.x, r.y + r.h, _shadow_width);
	cairo_pattern_add_color_stop_rgba(r2grad, 0.0, 0.0, 0.0, 0.0, 0.20);
	cairo_pattern_add_color_stop_rgba(r2grad, 0.5, 0.0, 0.0, 0.0, 0.05);
	cairo_pattern_add_color_stop_rgba(r2grad, 1.0, 0.0, 0.0, 0.0, 0.00);
	cairo_mask(cr, r2grad);
	cairo_pattern_destroy(r2grad);

	/** draw bottom-right corner **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x + r.w, r.y + r.h, _shadow_width, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * r3grad = cairo_pattern_create_radial(r.x + r.w, r.y + r.h, 0.0, r.x + r.w, r.y + r.h, _shadow_width);
	cairo_pattern_add_color_stop_rgba(r3grad, 0.0, 0.0, 0.0, 0.0, 0.20);
	cairo_pattern_add_color_stop_rgba(r3grad, 0.5, 0.0, 0.0, 0.0, 0.05);
	cairo_pattern_add_color_stop_rgba(r3grad, 1.0, 0.0, 0.0, 0.0, 0.00);
	cairo_mask(cr, r3grad);
	cairo_pattern_destroy(r3grad);

	cairo_restore(cr);

}

/** append vector to the end of list **/
template<typename T>
std::vector<T> & operator +=(std::vector<T> & a, std::vector<T> const & b) {
	a.insert(a.end(), b.begin(), b.end());
	return a;
}

template<typename T>
std::vector<T> operator +(std::vector<T> const & a, std::vector<T> const & b) {
	std::vector<T> ret{a};
	return (ret += b);
}

template<typename T>
std::vector<T> & operator +=(std::vector<T> & a, T const & b) {
	a.push_back(b);
	return a;
}

template<typename T>
std::vector<T> operator +(std::vector<T> const & a, T const & b) {
	std::vector<T> ret{a};
	return (ret += b);
}

inline void cairo_clip(cairo_t * cr, rect const & clip) {
	cairo_reset_clip(cr);
	cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
	::cairo_clip(cr);
}

template<typename T>
class _format {
	std::ios::fmtflags flags;
	int width;
	int precision;
	char fill;

	T const & data;

	int read_int(std::string const & s, int pos, int & value) {
		bool cont = true;
		do {
			switch(s[pos]) {
			case '0':
				value = value*10+0;
				pos += 1;
				break;
			case '1':
				value = value*10+1;
				pos += 1;
				break;
			case '2':
				value = value*10+2;
				pos += 1;
				break;
			case '3':
				value = value*10+3;
				pos += 1;
				break;
			case '4':
				value = value*10+4;
				pos += 1;
				break;
			case '5':
				value = value*10+5;
				pos += 1;
				break;
			case '6':
				value = value*10+6;
				pos += 1;
				break;
			case '7':
				value = value*10+7;
				pos += 1;
				break;
			case '8':
				value = value*10+8;
				pos += 1;
				break;
			case '9':
				value = value*10+9;
				pos += 1;
				break;
			default:
				cont = false;
				break;
			}
		} while(cont);
		return pos;
	}


public:
	_format(std::string const & s, T const & data) : flags(), width(0), precision(0), fill(' '), data(data) {
		int pos = 0;
		switch(s[0]) {
		case '-':
			flags |= std::ios::left;
			pos += 1;
			break;
		case '+':
			flags |= std::ios::showpos;
			pos += 1;
			break;
		case '#':
			flags |= std::ios::showbase;
			pos += 1;
			break;
		case '0':
			fill = '0';
			pos += 1;
			break;
		case ' ':
			fill = ' ';
			pos += 1;
			break;
		case 0:
			break;
		default:
			break;
		}

		pos = read_int(s, pos, width);

		if(s[pos] == '.') {
			pos += 1;
			pos = read_int(s, pos, precision);
		}

		switch(s[pos]) {
			case 'g':
				flags |= std::ios::scientific;
				pos += 1;
				break;
			case 'x':
				flags |= std::ios::hex;
				pos += 1;
				break;
			case 'X':
				flags |= std::ios::hex | std::ios::uppercase;
				pos += 1;
				break;
			case 'e':
				flags |= std::ios::scientific;
				pos += 1;
				break;
			case 'E':
				flags |= std::ios::scientific | std::ios::uppercase;
				pos += 1;
				break;
			case 'o':
				flags |= std::ios::oct;
				pos += 1;
				break;
			case 0:
				break;
			default:
				break;
		}

	}

	std::string str() {
		std::ostringstream os;
		/** set new flags and backup previous **/
		os.flags(flags);
		os.width(width);
		os.precision(precision);
		os.fill(fill);
		os << data;
		return os.str();
	}

};

template<typename T>
std::string format(std::string const & s, T const & data) {
	_format<T> tmp{s, data};
	return tmp.str();
}

template<typename T>
std::list<T> make_list(std::vector<T> const & v) {
	return std::list<T>{v.begin(), v.end()};
}

static unsigned int const ALL_DESKTOP = static_cast<unsigned int>(-1);


template<typename ... F>
class signal_t {
	using _func_t = std::function<void(F ...)>;
	std::list<std::weak_ptr<_func_t> > _callback_list;

public:

	using signal_func_t = std::shared_ptr<_func_t>;

	~signal_t() {

	}

	/**
	 * Connect a function to this signal and return a reference to this function. This function
	 * is not owned by the signal (i.e. signal only keep a weak ptr to the function). The caller
	 * must keep the returned shared ptr until he want to remove the signal. The caller can
	 * disconnect signal or turn the shared_ptr to nullptr to remove the handler from the signal queue.
	 **/
	template<typename T0>
	signal_func_t connect(T0 * ths, void(T0::*func)(F ...)) {
		auto ret = std::make_shared<_func_t>([ths, func](F ... args) -> void { (ths->*func)(args...); });
		_callback_list.push_back(ret);
		return ret;
	}

	signal_func_t connect(signal_func_t func) {
		_callback_list.push_back(func);
		return func;
	}

	template<typename G>
	signal_func_t connect(G func) {
		auto ret = std::make_shared<_func_t>(func);
		_callback_list.push_back(ret);
		return ret;
	}

	void disconnect(signal_func_t f) {
		auto i = _callback_list.begin();
		while(i != _callback_list.end()) {
			if((*i).expired()) {
				i = _callback_list.erase(i);
			} else {
				if(f == (*i).lock()) {
					i = _callback_list.erase(i);
				} else {
					++i;
				}
			}
		}
	}

	void signal(F ... args) {
		/** cleanup the content of callback list **/
		_callback_list.remove_if([](std::weak_ptr<_func_t> & wfunc) -> bool { return wfunc.expired(); });

		/**
		 * Copy the list of callback to avoid issue
		 * if 'disconnect' is called during the signal.
		 **/
		auto callbacks = _callback_list;

		for(auto wfunc: callbacks) {
			if(not wfunc.expired()) {
				/**
				 * Hold the function to avoid destroy
				 * during the signal handling.
				 **/
				auto func = wfunc.lock();
				(*func)(args...);
			}
		}
	}

};

template<typename T>
void move_front(std::list<weak_ptr<T>> & l, shared_ptr<T> const & v) {
	auto pos = std::find_if(l.begin(), l.end(), [v](weak_ptr<T> & l) { return l.lock() == v; });
	if(pos != l.end()) {
		l.splice(l.begin(), l, pos);
	} else {
		l.push_front(v);
	}
}

template<typename T>
void move_back(std::list<weak_ptr<T>> & l, shared_ptr<T> const & v) {
	auto pos = std::find_if(l.begin(), l.end(), [v](weak_ptr<T> & l) { return l.lock() == v; });
	if(pos != l.end()) {
		l.splice(l.end(), l, pos);
	} else {
		l.push_back(v);
	}
}

template<typename T>
void move_front(std::list<T> & l, T const & v) {
	auto pos = std::find(l.begin(), l.end(), v);
	if(pos != l.end()) {
		l.splice(l.begin(), l, pos);
	} else {
		l.push_front(v);
	}
}

template<typename T>
void move_back(std::list<T> & l, T const & v) {
	auto pos = std::find(l.begin(), l.end(), v);
	if(pos != l.end()) {
		l.splice(l.end(), l, pos);
	} else {
		l.push_back(v);
	}
}


static std::string format(char const * fmt, ...) {
	va_list l;
	va_start(l, fmt);
	int n = vsnprintf(nullptr, 0, fmt, l);
	va_end(l);
	std::string ret(n, '#');
	va_start(l, fmt);
	vsnprintf(&ret[0], n+1, fmt, l);
	va_end(l);
	return ret;
}

enum corner_mask_e : uint8_t {
	CAIRO_CORNER_TOPLEFT   = 0x01U,
	CAIRO_CORNER_TOPRIGHT  = 0x02U,
	CAIRO_CORNER_BOTLEFT   = 0x04U,
	CAIRO_CORNER_BOTRIGHT  = 0x08U,
	CAIRO_CORNER_ALL       = 0x0fU,
	CAIRO_CORNER_TOP       = 0x03U,
	CAIRO_CORNER_BOT       = 0x0cU,
	CAIRO_CORNER_LEFT      = 0x05U,
	CAIRO_CORNER_RIGHT     = 0x0aU
};

/**
 * Draw rectangle with all corner rounded
 **/
void cairo_rectangle_arc_corner(cairo_t * cr, double x, double y, double w, double h, double radius, uint8_t corner_mask);
void cairo_rectangle_arc_corner(cairo_t * cr, rect const & position, double radius, uint8_t corner_mask);






}


#endif /* UTILS_HXX_ */
