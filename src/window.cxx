/*
 * window.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <cairo.h>
#include <cairo-xlib.h>

#include <iostream>
#include <sstream>
#include <cassert>
#include <stdint.h>

#include "window.hxx"
#include "xconnection.hxx"
#include "window_properties_handler.hxx"

namespace page {

long int const ClientEventMask = (StructureNotifyMask | PropertyChangeMask);

Atom window_t::A(char const * aname) {
	return _cnx.get_atom(aname);
}

window_t::window_t(xconnection_t &cnx, Window w) :
	_cnx(cnx), id(w)
{

	_wm_hints.value = XAllocWMHints();

	_lock_count = 0;

	/* store the ICCCM WM_STATE : WithDraw, Iconic or Normal */
	_wm_state.value = WithdrawnState;

	_is_managed = false;

	/* store if wm must do input focus */
	_wm_input_focus = true;
	_is_lock = false;

	_net_wm_desktop.value = 0;

	_wm_transient_for.value = None;

	_net_wm_user_time.value = 0;

    _above = None;

    XShapeSelectInput(_cnx.dpy, w, ShapeNotifyMask);

}

window_t::~window_t() {



	XFree(_wm_hints.value);

}

void window_t::read_wm_hints() {
	XFree(_wm_hints.value);
	_wm_hints.value = _cnx.get_wm_hints(id);
	if(_wm_hints.value != 0) {
		_wm_input_focus = (_wm_hints.value->input == True) ? true : false;
	}
}

void window_t::read_wm_normal_hints() {
	if(!_wm_normal_hints.is_durty)
		return;
	_wm_normal_hints.is_durty = false;

	long size_hints_flags;
	if (!XGetWMNormalHints(_cnx.dpy, id, &_wm_normal_hints.value, &size_hints_flags)) {
		_wm_normal_hints.value.flags = 0;
		_wm_normal_hints.has_value = false;
	} else {
		_wm_normal_hints.has_value = true;
	}
}


void window_t::read_wm_name() {
	if(!_wm_name.is_durty)
		return;
	_wm_name.is_durty = false;

	XTextProperty name;
	_cnx.get_text_property(id, &name, WM_NAME);
	if (name.nitems != 0) {
		_wm_name.has_value = true;
		_wm_name.value = (char const *) name.value;
	} else {
		_wm_name.has_value = false;
		_wm_name.value.clear();
	}
	XFree(name.value);
	return;
}

void window_t::read_net_wm_name() {
	if(!_net_wm_name.is_durty)
		return;
	_net_wm_name.is_durty = false;

	XTextProperty name;
	_cnx.get_text_property(id, &name, _NET_WM_NAME);
	if (name.nitems != 0) {
		_net_wm_name.has_value = true;
		_net_wm_name.value = (char const *) name.value;
	} else {
		_net_wm_name.has_value = false;
		_net_wm_name.value.clear();
	}
	XFree(name.value);
	return;
}


void window_t::read_net_wm_window_type() {
	if (!_net_wm_window_type.is_durty)
		return;
	_net_wm_window_type.is_durty = false;

	_net_wm_window_type.has_value = ::page::read_net_wm_window_type(_cnx.dpy, id,
			_net_wm_window_type.value);

}

void window_t::read_net_wm_state() {
	if (!_net_wm_state.is_durty)
		return;
	_net_wm_state.is_durty = false;

	_net_wm_state.has_value = ::page::read_net_wm_state(_cnx.dpy, id,
			_net_wm_state.value);
}



void window_t::read_net_wm_protocols() {
	if(!_net_wm_protocols.is_durty)
		return;
	_net_wm_protocols.is_durty = false;

	_net_wm_protocols.has_value = ::page::read_net_wm_protocols(_cnx.dpy, id,
			_net_wm_protocols.value);
}

void window_t::read_net_wm_partial_struct() {
	if(!_net_wm_partial_struct.is_durty)
		return;
	_net_wm_partial_struct.is_durty = false;

	_net_wm_partial_struct.has_value = ::page::read_net_wm_partial_struct(_cnx.dpy, id,
			_net_wm_partial_struct.value);
}

void window_t::read_net_wm_user_time() {
	if(!_net_wm_user_time.is_durty)
		return;
	_net_wm_user_time.is_durty = false;

	_net_wm_user_time.has_value = ::page::read_net_wm_user_time(_cnx.dpy, id,
			_net_wm_user_time.value);

}


void window_t::read_net_wm_desktop() {
	if(!_net_wm_desktop.is_durty)
		return;
	_net_wm_desktop.is_durty = false;

	_net_wm_desktop.has_value = ::page::read_net_wm_desktop(_cnx.dpy, id,
			_net_wm_desktop.value);

}

void window_t::read_net_wm_icon() {
	if (!_net_wm_icon.is_durty)
		return;
	_net_wm_icon.is_durty = false;

	_net_wm_icon.has_value = ::page::read_net_wm_icon(_cnx.dpy,
			id, _net_wm_icon.value);

}

void window_t::read_wm_state() {
	if(!_wm_state.is_durty)
		return;
	_wm_state.is_durty = false;

	int format;
	long * p = NULL;
	unsigned long n, extra;
	Atom real;

	_wm_state.has_value = false;
	_wm_state.value = WithdrawnState;

	if (_cnx.get_window_property(id, WM_STATE, 0L, 2L, False, WM_STATE, &real,
			&format, &n, &extra, (unsigned char **) &p) == Success) {
		if (n != 0 && p != NULL && format == 32
				&& real == _cnx.get_atom(WM_STATE)) {
			_wm_state.value = p[0];
			_wm_state.has_value = true;
		}
		XFree(p);
	}

}

void window_t::process_configure_notify_event(XConfigureEvent const & e) {
	assert(e.window == id);
	_window_attributes.value.x = e.x;
	_window_attributes.value.y = e.y;
	_window_attributes.value.width = e.width;
	_window_attributes.value.height = e.height;
	_window_attributes.value.border_width = e.border_width;
	_window_attributes.value.override_redirect = e.override_redirect;
	_above = e.above;

}

void window_t::read_wm_transient_for() {
	if(!_wm_transient_for.is_durty)
		return;
	_wm_transient_for.is_durty = false;

	_wm_transient_for.has_value = ::page::read_wm_transient_for(_cnx.dpy,
			id, _wm_transient_for.value);
}

void window_t::read_window_attributes() {
	if(!_window_attributes.is_durty)
		return;
	_window_attributes.is_durty = false;

	_window_attributes.has_value = (_cnx.get_window_attributes(id, &_window_attributes.value) != 0);
}

void window_t::read_shape_region() {
	if(!_shape_region.is_durty)
		return;
	_shape_region.is_durty = false;

	int count, ordering;
	XRectangle * recs = XShapeGetRectangles(_cnx.dpy, id, ShapeBounding, &count, &ordering);

	_shape_region.value.clear();

	if(recs != NULL) {
		_shape_region.has_value = true;
		for(int i = 0; i < count; ++i) {
			_shape_region.value = _shape_region.value + box_int_t(recs[i]);
		}
		/* In doubt */
		XFree(recs);
	} else {
		_shape_region.has_value = false;
		_shape_region.value.clear();
	}
}

void window_t::read_wm_class() {
	if(!_wm_class.is_durty)
		return;
	_wm_class.is_durty = false;

	XClassHint wm_class_hint;
	if(XGetClassHint(_cnx.dpy, id, &wm_class_hint) != 0) {
		_wm_class.has_value = true;
		_wm_class.value.res_class = wm_class_hint.res_class;
		_wm_class.value.res_name = wm_class_hint.res_name;
	} else {
		_wm_class.has_value = false;
		_wm_class.value.res_class.clear();
		_wm_class.value.res_name.clear();
	}
}


box_int_t window_t::get_size() {
	read_window_attributes();
	return box_int_t(_window_attributes.value.x, _window_attributes.value.y, _window_attributes.value.width, _window_attributes.value.height);
}

}

