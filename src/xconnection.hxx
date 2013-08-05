/*
 * xconnection.hxx
 *
 * copyright (2011) Benoit Gschwind
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

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <algorithm>
#include <list>
#include <vector>
#include <stdexcept>
#include <map>
#include <cstring>

#include "box.hxx"
#include "atoms.hxx"
#include "x11_func_name.hxx"
#include "utils.hxx"
#include "properties_cache.hxx"
#include "window_attributes_cache.hxx"

//#define cnx_printf(args...) printf(args)

#define cnx_printf(args...) do { } while(false)

using namespace std;

namespace page {

struct event_t {
	unsigned long serial;
	int type;
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

	atom_handler_t _A;

private:
	/* that allow error_handler to bind display to connection */
	static std::map<Display *, xconnection_t *> open_connections;
	std::map<int, char const * const *> extension_request_name_map;

	properties_cache_t _pcache;
	window_attributes_cache_t _acache;

public:

	/* main screen */
	int _screen;
	/* the root window ID */
	Window xroot;

	std::list<event_t> pending;

	int grab_count;

	int (*old_error_handler)(Display * _dpy, XErrorEvent * ev);

	int connection_fd;

	std::list<xevent_handler_t *> event_handler_list;

	map<Atom, string> _atom_to_name;
	map<string, Atom> _name_to_atom;

public:

	xconnection_t();

	void grab();
	void ungrab();
	bool is_not_grab();

	static int error_handler(Display * dpy, XErrorEvent * ev);
	void allow_input_passthrough(Window w);

	void unmap(Window w);
	void reparentwindow(Window w, Window parent, int x, int y);
	void map_window(Window w);
	bool find_pending_event(event_t & e);

	unsigned long int last_serial;

	class serial_filter {
		unsigned long int last_serial;
	public:
		serial_filter(unsigned long int serial);
		bool operator() (event_t e);
	};

	void xnextevent(XEvent * ev);

	/* this fonction come from xcompmgr
	 * it is intend to make page as composite manager */
	bool register_cm(Window w);
	bool register_wm(bool replace, Window w);
	void add_to_save_set(Window w);
	void remove_from_save_set(Window w);
	void select_input(Window w, long int mask);
	void move_resize(Window w, box_int_t const & size);
	void set_window_border_width(Window w, unsigned int width);

	void raise_window(Window w);
	void add_event_handler(xevent_handler_t * func);
	void remove_event_handler(xevent_handler_t * func);
	void process_next_event();
	bool process_check_event();

	int change_property(Window w, atom_e property, atom_e type, int format,
			int mode, unsigned char const * data, int nelements);

	Status get_window_attributes(Window w,
			XWindowAttributes * window_attributes_return);

	Status get_text_property(Window w, XTextProperty * text_prop_return,
			atom_e property);

	int lower_window(Window w);

	int configure_window(Window w, unsigned int value_mask,
			XWindowChanges * values);


	Status send_event(Window w, Bool propagate, long event_mask,
			XEvent* event_send);

	int set_input_focus(Window focus, int revert_to, Time time);

	Window create_window(Visual * visual, int x, int y, unsigned w, unsigned h);

	void fake_configure(Window w, box_int_t location, int border_width);
	string const & get_atom_name(Atom a);

	void init_composite_overlay();

	void add_select_input(Window w, long mask);

	Atom get_atom(char const * name);

	string const & atom_to_name(Atom a);
	Atom const & name_to_atom(char const * name);

	int fd() {
		return connection_fd;
	}

	Window get_root_window() {
		return xroot;
	}

	Atom A(atom_e atom) {
		return _A(atom);
	}

	box_int_t get_window_position(Window w) {
		XWindowAttributes const * wa = _acache.get_window_attributes(dpy, w);
		if (wa != 0) {
			return box_int_t(wa->x, wa->y, wa->width, wa->height);
		} else {
			return box_int_t();
		}
	}

	box_int_t get_root_size() {
		return get_window_position(xroot);
	}

	int screen() {
		return _screen;
	}

	void process_cache_event(XEvent const & e);

private:
	char * _get_atom_name(Atom atom);
	static int _return_true(Display * dpy, XEvent * ev, XPointer none);

	XWMHints * _get_wm_hints(Window w);

public:

	XWindowAttributes const * get_window_attributes(Window w) {
		return _acache.get_window_attributes(dpy, w);
	}

	template<typename T>
	void write_window_property(Display * dpy, Window win, Atom prop,
			Atom type, vector<T> & v) {
		XChangeProperty(dpy, win, prop, type, _format<T>::size,
		PropModeReplace, (unsigned char *) &v[0], v.size());
	}

	template<typename T> bool read_list(Display * dpy, Window w, Atom prop,
			Atom type, list<T> * list = 0) {
		if (list == 0) {
			if (_pcache.get_window_property<T>(dpy, w, prop, type)) {
				return true;
			}
		} else {
			vector<long> tmp;
			if (_pcache.get_window_property(dpy, w, prop, type, &tmp)) {
				list->clear();
				list->insert(list->end(), tmp.begin(), tmp.end());
				return true;
			}
		}
		return false;
	}

	template<typename T> bool read_value(Display * dpy, Window w, Atom prop,
			Atom type, T * value = 0) {
		if (value == 0) {
			if (_pcache.get_window_property<T>(dpy, w, prop, type)) {
				return true;
			}
		} else {
			vector<long> tmp;
			if (_pcache.get_window_property(dpy, w, prop, type, &tmp)) {
				if (tmp.size() > 0) {
					*value = tmp[0];
					return true;
				}
			}
		}
		return false;
	}

	bool read_text(Display * dpy, Window w, Atom prop, string & value) {
		XTextProperty xname;
		XGetTextProperty(dpy, w, &xname, prop);
		if (xname.nitems != 0) {
			value = (char *)xname.value;
			XFree(xname.value);
			return true;
		}
		return false;
	}


	bool read_utf8_string(Display * dpy, Window w, Atom prop, string * value = 0) {
		if (value != 0) {
			vector<char> data;
			bool ret = _pcache.get_window_property(dpy, w, prop, A(UTF8_STRING), &data);
			data.resize(data.size() + 1, 0);
			*value = &data[0];
			return ret;
		} else {
			return _pcache.get_window_property<char>(dpy, w, prop, A(UTF8_STRING));
		}
	}


	bool read_wm_name(Window w, string & name) {
		return read_text(dpy, w, A(WM_NAME), name);
	}

	bool read_net_wm_name(Window w, string * name = 0) {
		return read_utf8_string(dpy, w, A(_NET_WM_NAME), name);
	}

	bool read_wm_state(Window w, long * value = 0) {
		return read_value(dpy, w, A(WM_STATE), A(WM_STATE), value);
	}

	bool read_wm_transient_for(Window w, Window * value = 0) {
		return read_value(dpy, w, A(WM_TRANSIENT_FOR), A(WINDOW), value);
	}

	bool read_net_wm_window_type(Window w, list<Atom> * list = 0) {
		bool ret = read_list(dpy, w, A(_NET_WM_WINDOW_TYPE), A(ATOM), list);
		return ret;
	}

	bool read_net_wm_state(Window w, list<Atom> * list = 0) {
		return read_list(dpy, w, A(_NET_WM_STATE), A(ATOM), list);
	}

	bool read_net_wm_protocols(Window w, list<Atom> * list = 0) {
		return read_list(dpy, w, A(WM_PROTOCOLS), A(ATOM), list);
	}

	bool read_net_wm_partial_struct(Window w,
			vector<long> * list = 0) {
		return _pcache.get_window_property(dpy, w, A(_NET_WM_STRUT_PARTIAL),
				A(CARDINAL), list);
	}

	bool read_net_wm_icon(Window w, vector<long> * list = 0) {
		return _pcache.get_window_property(dpy, w, A(_NET_WM_ICON), A(CARDINAL), list);
	}

	bool read_net_wm_user_time(Window w, long * value = 0) {
		return read_value(dpy, w, A(_NET_WM_USER_TIME), A(CARDINAL), value);
	}

	bool read_net_wm_desktop(Window w, long * value = 0) {
		return read_value(dpy, w, A(_NET_WM_DESKTOP), A(CARDINAL), value);
	}

	bool read_net_wm_allowed_actions(Window w, list<Atom> * list = 0) {
		return read_list(dpy, w, A(_NET_WM_ALLOWED_ACTIONS), A(ATOM), list);
	}

	struct wm_class {
		string res_name;
		string res_class;
	};

	bool read_wm_class(Window w, wm_class * res = 0) {
		if(res == 0) {
			return _pcache.get_window_property<char>(dpy, w, A(WM_CLASS), A(STRING));
		} else {
			vector<char> tmp;
			if(_pcache.get_window_property<char>(dpy, w, A(WM_CLASS), A(STRING), &tmp)) {
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
		if (_pcache.get_window_property<long>(dpy, w, A(WM_HINTS), A(WM_HINTS),
				&tmp)) {
			if (tmp.size() == 9) {
				if (hints) {
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

	bool read_wm_normal_hints(Window w, XSizeHints * size_hints = 0) {
		vector<long> tmp;
		if (_pcache.get_window_property<long>(dpy, w, A(WM_NORMAL_HINTS),
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
		write_window_property(dpy, xroot, A(_NET_ACTIVE_WINDOW), A(WINDOW), v);
	}

	int move_window(Window w, int x, int y) {
		XWindowAttributes * wa = _acache.get_window_attributes(dpy, w);
		wa->x = x;
		wa->y = y;
		return XMoveWindow(dpy, w, x, y);
	}

private:

	void update_process_configure_notify_event(XConfigureEvent const & e) {

		XWindowAttributes * wa = _acache.get_window_attributes(dpy, e.window);
		if (wa != 0) {
			wa->x = e.x;
			wa->y = e.y;
			wa->width = e.width;
			wa->height= e.height;
			wa->border_width = e.border_width;
			wa->override_redirect = e.override_redirect;
		}
	}

};

}


#endif /* XCONNECTION_HXX_ */
