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

#include <cstdio>
#include <cstring>

#include <map>
#include <set>
#include <list>
#include <vector>
#include <algorithm>
#include <limits>
#include <string>
#include <stdexcept>

#include <X11/X.h>
#include <X11/Xutil.h>

#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xdbe.h>

#include "x11_func_name.hxx"
#include "key_desc.hxx"

using namespace std;

namespace page {


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
bool has_key(map<T, _> const & x, T const & key) {
	typename map<T, _>::const_iterator i = x.find(key);
	return i != x.end();
}

template<typename T>
bool has_key(set<T> const & x, T const & key) {
	typename set<T>::const_iterator i = x.find(key);
	return i != x.end();
}

template<typename T>
bool has_key(list<T> const & x, T const & key) {
	typename list<T>::const_iterator i = find(x.begin(), x.end(), key);
	return i != x.end();
}

template<typename K, typename V>
list<V> list_values(map<K, V> const & x) {
	list<V> ret;
	typename map<K, V>::const_iterator i = x.begin();
	while(i != x.end()) {
		ret.push_back(i->second);
		++i;
	}

	return ret;
}

template<typename _0, typename _1>
_1 get_safe_value(map<_0, _1> & x, _0 key, _1 def) {
	typename map<_0, _1>::iterator i = x.find(key);
	if (i != x.end())
		return i->second;
	else
		return def;
}

template<typename T> struct _format {
	static const int size = 0;
};

template<> struct _format<long> {
	static const int size = 32;
};

template<> struct _format<unsigned long> {
	static const int size = 32;
};

template<> struct _format<short> {
	static const int size = 16;
};

template<> struct _format<unsigned short> {
	static const int size = 16;
};

template<> struct _format<char> {
	static const int size = 8;
};

template<> struct _format<unsigned char> {
	static const int size = 8;
};


template<typename T>
void write_window_property(Display * dpy, Window win, Atom prop,
		Atom type, vector<T> & v) {
	XChangeProperty(dpy, win, prop, type, _format<T>::size,
	PropModeReplace, (unsigned char *) &v[0], v.size());
}


/** undocumented : http://lists.freedesktop.org/pipermail/xorg/2005-January/005954.html **/
inline void allow_input_passthrough(Display * dpy, Window w) {
	XserverRegion region = XFixesCreateRegion(dpy, NULL, 0);
	/**
	 * Shape for the entire of window.
	 **/
	XFixesSetWindowShapeRegion(dpy, w, ShapeBounding, 0, 0, None);
	/**
	 * input shape was introduced by Keith Packard to define an input area of
	 * window by default is the ShapeBounding which is used. here we set input
	 * area an empty region.
	 **/
	XFixesSetWindowShapeRegion(dpy, w, ShapeInput, 0, 0, region);
	XFixesDestroyRegion(dpy, region);
}

inline void disable_input_passthrough(Display * dpy, Window w) {
	XFixesSetWindowShapeRegion(dpy, w, ShapeBounding, 0, 0, None);
	XFixesSetWindowShapeRegion(dpy, w, ShapeInput, 0, 0, None);
}

static int error_handler(Display * dpy, XErrorEvent * ev) {
//	fprintf(stderr,"#%08lu ERROR, major_code: %u, minor_code: %u, error_code: %u\n",
//			ev->serial, ev->request_code, ev->minor_code, ev->error_code);
//
//	static const unsigned int XFUNCSIZE = (sizeof(x_function_codes)/sizeof(char *));
//
//	if (ev->request_code < XFUNCSIZE) {
//		char const * func_name = x_function_codes[ev->request_code];
//		char error_text[1024];
//		error_text[0] = 0;
//		XGetErrorText(dpy, ev->error_code, error_text, 1024);
//		fprintf(stderr, "#%08lu ERROR, %s : %s\n", ev->serial, func_name, error_text);
//	}
	return 0;
}

inline void compute_client_size_with_constraint(XSizeHints & size_hints,
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


inline struct timespec operator +(struct timespec const & a,
		struct timespec const & b) {
	struct timespec ret;
	ret.tv_nsec = (a.tv_nsec + b.tv_nsec) % 1000000000L;
	ret.tv_sec = (a.tv_sec + b.tv_sec)
			+ ((a.tv_nsec + b.tv_nsec) / 1000000000L);
	return ret;
}

inline bool operator <(struct timespec const & a, struct timespec const & b) {
	return (a.tv_sec < b.tv_sec)
			|| (a.tv_sec == b.tv_sec and a.tv_nsec < b.tv_nsec);
}

inline bool operator >(struct timespec const & a, struct timespec const & b) {
	return b < a;
}

inline struct timespec new_timespec(long sec, long nsec) {
	struct timespec ret;
	ret.tv_nsec = nsec % 1000000000L;
	ret.tv_sec = sec + (nsec / 1000000000L);
	return ret;
}

/** positive diff **/
inline struct timespec pdiff(struct timespec const & a,
		struct timespec const & b) {
	struct timespec ret;

	if(b < a) {
		ret.tv_nsec = (a.tv_nsec - b.tv_nsec);
		ret.tv_sec = (a.tv_sec - b.tv_sec);
		if(ret.tv_nsec < 0) {
			ret.tv_sec -= 1;
			ret.tv_nsec += 1000000000L;
		}
	} else {
		ret.tv_nsec = (b.tv_nsec - a.tv_nsec);
		ret.tv_sec = (b.tv_sec - a.tv_sec);
		if(ret.tv_nsec < 0) {
			ret.tv_sec -= 1;
			ret.tv_nsec += 1000000000L;
		}
	}

	return ret;
}



template<typename T>
vector<T> * get_window_property(Display * dpy, Window win, Atom prop, Atom type) {

	//printf("try read win = %lu, %lu(%lu)\n", win, prop, type);

	int status;
	Atom actual_type;
	int actual_format; /* can be 8, 16 or 32 */
	unsigned long int nitems;
	unsigned long int bytes_left;
	long int offset = 0L;

	vector<T> * buffer = new vector<T>();

	do {
		unsigned char * tbuff;

		status = XGetWindowProperty(dpy, win, prop, offset,
				std::numeric_limits<long int>::max(), False, type, &actual_type,
				&actual_format, &nitems, &bytes_left, &tbuff);

		if (status == Success) {

			if (actual_format == _format<T>::size && nitems > 0) {
				T * data = reinterpret_cast<T*>(tbuff);
				buffer->insert(buffer->end(), &data[0], &data[nitems]);
			} else {
				status = BadValue;
				XFree(tbuff);
				break;
			}

			if (bytes_left != 0) {
				printf("some bits lefts\n");
				//assert((nitems * actual_format % 32) == 0);
				//assert(bytes_left % (actual_format/4) == 0);
				offset += (nitems * actual_format) / 32;
			}

			XFree(tbuff);
		} else {
			break;
		}

	} while (bytes_left != 0);


	if (status == Success) {
		return buffer;
	} else {
		delete buffer;
		return 0;
	}
}

template<typename T> void safe_delete(T & p) {
	if (p != nullptr) {
		delete p;
		p = nullptr;
	}
}

template<typename T> T * safe_copy(T * p) {
	if (p != nullptr) {
		return new T(*p);
	} else {
		return nullptr;
	}
}

template<typename T>
list<T> * read_list(Display * dpy, Window w, Atom prop, Atom type) {
	vector<T> * tmp = get_window_property<T>(dpy, w, prop, type);
	if (tmp != 0) {
		list<T> * ret = new list<T>(tmp->begin(), tmp->end());
		delete tmp;
		return ret;
	} else {
		return 0;
	}
}

template<typename T>
T * read_value(Display * dpy, Window w, Atom prop, Atom type) {
	vector<T> * tmp = get_window_property<T>(dpy, w, prop, type);
	if (tmp != 0) {
		if (tmp->size() > 0) {
			T * ret = new T(tmp->front());
			delete tmp;
			return ret;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

inline string * read_text(Display * dpy, Window w, Atom prop, Atom type) {
	vector<char> * tmp = get_window_property<char>(dpy, w, prop, type);
	if (tmp != nullptr) {
		string * ret = new string(tmp->begin(), tmp->end());
		delete tmp;
		return ret;
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
list<T0 *> filter_class(list<T1 *> x) {
	list<T0 *> ret;
	for (auto i : x) {
		T0 * n = dynamic_cast<T0 *>(i);
		if (n != nullptr) {
			ret.push_back(n);
		}
	}
	return ret;
}

/**
 * Parse string like "mod4+f" to modifier mask (mod) and keysym (ks)
 **/
inline void find_key_from_string(string const desc, key_desc_t & k) {
	std::size_t pos = desc.find("+");
	if(pos == string::npos)
		throw std::runtime_error("key description does not match modifier+keysym");
	string modifier = desc.substr(0, pos);
	string key = desc.substr(pos+1);

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

}


#endif /* UTILS_HXX_ */
