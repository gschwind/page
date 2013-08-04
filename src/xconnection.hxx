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

#include "x11_func_name.hxx"
#include "utils.hxx"
#include "window.hxx"
#include "properties_cache.hxx"

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

public:

	/* main screen */
	int _screen;
	/* the root window ID */
	Window xroot;

	/* root window atributes */
	XWindowAttributes root_wa;
	/* size of default root window */
	box_t<int> root_size;

	std::list<event_t> pending;

	int grab_count;

	int (*old_error_handler)(Display * _dpy, XErrorEvent * ev);

	int connection_fd;

	std::list<xevent_handler_t *> event_handler_list;

	map<Atom, string> _atom_to_name;
	map<string, Atom> _name_to_atom;

	map<Window, window_t *> window_cache;

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

	window_t * get_window_t(Window w) {
		map<Window, window_t *>::iterator i = window_cache.find(w);
		if(i != window_cache.end()) {
			return i->second;
		} else {
			window_t * x = new window_t(w);
			XShapeSelectInput(dpy, w, ShapeNotifyMask);
			/** add our mask to update properties cache **/
			unsigned long mask = x->get_window_attributes().your_event_mask;
			window_cache[w] = x;
			return x;
		}
	}

	void destroy_cache(Window w) {
		map<Window, window_t *>::iterator i = window_cache.find(w);
		if(i != window_cache.end()) {
			delete i->second;
			window_cache.erase(i);
		}
	}

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
		XWindowAttributes wa;
		XGetWindowAttributes(dpy, w, &wa);
		return box_int_t(wa.x, wa.y, wa.width, wa.height);
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

	bool has_window_attributes(Window w) {
		window_t * wc = get_window_t(w);
		update_window_attributes(wc);
		return wc->has_window_attributes();
	}

	XWindowAttributes const & get_window_attributes(Window w) {
		window_t * wc = get_window_t(w);
		update_window_attributes(wc);
		return wc->get_window_attributes();
	}


	template<typename T>
	void write_window_property(Display * dpy, Window win, Atom prop,
			Atom type, vector<T> & v) {

		printf("try write win = %lu, %lu(%lu)\n", win, prop, type);
		if(_pcache.miss() != 0)
		printf("HIT/MISS %f %d %d\n", (double)_pcache.hit()/(_pcache.miss()+_pcache.hit()), _pcache.hit(), _pcache.miss());
		fflush(stdout);

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


	bool read_wm_name(Window w, string & name) {
		return read_text(dpy, w, A(WM_NAME), name);
	}

	bool read_net_wm_name(Window w, string & name) {
		return read_text(dpy, w, A(_NET_WM_NAME), name);
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

	bool read_wm_state(Window w, long * value = 0) {
		return read_value(dpy, w, A(WM_STATE), A(WM_STATE), value);
	}

	bool read_wm_transient_for(Window w, Window * value = 0) {
		return read_value(dpy, w, A(WM_TRANSIENT_FOR), A(WINDOW), value);
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
				int x_name = strnlen(&tmp[0], tmp.size());
				int x_class = 0;
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

	void write_net_wm_allowed_actions(Display * dpy, Window w, list<Atom> & list) {
		vector<long> v(list.begin(), list.end());
		write_window_property(dpy, w, A(_NET_WM_ALLOWED_ACTIONS), A(ATOM), v);
	}

public:
	void write_net_wm_state(Display * dpy, Window w, list<Atom> & list) {
		vector<long> v(list.begin(), list.end());
		char const * prop = XGetAtomName(dpy, A(_NET_WM_STATE));
		printf("XXXX %s\n", prop);

		write_window_property(dpy, w, A(_NET_WM_STATE), A(ATOM), v);
	}

	void write_wm_state(Display * dpy, Window w, long state, Window icon) {
		vector<long> v(2);
		v[0] = state;
		v[1] = icon;
		write_window_property(dpy, w, A(WM_STATE), A(WM_STATE), v);
	}

private:

	void update_process_configure_notify_event(window_t * w, XConfigureEvent const & e) {
		assert(e.window == w->id);
		w->_window_attributes.value.x = e.x;
		w->_window_attributes.value.y = e.y;
		w->_window_attributes.value.width = e.width;
		w->_window_attributes.value.height = e.height;
		w->_window_attributes.value.border_width = e.border_width;
		w->_window_attributes.value.override_redirect = e.override_redirect;
	}

	void update_window_attributes(window_t * w) {
		if(!w->_window_attributes.is_durty)
			return;
		w->_window_attributes.is_durty = true;

		if(XGetWindowAttributes(dpy, w->id, &w->_window_attributes.value)) {
			w->_window_attributes.has_value = true;
		} else {
			w->_window_attributes.has_value = false;
		}
	}



};

}


#endif /* XCONNECTION_HXX_ */
