/*
 * window_properties_handler.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */


#include <X11/X.h>
#include <X11/Xutil.h>

#include <limits>
#include <cstdio>

#include "window_properties_handler.hxx"
#include "atoms.hxx"

namespace page {



template<typename T> bool read_list(Display * dpy, Window w, char const * prop, char const * type, list<T> & list) {
	vector<long> tmp;
	if(get_window_property(dpy, w, prop, type, tmp)) {
		list.clear();
		list.insert(list.end(), tmp.begin(), tmp.end());
		return true;
	}
	return false;
}

template<typename T> bool read_value(Display * dpy, Window w, char const * prop,
		char const * type, T & value) {
	vector<long> tmp;
	if (get_window_property(dpy, w, prop, type, tmp)) {
		if (tmp.size() > 0) {
			value = tmp[0];
			return true;
		}
	}
	return false;
}

bool read_text(Display * dpy, Window w, char const * prop, string & value) {
	XTextProperty xname;
	XGetTextProperty(dpy, w, &xname, XInternAtom(dpy, prop, False));
	if (xname.nitems != 0) {
		value = (char *)xname.value;
		XFree(xname.value);
		return true;
	}
	return false;
}

bool read_wm_name(Display * dpy, Window w, string & name) {
	return read_text(dpy, w, WM_NAME, name);
}

bool read_net_wm_name(Display * dpy, Window w, string & name) {
	return read_text(dpy, w, _NET_WM_NAME, name);
}

bool read_net_wm_window_type(Display * dpy, Window w, list<Atom> & list) {
	bool ret = read_list(dpy, w, _NET_WM_WINDOW_TYPE, ATOM, list);
	printf("Atom read %lu\n", list.size());
	return ret;
}

bool read_net_wm_state(Display * dpy, Window w, list<Atom> & list) {
	return read_list(dpy, w, _NET_WM_STATE, ATOM, list);
}

bool read_net_wm_protocols(Display * dpy, Window w, list<Atom> & list) {
	return read_list(dpy, w, WM_PROTOCOLS, ATOM, list);
}

bool read_net_wm_partial_struct(Display * dpy, Window w, vector<long> & list) {
	return get_window_property(dpy, w, _NET_WM_STRUT_PARTIAL, CARDINAL, list);
}

bool read_net_wm_icon(Display * dpy, Window w, vector<long> & list) {
	return get_window_property(dpy, w, _NET_WM_ICON, CARDINAL, list);
}

bool read_net_wm_user_time(Display * dpy, Window w, long & value) {
	return read_value(dpy, w, _NET_WM_USER_TIME, CARDINAL, value);
}

bool read_net_wm_desktop(Display * dpy, Window w, long & value) {
	return read_value(dpy, w, _NET_WM_DESKTOP, CARDINAL, value);
}

bool read_wm_state(Display * dpy, Window w, long & value) {
	return read_value(dpy, w, WM_STATE, WM_STATE, value);
}

bool read_wm_transient_for(Display * dpy, Window w, Window & value) {
	return read_value(dpy, w, WM_TRANSIENT_FOR, WINDOW, value);
}

bool read_wm_class(Display * dpy, Window w, string & clss, string & name) {
	XClassHint wm_class_hint;
	if(XGetClassHint(dpy, w, &wm_class_hint) != 0) {
		clss = wm_class_hint.res_class;
		name = wm_class_hint.res_name;
		return true;
	}
	return false;
}

void write_net_wm_allowed_actions(Display * dpy, Window w, list<Atom> & list) {
	vector<long> v(list.begin(), list.end());
	write_window_property(dpy, w, _NET_WM_ALLOWED_ACTIONS, ATOM, v);
}

void write_net_wm_state(Display * dpy, Window w, list<Atom> & list) {
	vector<long> v(list.begin(), list.end());
	write_window_property(dpy, w, _NET_WM_STATE, ATOM, v);
}

void write_wm_state(Display * dpy, Window w, long state, Window icon) {
	vector<long> v(2);
	v[0] = state;
	v[1] = icon;
	write_window_property(dpy, w, WM_STATE, WM_STATE, v);
}

}


