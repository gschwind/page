/*
 * window_properties_handler.hxx
 *
 *  Created on: 3 août 2013
 *      Author: bg
 */

#ifndef WINDOW_PROPERTIES_HANDLER_HXX_
#define WINDOW_PROPERTIES_HANDLER_HXX_

#include <X11/X.h>
#include <vector>
#include <list>
#include <string>

using namespace std;

namespace page {

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
bool get_window_property(Display * dpy, Window win, char const * prop,
		char const * type, vector<T> & v) {

	printf("try read win = %lu, %s(%s)\n", win, prop, type);

	int res;
	unsigned char * xdata = 0;
	Atom ret_type;
	int ret_size;
	unsigned long int ret_items, bytes_left;
	T * data;

	bool ret = false;

	Atom aprop = XInternAtom(dpy, prop, False);
	Atom atype;

	if (type == NULL) {
		atype = AnyPropertyType;
	} else {
		atype = XInternAtom(dpy, type, False);
	}

	res = XGetWindowProperty(dpy, win, aprop, 0L,
			std::numeric_limits<long int>::max(), False, atype, &ret_type,
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
void write_window_property(Display * dpy, Window win, char const * prop,
		char const * type, vector<T> & v) {

	printf("try write win = %lu, %s(%s)\n", win, prop, type);
	fflush(stdout);

	Atom aprop = XInternAtom(dpy, prop, False);
	Atom atype = XInternAtom(dpy, type, False);

	XChangeProperty(dpy, win, aprop, atype, _format<T>::size,
	PropModeReplace, (unsigned char *) &v[0], v.size());

}

bool read_wm_name(Display * dpy, Window w, string & name);
bool read_net_wm_name(Display * dpy, Window w, string & name);
bool read_net_wm_window_type(Display * dpy, Window w, list<Atom> & list);
bool read_net_wm_state(Display * dpy, Window w, list<Atom> & list);
bool read_net_wm_protocols(Display * dpy, Window w, list<Atom> & list);
bool read_net_wm_partial_struct(Display * dpy, Window w, vector<long> & list);
bool read_net_wm_icon(Display * dpy, Window w, vector<long> & list);
bool read_net_wm_user_time(Display * dpy, Window w, long & value);
bool read_net_wm_desktop(Display * dpy, Window w, long & value);
bool read_wm_state(Display * dpy, Window w, long & value);
bool read_wm_transient_for(Display * dpy, Window w, Window & value);
bool read_wm_class(Display * dpy, Window w, string & clss, string & name);

void write_net_wm_allowed_actions(Display * dpy, Window w, list<Atom> & list);
void write_net_wm_state(Display * dpy, Window w, list<Atom> & list);
void write_wm_state(Display * dpy, Window w, long state, Window icon);

}


#endif /* WINDOW_PROPERTIES_HANDLER_HXX_ */
