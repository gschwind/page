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

namespace page {

long int const ClientEventMask = (StructureNotifyMask | PropertyChangeMask);

window_t::window_t(xconnection_t &cnx, Window w) :
	_cnx(cnx), _xwin(w)
{

	_wm_normal_hints = XAllocSizeHints();
	_wm_hints = XAllocWMHints();

	_lock_count = 0;

	/* store the ICCCM WM_STATE : WithDraw, Iconic or Normal */
	_wm_state = WithdrawnState;

	_net_wm_type = atom_list_t();
	_net_wm_state = atom_set_t();
	_net_wm_protocols = atom_set_t();
	_net_wm_allowed_actions = atom_set_t();

	_is_managed = false;

	_has_wm_name = false;
	_has_net_wm_name = false;
	_has_partial_struct = false;
	/* store if wm must do input focus */
	_wm_input_focus = true;
	_is_lock = false;

	_has_wm_normal_hints = false;
	_has_transient_for = false;
	_has_wm_state = false;
	_has_net_wm_desktop = false;

	_net_wm_desktop = 0;

	/* the name of window */
	_wm_name = "";
	_net_wm_name = "";

	memset(_partial_struct, 0, sizeof(_partial_struct));

	_transient_for = None;

	_user_time = 0;

	_net_wm_icon_size = 0;
	_net_wm_icon_data = 0;


	memset(&_wa, 0, sizeof(XWindowAttributes));

    _above = None;

    _type = PAGE_NORMAL_WINDOW_TYPE;


}

box_int_t window_t::get_size() {
	return box_int_t(_wa.x, _wa.y, _wa.width, _wa.height);
}

void window_t::write_net_frame_extents() {
	/* set border */
	_cnx.set_window_border_width(_xwin, 0);

	/* set frame extend to 0 (I don't know why a client need this data) */
	long frame_extends[4] = { 0, 0, 0, 0 };
	_cnx.change_property(_xwin, _cnx.atoms._NET_FRAME_EXTENTS,
			_cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(frame_extends), 4);

}

window_t::~window_t() {

	XFree(_wm_normal_hints);
	XFree(_wm_hints);

}

void window_t::read_wm_hints() {
	XFree(_wm_hints);
	_wm_hints = _cnx.get_wm_hints(_xwin);
}


void window_t::read_wm_normal_hints() {
	_has_wm_normal_hints = true;

	long size_hints_flags;
	if (!XGetWMNormalHints(_cnx.dpy, _xwin, _wm_normal_hints, &size_hints_flags)) {
		/* size is uninitialized, ensure that size.flags aren't used */
		_wm_normal_hints->flags = 0;
		_has_wm_normal_hints = false;
		//printf("no WMNormalHints\n");
	}
	//printf("x: %d y: %d w: %d h: %d\n", wm_normal_hints->x, wm_normal_hints->y, wm_normal_hints->width, wm_normal_hints->height);
	if(_wm_normal_hints->flags & PMaxSize) {
		//printf("max w: %d max h: %d \n", _wm_normal_hints->max_width, _wm_normal_hints->max_height);
	}

	if(_wm_normal_hints->flags & PMinSize) {
		//printf("min w: %d min h: %d \n", _wm_normal_hints->min_width, _wm_normal_hints->min_height);
	}

}


void window_t::read_vm_name() {
	_has_wm_name = false;
	XTextProperty name;
	_cnx.get_text_property(_xwin, &name, _cnx.atoms.WM_NAME);
	if (!name.nitems) {
		XFree(name.value);
		_wm_name = "";
		return;
	}
	_has_wm_name = true;
	_wm_name = (char const *) name.value;
	XFree(name.value);
	return;
}

void window_t::read_net_vm_name() {
	_has_net_wm_name = false;
	XTextProperty name;
	_cnx.get_text_property(_xwin, &name, _cnx.atoms._NET_WM_NAME);
	if (!name.nitems) {
		XFree(name.value);
		_net_wm_name = "";
		return;
	}
	_has_net_wm_name = true;
	_net_wm_name = (char const *) name.value;
	XFree(name.value);
	return;
}


void window_t::read_net_wm_type() {
	unsigned int num;
	long * val = get_properties32(_cnx.atoms._NET_WM_WINDOW_TYPE, _cnx.atoms.ATOM,
			&num);
	if (val) {
		_net_wm_type.clear();
		/* use the first value that we know about in the array */
		for (unsigned i = 0; i < num; ++i) {
			_net_wm_type.push_back(val[i]);
		}
		delete[] val;
	}

	//print_net_wm_window_type();

	_type = find_window_type(_cnx, _xwin, _wa.c_class);

}

void window_t::read_net_wm_state() {
	/* take _NET_WM_STATE */
	unsigned int n;
	long * net_wm_state = get_properties32(_cnx.atoms._NET_WM_STATE,
			_cnx.atoms.ATOM, &n);
	if (net_wm_state) {
		this->_net_wm_state.clear();
		for (unsigned int i = 0; i < n; ++i) {
			this->_net_wm_state.insert(net_wm_state[i]);
		}
		delete[] net_wm_state;
	}

	//print_net_wm_state();
}



void window_t::read_net_wm_protocols() {
	_net_wm_protocols.clear();
	int count;
	Atom * atoms_list;
	if (XGetWMProtocols(_cnx.dpy, _xwin, &atoms_list, &count)) {
		for (int i = 0; i < count; ++i) {
			_net_wm_protocols.insert(atoms_list[i]);
		}
		XFree(atoms_list);
	}
}

void window_t::read_partial_struct() {
	unsigned int n;
	long * p_struct = get_properties32(_cnx.atoms._NET_WM_STRUT_PARTIAL,
			_cnx.atoms.CARDINAL, &n);

	if (p_struct && n == 12) {
//		printf(
//				"partial struct %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n",
//				partial_struct[0], partial_struct[1], partial_struct[2],
//				partial_struct[3], partial_struct[4], partial_struct[5],
//				partial_struct[6], partial_struct[7], partial_struct[8],
//				partial_struct[9], partial_struct[10], partial_struct[11]);

		_has_partial_struct = true;
		memcpy(_partial_struct, p_struct, sizeof(long) * 12);
	} else {
		_has_partial_struct = false;
	}

	if(p_struct != 0)
		delete[] p_struct;

}

void window_t::read_net_wm_user_time() {
	unsigned int n;
	long * time = get_properties32(_cnx.atoms._NET_WM_USER_TIME, _cnx.atoms.CARDINAL, &n);
	if(time) {
		_user_time = time[0];
	} else {
		_user_time = 0;
	}
}


void window_t::read_net_wm_desktop() {
	unsigned int n;
	long * desktop = get_properties32(_cnx.atoms._NET_WM_DESKTOP, _cnx.atoms.CARDINAL, &n);
	if(desktop) {
		_has_net_wm_desktop = true;
		_net_wm_desktop = desktop[0];
	} else {
		_has_net_wm_desktop = false;
		_net_wm_desktop = 0;
	}
}


void window_t::set_focused() {
	_net_wm_state.insert(_cnx.atoms._NET_WM_STATE_FOCUSED);
	write_net_wm_state();
}

void window_t::unset_focused() {
	_net_wm_state.erase(_cnx.atoms._NET_WM_STATE_FOCUSED);
	write_net_wm_state();
}

void window_t::set_default_action() {
	_net_wm_allowed_actions.clear();
	_net_wm_allowed_actions.insert(_cnx.atoms._NET_WM_ACTION_CLOSE);
	_net_wm_allowed_actions.insert(_cnx.atoms._NET_WM_ACTION_FULLSCREEN);
	write_net_wm_allowed_actions();
}

void window_t::set_dock_action() {
	_net_wm_allowed_actions.clear();
	_net_wm_allowed_actions.insert(_cnx.atoms._NET_WM_ACTION_CLOSE);
	write_net_wm_allowed_actions();
}


void window_t::select_input(long int mask) {
	_cnx.select_input(_xwin, mask);
}

void window_t::print_net_wm_window_type() {
	unsigned int num;
	long * val = get_properties32(_cnx.atoms._NET_WM_WINDOW_TYPE,
			_cnx.atoms.ATOM, &num);
	if (val) {
		/* use the first value that we know about in the array */
		for (unsigned i = 0; i < num; ++i) {
			char * name = XGetAtomName(_cnx.dpy, val[i]);
			//printf("_NET_WM_WINDOW_TYPE = \"%s\"\n", name);
			XFree(name);
		}
		delete[] val;
	}
}

void window_t::print_net_wm_state() {
	unsigned int num;
	long * val = get_properties32(_cnx.atoms._NET_WM_STATE,
			_cnx.atoms.ATOM, &num);
	printf("_NET_WM_STATE =");
	if (val) {
		/* use the first value that we know about in the array */
		for (unsigned i = 0; i < num; ++i) {
			char * name = XGetAtomName(_cnx.dpy, val[i]);
			printf(" \"%s\"", name);
			XFree(name);
		}
		delete[] val;
	}
	printf("\n");
}

bool window_t::is_input_only() {
	return _wa.c_class == InputOnly;
}

void window_t::add_to_save_set() {
	_cnx.add_to_save_set(_xwin);
}

void window_t::remove_from_save_set() {
	_cnx.remove_from_save_set(_xwin);
}

bool window_t::is_visible() {
	return (is_map() && !is_input_only());
}

long window_t::get_net_user_time() {
	return _user_time;
}

bool window_t::override_redirect() {
	return (_wa.override_redirect == True) ? true : false;
}

Window window_t::transient_for() {
	return _transient_for;
}

void window_t::read_icon_data() {
	if(_net_wm_icon_data != 0)
		delete[] _net_wm_icon_data;
	_net_wm_icon_data = get_properties32(_cnx.atoms._NET_WM_ICON, _cnx.atoms.CARDINAL, &_net_wm_icon_size);

}

void window_t::read_all() {
	_wm_input_focus = true;
	read_wm_hints();
	read_net_vm_name();
	read_vm_name();
	read_wm_normal_hints();
	read_net_wm_type();
	read_net_wm_state();
	read_net_wm_protocols();
	read_net_wm_user_time();
	read_wm_state();
	read_transient_for();
	read_icon_data();
	read_partial_struct();
	read_net_wm_desktop();
	read_window_attributes();
}

void window_t::read_when_mapped() {
	read_window_attributes();
	_wm_input_focus = true;
	read_wm_hints();
	read_wm_normal_hints();
	read_vm_name();
	read_transient_for();
	read_net_wm_protocols();
	read_net_vm_name();
	read_net_wm_type();
	read_net_wm_state();
	read_net_wm_user_time();
	read_icon_data();
	read_partial_struct();
	read_net_wm_desktop();
}

void window_t::get_icon_data(long const *& data, int & size) {
	data = _net_wm_icon_data;
	size = _net_wm_icon_size;
}

void window_t::read_wm_state() {
	int format;
	long * p = NULL;
	unsigned long n, extra;
	Atom real;

	_has_wm_state = false;
	_wm_state = WithdrawnState;

	if (_cnx.get_window_property(_xwin, _cnx.atoms.WM_STATE, 0L, 2L, False,
			_cnx.atoms.WM_STATE, &real, &format, &n, &extra,
			(unsigned char **) &p) == Success) {
		if (n != 0 && format == 32 && real == _cnx.atoms.WM_STATE) {
			_wm_state = p[0];
			_has_wm_state = true;
		}
		XFree(p);
	}

}

void window_t::map() {
	_wa.map_state = IsViewable;
	_cnx.map_window(_xwin);
}

void window_t::unmap() {
	_cnx.unmap(_xwin);
}

/* check if client is still alive */
bool window_t::try_lock() {
	_cnx.grab();
	XEvent e;
	if (XCheckTypedWindowEvent(_cnx.dpy, _xwin, DestroyNotify, &e)) {
		XPutBackEvent(_cnx.dpy, &e);
		_cnx.ungrab();
		return false;
	}
	_is_lock = true;
	return true;
}

void window_t::unlock() {
	_is_lock = false;
	_cnx.ungrab();
}

void window_t::focus() {

	if (is_map() && _wm_input_focus) {
		_cnx.set_input_focus(_xwin, RevertToParent, _cnx.last_know_time);
	}

	if (_net_wm_protocols.find(_cnx.atoms.WM_TAKE_FOCUS)
			!= _net_wm_protocols.end()) {
		XEvent ev;
		ev.xclient.display = _cnx.dpy;
		ev.xclient.type = ClientMessage;
		ev.xclient.format = 32;
		ev.xclient.message_type = _cnx.atoms.WM_PROTOCOLS;
		ev.xclient.window = _xwin;
		ev.xclient.data.l[0] = _cnx.atoms.WM_TAKE_FOCUS;
		ev.xclient.data.l[1] = _cnx.last_know_time;
		_cnx.send_event(_xwin, False, NoEventMask, &ev);
	}

	_net_wm_state.insert(_cnx.atoms._NET_WM_STATE_FOCUSED);
	write_net_wm_state();

}



/* inspired from dwm */
std::string window_t::get_title() {
	std::string name;
	if (_has_net_wm_name) {
		name = _net_wm_name;
		return name;
	}

	if (_has_wm_name) {
		name = _wm_name;
		return name;
	}

	std::stringstream s(std::stringstream::in | std::stringstream::out);
	s << "#" << (_xwin) << " (noname)";
	name = s.str();
	return name;
}

void window_t::write_net_wm_state() const {
	long * new_state = new long[_net_wm_state.size() + 1]; // a less one long
	atom_set_t::const_iterator iter = _net_wm_state.begin();
	int i = 0;
	while (iter != _net_wm_state.end()) {
		new_state[i] = *iter;
		++iter;
		++i;
	}

	_cnx.change_property(_xwin, _cnx.atoms._NET_WM_STATE, _cnx.atoms.ATOM, 32,
			PropModeReplace, (unsigned char *) new_state, i);
	delete[] new_state;
}

void window_t::write_net_wm_allowed_actions() {
	long * data = new long[_net_wm_allowed_actions.size() + 1]; // at less one long
	std::set<Atom>::iterator iter = _net_wm_allowed_actions.begin();
	int i = 0;
	while (iter != _net_wm_allowed_actions.end()) {
		data[i] = *iter;
		++iter;
		++i;
	}

	_cnx.change_property(_xwin, _cnx.atoms._NET_WM_ALLOWED_ACTIONS,
			_cnx.atoms.ATOM, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(data), i);
	delete[] data;
}

void window_t::move_resize(box_int_t const & location) {
	notify_move_resize(location);
	_cnx.move_resize(_xwin, location);
}

/**
 * Update the ICCCM WM_STATE window property
 * @input state should be NormalState, IconicState or WithDrawnState
 **/
void window_t::write_wm_state(long state) {

	/* NOTE : ICCCM CARD32 mean "long" C type */
	_wm_state = state;

	struct {
		long state;
		Window icon;
	} data;

	data.state = state;
	data.icon = None;

	if (_wm_state != WithdrawnState) {
		_cnx.change_property(_xwin, _cnx.atoms.WM_STATE, _cnx.atoms.WM_STATE,
				32, PropModeReplace, reinterpret_cast<unsigned char *>(&data),
				2);
	} else {
		XDeleteProperty(_cnx.dpy, _xwin, _cnx.atoms.WM_STATE);
	}
}

bool window_t::is_fullscreen() {
	return _net_wm_state.find(_cnx.atoms._NET_WM_STATE_FULLSCREEN)
			!= _net_wm_state.end();
}

bool window_t::has_wm_type(Atom x) {
	return _net_wm_state.find(x)
			!= _net_wm_state.end();
}

bool window_t::demands_atteniion() {
	return false;
	return _net_wm_state.find(_cnx.atoms._NET_WM_STATE_DEMANDS_ATTENTION)
			!= _net_wm_state.end();
}


void window_t::set_net_wm_state(Atom x) {
	_net_wm_state.insert(x);
	write_net_wm_state();
}

void window_t::unset_net_wm_state(Atom x) {
	_net_wm_state.erase(x);
	write_net_wm_state();
}

void window_t::toggle_net_wm_state(Atom x) {
	if(_net_wm_state.find(x)
			!= _net_wm_state.end()) {
		unset_net_wm_state(x);
	} else {
		set_net_wm_state(x);
	}
}

long const * window_t::get_partial_struct() {
	if (_has_partial_struct) {
		return _partial_struct;
	} else {
		return 0;
	}
}

void window_t::set_fullscreen() {
	/* update window state */
	_net_wm_state.insert(_cnx.atoms._NET_WM_STATE_FULLSCREEN);
	write_net_wm_state();
}

void window_t::unset_fullscreen() {
	/* update window state */
	_net_wm_state.erase(_cnx.atoms._NET_WM_STATE_FULLSCREEN);
	write_net_wm_state();
}

void window_t::process_configure_notify_event(XConfigureEvent const & e) {
	assert(e.window == _xwin);
	_wa.x = e.x;
	_wa.y = e.y;
	_wa.width = e.width;
	_wa.height = e.height;
	_wa.border_width = e.border_width;
	_wa.override_redirect = e.override_redirect;
	_above = e.above;
}

void window_t::read_transient_for() {
	unsigned int n;
	long * data = get_properties32(_cnx.atoms.WM_TRANSIENT_FOR, _cnx.atoms.WINDOW, &n);
	if(data && n == 1) {
		_has_transient_for = true;
		_transient_for = data[0];
		delete[] data;
	} else {
		_has_transient_for = false;
		_transient_for = None;
	}

	//printf("Transient for %lu\n", _transient_for);

}

void window_t::map_notify() {
	_wa.map_state = IsViewable;
}

void window_t::unmap_notify() {
	_wa.map_state = IsUnmapped;
}

bool window_t::has_net_wm_desktop() {
	return _has_net_wm_desktop;
}

page_window_type_e window_t::find_window_type_pass1(xconnection_t const & cnx, Atom wm_window_type) {
	if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_DESKTOP) {
		return PAGE_NORMAL_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_DOCK) {
		return PAGE_DOCK_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_TOOLBAR) {
		return PAGE_FLOATING_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_MENU) {
		return PAGE_OVERLAY_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_UTILITY) {
		return PAGE_FLOATING_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_SPLASH) {
		return PAGE_OVERLAY_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_DIALOG) {
		return PAGE_FLOATING_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_DROPDOWN_MENU) {
		return PAGE_OVERLAY_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_POPUP_MENU) {
		return PAGE_OVERLAY_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_TOOLTIP) {
		return PAGE_TOOLTIP;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_NOTIFICATION) {
		return PAGE_NOTIFY_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_COMBO) {
		return PAGE_OVERLAY_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_DND) {
		return PAGE_OVERLAY_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_NORMAL) {
		return PAGE_NORMAL_WINDOW_TYPE;
	}

	return PAGE_UNKNOW_WINDOW_TYPE;

}

std::list<Atom> window_t::get_net_wm_type() {
	std::list<Atom> net_wm_type;

	unsigned int num;
	long * val = get_properties32(_cnx.atoms._NET_WM_WINDOW_TYPE, _cnx.atoms.ATOM,
			&num);
	if (val) {
		/* use the first value that we know about in the array */
		for (unsigned i = 0; i < num; ++i) {
			net_wm_type.push_back(val[i]);
		}
		delete[] val;
	} else {
		/*
		 * Fallback from ICCCM.
		 *
		 */

		if(!override_redirect()) {
			/* Managed windows */
			if(!_has_transient_for) {
				net_wm_type.push_back(_cnx.atoms._NET_WM_WINDOW_TYPE_DIALOG);
			} else {
				net_wm_type.push_back(_cnx.atoms._NET_WM_WINDOW_TYPE_DIALOG);
			}

		} else {
			/* Override-redirected windows */
			net_wm_type.push_back(_cnx.atoms._NET_WM_WINDOW_TYPE_DIALOG);
		}

	}

	return net_wm_type;

}

page_window_type_e window_t::find_window_type(xconnection_t const & cnx, Window w, int c_class) {
	/* this function try to figure out the type of a given window
	 * It seems that overide redirect is most important than
	 * _NET_WINDOW_TYPE
	 */
	if(c_class == InputOnly) {
		return PAGE_INPUT_ONLY_TYPE;
	}

	std::list<Atom> net_wm_type = get_net_wm_type();

	if(net_wm_type.empty()) {
		/* should never happen */
		throw std::runtime_error("_NET_WM_TYPE fail this should never happen");
	}


	if(override_redirect()) {
		/* override redirect window */
		return PAGE_OVERLAY_WINDOW_TYPE;
	} else {
		/* Managed windows */
		page_window_type_e ret = PAGE_UNKNOW_WINDOW_TYPE;
		std::list<Atom>::iterator i = net_wm_type.begin();
		while(ret == PAGE_UNKNOW_WINDOW_TYPE && i != net_wm_type.end()) {
			ret = find_window_type_pass1(cnx, *i);
		}

		if (ret != PAGE_UNKNOW_WINDOW_TYPE) {
			return ret;
		} else {
			/* should never happen, as standard spesify */
			throw std::runtime_error("Unknow window type, no BASE type is setup, this should never happen");
		}
	}

}

//void window_t::update_vm_hints() {
//	XWMHints const * hints = read_wm_hints();
//	if (hints) {
//		if ((hints->flags & InputHint)&& hints->input != True)
//			wm_input_focus = false;
//	}
//}

XWMHints const * window_t::get_wm_hints() {
	return _wm_hints;
}

XSizeHints const * window_t::get_wm_normal_hints() {
	return _wm_normal_hints;
}

page_window_type_e window_t::get_window_type() {
	return _type;
}

bool window_t::is_window(Window w) {
	return w == _xwin;
}

Window window_t::get_xwin() {
	return _xwin;
}

long window_t::get_wm_state() {
	return _wm_state;
}

bool window_t::is_map() {
	return _wa.map_state != IsUnmapped;
}

void window_t::delete_window(Time t) {
	XEvent ev;
	ev.xclient.display = _cnx.dpy;
	ev.xclient.type = ClientMessage;
	ev.xclient.format = 32;
	ev.xclient.message_type = _cnx.atoms.WM_PROTOCOLS;
	ev.xclient.window = _xwin;
	ev.xclient.data.l[0] = _cnx.atoms.WM_DELETE_WINDOW;
	ev.xclient.data.l[1] = t;
	_cnx.send_event(_xwin, False, NoEventMask, &ev);
}

bool window_t::check_normal_hints_constraint(int width, int heigth) {
	if(!_has_wm_normal_hints)
		return true;

	if(_wm_normal_hints->flags & PMinSize) {
		if(width < _wm_normal_hints->min_width || heigth < _wm_normal_hints->min_height) {
			return false;
		}
	}

	if(_wm_normal_hints->flags & PMaxSize) {
		if(width > _wm_normal_hints->max_width || heigth > _wm_normal_hints->max_height) {
			return false;
		}
	}

	return true;
}


bool window_t::get_has_wm_state() {
	return _has_wm_state;
}

void window_t::notify_move_resize(box_int_t const & area) {
	_wa.x = area.x;
	_wa.y = area.y;
	_wa.width = area.w;
	_wa.height = area.h;
}

void window_t::iconify() {
	unmap();
	write_wm_state(IconicState);
	set_net_wm_state(_cnx.atoms._NET_WM_STATE_HIDDEN);
}

void window_t::normalize() {
	map();
	write_wm_state(NormalState);
	unset_net_wm_state(_cnx.atoms._NET_WM_STATE_HIDDEN);
}

bool window_t::is_hidden() {
	return _net_wm_state.find(_cnx.atoms._NET_WM_STATE_HIDDEN) != _net_wm_state.end();
}

bool window_t::is_notification() {
	return std::find(_net_wm_type.begin(), _net_wm_type.end(), _cnx.atoms._NET_WM_WINDOW_TYPE_NOTIFICATION) != _net_wm_type.end();
}

int window_t::get_initial_state() {
	if (_wm_hints) {
		//printf("XXXXXXXXXXXXXXXX initial_state = %d\n", _wm_hints->initial_state);
		return _wm_hints->initial_state;
	}
	return WithdrawnState;
}

void window_t::fake_configure(box_int_t location, int border_width) {
	_cnx.fake_configure(_xwin, location, border_width);
}

void window_t::reparent(Window parent, int x, int y) {
	_cnx.reparentwindow(_xwin, parent, x, y);
}

cairo_surface_t * window_t::create_cairo_surface() {
	return cairo_xlib_surface_create(_cnx.dpy, _xwin, _wa.visual, _wa.width,
			_wa.height);
}

Display * window_t::get_display() {
	return _cnx.dpy;
}

Visual * window_t::get_visual() {
	return _wa.visual;
}

int window_t::get_depth() {
	return _wa.depth;
}

bool window_t::read_window_attributes() {
	return (_cnx.get_window_attributes(_xwin, &_wa) != 0);
}

void window_t::grab_button(int button) {
	XGrabButton(_cnx.dpy, Button1, AnyModifier, _xwin, False, ButtonPressMask, GrabModeSync, GrabModeAsync, _cnx.xroot, None);
}

void window_t::set_managed(bool state) {
	_is_managed = state;
}

bool window_t::is_managed() {
	return _is_managed;
}

}

