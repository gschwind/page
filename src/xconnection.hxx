/*
 * xconnection.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef XCONNECTION_HXX_
#define XCONNECTION_HXX_

#include <X11/X.h>
#include <X11/Xutil.h>

#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include <glib.h>

#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <algorithm>
#include <list>
#include <vector>
#include <stdexcept>
#include <map>
#include <cstring>

#include "page_exception.hxx"

#include "box.hxx"
#include "atoms.hxx"
#include "x11_func_name.hxx"
#include "utils.hxx"
#include "properties_cache.hxx"
#include "window_attributes_cache.hxx"


using namespace std;

namespace page {


#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)

#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)

#define MWM_INPUT_MODELESS 0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL 2
#define MWM_INPUT_FULL_APPLICATION_MODAL 3
#define MWM_INPUT_APPLICATION_MODAL MWM_INPUT_PRIMARY_APPLICATION_MODAL

struct motif_wm_hints_t {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long input_mode;
	unsigned long status;
};

struct event_t {
	unsigned long serial;
	int type;

	event_t & operator=(event_t const & x) {
		if (this != &x) {
			this->serial = x.serial;
			this->type = x.type;
		}
		return *this;

	}

	event_t(event_t const & x) {
		this->serial = x.serial;
		this->type = x.type;
	}

	event_t(Display * dpy, int type) {
		this->serial = XNextRequest(dpy);
		this->type = type;
	}

	event_t(unsigned long serial, int type) {
		this->serial = serial;
		this->type = type;
	}
};

struct xevent_handler_t {
	virtual ~xevent_handler_t() {
	}
	virtual void process_event(XEvent const & e) = 0;
};


/**
 * Structure to handle X connection context.
 */
struct xconnection_t {

	Display * dpy;

	/* overlay composite */
	Window composite_overlay;

	/*damage event handler */
	int damage_opcode, damage_event, damage_error;

	/* composite event handler */
	int composite_opcode, composite_event, composite_error;

	/* fixes event handler */
	int fixes_opcode, fixes_event, fixes_error;

	/* xshape extension handler */
	int xshape_opcode, xshape_event, xshape_error;

	/* xrandr extension handler */
	int xrandr_opcode, xrandr_event, xrandr_error;

	/* GLX extension handler */
	int glx_opcode, glx_event, glx_error;

	atom_handler_t _A;

public:

	std::list<event_t> pending;

	int grab_count;

	int (*old_error_handler)(Display * _dpy, XErrorEvent * ev);

	int connection_fd;

	std::list<xevent_handler_t *> event_handler_list;

	map<Atom, string> _atom_to_name;
	map<string, Atom> _name_to_atom;

public:

	unsigned long int last_serial;

	class serial_filter {
		unsigned long int last_serial;
	public:
		serial_filter(unsigned long int serial) : last_serial(serial) {

		}

		bool operator() (event_t e) {
			return (e.serial < last_serial);
		}

	};

	int fd() {
		return connection_fd;
	}

	Window get_root_window() {
		return DefaultRootWindow(dpy);
	}

	Atom A(atom_e atom) {
		return _A(atom);
	}

	int screen() {
		return DefaultScreen(dpy);
	}

public:

	bool read_utf8_string(Display * dpy, Window w, Atom prop, string * value = 0) {
		if (value != 0) {
			vector<char> data;
			bool ret = ::page::get_window_property<char>(dpy, w, prop, A(UTF8_STRING), &data);
			data.resize(data.size() + 1, 0);
			*value = &data[0];
			return ret;
		} else {
			return ::page::get_window_property<char>(dpy, w, prop, A(UTF8_STRING));
		}
	}


	bool read_wm_name(Window w, string & name) {
		return ::page::read_text(dpy, w, A(WM_NAME), name);
	}

	bool read_net_wm_name(Window w, string * name = 0) {
		return read_utf8_string(dpy, w, A(_NET_WM_NAME), name);
	}

	bool read_wm_state(Window w, long * value = 0) {
		return ::page::read_value(dpy, w, A(WM_STATE), A(WM_STATE), value);
	}

	bool read_wm_transient_for(Window w, Window * value = 0) {
		return ::page::read_value(dpy, w, A(WM_TRANSIENT_FOR), A(WINDOW), value);
	}

	bool read_net_wm_window_type(Window w, list<Atom> * list = 0) {
		return ::page::read_list(dpy, w, A(_NET_WM_WINDOW_TYPE), A(ATOM), list);
	}

	bool read_net_wm_state(Window w, list<Atom> * list = 0) {
		return ::page::read_list(dpy, w, A(_NET_WM_STATE), A(ATOM), list);
	}

	bool read_net_wm_protocols(Window w, list<Atom> * list = 0) {
		return ::page::read_list(dpy, w, A(WM_PROTOCOLS), A(ATOM), list);
	}

	bool read_net_wm_partial_struct(Window w, vector<long> * list = 0) {
		return ::page::get_window_property<long>(dpy, w, A(_NET_WM_STRUT_PARTIAL), A(CARDINAL),
				list);
	}

	bool read_net_wm_icon(Window w, vector<long> * list = 0) {
		return ::page::get_window_property<long>(dpy, w, A(_NET_WM_ICON), A(CARDINAL), list);
	}

	bool read_net_wm_user_time(Window w, long * value = 0) {
		return ::page::read_value(dpy, w, A(_NET_WM_USER_TIME), A(CARDINAL), value);
	}

	bool read_net_wm_desktop(Window w, long * value = 0) {
		return ::page::read_value(dpy, w, A(_NET_WM_DESKTOP), A(CARDINAL), value);
	}

	bool read_net_wm_allowed_actions(Window w, list<Atom> * list = 0) {
		return ::page::read_list(dpy, w, A(_NET_WM_ALLOWED_ACTIONS), A(ATOM), list);
	}

	bool read_net_wm_user_time(Window w, Time * time) {
		return ::page::read_value(dpy, w, A(_NET_WM_USER_TIME), A(CARDINAL), time);
	}

	bool read_net_wm_user_time_window(Window w, Window * time_window) {
		return ::page::read_value(dpy, w, A(_NET_WM_USER_TIME_WINDOW), A(WINDOW), time_window);
	}


	struct wm_class {
		string res_name;
		string res_class;
	};

	bool read_wm_class(Window w, wm_class * res = 0) {
		if(res == 0) {
			return ::page::get_window_property<char>(dpy, w, A(WM_CLASS), A(STRING));
		} else {
			vector<char> tmp;
			if(::page::get_window_property<char>(dpy, w, A(WM_CLASS), A(STRING), &tmp)) {
				unsigned int x_name = strnlen(&tmp[0], tmp.size());
				unsigned int x_class = 0;
				if(x_name < tmp.size()) {
					x_class = strnlen(&tmp[x_name+1], tmp.size() - x_name - 1);
				}

				if(x_name + x_class + 1 < tmp.size()) {
					res->res_name = &tmp[0];
					res->res_class = &tmp[x_name+1];
					return true;
				}
			}
		}
		return false;
	}

	bool read_wm_hints(Window w, XWMHints * hints = 0) {
		vector<long> tmp;
		if (::page::get_window_property<long>(dpy, w, A(WM_HINTS), A(WM_HINTS),
				&tmp)) {
			if (tmp.size() == 9) {
				if (hints != 0) {
					hints->flags = tmp[0];
					hints->input = tmp[1];
					hints->initial_state = tmp[2];
					hints->icon_pixmap = tmp[3];
					hints->icon_window = tmp[4];
					hints->icon_x = tmp[5];
					hints->icon_y = tmp[6];
					hints->icon_mask = tmp[7];
					hints->window_group = tmp[8];
				}
				return true;
			}
		}
		return false;
	}

	bool read_motif_wm_hints(Window w, motif_wm_hints_t * hints = 0) {
		vector<long> tmp;
		if (::page::get_window_property<long>(dpy, w, A(_MOTIF_WM_HINTS), A(_MOTIF_WM_HINTS),
				&tmp)) {
			if (tmp.size() == 5) {
				if (hints != 0) {
					hints->flags = tmp[0];
					hints->functions = tmp[1];
					hints->decorations = tmp[2];
					hints->input_mode = tmp[3];
					hints->status = tmp[4];
				}
				return true;
			}
		}
		return false;
	}

	bool read_wm_normal_hints(Window w, XSizeHints * size_hints = 0) {
		vector<long> tmp;
		if (::page::get_window_property<long>(dpy, w, A(WM_NORMAL_HINTS),
				A(WM_SIZE_HINTS), &tmp)) {

			if (tmp.size() == 18) {
				if (size_hints) {
					size_hints->flags = tmp[0];
					size_hints->x = tmp[1];
					size_hints->y = tmp[2];
					size_hints->width = tmp[3];
					size_hints->height = tmp[4];
					size_hints->min_width = tmp[5];
					size_hints->min_height = tmp[6];
					size_hints->max_width = tmp[7];
					size_hints->max_height = tmp[8];
					size_hints->width_inc = tmp[9];
					size_hints->height_inc = tmp[10];
					size_hints->min_aspect.x = tmp[11];
					size_hints->min_aspect.y = tmp[12];
					size_hints->max_aspect.x = tmp[13];
					size_hints->max_aspect.y = tmp[14];
					size_hints->base_width = tmp[15];
					size_hints->base_height = tmp[16];
					size_hints->win_gravity = tmp[17];
				}
				return true;
			}
		}
		return false;
	}

	void write_net_wm_allowed_actions(Window w, list<Atom> & list) {
		vector<long> v(list.begin(), list.end());
		write_window_property(dpy, w, A(_NET_WM_ALLOWED_ACTIONS), A(ATOM), v);
	}

	void write_net_wm_state(Window w, list<Atom> & list) {
		vector<long> v(list.begin(), list.end());
		write_window_property(dpy, w, A(_NET_WM_STATE), A(ATOM), v);
	}

	void write_wm_state(Window w, long state, Window icon) {
		vector<long> v(2);
		v[0] = state;
		v[1] = icon;
		write_window_property(dpy, w, A(WM_STATE), A(WM_STATE), v);
	}

	void write_net_active_window(Window w) {
		vector<long> v(1);
		v[0] = w;
		write_window_property(dpy, DefaultRootWindow(dpy), A(_NET_ACTIVE_WINDOW), A(WINDOW), v);
	}

	int move_window(Window w, int x, int y) {
		return XMoveWindow(dpy, w, x, y);
	}




	xconnection_t() {
		old_error_handler = XSetErrorHandler(error_handler);

		dpy = XOpenDisplay(0);
		if (dpy == NULL) {
			throw std::runtime_error("Could not open display");
		} else {
			cnx_printf("Open display : Success\n");
		}

		/** for testing **/
		//XSynchronize(dpy, True);

		connection_fd = ConnectionNumber(dpy);

		grab_count = 0;

		select_input(DefaultRootWindow(dpy), SubstructureNotifyMask);

		// Check if composite is supported.



		if (!check_composite_extension(dpy, &composite_opcode, &composite_event,
				&composite_error)) {
			throw std::runtime_error("X Server doesn't support Composite 0.4");
		}

		// check/init Damage.
		if (!check_damage_extension(dpy, &damage_opcode, &damage_event,
				&damage_error)) {
			throw std::runtime_error("Damage extension is not supported");
		}

		if (!check_shape_extension(dpy, &xshape_opcode, &xshape_event,
				&xshape_error)) {
			throw std::runtime_error(SHAPENAME " extension is not supported");
		}

		if (!check_randr_extension(dpy, &xrandr_opcode, &xrandr_event,
				&xrandr_error)) {
			throw std::runtime_error(RANDR_NAME " extension is not supported");
		}

		_A = atom_handler_t(dpy);

	}


	~xconnection_t() {

		_atom_to_name.clear();
		_name_to_atom.clear();

		XCloseDisplay(dpy);
	}

	void grab() {
		if (grab_count == 0) {
			cnx_printf("XGrabServer\n");
			XGrabServer(dpy);
			cnx_printf("XSync\n");
			XSync(dpy, False);
		}
		++grab_count;
	}

	void ungrab() {
		if (grab_count == 0) {
			fprintf(stderr, "TRY TO UNGRAB NOT GRABBED CONNECTION!\n");
			return;
		}
		--grab_count;
		if (grab_count == 0) {
			cnx_printf("XUngrabServer\n");
			XUngrabServer(dpy);
			cnx_printf("XFlush\n");
			XFlush(dpy);
		}
	}

	bool is_not_grab() {
		return grab_count == 0;
	}

	void unmap(Window w) {
		cnx_printf("X_UnmapWindow: win = %lu\n", w);
		pending.push_back(event_t(dpy, UnmapNotify));
		XUnmapWindow(dpy, w);
	}

	void reparentwindow(Window w, Window parent, int x, int y) {
		cnx_printf("Reparent serial: #%lu win: #%lu, parent: #%lu\n", w, parent);
		pending.push_back(event_t(dpy, UnmapNotify));
		XReparentWindow(dpy, w, parent, x, y);
	}

	void map_window(Window w) {
		cnx_printf("X_MapWindow: win = %lu\n", w);
		pending.push_back(event_t(dpy, MapNotify));
		XMapWindow(dpy, w);
	}

	bool find_pending_event(event_t e) {
		std::list<event_t>::iterator i = pending.begin();
		while (i != pending.end()) {
			if (e.type == (*i).type && e.serial == (*i).serial)
				return true;
			++i;
		}
		return false;
	}

	void xnextevent(XEvent * ev) {
		XNextEvent(dpy, ev);
		last_serial = ev->xany.serial;
		serial_filter filter(ev->xany.serial);
		std::remove_if(pending.begin(), pending.end(), filter);
	}

	/* this function come from xcompmgr
	 * it is intend to make page as composite manager */



	bool register_wm(bool replace, Window w) {
	    Atom wm_sn_atom;
	    Window current_wm_sn_owner;

	    static char wm_sn[] = "WM_Sxx";
	    snprintf(wm_sn, sizeof(wm_sn), "WM_S%d", DefaultScreen(dpy));
	    wm_sn_atom = XInternAtom(dpy, wm_sn, FALSE);

	    current_wm_sn_owner = XGetSelectionOwner(dpy, wm_sn_atom);
	    if (current_wm_sn_owner == w)
	        current_wm_sn_owner = None;
	    if (current_wm_sn_owner != None) {
	        if (!replace) {
	            printf("A window manager is already running on screen %d",
	            		DefaultScreen(dpy));
	            return false;
	        } else {
	            /* We want to find out when the current selection owner dies */
	            XSelectInput(dpy, current_wm_sn_owner, StructureNotifyMask);
	            XSync(dpy, FALSE);

	            XSetSelectionOwner(dpy, wm_sn_atom, w, CurrentTime);

	            if (XGetSelectionOwner(dpy, wm_sn_atom) != w) {
	                printf("Could not acquire window manager selection on screen %d",
	                		DefaultScreen(dpy));
	                return false;
	            }

	            /* Wait for old window manager to go away */
	            if (current_wm_sn_owner != None) {
	              unsigned long wait = 0;
	              const unsigned long timeout = G_USEC_PER_SEC * 15; /* wait for 15s max */

	              XEvent ev;

	              while (wait < timeout) {
	                  /* Checks the local queue and incoming events for this event */
	                  if (XCheckTypedWindowEvent(dpy, current_wm_sn_owner, DestroyNotify, &ev) == True)
	                      break;
	                  g_usleep(G_USEC_PER_SEC / 10);
	                  wait += G_USEC_PER_SEC / 10;
	              }

	              if (wait >= timeout) {
	                  printf("The WM on screen %d is not exiting", DefaultScreen(dpy));
	                  return false;
	              }
	            }

	        }

	    } else {
	        XSetSelectionOwner(dpy, wm_sn_atom, w, CurrentTime);

	        if (XGetSelectionOwner(dpy, wm_sn_atom) != w) {
	            printf("Could not acquire window manager selection on screen %d",
	            		DefaultScreen(dpy));
	            return false;
	        }
	    }

	    return true;
	}


	void add_to_save_set(Window w) {
		cnx_printf("XAddToSaveSet: win = %lu\n", w);
		XAddToSaveSet(dpy, w);
	}

	void remove_from_save_set(Window w) {
		cnx_printf("XRemoveFromSaveSet: win = %lu\n", w);
		XRemoveFromSaveSet(dpy, w);
	}

	void select_input(Window w, long int mask) {

		cnx_printf("XSelectInput: win = %lu, mask = %08lx\n", w,
				(unsigned long) mask);
		/** add StructureNotifyMask and PropertyNotify to manage properties cache **/
		mask |= StructureNotifyMask | PropertyChangeMask;
		XSelectInput(dpy, w, mask);
	}

	void move_resize(Window w, box_int_t const & size) {

		cnx_printf("XMoveResizeWindow: win = %lu, %dx%d+%d+%d\n", w,
				size.w, size.h, size.x, size.y);

		XMoveResizeWindow(dpy, w, size.x, size.y, size.w, size.h);

	}

	void set_window_border_width(Window w, unsigned int width) {

		cnx_printf("XSetWindowBorderWidth: win = %lu, width = %u\n", w,
				width);
		pending.push_back(event_t(dpy, ConfigureNotify));
		XSetWindowBorderWidth(dpy, w, width);
	}

	void raise_window(Window w) {
		cnx_printf("XRaiseWindow: win = %lu\n", w);
		pending.push_back(event_t(dpy, ConfigureNotify));
		XRaiseWindow(dpy, w);
	}

	void add_event_handler(xevent_handler_t * func) {
		event_handler_list.push_back(func);
	}

	void remove_event_handler(xevent_handler_t * func) {
		page_assert(
				std::find(event_handler_list.begin(), event_handler_list.end(), func) != event_handler_list.end());
		event_handler_list.remove(func);
	}

	void process_next_event() {
		XEvent e;
		xnextevent(&e);

		/* since event handler can be removed on event, we copy it
		 * and check for event removed each time.
		 */
		std::vector<xevent_handler_t *> v(event_handler_list.begin(),
				event_handler_list.end());
		for (unsigned int i = 0; i < v.size(); ++i) {
			if (std::find(event_handler_list.begin(), event_handler_list.end(),
					v[i]) != event_handler_list.end()) {
				v[i]->process_event(e);
			}
		}
	}

	int _return_true(Display * dpy, XEvent * ev, XPointer none) {
		return True;
	}
	/**
	 * Check event without blocking.
	 **/
	bool process_check_event() {
		XEvent e;

		int x;
		/** Passive check of events in queue, never flush any thing **/
		if ((x = XEventsQueued(dpy, QueuedAlready)) > 0) {
			/** should not block or flush **/
			xnextevent(&e);

			/**
			 * since event handler can be removed on event, we copy it
			 * and check for event removed each time.
			 **/
			std::vector<xevent_handler_t *> v(event_handler_list.begin(),
					event_handler_list.end());
			for (unsigned int i = 0; i < v.size(); ++i) {
				if (std::find(event_handler_list.begin(), event_handler_list.end(),
						v[i]) != event_handler_list.end()) {
					v[i]->process_event(e);
				}
			}

			return true;
		} else {
			return false;
		}
	}

	int change_property(Window w, atom_e property, atom_e type,
			int format, int mode, unsigned char const * data, int nelements) {

		cnx_printf("XChangeProperty: win = %lu\n", w);
		return XChangeProperty(dpy, w, A(property), A(type), format, mode, data,
				nelements);
	}

	Status get_window_attributes(Window w,
			XWindowAttributes * window_attributes_return) {
		cnx_printf("XGetWindowAttributes: win = %lu\n", w);
		return XGetWindowAttributes(dpy, w, window_attributes_return);
	}

	Status get_text_property(Window w,
			XTextProperty * text_prop_return, atom_e property) {

		cnx_printf("XGetTextProperty: win = %lu\n", w);
		return XGetTextProperty(dpy, w, text_prop_return, A(property));
	}

	int lower_window(Window w) {

		cnx_printf("XLowerWindow: win = %lu\n", w);
		return XLowerWindow(dpy, w);
	}

	int configure_window(Window w, unsigned int value_mask,
			XWindowChanges * values) {

		cnx_printf("XConfigureWindow: win = %lu\n", w);
		if(value_mask & CWX)
			cnx_printf("has x: %d\n", values->x);
		if(value_mask & CWY)
			cnx_printf("has y: %d\n", values->y);
		if(value_mask & CWWidth)
			cnx_printf("has width: %d\n", values->width);
		if(value_mask & CWHeight)
			cnx_printf("has height: %d\n", values->height);

		if(value_mask & CWSibling)
			cnx_printf("has sibling: %lu\n", values->sibling);
		if(value_mask & CWStackMode)
			cnx_printf("has stack mode: %d\n", values->stack_mode);

		if(value_mask & CWBorderWidth)
			cnx_printf("has border: %d\n", values->border_width);
		return XConfigureWindow(dpy, w, value_mask, values);
	}

	char * _get_atom_name(Atom atom) {

		cnx_printf("XGetAtomName: atom = %lu\n", atom);
		return XGetAtomName(dpy, atom);
	}

	Status send_event(Window w, Bool propagate, long event_mask,
			XEvent* event_send) {

		cnx_printf("XSendEvent: win = %lu\n", w);
		return XSendEvent(dpy, w, propagate, event_mask, event_send);
	}

	int set_input_focus(Window focus, int revert_to, Time time) {
		cnx_printf("XSetInputFocus: win = %lu, time = %lu\n", focus, time);
		fflush(stdout);
		return XSetInputFocus(dpy, focus, revert_to, time);
	}

	void fake_configure(Window w, box_int_t location, int border_width) {
		XEvent ev;
		ev.xconfigure.type = ConfigureNotify;
		ev.xconfigure.display = dpy;
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

	string const & get_atom_name(Atom a) {
		return atom_to_name(a);
	}

	Atom get_atom(char const * name) {
		return name_to_atom(name);
	}

	string const & atom_to_name(Atom atom) {

		map<Atom, string>::iterator i = _atom_to_name.find(atom);
		if(i != _atom_to_name.end()) {
			return i->second;
		} else {
			char * name = _get_atom_name(atom);
			if(name == 0) {
				stringstream os;
				os << "UNKNOWN_" << hex << atom;
				_name_to_atom[os.str()] = atom;
				_atom_to_name[atom] = os.str();
			} else {
				string sname(name);
				_name_to_atom[sname] = atom;
				_atom_to_name[atom] = sname;
				XFree(name);
			}
			return _atom_to_name[atom];
		}
	}

	Atom const & name_to_atom(char const * name) {
		static Atom none = None;
		if(name == 0)
			return none;

		map<string, Atom>::iterator i = _name_to_atom.find(string(name));
		if(i != _name_to_atom.end()) {
			return i->second;
		} else {
			Atom a = XInternAtom(dpy, name, False);
			_name_to_atom[string(name)] = a;
			_atom_to_name[a] = string(name);
			return _name_to_atom[string(name)];
		}
	}

	bool motif_has_border(Window w) {
		motif_wm_hints_t motif_hints;
		if (read_motif_wm_hints(w, &motif_hints)) {
			if (motif_hints.flags & MWM_HINTS_DECORATIONS) {
				if (not (motif_hints.decorations & MWM_DECOR_BORDER)
						and not ((motif_hints.decorations & MWM_DECOR_ALL))) {
					return false;
				}
			}
		}
		return true;
	}

private:

	void cnx_printf(char const * str, ...) {
		if (false) {
			va_list args;
			va_start(args, str);
			char buffer[1024];
			unsigned long serial = XNextRequest(dpy);
			snprintf(buffer, 1024, ">%08lu ", serial);
			unsigned int len = strnlen(buffer, 1024);
			vsnprintf(&buffer[len], 1024 - len, str, args);
			printf("%s", buffer);
		}
	}

};

}


#endif /* XCONNECTION_HXX_ */
