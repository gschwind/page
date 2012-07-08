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

#include "window.hxx"

namespace page {

long int const ClientEventMask = (StructureNotifyMask | PropertyChangeMask);

window_t::window_map_t window_t::created_window;

window_t::window_t(xconnection_t &cnx, Window w, XWindowAttributes const & wa) :
		cnx(cnx), xwin(w), has_partial_struct(false), lock_count(0), is_lock(
				false), window_surf(0), damage(0), opacity(1.0), has_wm_name(
				false), has_net_wm_name(false), wm_name(""), net_wm_name(""), name(
				"") {

	assert(created_window.find(w) == created_window.end());
	created_window[w] = this;

	if (wa.map_state == IsUnmapped) {
		_is_map = false;
	} else {
		_is_map = true;
	}

	memset(partial_struct, 0, sizeof(partial_struct));
	size.x = wa.x;
	size.y = wa.y;
	size.w = wa.width;
	size.h = wa.height;
	visual = wa.visual;
	w_class = wa.c_class;

	_override_redirect = (wa.override_redirect ? true : false);

	read_all();

	/* save in case of page failure */
	//cnx.add_to_save_set(xwin);

	create_render_context();

	cnx.select_input(xwin, PropertyChangeMask);

}

void window_t::create_render_context() {
	assert(window_surf == 0);
	assert(damage == 0);
	/* redirect to back buffer */
	XCompositeRedirectWindow(cnx.dpy, xwin, CompositeRedirectManual);
	/* create the cairo surface */
	window_surf = cairo_xlib_surface_create(cnx.dpy, xwin, visual, size.w,
			size.h);
	/* track update */
	damage = XDamageCreate(cnx.dpy, xwin, XDamageReportNonEmpty);
	/* clear damaged area */
	if (damage)
		XDamageSubtract(cnx.dpy, damage, None, None);

}

void window_t::destroy_render_context() {
	if (damage != 0) {
		XDamageDestroy(cnx.dpy, damage);
		damage = 0;
	}

	if (window_surf != 0) {
		cairo_surface_destroy(window_surf);
		window_surf = 0;
	}

	XCompositeUnredirectWindow(cnx.dpy, xwin, CompositeRedirectManual);
}

void window_t::setup_extends() {
	/* set border */
	cnx.set_window_border_width(xwin, 0);

	/* set frame extend to 0 (I don't know why a client need this data) */
	long frame_extends[4] = { 0, 0, 0, 0 };
	XChangeProperty(cnx.dpy, xwin, cnx.atoms._NET_FRAME_EXTENTS,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(frame_extends), 4);

}

window_t::~window_t() {
	assert(created_window.find(xwin) != created_window.end());
	created_window.erase(xwin);

	write_wm_state(WithdrawnState);
	XDeleteProperty(cnx.dpy, xwin, cnx.atoms._NET_FRAME_EXTENTS);

	cnx.select_input(xwin, NoEventMask);
	destroy_render_context();

	XRemoveFromSaveSet(cnx.dpy, xwin);
}

void window_t::read_all() {

	wm_input_focus = false;
	XWMHints * hints = read_wm_hints();
	if (hints) {
		if ((hints->flags & InputHint) && hints->input == True)
			wm_input_focus = true;
		XFree(hints);
	}

	read_net_vm_name();
	read_vm_name();
	read_title();
	read_size_hints();
	read_net_wm_type();
	read_net_wm_state();
	read_net_wm_protocols();

	wm_state = read_wm_state();

}

long window_t::read_wm_state() {
	int format;
	long result = -1;
	long * p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(cnx.dpy, xwin, cnx.atoms.WM_STATE, 0L, 2L, False,
			cnx.atoms.WM_STATE, &real, &format, &n, &extra,
			(unsigned char **) &p) != Success)
		return WithdrawnState;

	if (n != 0 && format == 32 && real == cnx.atoms.WM_STATE) {
		result = p[0];
	} else {
		printf("Error in WM_STATE %lu %d %lu\n", n, format, real);
		return WithdrawnState;
	}
	XFree(p);
	return result;
}

void window_t::map() {
	_is_map = true;
	cnx.map(xwin);
}

void window_t::unmap() {
	_is_map = false;
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
		printf("Focus #%x\n", (unsigned int) xwin);
		//cnx.raise_window(xwin);
		XSetInputFocus(cnx.dpy, xwin, RevertToParent, cnx.last_know_time);
	}

	if (net_wm_protocols.find(cnx.atoms.WM_TAKE_FOCUS)
			!= net_wm_protocols.end()) {
		printf("TAKE_FOCUS\n");
		//cnx.raise_window(xwin);
		XEvent ev;
		ev.xclient.display = cnx.dpy;
		ev.xclient.type = ClientMessage;
		ev.xclient.format = 32;
		ev.xclient.message_type = cnx.atoms.WM_PROTOCOLS;
		ev.xclient.window = xwin;
		ev.xclient.data.l[0] = cnx.atoms.WM_TAKE_FOCUS;
		ev.xclient.data.l[1] = cnx.last_know_time;
		XSendEvent(cnx.dpy, xwin, False, NoEventMask, &ev);
	}

	net_wm_state.insert(cnx.atoms._NET_WM_STATE_FOCUSED);
	write_net_wm_state();

	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_ACTIVE_WINDOW,
			cnx.atoms.WINDOW, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&(xwin)), 1);

}

void window_t::read_size_hints() {
	long msize;
	if (!XGetWMNormalHints(cnx.dpy, xwin, &hints, &msize)) {
		/* size is uninitialized, ensure that size.flags aren't used */
		hints.flags = 0;
		printf("no WMNormalHints\n");
	}
}

XWMHints *
window_t::read_wm_hints() {
	return XGetWMHints(cnx.dpy, xwin);
}

std::string const &
window_t::read_vm_name() {
	has_wm_name = false;
	char **list = NULL;
	XTextProperty name;
	XGetTextProperty(cnx.dpy, xwin, &name, cnx.atoms.WM_NAME);
	if (!name.nitems) {
		XFree(name.value);
		wm_name = "none";
		return wm_name;
	}
	has_wm_name = true;
	wm_name = (char const *) name.value;
	XFree(name.value);
	return wm_name;
}

std::string const &
window_t::read_net_vm_name() {
	has_net_wm_name = false;
	char **list = NULL;
	XTextProperty name;
	XGetTextProperty(cnx.dpy, xwin, &name, cnx.atoms._NET_WM_NAME);
	if (!name.nitems) {
		XFree(name.value);
		net_wm_name = "none";
		return net_wm_name;
	}
	has_net_wm_name = true;
	net_wm_name = (char const *) name.value;
	XFree(name.value);
	return net_wm_name;
}

/* inspired from dwm */
std::string const &
window_t::read_title() {
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

void window_t::read_net_wm_type() {
	unsigned int num;
	long * val = get_properties32(cnx.atoms._NET_WM_WINDOW_TYPE, cnx.atoms.ATOM,
			&num);
	if (val) {
		net_wm_type.clear();
		/* use the first value that we know about in the array */
		for (unsigned i = 0; i < num; ++i) {
			net_wm_type.insert(val[i]);
		}
		delete[] val;
	}
}

void window_t::read_net_wm_state() {
	/* take _NET_WM_STATE */
	unsigned int n;
	long * net_wm_state = get_properties32(cnx.atoms._NET_WM_STATE,
			cnx.atoms.ATOM, &n);
	if (net_wm_state) {
		this->net_wm_state.clear();
		for (int i = 0; i < n; ++i) {
			this->net_wm_state.insert(net_wm_state[i]);
		}
		delete[] net_wm_state;
	}
}

void window_t::read_net_wm_protocols() {
	/* take _NET_WM_STATE */
	unsigned int n;
	long * wm_protocols = get_properties32(cnx.atoms.WM_PROTOCOLS,
			cnx.atoms.ATOM, &n);
	if (wm_protocols) {
		this->net_wm_protocols.clear();
		for (int i = 0; i < n; ++i) {
			this->net_wm_protocols.insert(wm_protocols[i]);
		}
		delete[] wm_protocols;
	}
}

void window_t::write_net_wm_state() {
	int size = net_wm_state.size();
	long * new_state = new long[size];
	std::set<Atom>::iterator iter = net_wm_state.begin();
	int i = 0;
	while (iter != net_wm_state.end()) {
		new_state[i] = *iter;
		++iter;
		++i;
	}

	XChangeProperty(cnx.dpy, cnx.xroot, cnx.atoms._NET_WM_STATE, cnx.atoms.ATOM,
			32, PropModeReplace, reinterpret_cast<unsigned char *>(new_state),
			size);
	delete[] new_state;
}

void window_t::write_net_wm_allowed_actions() {
	int size = net_wm_allowed_actions.size();
	long * data = new long[size];
	std::set<Atom>::iterator iter = net_wm_allowed_actions.begin();
	int i = 0;
	while (iter != net_wm_allowed_actions.end()) {
		data[i] = *iter;
		++iter;
		++i;
	}

	XChangeProperty(cnx.dpy, xwin, cnx.atoms._NET_WM_ALLOWED_ACTIONS,
			cnx.atoms.ATOM, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(data), size);
	delete[] data;
}

void window_t::move_resize(box_int_t const & location) {
	size = location;
	cnx.move_resize(xwin, size);
	cairo_surface_flush(window_surf);
	cairo_xlib_surface_set_size(window_surf, size.w, size.h);
	mark_dirty();
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
		XChangeProperty(cnx.dpy, xwin, cnx.atoms.WM_STATE, cnx.atoms.WM_STATE,
				32, PropModeReplace, reinterpret_cast<unsigned char *>(&data),
				2);
	} else {
		XDeleteProperty(cnx.dpy, xwin, cnx.atoms.WM_STATE);
	}
}

bool window_t::is_dock() {
	return net_wm_type.find(cnx.atoms._NET_WM_WINDOW_TYPE_DOCK)
			!= net_wm_type.end();
}

bool window_t::is_fullscreen() {
	return net_wm_state.find(cnx.atoms._NET_WM_STATE_FULLSCREEN)
			!= net_wm_state.end();
}

long const *
window_t::read_partial_struct() {
	unsigned int n;
	long * p_struct = get_properties32(cnx.atoms._NET_WM_STRUT_PARTIAL,
			cnx.atoms.CARDINAL, &n);

	if (p_struct && n == 12) {
		printf(
				"partial struct %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n",
				partial_struct[0], partial_struct[1], partial_struct[2],
				partial_struct[3], partial_struct[4], partial_struct[5],
				partial_struct[6], partial_struct[7], partial_struct[8],
				partial_struct[9], partial_struct[10], partial_struct[11]);

		has_partial_struct = true;
		memcpy(partial_struct, p_struct, sizeof(long) * 12);
		delete[] p_struct;
	}

	if (has_partial_struct) {
		return partial_struct;
	} else {
		return 0;
	}

}

void window_t::repair1(cairo_t * cr, box_int_t const & area) {
	assert(window_surf != 0);
	box_int_t clip = area & size;
	//printf("repair popup %p %dx%d+%d+%d\n", this, clip.w, clip.h, clip.x, clip.y);
	if (clip.w > 0 && clip.h > 0) {
		cairo_save(cr);
		cairo_set_source_surface(cr, window_surf, size.x, size.y);
		cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h);
		cairo_clip(cr);
		cairo_paint_with_alpha(cr, opacity);
		cairo_restore(cr);
	}
}

box_int_t window_t::get_absolute_extend() {
	return size;
}

void window_t::reconfigure(box_int_t const & area) {
	move_resize(area);
}

void window_t::mark_dirty() {
	cairo_surface_flush(window_surf);
	cairo_surface_mark_dirty(window_surf);
}

void window_t::mark_dirty_retangle(box_int_t const & area) {
	cairo_surface_flush(window_surf);
	cairo_surface_mark_dirty_rectangle(window_surf, area.x, area.y, area.w,
			area.h);
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

	size.x = e.x;
	size.y = e.y;
	size.w = e.width;
	size.h = e.height;

	cairo_surface_flush(window_surf);
	cairo_xlib_surface_set_size(window_surf, size.w, size.h);
	mark_dirty();

}

void window_t::process_configure_request_event(
		XConfigureRequestEvent const & e) {
	assert(e.window == xwin);

	size.x = e.x;
	size.y = e.y;
	size.w = e.width;
	size.h = e.height;

	cairo_surface_flush(window_surf);
	cairo_xlib_surface_set_size(window_surf, size.w, size.h);
	mark_dirty();

}

}
