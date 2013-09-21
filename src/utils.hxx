/*
 * utils.hxx
 *
 *  Created on: 4 août 2013
 *      Author: bg
 */

#ifndef UTILS_HXX_
#define UTILS_HXX_

#include <cstdio>

#include <map>
#include <set>
#include <list>
#include <vector>
#include <algorithm>
#include <limits>
#include <string>

#include <X11/X.h>
#include <X11/Xutil.h>

#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>

#include "x11_func_name.hxx"

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
bool get_window_property(Display * dpy, Window win, Atom prop,
		Atom type, vector<T> * v = 0) {

	//printf("try read win = %lu, %lu(%lu)\n", win, prop, type);

	int res;
	unsigned char * xdata = 0;
	Atom ret_type;
	int ret_size;
	unsigned long int ret_items, bytes_left;
	T * data;

	bool ret = false;

	res = XGetWindowProperty(dpy, win, prop, 0L,
			std::numeric_limits<long int>::max(), False, type, &ret_type,
			&ret_size, &ret_items, &bytes_left, &xdata);
	if (res == Success) {
		if (bytes_left != 0)
			printf("some bits lefts\n");
		if (ret_size == _format<T>::size && ret_items > 0) {
			data = reinterpret_cast<T*>(xdata);
			if (v != 0) {
				v->assign(&data[0], &data[ret_items]);
			}
			ret = true;
		}
	}

	XFree(xdata);

	return ret;
}

template<typename T>
bool get_window_property_void(Display * dpy, Window win, Atom prop, Atom type,
		unsigned long int * nitems, void ** data) {

	//printf("try read win = %lu, %lu(%lu)\n", win, prop, type);

	int res;
	unsigned char * xdata = 0;
	Atom ret_type;
	int ret_size;
	unsigned long int ret_items, bytes_left;

	bool ret = false;

	*nitems = 0;
	*data = 0;

	res = XGetWindowProperty(dpy, win, prop, 0L,
			std::numeric_limits<long int>::max(), False, type, &ret_type,
			&ret_size, &ret_items, &bytes_left, &xdata);
	if (res == Success) {
		if (bytes_left != 0)
			printf("some bits lefts\n");
		if (ret_size == _format<T>::size && ret_items > 0) {
			*nitems = ret_items;
			*data = reinterpret_cast<void*>(xdata);
			ret = true;
		}
	}

	return ret;
}

template<typename T>
void write_window_property(Display * dpy, Window win, Atom prop,
		Atom type, vector<T> & v) {
	XChangeProperty(dpy, win, prop, type, _format<T>::size,
	PropModeReplace, (unsigned char *) &v[0], v.size());
}

template<typename T> bool read_list(Display * dpy, Window w, Atom prop,
		Atom type, list<T> * list = 0) {
	if (list != 0) {
		vector<T> tmp;
		if (get_window_property<T>(dpy, w, prop, type, &tmp)) {
			list->clear();
			list->insert(list->end(), tmp.begin(), tmp.end());
			return true;
		}
		return false;
	} else {
		return get_window_property<T>(dpy, w, prop, type);
	}
}

template<typename T> bool read_value(Display * dpy, Window w, Atom prop,
		Atom type, T * value = 0) {
	if (value != 0) {
		vector<T> tmp;
		if (get_window_property<T>(dpy, w, prop, type, &tmp)) {
			if (tmp.size() > 0) {
				*value = tmp[0];
				return true;
			}
		}
		return false;
	} else {
		return get_window_property<T>(dpy, w, prop, type);
	}
}

inline bool read_text(Display * dpy, Window w, Atom prop, string & value) {
	XTextProperty xname;
	XGetTextProperty(dpy, w, &xname, prop);
	if (xname.nitems != 0) {
		value = (char *)xname.value;
		XFree(xname.value);
		return true;
	}
	return false;
}

/** undocumented : http://lists.freedesktop.org/pipermail/xorg/2005-January/005954.html **/
inline void allow_input_passthrough(Display * dpy, Window w) {
	XserverRegion region = XFixesCreateRegion(dpy, NULL, 0);
	/**
	 * Shape for the entire of window.
	 **/
	XFixesSetWindowShapeRegion(dpy, w, ShapeBounding, 0, 0, 0);
	/**
	 * input shape was introduced by Keith Packard to define an input area of
	 * window by default is the ShapeBounding which is used. here we set input
	 * area an empty region.
	 **/
	XFixesSetWindowShapeRegion(dpy, w, ShapeInput, 0, 0, region);
	XFixesDestroyRegion(dpy, region);
}

inline bool check_composite_extension(Display * dpy, int * opcode, int * event,
		int * error) {
	// Check if composite is supported.
	if (XQueryExtension(dpy, COMPOSITE_NAME, opcode, event, error)) {
		int major = 0, minor = 0; // The highest version we support
		XCompositeQueryVersion(dpy, &major, &minor);
		if (major != 0 || minor < 4) {
			return false;
		} else {
			printf("using composite %d.%d\n", major, minor);
			return true;
		}
	} else {
		return false;
	}
}

inline bool check_damage_extension(Display * dpy, int * opcode, int * event,
		int * error) {
	if (!XQueryExtension(dpy, DAMAGE_NAME, opcode, event, error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XDamageQueryVersion(dpy, &major, &minor);
		printf("Damage Extension version %d.%d found\n", major, minor);
		printf("Damage error %d, Damage event %d\n", *error, *event);
		return true;
	}
}

inline bool check_shape_extension(Display * dpy, int * opcode, int * event,
		int * error) {
	if (!XQueryExtension(dpy, SHAPENAME, opcode, event, error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XShapeQueryVersion(dpy, &major, &minor);
		printf("Shape Extension version %d.%d found\n", major, minor);
		return true;
	}
}

inline bool check_randr_extension(Display * dpy, int * opcode, int * event,
		int * error) {
	if (!XQueryExtension(dpy, RANDR_NAME, opcode, event, error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XRRQueryVersion(dpy, &major, &minor);
		printf(RANDR_NAME " Extension version %d.%d found\n", major, minor);
		return true;
	}
}

static int error_handler(Display * dpy, XErrorEvent * ev) {
	fprintf(stderr,"#%08lu ERROR, major_code: %u, minor_code: %u, error_code: %u\n",
			ev->serial, ev->request_code, ev->minor_code, ev->error_code);

	static const unsigned int XFUNCSIZE = (sizeof(x_function_codes)/sizeof(char *));

	if (ev->request_code < XFUNCSIZE) {
		char const * func_name = x_function_codes[ev->request_code];
		char error_text[1024];
		error_text[0] = 0;
		XGetErrorText(dpy, ev->error_code, error_text, 1024);
		fprintf(stderr, "#%08lu ERROR, %s : %s\n", ev->serial, func_name, error_text);
	}
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

}


#endif /* UTILS_HXX_ */
