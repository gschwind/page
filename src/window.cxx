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

window_t::window_t(xconnection_t &cnx, Window w, XWindowAttributes const & wa) :
	cnx(cnx),
	xwin(w),
	wm_normal_hints(0),
	wm_hints(0),
	lock_count(0),
	wm_state(0),
	net_wm_type(),
	net_wm_state(),
	net_wm_protocols(),
	net_wm_allowed_actions(),
	has_wm_name(false),
	has_net_wm_name(false),
	has_partial_struct(false),
	wm_input_focus(false),
	_is_map(false),
	is_lock(false),
	wm_name(),
	net_wm_name(),
	_transient_for(None),
	sibbling_childs()
{


	/* Copy windows attributes */
	position = box_int_t(wa.x, wa.y, wa.width, wa.height);
    border_width = wa.border_width;
    depth = wa.depth;
    visual = wa.visual;
    root = wa.root;
    c_class = wa.c_class;
    bit_gravity = wa.bit_gravity;
    win_gravity = wa.win_gravity;
    save_under = wa.save_under;
    colormap = wa.colormap;
    map_installed = wa.map_installed;
    map_state = wa.map_state;
    screen = wa.screen;

	_overide_redirect = (wa.override_redirect == True) ? true : false;
	_is_map = (wa.map_state == IsUnmapped) ? false : true;

	memset(partial_struct, 0, sizeof(partial_struct));

	wm_input_focus = false;
	wm_state = WithdrawnState;

	net_wm_icon_size = 0;
	net_wm_icon_data = 0;

	/*
	 * to ensure the state of transient_for and struct_partial, we select input for property change and then
	 * we sync wuth server, and then check window state, if window property change
	 * we get it.
	 */
	//cnx.select_input(xwin, PropertyChangeMask | StructureNotifyMask);
	/* sync and flush, now we are sure that property change are listen */
	XFlush(cnx.dpy);
	XSync(cnx.dpy, False);

	XGrabButton(cnx.dpy, Button1, AnyModifier, xwin, False, ButtonPressMask, GrabModeSync, GrabModeAsync, cnx.xroot, None);

	/* read transient_for state */
	wm_normal_hints = XAllocSizeHints();
	wm_hints = XAllocWMHints();

	//read_all();
}

box_int_t window_t::get_size() {
	return position;
}

void window_t::setup_extends() {
	/* set border */
	cnx.set_window_border_width(xwin, 0);

	/* set frame extend to 0 (I don't know why a client need this data) */
	long frame_extends[4] = { 0, 0, 0, 0 };
	cnx.change_property(xwin, cnx.atoms._NET_FRAME_EXTENTS,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(frame_extends), 4);

}

window_t::~window_t() {

	XFree(wm_normal_hints);
	XFree(wm_hints);

}

void window_t::read_wm_hints() {
	XFree(wm_hints);
	wm_hints = cnx.get_wm_hints(xwin);
}


void window_t::read_wm_normal_hints() {
	has_wm_normal_hints = true;

	long size_hints_flags;
	if (!XGetWMNormalHints(cnx.dpy, xwin, wm_normal_hints, &size_hints_flags)) {
		/* size is uninitialized, ensure that size.flags aren't used */
		wm_normal_hints->flags = 0;
		has_wm_normal_hints = false;
		printf("no WMNormalHints\n");
	}
	//printf("x: %d y: %d w: %d h: %d\n", wm_normal_hints->x, wm_normal_hints->y, wm_normal_hints->width, wm_normal_hints->height);
	if(wm_normal_hints->flags & PMaxSize) {
		printf("max w: %d max h: %d \n", wm_normal_hints->max_width, wm_normal_hints->max_height);
	}

	if(wm_normal_hints->flags & PMinSize) {
		printf("min w: %d min h: %d \n", wm_normal_hints->min_width, wm_normal_hints->min_height);
	}

}


void window_t::read_vm_name() {
	has_wm_name = false;
	XTextProperty name;
	cnx.get_text_property(xwin, &name, cnx.atoms.WM_NAME);
	if (!name.nitems) {
		XFree(name.value);
		wm_name = "";
		return;
	}
	has_wm_name = true;
	wm_name = (char const *) name.value;
	XFree(name.value);
	return;
}

void window_t::read_net_vm_name() {
	has_net_wm_name = false;
	XTextProperty name;
	cnx.get_text_property(xwin, &name, cnx.atoms._NET_WM_NAME);
	if (!name.nitems) {
		XFree(name.value);
		net_wm_name = "";
		return;
	}
	has_net_wm_name = true;
	net_wm_name = (char const *) name.value;
	XFree(name.value);
	return;
}


void window_t::read_net_wm_type() {
	unsigned int num;
	long * val = get_properties32(cnx.atoms._NET_WM_WINDOW_TYPE, cnx.atoms.ATOM,
			&num);
	if (val) {
		net_wm_type.clear();
		/* use the first value that we know about in the array */
		for (unsigned i = 0; i < num; ++i) {
			net_wm_type.push_back(val[i]);
		}
		delete[] val;
	}

	print_net_wm_window_type();

	type = find_window_type(cnx, xwin, c_class);

}

void window_t::read_net_wm_state() {
	/* take _NET_WM_STATE */
	unsigned int n;
	long * net_wm_state = get_properties32(cnx.atoms._NET_WM_STATE,
			cnx.atoms.ATOM, &n);
	if (net_wm_state) {
		this->net_wm_state.clear();
		for (unsigned int i = 0; i < n; ++i) {
			this->net_wm_state.insert(net_wm_state[i]);
		}
		delete[] net_wm_state;
	}

	print_net_wm_state();
}

void window_t::read_net_wm_protocols() {
	net_wm_protocols.clear();
	int count;
	Atom * atoms_list;
	if (XGetWMProtocols(cnx.dpy, xwin, &atoms_list, &count)) {
		for (int i = 0; i < count; ++i) {
			net_wm_protocols.insert(atoms_list[i]);
		}
		XFree(atoms_list);
	}
}

void window_t::read_partial_struct() {
	unsigned int n;
	long * p_struct = get_properties32(cnx.atoms._NET_WM_STRUT_PARTIAL,
			cnx.atoms.CARDINAL, &n);

	if (p_struct && n == 12) {
//		printf(
//				"partial struct %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n",
//				partial_struct[0], partial_struct[1], partial_struct[2],
//				partial_struct[3], partial_struct[4], partial_struct[5],
//				partial_struct[6], partial_struct[7], partial_struct[8],
//				partial_struct[9], partial_struct[10], partial_struct[11]);

		has_partial_struct = true;
		memcpy(partial_struct, p_struct, sizeof(long) * 12);
	} else {
		has_partial_struct = false;
	}

	if(p_struct != 0)
		delete[] p_struct;

}

void window_t::read_net_wm_user_time() {
	unsigned int n;
	long * time = get_properties32(cnx.atoms._NET_WM_USER_TIME, cnx.atoms.CARDINAL, &n);
	if(time) {
		user_time = time[0];
	} else {
		user_time = 0;
	}
}


void window_t::set_focused() {
	net_wm_state.insert(cnx.atoms._NET_WM_STATE_FOCUSED);
	write_net_wm_state();
}

void window_t::unset_focused() {
	net_wm_state.erase(cnx.atoms._NET_WM_STATE_FOCUSED);
	write_net_wm_state();
}

void window_t::set_default_action() {
	net_wm_allowed_actions.clear();
	net_wm_allowed_actions.insert(cnx.atoms._NET_WM_ACTION_CLOSE);
	net_wm_allowed_actions.insert(cnx.atoms._NET_WM_ACTION_FULLSCREEN);
	write_net_wm_allowed_actions();
}

void window_t::set_dock_action() {
	net_wm_allowed_actions.clear();
	net_wm_allowed_actions.insert(cnx.atoms._NET_WM_ACTION_CLOSE);
	write_net_wm_allowed_actions();
}


void window_t::select_input(long int mask) {
	cnx.select_input(xwin, mask);
}

void window_t::print_net_wm_window_type() {
	unsigned int num;
	long * val = get_properties32(cnx.atoms._NET_WM_WINDOW_TYPE,
			cnx.atoms.ATOM, &num);
	if (val) {
		/* use the first value that we know about in the array */
		for (unsigned i = 0; i < num; ++i) {
			char * name = XGetAtomName(cnx.dpy, val[i]);
			printf("_NET_WM_WINDOW_TYPE = \"%s\"\n", name);
			XFree(name);
		}
		delete[] val;
	}
}

void window_t::print_net_wm_state() {
	unsigned int num;
	long * val = get_properties32(cnx.atoms._NET_WM_STATE,
			cnx.atoms.ATOM, &num);
	printf("_NET_WM_STATE =");
	if (val) {
		/* use the first value that we know about in the array */
		for (unsigned i = 0; i < num; ++i) {
			char * name = XGetAtomName(cnx.dpy, val[i]);
			printf(" \"%s\"", name);
			XFree(name);
		}
		delete[] val;
	}
	printf("\n");
}

bool window_t::is_input_only() {
	return c_class == InputOnly;
}

void window_t::add_to_save_set() {
	cnx.add_to_save_set(xwin);
}

void window_t::remove_from_save_set() {
	cnx.remove_from_save_set(xwin);
}

bool window_t::is_visible() {
	return is_map() && !is_input_only();
}

long window_t::get_net_user_time() {
	return user_time;
}

bool window_t::override_redirect() {
	return _overide_redirect;
}

Window window_t::transient_for() {
	return _transient_for;
}

void window_t::read_icon_data() {
	if(net_wm_icon_data != 0)
		delete[] net_wm_icon_data;
	net_wm_icon_data = get_properties32(cnx.atoms._NET_WM_ICON, cnx.atoms.CARDINAL, &net_wm_icon_size);

}

void window_t::read_all() {
	wm_input_focus = true;
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
}

void window_t::get_icon_data(long const *& data, int & size) {
	data = net_wm_icon_data;
	size = net_wm_icon_size;
}

void window_t::read_wm_state() {
	int format;
	long * p = NULL;
	unsigned long n, extra;
	Atom real;

	has_wm_state = false;
	wm_state = WithdrawnState;

	if (cnx.get_window_property(xwin, cnx.atoms.WM_STATE, 0L, 2L, False,
			cnx.atoms.WM_STATE, &real, &format, &n, &extra,
			(unsigned char **) &p) == Success) {
		if (n != 0 && format == 32 && real == cnx.atoms.WM_STATE) {
			wm_state = p[0];
			has_wm_state = true;
		}
		XFree(p);
	}

}

void window_t::map() {
	_is_map = true;
	cnx.map(xwin);
}

void window_t::unmap() {
	cnx.unmap(xwin);
}

/* check if client is still alive */
bool window_t::try_lock() {
	cnx.grab();
	XEvent e;
	if (XCheckTypedWindowEvent(cnx.dpy, xwin, DestroyNotify, &e)) {
		XPutBackEvent(cnx.dpy, &e);
		cnx.ungrab();
		return false;
	}
	is_lock = true;
	return true;
}

void window_t::unlock() {
	is_lock = false;
	cnx.ungrab();
}

void window_t::focus() {

	if (_is_map && wm_input_focus) {
		cnx.set_input_focus(xwin, RevertToParent, cnx.last_know_time);
	}

	if (net_wm_protocols.find(cnx.atoms.WM_TAKE_FOCUS)
			!= net_wm_protocols.end()) {
		XEvent ev;
		ev.xclient.display = cnx.dpy;
		ev.xclient.type = ClientMessage;
		ev.xclient.format = 32;
		ev.xclient.message_type = cnx.atoms.WM_PROTOCOLS;
		ev.xclient.window = xwin;
		ev.xclient.data.l[0] = cnx.atoms.WM_TAKE_FOCUS;
		ev.xclient.data.l[1] = cnx.last_know_time;
		cnx.send_event(xwin, False, NoEventMask, &ev);
	}

	net_wm_state.insert(cnx.atoms._NET_WM_STATE_FOCUSED);
	write_net_wm_state();

}



/* inspired from dwm */
std::string window_t::get_title() {
	std::string name;
	if (has_net_wm_name) {
		name = net_wm_name;
		return name;
	}

	if (has_wm_name) {
		name = wm_name;
		return name;
	}

	std::stringstream s(std::stringstream::in | std::stringstream::out);
	s << "#" << (xwin) << " (noname)";
	name = s.str();
	return name;
}

void window_t::write_net_wm_state() const {
	long * new_state = new long[net_wm_state.size() + 1]; // a less one long
	atom_set_t::const_iterator iter = net_wm_state.begin();
	int i = 0;
	while (iter != net_wm_state.end()) {
		new_state[i] = *iter;
		++iter;
		++i;
	}

	cnx.change_property(xwin, cnx.atoms._NET_WM_STATE, cnx.atoms.ATOM, 32,
			PropModeReplace, (unsigned char *) new_state, i);
	delete[] new_state;
}

void window_t::write_net_wm_allowed_actions() {
	long * data = new long[net_wm_allowed_actions.size() + 1]; // at less one long
	std::set<Atom>::iterator iter = net_wm_allowed_actions.begin();
	int i = 0;
	while (iter != net_wm_allowed_actions.end()) {
		data[i] = *iter;
		++iter;
		++i;
	}

	cnx.change_property(xwin, cnx.atoms._NET_WM_ALLOWED_ACTIONS,
			cnx.atoms.ATOM, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(data), i);
	delete[] data;
}

void window_t::move_resize(box_int_t const & location) {
	position = location;
	cnx.move_resize(xwin, location);
}

/**
 * Update the ICCCM WM_STATE window property
 * @input state should be NormalState, IconicState or WithDrawnState
 **/
void window_t::write_wm_state(long state) {

	/* NOTE : ICCCM CARD32 mean "long" C type */
	wm_state = state;

	struct {
		long state;
		Window icon;
	} data;

	data.state = state;
	data.icon = None;

	if (wm_state != WithdrawnState) {
		cnx.change_property(xwin, cnx.atoms.WM_STATE, cnx.atoms.WM_STATE,
				32, PropModeReplace, reinterpret_cast<unsigned char *>(&data),
				2);
	} else {
		XDeleteProperty(cnx.dpy, xwin, cnx.atoms.WM_STATE);
	}
}

bool window_t::is_fullscreen() {
	return net_wm_state.find(cnx.atoms._NET_WM_STATE_FULLSCREEN)
			!= net_wm_state.end();
}

bool window_t::has_wm_type(Atom x) {
	return net_wm_state.find(x)
			!= net_wm_state.end();
}

bool window_t::demands_atteniion() {
	return false;
	return net_wm_state.find(cnx.atoms._NET_WM_STATE_DEMANDS_ATTENTION)
			!= net_wm_state.end();
}


void window_t::set_net_wm_state(Atom x) {
	net_wm_state.insert(x);
	write_net_wm_state();
}

void window_t::unset_net_wm_state(Atom x) {
	net_wm_state.erase(x);
	write_net_wm_state();
}

void window_t::toggle_net_wm_state(Atom x) {
	if(net_wm_state.find(x)
			!= net_wm_state.end()) {
		unset_net_wm_state(x);
	} else {
		set_net_wm_state(x);
	}
}

long const * window_t::get_partial_struct() {
	if (has_partial_struct) {
		return partial_struct;
	} else {
		return 0;
	}
}

void window_t::set_fullscreen() {
	/* update window state */
	net_wm_state.insert(cnx.atoms._NET_WM_STATE_FULLSCREEN);
	write_net_wm_state();
}

void window_t::unset_fullscreen() {
	/* update window state */
	net_wm_state.erase(cnx.atoms._NET_WM_STATE_FULLSCREEN);
	write_net_wm_state();
}

void window_t::process_configure_notify_event(XConfigureEvent const & e) {
	assert(e.window == xwin);
	position = box_int_t(e.x, e.y, e.width, e.height);
	border_width = e.border_width;
	_overide_redirect = (e.override_redirect == True)?true:false;
	above = e.above;
}

void window_t::read_transient_for() {
	unsigned int n;
	long * data = get_properties32(cnx.atoms.WM_TRANSIENT_FOR, cnx.atoms.WINDOW, &n);
	if(data && n == 1) {
		has_transient_for = true;
		_transient_for = data[0];
		delete[] data;
	} else {
		has_transient_for = false;
		_transient_for = None;
	}
}

void window_t::map_notify() {
	_is_map = true;
}

void window_t::unmap_notify() {
	_is_map = false;
}

void window_t::apply_size_constraint() {
	/* default size if no size_hints is provided */
	XSizeHints const & size_hints = *wm_normal_hints;

	/* no vm hints */
	if(wm_normal_hints->flags == 0)
		return;

	int max_width = position.w;
	int max_height = position.h;

	if (size_hints.flags & PMaxSize) {
		if (max_width > size_hints.max_width)
			max_width = size_hints.max_width;
		if (max_height > size_hints.max_height)
			max_height = size_hints.max_height;
	}

	if (size_hints.flags & PBaseSize) {
		if (max_width < size_hints.base_width)
			max_width = size_hints.base_width;
		if (max_height < size_hints.base_height)
			max_height = size_hints.base_height;
	} else if (size_hints.flags & PMinSize) {
		if (max_width < size_hints.min_width)
			max_width = size_hints.min_width;
		if (max_height < size_hints.min_height)
			max_height = size_hints.min_height;
	}

	if (size_hints.flags & PAspect) {
		if (size_hints.flags & PBaseSize) {
			/* ICCCM say if base is set substract base before aspect checking ref : ICCCM*/
			if ((max_width - size_hints.base_width) * size_hints.min_aspect.y
					< (max_height - size_hints.base_height)
							* size_hints.min_aspect.x) {
				/* reduce h */
				max_height = size_hints.base_height
						+ ((max_width - size_hints.base_width)
								* size_hints.min_aspect.y)
								/ size_hints.min_aspect.x;

			} else if ((max_width - size_hints.base_width)
					* size_hints.max_aspect.y
					> (max_height - size_hints.base_height)
							* size_hints.max_aspect.x) {
				/* reduce w */
				max_width = size_hints.base_width
						+ ((max_height - size_hints.base_height)
								* size_hints.max_aspect.x)
								/ size_hints.max_aspect.y;
			}
		} else {
			if (max_width * size_hints.min_aspect.y
					< max_height * size_hints.min_aspect.x) {
				/* reduce h */
				max_height = (max_width * size_hints.min_aspect.y)
						/ size_hints.min_aspect.x;

			} else if (max_width * size_hints.max_aspect.y
					> max_height * size_hints.max_aspect.x) {
				/* reduce w */
				max_width = (max_height * size_hints.max_aspect.x)
						/ size_hints.max_aspect.y;
			}
		}

	}

	if (size_hints.flags & PResizeInc) {
		max_width -=
				((max_width - size_hints.base_width) % size_hints.width_inc);
		max_height -= ((max_height - size_hints.base_height)
				% size_hints.height_inc);
	}

	printf("XXXX %d %d \n", max_width, max_height);

	//if(max_height != size.h || max_width != size.w) {
		move_resize(box_int_t(position.x, position.y, max_width, max_height));
	//}

}

page_window_type_e window_t::find_window_type_pass1(xconnection_t const & cnx, Atom wm_window_type) {
	if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_DESKTOP) {
		return PAGE_NORMAL_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_DOCK) {
		return PAGE_OVERLAY_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_TOOLBAR) {
		return PAGE_NORMAL_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_MENU) {
		return PAGE_OVERLAY_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_UTILITY) {
		return PAGE_NORMAL_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_SPLASH) {
		return PAGE_OVERLAY_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_DIALOG) {
		return PAGE_FLOATING_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_DROPDOWN_MENU) {
		return PAGE_OVERLAY_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_POPUP_MENU) {
		return PAGE_OVERLAY_WINDOW_TYPE;
	} else if (wm_window_type == cnx.atoms._NET_WM_WINDOW_TYPE_TOOLTIP) {
		return PAGE_OVERLAY_WINDOW_TYPE;
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
	long * val = get_properties32(cnx.atoms._NET_WM_WINDOW_TYPE, cnx.atoms.ATOM,
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
			if(!has_transient_for) {
				net_wm_type.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_NORMAL);
			} else {
				net_wm_type.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_DIALOG);
			}

		} else {
			/* Override-redirected windows */
			net_wm_type.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_NORMAL);
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
	return wm_hints;
}

XSizeHints const * window_t::get_wm_normal_hints() {
	return wm_normal_hints;
}

void window_t::add_sibbling_child(window_t * c) {
	sibbling_childs.insert(c);
}

void window_t::remove_sibbling_child(window_t * c) {
	sibbling_childs.erase(c);
}

window_set_t window_t::get_sibbling_childs() {
	return sibbling_childs;
}

page_window_type_e window_t::get_window_type() {
	return type;
}

bool window_t::is_window(Window w) {
	return w == xwin;
}

Window window_t::get_xwin() {
	return xwin;
}

long window_t::get_wm_state() {
	return wm_state;
}

bool window_t::is_map() {
	return _is_map;
}

void window_t::delete_window(Time t) {
	XEvent ev;
	ev.xclient.display = cnx.dpy;
	ev.xclient.type = ClientMessage;
	ev.xclient.format = 32;
	ev.xclient.message_type = cnx.atoms.WM_PROTOCOLS;
	ev.xclient.window = xwin;
	ev.xclient.data.l[0] = cnx.atoms.WM_DELETE_WINDOW;
	ev.xclient.data.l[1] = t;
	cnx.send_event(xwin, False, NoEventMask, &ev);
}

bool window_t::check_normal_hints_constraint(int width, int heigth) {
	if(!has_wm_normal_hints)
		return true;

	if(wm_normal_hints->flags & PMinSize) {
		if(width < wm_normal_hints->min_width || heigth < wm_normal_hints->min_height) {
			return false;
		}
	}

	if(wm_normal_hints->flags & PMaxSize) {
		if(width > wm_normal_hints->max_width || heigth > wm_normal_hints->max_height) {
			return false;
		}
	}

	return true;
}


bool window_t::get_has_wm_state() {
	return has_wm_state;
}

void window_t::notify_move_resize(box_int_t const & area) {
	position = area;
}

void window_t::iconify() {
	unmap();
	write_wm_state(IconicState);
	set_net_wm_state(cnx.atoms._NET_WM_STATE_HIDDEN);
}

void window_t::normalize() {
	map();
	write_wm_state(NormalState);
	unset_net_wm_state(cnx.atoms._NET_WM_STATE_HIDDEN);
}

bool window_t::is_hidden() {
	return net_wm_state.find(cnx.atoms._NET_WM_STATE_HIDDEN) != net_wm_state.end();
}

bool window_t::is_notification() {
	return std::find(net_wm_type.begin(), net_wm_type.end(), cnx.atoms._NET_WM_WINDOW_TYPE_NOTIFICATION) != net_wm_type.end();
}

int window_t::get_initial_state() {
	if (wm_hints) {
		return wm_hints->initial_state;
	}
	return WithdrawnState;
}


}

