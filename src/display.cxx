/*
 * display.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "display.hxx"

#include <cassert>
#include <poll.h>
#include "time.hxx"

namespace page {

static int surf_count = 0;

void display_t::create_surf(char const * f, int l) {
	surf_count += 1;
	//printf("%s:%d surf_count = %d\n", f, l, surf_count);
}
void display_t::destroy_surf(char const * f, int l) {
	surf_count -= 1;
	//printf("%s:%d surf_count = %d\n", f, l, surf_count);
	assert(surf_count >= 0);
}

int display_t::get_surf_count() {
	return surf_count;
}

static int context_count = 0;

void display_t::create_context(char const * f, int l) {
	context_count += 1;
	//printf("%s:%d context_count = %d\n", f, l, context_count);
}
void display_t::destroy_context(char const * f, int l) {
	context_count -= 1;
	//printf("%s:%d context_count = %d\n", f, l, context_count);
	assert(context_count >= 0);
}

int display_t::get_context_count() {
	return context_count;
}


int display_t::fd() {
	return _fd;
}

Window display_t::root() {
	return DefaultRootWindow(_dpy);
}

/* conveniant macro to get atom XID */
Atom display_t::A(atom_e atom) {
	return (*_A)(atom);
}

int display_t::screen() {
	return DefaultScreen(_dpy);
}

vector<string> * display_t::read_wm_class(Window w) {
	vector<char> * tmp = ::page::get_window_property<char>(_dpy, w, A(WM_CLASS), A(STRING));

	if(tmp == nullptr)
		return nullptr;

	vector<char>::iterator x = find(tmp->begin(), tmp->end(), 0);
	if(x != tmp->end()) {
		vector<string> * ret = new vector<string>;
		ret->push_back(string(tmp->begin(), x));
		auto x1 = find(++x, tmp->end(), 0);
		ret->push_back(string(x, x1));
		delete tmp;
		return ret;
	}

	delete tmp;
	return 0;
}

XWMHints * display_t::read_wm_hints(Window w) {
	vector<long> * tmp = ::page::get_window_property<long>(_dpy, w, A(WM_HINTS),
			A(WM_HINTS));
	if (tmp != 0) {
		if (tmp->size() == 9) {
			XWMHints * hints = new XWMHints;
			if (hints != 0) {
				hints->flags = (*tmp)[0];
				hints->input = (*tmp)[1];
				hints->initial_state = (*tmp)[2];
				hints->icon_pixmap = (*tmp)[3];
				hints->icon_window = (*tmp)[4];
				hints->icon_x = (*tmp)[5];
				hints->icon_y = (*tmp)[6];
				hints->icon_mask = (*tmp)[7];
				hints->window_group = (*tmp)[8];
			}
			delete tmp;
			return hints;
		}
		delete tmp;
	}
	return 0;
}

motif_wm_hints_t * display_t::read_motif_wm_hints(Window w) {
	vector<long> * tmp = ::page::get_window_property<long>(_dpy, w,
			A(_MOTIF_WM_HINTS), A(_MOTIF_WM_HINTS));
	if (tmp != 0) {
		motif_wm_hints_t * hints = new motif_wm_hints_t;
		if (tmp->size() == 5) {
			if (hints != 0) {
				hints->flags = (*tmp)[0];
				hints->functions = (*tmp)[1];
				hints->decorations = (*tmp)[2];
				hints->input_mode = (*tmp)[3];
				hints->status = (*tmp)[4];
			}
			delete tmp;
			return hints;
		}
		delete tmp;
	}
	return 0;
}

XSizeHints * display_t::read_wm_normal_hints(Window w) {
	vector<long> * tmp = ::page::get_window_property<long>(_dpy, w,
			A(WM_NORMAL_HINTS), A(WM_SIZE_HINTS));
	if (tmp != 0) {
		if (tmp->size() == 18) {
			XSizeHints * size_hints = new XSizeHints;
			if (size_hints) {
				size_hints->flags = (*tmp)[0];
				size_hints->x = (*tmp)[1];
				size_hints->y = (*tmp)[2];
				size_hints->width = (*tmp)[3];
				size_hints->height = (*tmp)[4];
				size_hints->min_width = (*tmp)[5];
				size_hints->min_height = (*tmp)[6];
				size_hints->max_width = (*tmp)[7];
				size_hints->max_height = (*tmp)[8];
				size_hints->width_inc = (*tmp)[9];
				size_hints->height_inc = (*tmp)[10];
				size_hints->min_aspect.x = (*tmp)[11];
				size_hints->min_aspect.y = (*tmp)[12];
				size_hints->max_aspect.x = (*tmp)[13];
				size_hints->max_aspect.y = (*tmp)[14];
				size_hints->base_width = (*tmp)[15];
				size_hints->base_height = (*tmp)[16];
				size_hints->win_gravity = (*tmp)[17];
			}
			delete tmp;
			return size_hints;
		}
		delete tmp;
	}
	return 0;
}

void display_t::write_net_wm_allowed_actions(Window w, list<Atom> & list) {
	vector<long> v(list.begin(), list.end());
	write_window_property(_dpy, w, A(_NET_WM_ALLOWED_ACTIONS), A(ATOM), v);
}

void display_t::write_net_wm_state(Window w, list<Atom> & list) {
	vector<long> v(list.begin(), list.end());
	write_window_property(_dpy, w, A(_NET_WM_STATE), A(ATOM), v);
}

void display_t::write_wm_state(Window w, long state, Window icon) {
	vector<long> v(2);
	v[0] = state;
	v[1] = icon;
	write_window_property(_dpy, w, A(WM_STATE), A(WM_STATE), v);
}

void display_t::write_net_active_window(Window w) {
	vector<long> v(1);
	v[0] = w;
	write_window_property(_dpy, DefaultRootWindow(_dpy), A(_NET_ACTIVE_WINDOW),
			A(WINDOW), v);
}

int display_t::move_window(Window w, int x, int y) {
	//printf("XMoveWindow #%lu %d %d\n", w, x, y);
	return XMoveWindow(_dpy, w, x, y);
}

display_t::display_t() {
	old_error_handler = XSetErrorHandler(error_handler);

	_dpy = XOpenDisplay(0);
	if (_dpy == NULL) {
		throw std::runtime_error("Could not open display");
	} else {
		cnx_printf("Open display : Success\n");
	}

	_fd = ConnectionNumber(_dpy);

	grab_count = 0;

	_A = shared_ptr<atom_handler_t>(new atom_handler_t(_dpy));

}

display_t::~display_t() {
	XCloseDisplay(_dpy);
}

void display_t::grab() {
	if (grab_count == 0) {
		XGrabServer(_dpy);
		//XSync(_dpy, False);
	}
	++grab_count;
}

void display_t::ungrab() {
	if (grab_count == 0) {
		fprintf(stderr, "TRY TO UNGRAB NOT GRABBED CONNECTION!\n");
		return;
	}
	--grab_count;
	if (grab_count == 0) {
		/* Flush pending events, and wait for that are applied */
		XSync(_dpy, False);
		/* allow other client to make request to the server */
		XUngrabServer(_dpy);
		/* Ungrab the server immediately */
		XFlush(_dpy);
	}
}

bool display_t::is_not_grab() {
	return grab_count == 0;
}

void display_t::unmap(Window w) {
	cnx_printf("X_UnmapWindow: win = %lu\n", w);
	XUnmapWindow(_dpy, w);
}

void display_t::reparentwindow(Window w, Window parent, int x, int y) {
	cnx_printf("Reparent serial: #%lu win: #%lu, parent: #%lu\n", w, parent);
	XReparentWindow(_dpy, w, parent, x, y);
}

void display_t::map_window(Window w) {
	cnx_printf("X_MapWindow: win = %lu\n", w);
	XMapWindow(_dpy, w);
}

void display_t::xnextevent(XEvent * ev) {
	XNextEvent(_dpy, ev);
}

/* this function come from xcompmgr
 * it is intend to make page as composite manager */

bool display_t::register_wm(bool replace, Window w) {
	Atom wm_sn_atom;
	Window current_wm_sn_owner;

	static char wm_sn[] = "WM_Sxx";
	snprintf(wm_sn, sizeof(wm_sn), "WM_S%d", screen());
	wm_sn_atom = XInternAtom(_dpy, wm_sn, FALSE);

	current_wm_sn_owner = XGetSelectionOwner(_dpy, wm_sn_atom);
	if (current_wm_sn_owner == w)
		current_wm_sn_owner = None;
	if (current_wm_sn_owner != None) {
		if (!replace) {
			printf("A window manager is already running on screen %d",
					DefaultScreen(_dpy));
			return false;
		} else {
			/* We want to find out when the current selection owner dies */
			XSelectInput(_dpy, current_wm_sn_owner, StructureNotifyMask);
			XSync(_dpy, FALSE);

			XSetSelectionOwner(_dpy, wm_sn_atom, w, CurrentTime);

			if (XGetSelectionOwner(_dpy, wm_sn_atom) != w) {
				printf(
						"Could not acquire window manager selection on screen %d",
						screen());
				return false;
			}

			/* Wait for old window manager to go away */
			if (current_wm_sn_owner != None) {
				page::time_t end;
				page::time_t cur;

				struct pollfd fds[1];
				fds[0].fd = fd();
				fds[0].events = POLLIN | POLLOUT | POLLERR | POLLHUP;

				cur.get_time();
				end = cur + page::time_t(15L, 1000000000L);

				XEvent ev;

				while (cur < end) {
					int timeout = static_cast<int64_t>(end - cur) / 1000000000L;
					poll(fds, 1, timeout);
					/* Checks the local queue and incoming events for this event */
					if (XCheckTypedWindowEvent(_dpy, current_wm_sn_owner,
					DestroyNotify, &ev) == True)
						break;
					cur.get_time();
				}

				if (cur >= end) {
					printf("The WM on screen %d is not exiting", screen());
					return false;
				}
			}

		}
	} else {
		XSetSelectionOwner(_dpy, wm_sn_atom, w, CurrentTime);

		if (XGetSelectionOwner(_dpy, wm_sn_atom) != w) {
			printf("Could not acquire window manager selection on screen %d",
					screen());
			return false;
		}
	}

	return true;
}


/**
 * Register composite manager. if another one is in place just fail to take
 * the ownership.
 **/
bool display_t::register_cm(Window w) {
	Window current_cm;
	Atom a_cm;
	static char net_wm_cm[] = "_NET_WM_CM_Sxx";
	snprintf(net_wm_cm, sizeof(net_wm_cm), "_NET_WM_CM_S%d", screen());
	a_cm = XInternAtom(_dpy, net_wm_cm, False);

	/** read if there is a compositor **/
	current_cm = XGetSelectionOwner(_dpy, a_cm);
	if (current_cm != None) {
		printf("Another composite manager is running\n");
		return false;
	} else {

		/** become the compositor **/
		XSetSelectionOwner(_dpy, a_cm, w, CurrentTime);

		/** check is we realy are the current compositor **/
		if (XGetSelectionOwner(_dpy, a_cm) != w) {
			printf("Could not acquire window manager selection on screen %d\n",
					screen());
			return false;
		}

		printf("Composite manager is registered on screen %d\n", screen());
		return true;
	}
}

void display_t::add_to_save_set(Window w) {
	cnx_printf("XAddToSaveSet: win = %lu\n", w);
	XAddToSaveSet(_dpy, w);
}

void display_t::remove_from_save_set(Window w) {
	cnx_printf("XRemoveFromSaveSet: win = %lu\n", w);
	XRemoveFromSaveSet(_dpy, w);
}

void display_t::move_resize(Window w, rectangle const & size) {

	//printf("XMoveResizeWindow: win = %lu, %fx%f+%f+%f\n", w, size.w, size.h,
	//		size.x, size.y);

	XMoveResizeWindow(_dpy, w, size.x, size.y, size.w, size.h);

}

void display_t::set_window_border_width(Window w, unsigned int width) {
	cnx_printf("XSetWindowBorderWidth: win = %lu, width = %u\n", w, width);
	XSetWindowBorderWidth(_dpy, w, width);
}

void display_t::raise_window(Window w) {
	cnx_printf("XRaiseWindow: win = %lu\n", w);
	XRaiseWindow(_dpy, w);
}

void display_t::delete_property(Window w, atom_e property) {
	XDeleteProperty(_dpy, w, A(property));
}

Status display_t::get_window_attributes(Window w,
		XWindowAttributes * window_attributes_return) {
	cnx_printf("XGetWindowAttributes: win = %lu\n", w);
	return XGetWindowAttributes(_dpy, w, window_attributes_return);
}

Status display_t::get_text_property(Window w, XTextProperty * text_prop_return,
		atom_e property) {

	cnx_printf("XGetTextProperty: win = %lu\n", w);
	return XGetTextProperty(_dpy, w, text_prop_return, A(property));
}

int display_t::lower_window(Window w) {

	cnx_printf("XLowerWindow: win = %lu\n", w);
	return XLowerWindow(_dpy, w);
}

int display_t::configure_window(Window w, unsigned int value_mask,
		XWindowChanges * values) {

	cnx_printf("XConfigureWindow: win = %lu\n", w);
	if (value_mask & CWX)
		cnx_printf("has x: %d\n", values->x);
	if (value_mask & CWY)
		cnx_printf("has y: %d\n", values->y);
	if (value_mask & CWWidth)
		cnx_printf("has width: %d\n", values->width);
	if (value_mask & CWHeight)
		cnx_printf("has height: %d\n", values->height);

	if (value_mask & CWSibling)
		cnx_printf("has sibling: %lu\n", values->sibling);
	if (value_mask & CWStackMode)
		cnx_printf("has stack mode: %d\n", values->stack_mode);

	if (value_mask & CWBorderWidth)
		cnx_printf("has border: %d\n", values->border_width);
	return XConfigureWindow(_dpy, w, value_mask, values);
}

static void _safe_xfree(void * x) {
	if (x != NULL)
		XFree(x);
}

/* used for debuging, do not optimize with some cache */
shared_ptr<char> display_t::get_atom_name(Atom atom) {
	cnx_printf("XGetAtomName: atom = %lu\n", atom);
	return shared_ptr<char>(XGetAtomName(_dpy, atom), _safe_xfree);
}

Status display_t::send_event(Window w, Bool propagate, long event_mask,
		XEvent* event_send) {

	cnx_printf("XSendEvent: win = %lu\n", w);
	return XSendEvent(_dpy, w, propagate, event_mask, event_send);
}

int display_t::set_input_focus(Window focus, int revert_to, Time time) {
	cnx_printf("XSetInputFocus: win = %lu, time = %lu\n", focus, time);
	fflush(stdout);
	return XSetInputFocus(_dpy, focus, revert_to, time);
}

void display_t::fake_configure(Window w, rectangle location, int border_width) {
	XEvent ev;
	ev.xconfigure.type = ConfigureNotify;
	ev.xconfigure.display = _dpy;
	ev.xconfigure.event = w;
	ev.xconfigure.window = w;
	ev.xconfigure.send_event = True;

	/* if ConfigureRequest happen, override redirect is False */
	ev.xconfigure.override_redirect = False;
	ev.xconfigure.border_width = border_width;
	ev.xconfigure.above = None;

	/* send mandatory fake event */
	ev.xconfigure.x = location.x;
	ev.xconfigure.y = location.y;
	ev.xconfigure.width = location.w;
	ev.xconfigure.height = location.h;

	send_event(w, False, StructureNotifyMask, &ev);

	//ev.xconfigure.event = xroot;
	//send_event(xroot, False, SubstructureNotifyMask, &ev);

}

bool display_t::motif_has_border(Window w) {
	motif_wm_hints_t * motif_hints = read_motif_wm_hints(w);
	if (motif_hints != 0) {
		if (motif_hints->flags & MWM_HINTS_DECORATIONS) {
			if (not (motif_hints->decorations & MWM_DECOR_BORDER)
					and not ((motif_hints->decorations & MWM_DECOR_ALL))) {
				delete motif_hints;
				return false;
			}
		}
		delete motif_hints;
	}
	return true;
}

string * display_t::read_wm_name(Window w) {
	return ::page::read_text(_dpy, w, A(WM_NAME), A(STRING));
}

string * display_t::read_wm_icon_name(Window w) {
	return ::page::read_text(_dpy, w, A(WM_ICON_NAME), A(STRING));
}

vector<Window> * display_t::read_wm_colormap_windows(Window w) {
	return ::page::get_window_property<Window>(_dpy, w, A(WM_COLORMAP_WINDOWS),
			A(WINDOW));
}

string * display_t::read_wm_client_machine(Window w) {
	return ::page::read_text(_dpy, w, A(WM_CLIENT_MACHINE), A(STRING));
}

string * display_t::read_net_wm_name(Window w) {
	return ::page::read_text(_dpy, w, A(_NET_WM_NAME), A(UTF8_STRING));
}

string * display_t::read_net_wm_visible_name(Window w) {
	return ::page::read_text(_dpy, w, A(_NET_WM_VISIBLE_NAME), A(UTF8_STRING));
}

string * display_t::read_net_wm_icon_name(Window w) {
	return ::page::read_text(_dpy, w, A(_NET_WM_ICON_NAME), A(UTF8_STRING));
}

string * display_t::read_net_wm_visible_icon_name(Window w) {
	return ::page::read_text(_dpy, w, A(_NET_WM_VISIBLE_ICON_NAME),
			A(UTF8_STRING));
}

long * display_t::read_wm_state(Window w) {
	return ::page::read_value<long>(_dpy, w, A(WM_STATE), A(WM_STATE));
}

Window * display_t::read_wm_transient_for(Window w) {
	return ::page::read_value<Window>(_dpy, w, A(WM_TRANSIENT_FOR), A(WINDOW));
}

list<Atom> * display_t::read_net_wm_window_type(Window w) {
	return ::page::read_list<Atom>(_dpy, w, A(_NET_WM_WINDOW_TYPE), A(ATOM));
}

list<Atom> * display_t::read_net_wm_state(Window w) {
	return ::page::read_list<Atom>(_dpy, w, A(_NET_WM_STATE), A(ATOM));
}

list<Atom> * display_t::read_net_wm_protocols(Window w) {
	return ::page::read_list<Atom>(_dpy, w, A(WM_PROTOCOLS), A(ATOM));
}

vector<long> * display_t::read_net_wm_struct(Window w) {
	return ::page::get_window_property<long>(_dpy, w, A(_NET_WM_STRUT),
			A(CARDINAL));
}

vector<long> * display_t::read_net_wm_struct_partial(Window w) {
	return ::page::get_window_property<long>(_dpy, w, A(_NET_WM_STRUT_PARTIAL),
			A(CARDINAL));
}

vector<long> * display_t::read_net_wm_icon_geometry(Window w) {
	return ::page::get_window_property<long>(_dpy, w, A(_NET_WM_ICON_GEOMETRY),
			A(CARDINAL));
}

vector<long> * display_t::read_net_wm_icon(Window w) {
	return ::page::get_window_property<long>(_dpy, w, A(_NET_WM_ICON),
			A(CARDINAL));
}

vector<long> * display_t::read_net_wm_opaque_region(Window w) {
	return ::page::get_window_property<long>(_dpy, w, A(_NET_WM_OPAQUE_REGION),
			A(CARDINAL));
}

vector<long> * display_t::read_net_frame_extents(Window w) {
	return ::page::get_window_property<long>(_dpy, w, A(_NET_FRAME_EXTENTS),
			A(CARDINAL));
}

unsigned long * display_t::read_net_wm_desktop(Window w) {
	return ::page::read_value<unsigned long>(_dpy, w, A(_NET_WM_DESKTOP),
			A(CARDINAL));
}

unsigned long * display_t::read_net_wm_pid(Window w) {
	return ::page::read_value<unsigned long>(_dpy, w, A(_NET_WM_PID),
			A(CARDINAL));
}

unsigned long * display_t::read_net_wm_bypass_compositor(Window w) {
	return ::page::read_value<unsigned long>(_dpy, w,
			A(_NET_WM_BYPASS_COMPOSITOR), A(CARDINAL));
}

list<Atom> * display_t::read_net_wm_allowed_actions(Window w) {
	return ::page::read_list<Atom>(_dpy, w, A(_NET_WM_ALLOWED_ACTIONS), A(ATOM));
}

Time * display_t::read_net_wm_user_time(Window w) {
	return ::page::read_value<Time>(_dpy, w, A(_NET_WM_USER_TIME), A(CARDINAL));
}

Window * display_t::read_net_wm_user_time_window(Window w) {
	return ::page::read_value<Window>(_dpy, w, A(_NET_WM_USER_TIME_WINDOW),
			A(WINDOW));
}

Display * display_t::dpy() {
	return _dpy;
}

bool display_t::check_composite_extension(int * opcode, int * event,
		int * error) {
	// Check if composite is supported.
	if (XQueryExtension(_dpy, COMPOSITE_NAME, opcode, event, error)) {
		int major = 0, minor = 0; // The highest version we support
		XCompositeQueryVersion(_dpy, &major, &minor);
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

bool display_t::check_damage_extension(int * opcode, int * event, int * error) {
	if (!XQueryExtension(_dpy, DAMAGE_NAME, opcode, event, error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XDamageQueryVersion(_dpy, &major, &minor);
		printf("Damage Extension version %d.%d found\n", major, minor);
		printf("Damage error %d, Damage event %d\n", *error, *event);
		return true;
	}
}

bool display_t::check_shape_extension(int * opcode, int * event, int * error) {
	if (!XQueryExtension(_dpy, SHAPENAME, opcode, event, error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XShapeQueryVersion(_dpy, &major, &minor);
		printf("Shape Extension version %d.%d found\n", major, minor);
		return true;
	}
}

bool display_t::check_randr_extension(int * opcode, int * event, int * error) {
	if (!XQueryExtension(_dpy, RANDR_NAME, opcode, event, error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XRRQueryVersion(_dpy, &major, &minor);
		printf(RANDR_NAME " Extension version %d.%d found\n", major, minor);
		return true;
	}
}

bool display_t::check_glx_extension(int * opcode, int * event, int * error) {
	if (!XQueryExtension(_dpy, GLX_EXTENSION_NAME, opcode, event, error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XRRQueryVersion(_dpy, &major, &minor);
		printf(GLX_EXTENSION_NAME " Extension version %d.%d found\n", major,
				minor);
		return true;
	}
}

bool display_t::check_dbe_extension(int * opcode, int * event, int * error) {
	if (!XQueryExtension(_dpy, DBE_PROTOCOL_NAME, opcode, event, error)) {
		return false;
	} else {
		int major = 0, minor = 0;
		XRRQueryVersion(_dpy, &major, &minor);
		printf(DBE_PROTOCOL_NAME " Extension version %d.%d found\n", major,
				minor);
		return true;
	}
}

}

