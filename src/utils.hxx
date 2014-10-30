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

#include <cairo/cairo.h>

#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <memory>

#include "box.hxx"
#include "key_desc.hxx"
#include "x11_func_name.hxx"

namespace page {

#define warn(test) \
	do { \
		if(not (test)) { \
			printf("WARN %s:%d (%s) fail!\n", __FILE__, __LINE__, #test); \
		} \
	} while(false)

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

//template<typename T> struct _format {
//	static const int size = 0;
//};
//
//template<> struct _format<long> {
//	static const int size = 32;
//};
//
//template<> struct _format<unsigned long> {
//	static const int size = 32;
//};
//
//template<> struct _format<short> {
//	static const int size = 16;
//};
//
//template<> struct _format<unsigned short> {
//	static const int size = 16;
//};
//
//template<> struct _format<char> {
//	static const int size = 8;
//};
//
//template<> struct _format<unsigned char> {
//	static const int size = 8;
//};


//template<typename T>
//void write_window_property(Display * dpy, Window win, Atom prop,
//		Atom type, std::vector<T> & v) {
//	XChangeProperty(dpy, win, prop, type, _format<T>::size,
//	PropModeReplace, (unsigned char *) &v[0], v.size());
//}



inline void compute_client_size_with_constraint(XSizeHints const & size_hints,
		unsigned int wished_width, unsigned int wished_height, unsigned int & width,
		unsigned int & height) {

	/* default size if no size_hints is provided */
	width = wished_width;
	height = wished_height;

	if (size_hints.flags & PMaxSize) {
		if ((int)wished_width > size_hints.max_width)
			wished_width = size_hints.max_width;
		if ((int)wished_height > size_hints.max_height)
			wished_height = size_hints.max_height;
	}

	if (size_hints.flags & PBaseSize) {
		if ((int)wished_width < size_hints.base_width)
			wished_width = size_hints.base_width;
		if ((int)wished_height < size_hints.base_height)
			wished_height = size_hints.base_height;
	} else if (size_hints.flags & PMinSize) {
		if ((int)wished_width < size_hints.min_width)
			wished_width = size_hints.min_width;
		if ((int)wished_height < size_hints.min_height)
			wished_height = size_hints.min_height;
	}

	if (size_hints.flags & PAspect) {
		if (size_hints.flags & PBaseSize) {
			/**
			 * ICCCM say if base is set subtract base before aspect checking
			 * reference: ICCCM
			 **/
			if ((wished_width - size_hints.base_width) * size_hints.min_aspect.y
					< (wished_height - size_hints.base_height)
							* size_hints.min_aspect.x) {
				/* reduce h */
				wished_height = size_hints.base_height
						+ ((wished_width - size_hints.base_width)
								* size_hints.min_aspect.y)
								/ size_hints.min_aspect.x;

			} else if ((wished_width - size_hints.base_width)
					* size_hints.max_aspect.y
					> (wished_height - size_hints.base_height)
							* size_hints.max_aspect.x) {
				/* reduce w */
				wished_width = size_hints.base_width
						+ ((wished_height - size_hints.base_height)
								* size_hints.max_aspect.x)
								/ size_hints.max_aspect.y;
			}
		} else {
			if (wished_width * size_hints.min_aspect.y
					< wished_height * size_hints.min_aspect.x) {
				/* reduce h */
				wished_height = (wished_width * size_hints.min_aspect.y)
						/ size_hints.min_aspect.x;

			} else if (wished_width * size_hints.max_aspect.y
					> wished_height * size_hints.max_aspect.x) {
				/* reduce w */
				wished_width = (wished_height * size_hints.max_aspect.x)
						/ size_hints.max_aspect.y;
			}
		}

	}

	if (size_hints.flags & PResizeInc) {
		wished_width -=
				((wished_width - size_hints.base_width) % size_hints.width_inc);
		wished_height -= ((wished_height - size_hints.base_height)
				% size_hints.height_inc);
	}

	width = wished_width;
	height = wished_height;

}


//inline struct timespec operator +(struct timespec const & a,
//		struct timespec const & b) {
//	struct timespec ret;
//	ret.tv_nsec = (a.tv_nsec + b.tv_nsec) % 1000000000L;
//	ret.tv_sec = (a.tv_sec + b.tv_sec)
//			+ ((a.tv_nsec + b.tv_nsec) / 1000000000L);
//	return ret;
//}
//
//inline bool operator <(struct timespec const & a, struct timespec const & b) {
//	return (a.tv_sec < b.tv_sec)
//			|| (a.tv_sec == b.tv_sec and a.tv_nsec < b.tv_nsec);
//}
//
//inline bool operator >(struct timespec const & a, struct timespec const & b) {
//	return b < a;
//}
//
//inline struct timespec new_timespec(long sec, long nsec) {
//	struct timespec ret;
//	ret.tv_nsec = nsec % 1000000000L;
//	ret.tv_sec = sec + (nsec / 1000000000L);
//	return ret;
//}
//
///** positive diff **/
//inline struct timespec pdiff(struct timespec const & a,
//		struct timespec const & b) {
//	struct timespec ret;
//
//	if(b < a) {
//		ret.tv_nsec = (a.tv_nsec - b.tv_nsec);
//		ret.tv_sec = (a.tv_sec - b.tv_sec);
//		if(ret.tv_nsec < 0) {
//			ret.tv_sec -= 1;
//			ret.tv_nsec += 1000000000L;
//		}
//	} else {
//		ret.tv_nsec = (b.tv_nsec - a.tv_nsec);
//		ret.tv_sec = (b.tv_sec - a.tv_sec);
//		if(ret.tv_nsec < 0) {
//			ret.tv_sec -= 1;
//			ret.tv_nsec += 1000000000L;
//		}
//	}
//
//	return ret;
//}



//template<typename T>
//std::vector<T> * get_window_property(Display * dpy, Window win, Atom prop, Atom type) {
//
//	//printf("try read win = %lu, %lu(%lu)\n", win, prop, type);
//
//	int status;
//	Atom actual_type;
//	int actual_format; /* can be 8, 16 or 32 */
//	unsigned long int nitems;
//	unsigned long int bytes_left;
//	long int offset = 0L;
//
//	std::vector<T> * buffer = new std::vector<T>();
//
//	do {
//		unsigned char * tbuff;
//
//		status = XGetWindowProperty(dpy, win, prop, offset,
//				std::numeric_limits<long int>::max(), False, type, &actual_type,
//				&actual_format, &nitems, &bytes_left, &tbuff);
//
//		if (status == Success) {
//
//			if (actual_format == _format<T>::size && nitems > 0) {
//				T * data = reinterpret_cast<T*>(tbuff);
//				buffer->insert(buffer->end(), &data[0], &data[nitems]);
//			} else {
//				status = BadValue;
//				XFree(tbuff);
//				break;
//			}
//
//			if (bytes_left != 0) {
//				printf("some bits lefts\n");
//				//assert((nitems * actual_format % 32) == 0);
//				//assert(bytes_left % (actual_format/4) == 0);
//				offset += (nitems * actual_format) / 32;
//			}
//
//			XFree(tbuff);
//		} else {
//			break;
//		}
//
//	} while (bytes_left != 0);
//
//
//	if (status == Success) {
//		return buffer;
//	} else {
//		delete buffer;
//		return 0;
//	}
//}

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

//template<typename T>
//std::list<T> * read_list(Display * dpy, Window w, Atom prop, Atom type) {
//	std::vector<T> * tmp = get_window_property<T>(dpy, w, prop, type);
//	if (tmp != 0) {
//		std::list<T> * ret = new std::list<T>(tmp->begin(), tmp->end());
//		delete tmp;
//		return ret;
//	} else {
//		return 0;
//	}
//}
//
//template<typename T>
//T * read_value(Display * dpy, Window w, Atom prop, Atom type) {
//	std::vector<T> * tmp = get_window_property<T>(dpy, w, prop, type);
//	if (tmp != 0) {
//		if (tmp->size() > 0) {
//			T * ret = new T(tmp->front());
//			delete tmp;
//			return ret;
//		} else {
//			return 0;
//		}
//	} else {
//		return 0;
//	}
//}

//inline std::string * read_text(Display * dpy, Window w, Atom prop, Atom type) {
//	std::vector<char> * tmp = get_window_property<char>(dpy, w, prop, type);
//	if (tmp != nullptr) {
//		std::string * ret = new std::string(tmp->begin(), tmp->end());
//		delete tmp;
//		return ret;
//	} else {
//		return nullptr;
//	}
//}

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
std::list<T0 *> filter_class(std::list<T1 *> const & x) {
	std::list<T0 *> ret;
	for (auto i : x) {
		T0 * n = dynamic_cast<T0 *>(i);
		if (n != nullptr) {
			ret.push_back(n);
		}
	}
	return ret;
}

template<typename T0, typename T1>
std::vector<T0 *> filter_class(std::vector<T1 *> const & x) {
	std::vector<T0 *> ret;
	for (auto i : x) {
		T0 * n = dynamic_cast<T0 *>(i);
		if (n != nullptr) {
			ret.push_back(n);
		}
	}
	return ret;
}

/**
 * Parse std::string like "mod4+f" to modifier mask (mod) and keysym (ks)
 **/
inline void find_key_from_string(std::string const desc, key_desc_t & k) {
	std::size_t pos = desc.find("+");
	if(pos == std::string::npos)
		throw std::runtime_error("key description does not match modifier+keysym");
	std::string modifier = desc.substr(0, pos);
	std::string key = desc.substr(pos+1);

	if(modifier == "shift") {
		k.mod = ShiftMask;
	} else if (modifier == "lock") {
		k.mod = LockMask;
	} else if (modifier == "control") {
		k.mod = ControlMask;
	} else if (modifier == "mod1") {
		k.mod = Mod1Mask;
	} else if (modifier == "mod2") {
		k.mod = Mod2Mask;
	} else if (modifier == "mod3") {
		k.mod = Mod3Mask;
	} else if (modifier == "mod4") {
		k.mod = Mod4Mask;
	} else if (modifier == "mod5") {
		k.mod = Mod5Mask;
	}

	k.ks = XStringToKeysym(key.c_str());
	if(k.ks == NoSymbol) {
		throw std::runtime_error("key binding not found");
	}
}

static void draw_outer_graddien(cairo_t * cr, i_rect r, double _shadow_width) {

	cairo_save(cr);

	/** draw left shawdow **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr,r.x - _shadow_width, r.y, _shadow_width, r.h);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * grad0 = cairo_pattern_create_linear(r.x - _shadow_width, 0.0, r.x, 0.0);
	cairo_pattern_add_color_stop_rgba(grad0, 0.0, 0.0, 0.0, 0.0, 0.0);
	cairo_pattern_add_color_stop_rgba(grad0, 1.0, 0.0, 0.0, 0.0, 0.2);
	cairo_mask(cr, grad0);
	cairo_pattern_destroy(grad0);

	/** draw right shadow **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x + r.w, r.y, _shadow_width, r.h);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * grad1 = cairo_pattern_create_linear(r.x + r.w, 0.0, r.x + r.w + _shadow_width, 0.0);
	cairo_pattern_add_color_stop_rgba(grad1, 1.0, 0.0, 0.0, 0.0, 0.0);
	cairo_pattern_add_color_stop_rgba(grad1, 0.0, 0.0, 0.0, 0.0, 0.2);
	cairo_mask(cr, grad1);
	cairo_pattern_destroy(grad1);

	/** draw top shadow **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x, r.y - _shadow_width, r.w, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * grad2 = cairo_pattern_create_linear(0.0, r.y - _shadow_width, 0.0, r.y);
	cairo_pattern_add_color_stop_rgba(grad2, 0.0, 0.0, 0.0, 0.0, 0.0);
	cairo_pattern_add_color_stop_rgba(grad2, 1.0, 0.0, 0.0, 0.0, 0.2);
	cairo_mask(cr, grad2);
	cairo_pattern_destroy(grad2);

	/** draw bottom shadow **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x, r.y + r.h, r.w, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * grad3 = cairo_pattern_create_linear(0.0, r.y + r.h, 0.0, r.y + r.h + _shadow_width);
	cairo_pattern_add_color_stop_rgba(grad3, 1.0, 0.0, 0.0, 0.0, 0.0);
	cairo_pattern_add_color_stop_rgba(grad3, 0.0, 0.0, 0.0, 0.0, 0.2);
	cairo_mask(cr, grad3);
	cairo_pattern_destroy(grad3);


	/** draw top-left corner **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x - _shadow_width, r.y - _shadow_width, _shadow_width, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * r0grad = cairo_pattern_create_radial(r.x, r.y, 0.0, r.x, r.y, _shadow_width);
	cairo_pattern_add_color_stop_rgba(r0grad, 0.0, 0.0, 0.0, 0.0, 0.2);
	cairo_pattern_add_color_stop_rgba(r0grad, 1.0, 0.0, 0.0, 0.0, 0.0);
	cairo_mask(cr, r0grad);
	cairo_pattern_destroy(r0grad);

	/** draw top-right corner **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x + r.w, r.y - _shadow_width, _shadow_width, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * r1grad = cairo_pattern_create_radial(r.x + r.w, r.y, 0.0, r.x + r.w, r.y, _shadow_width);
	cairo_pattern_add_color_stop_rgba(r1grad, 0.0, 0.0, 0.0, 0.0, 0.2);
	cairo_pattern_add_color_stop_rgba(r1grad, 1.0, 0.0, 0.0, 0.0, 0.0);
	cairo_mask(cr, r1grad);
	cairo_pattern_destroy(r1grad);

	/** bottom-left corner **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x - _shadow_width, r.y + r.h, _shadow_width, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * r2grad = cairo_pattern_create_radial(r.x, r.y + r.h, 0.0, r.x, r.y + r.h, _shadow_width);
	cairo_pattern_add_color_stop_rgba(r2grad, 0.0, 0.0, 0.0, 0.0, 0.2);
	cairo_pattern_add_color_stop_rgba(r2grad, 1.0, 0.0, 0.0, 0.0, 0.0);
	cairo_mask(cr, r2grad);
	cairo_pattern_destroy(r2grad);

	/** draw bottom-right corner **/
	cairo_reset_clip(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle(cr, r.x + r.w, r.y + r.h, _shadow_width, _shadow_width);
	cairo_clip(cr);
	::cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_pattern_t * r3grad = cairo_pattern_create_radial(r.x + r.w, r.y + r.h, 0.0, r.x + r.w, r.y + r.h, _shadow_width);
	cairo_pattern_add_color_stop_rgba(r3grad, 0.0, 0.0, 0.0, 0.0, 0.2);
	cairo_pattern_add_color_stop_rgba(r3grad, 1.0, 0.0, 0.0, 0.0, 0.0);
	cairo_mask(cr, r3grad);
	cairo_pattern_destroy(r3grad);

	cairo_restore(cr);

}

static void draw_outer_graddien2(cairo_t * cr, i_rect r, double _shadow_width, double radius) {

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

inline void cairo_clip(cairo_t * cr, i_rect const & clip) {
	cairo_reset_clip(cr);
	cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
	cairo_clip(cr);
}


}


#endif /* UTILS_HXX_ */
