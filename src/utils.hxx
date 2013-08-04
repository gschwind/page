/*
 * utils.hxx
 *
 *  Created on: 4 août 2013
 *      Author: bg
 */

#ifndef UTILS_HXX_
#define UTILS_HXX_

#include <X11/X.h>
#include <X11/Xutil.h>

#include <map>
#include <set>
#include <list>
#include <vector>
#include <algorithm>

using namespace std;

namespace page {

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

template<> struct _format<short> {
	static const int size = 16;
};

template<> struct _format<char> {
	static const int size = 8;
};

template<typename T>
bool get_window_property(Display * dpy, Window win, Atom prop,
		Atom type, vector<T> & v) {

	printf("try read win = %lu, %lu(%lu)\n", win, prop, type);

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
			v.clear();
			data = reinterpret_cast<T*>(xdata);
			v.assign(&data[0], &data[ret_items]);
			ret = true;
		}
		XFree(xdata);
	}

	return ret;
}

template<typename T>
bool get_window_property_void(Display * dpy, Window win, Atom prop, Atom type,
		unsigned long int * nitems, void ** data) {

	printf("try read win = %lu, %lu(%lu)\n", win, prop, type);

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

	printf("try write win = %lu, %lu(%lu)\n", win, prop, type);
	fflush(stdout);

	XChangeProperty(dpy, win, prop, type, _format<T>::size,
	PropModeReplace, (unsigned char *) &v[0], v.size());

}

template<typename T> bool read_list(Display * dpy, Window w, Atom prop,
		Atom type, list<T> & list) {
	vector<long> tmp;
	if (get_window_property(dpy, w, prop, type, tmp)) {
		list.clear();
		list.insert(list.end(), tmp.begin(), tmp.end());
		return true;
	}
	return false;
}

template<typename T> bool read_value(Display * dpy, Window w, Atom prop,
		Atom type, T & value) {
	vector<long> tmp;
	if (get_window_property(dpy, w, prop, type, tmp)) {
		if (tmp.size() > 0) {
			value = tmp[0];
			return true;
		}
	}
	return false;
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



}


#endif /* UTILS_HXX_ */
