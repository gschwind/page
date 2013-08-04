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
		XWindowAttributes const & wa = get_window_attributes(w);
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


	XWindowAttributes const & get_window_attributes(Window w) {
		window_t * wc = get_window_t(w);
		update_window_attributes(wc);
		return get_window_t(w)->get_window_attributes();
	}

	window_t::wm_class_t const & get_wm_class(Window w) {
		window_t * wc = get_window_t(w);
		update_wm_class(wc);
		return get_window_t(w)->get_wm_class();
	}

	unsigned long const get_wm_state(Window w) {
		window_t * wc = get_window_t(w);
		update_wm_class(wc);
		return get_window_t(w)->get_wm_state();
	}

	string const & get_wm_name(Window w)  {
		window_t * wc = get_window_t(w);
		update_wm_name(wc);
		return get_window_t(w)->get_wm_name();
	}

	Window const & get_wm_transient_for(Window w)  {
		window_t * wc = get_window_t(w);
		update_wm_transient_for(wc);
		return get_window_t(w)->get_wm_transient_for();
	}

	XSizeHints const & get_wm_normal_hints(Window w)  {
		window_t * wc = get_window_t(w);
		update_wm_normal_hints(wc);
		return get_window_t(w)->get_wm_normal_hints();
	}

	XWMHints const * get_wm_hints(Window w)  {
		window_t * wc = get_window_t(w);
		update_wm_hints(wc);
		return get_window_t(w)->get_wm_hints();
	}

	string const & get_net_wm_name(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_name(wc);
		return get_window_t(w)->get_net_wm_name();
	}

	list<Atom> const & get_net_wm_window_type(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_window_type(wc);
		return get_window_t(w)->get_net_wm_window_type();
	}

	list<Atom> const & get_net_wm_state(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_state(wc);
		return get_window_t(w)->get_net_wm_state();
	}

	list<Atom> const & get_net_wm_protocols(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_protocols(wc);
		return get_window_t(w)->get_net_wm_protocols();
	}

	vector<long> const & get_net_wm_partial_struct(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_partial_struct(wc);
		return get_window_t(w)->get_net_wm_partial_struct();
	}

	long const & get_net_wm_desktop(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_desktop(wc);
		return get_window_t(w)->get_net_wm_desktop();
	}

	long const & get_net_wm_user_time(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_user_time(wc);
		return get_window_t(w)->get_net_wm_user_time();
	}

	vector<long> const & get_net_wm_icon(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_icon(wc);
		return get_window_t(w)->get_net_wm_icon();
	}

	region_t<int> const & get_shape_region(Window w)  {
		window_t * wc = get_window_t(w);
		update_shape_region(wc);
		return get_window_t(w)->get_shape_region();
	}

	bool has_window_attributes(Window w) {
		window_t * wc = get_window_t(w);
		update_window_attributes(wc);
		return wc->has_window_attributes();
	}

	bool has_wm_class(Window w) {
		window_t * wc = get_window_t(w);
		update_wm_class(wc);
		return get_window_t(w)->has_wm_class();
	}

	bool has_wm_state(Window w) {
		window_t * wc = get_window_t(w);
		update_wm_state(wc);
		return get_window_t(w)->has_wm_state();
	}

	bool has_wm_name(Window w)  {
		window_t * wc = get_window_t(w);
		update_wm_name(wc);
		return get_window_t(w)->has_wm_name();
	}

	bool has_wm_transient_for(Window w)  {
		window_t * wc = get_window_t(w);
		update_wm_transient_for(wc);
		return get_window_t(w)->has_wm_transient_for();
	}

	bool has_wm_normal_hints(Window w)  {
		window_t * wc = get_window_t(w);
		update_wm_normal_hints(wc);
		return get_window_t(w)->has_wm_normal_hints();
	}

	bool has_wm_hints(Window w)  {
		window_t * wc = get_window_t(w);
		update_wm_hints(wc);
		return get_window_t(w)->has_wm_hints();
	}

	bool has_net_wm_name(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_name(wc);
		return get_window_t(w)->has_net_wm_name();
	}

	bool has_net_wm_window_type(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_window_type(wc);
		return get_window_t(w)->has_net_wm_window_type();
	}

	bool has_net_wm_state(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_state(wc);
		return get_window_t(w)->has_net_wm_state();
	}

	bool has_net_wm_protocols(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_protocols(wc);
		return get_window_t(w)->has_net_wm_protocols();
	}

	bool has_net_wm_partial_struct(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_partial_struct(wc);
		return get_window_t(w)->has_net_wm_partial_struct();
	}

	bool has_net_wm_desktop(Window w) {
		window_t * wc = get_window_t(w);
		update_net_wm_desktop(wc);
		return get_window_t(w)->has_net_wm_desktop();
	}

	bool has_net_wm_user_time(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_user_time(wc);
		return get_window_t(w)->has_net_wm_user_time();
	}

	bool has_net_wm_icon(Window w)  {
		window_t * wc = get_window_t(w);
		update_net_wm_icon(wc);
		return get_window_t(w)->has_net_wm_icon();
	}

	bool has_shape_region(Window w)  {
		window_t * wc = get_window_t(w);
		update_shape_region(wc);
		return get_window_t(w)->has_shape_region();
	}

private:

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
			Atom type, list<T> & list) {
		vector<long> tmp;
		if (_pcache.get_window_property(dpy, w, prop, type, tmp)) {
			list.clear();
			list.insert(list.end(), tmp.begin(), tmp.end());
			return true;
		}
		return false;
	}

	template<typename T> bool read_value(Display * dpy, Window w, Atom prop,
			Atom type, T & value) {
		vector<long> tmp;
		if (_pcache.get_window_property(dpy, w, prop, type, tmp)) {
			if (tmp.size() > 0) {
				value = tmp[0];
				return true;
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


	bool read_wm_name(Display * dpy, Window w, string & name) {
		return read_text(dpy, w, A(WM_NAME), name);
	}

	bool read_net_wm_name(Display * dpy, Window w, string & name) {
		return read_text(dpy, w, A(_NET_WM_NAME), name);
	}

	bool read_net_wm_window_type(Display * dpy, Window w, list<Atom> & list) {
		bool ret = read_list(dpy, w, A(_NET_WM_WINDOW_TYPE), A(ATOM), list);
		printf("Atom read %lu\n", list.size());
		return ret;
	}

	bool read_net_wm_state(Display * dpy, Window w, list<Atom> & list) {
		return read_list(dpy, w, A(_NET_WM_STATE), A(ATOM), list);
	}

	bool read_net_wm_protocols(Display * dpy, Window w, list<Atom> & list) {
		return read_list(dpy, w, A(WM_PROTOCOLS), A(ATOM), list);
	}

	bool read_net_wm_partial_struct(Display * dpy, Window w,
			vector<long> & list) {
		return _pcache.get_window_property(dpy, w, A(_NET_WM_STRUT_PARTIAL),
				A(CARDINAL), list);
	}

	bool read_net_wm_icon(Display * dpy, Window w, vector<long> & list) {
		return _pcache.get_window_property(dpy, w, A(_NET_WM_ICON), A(CARDINAL), list);
	}

	bool read_net_wm_user_time(Display * dpy, Window w, long & value) {
		return read_value(dpy, w, A(_NET_WM_USER_TIME), A(CARDINAL), value);
	}

	bool read_net_wm_desktop(Display * dpy, Window w, long & value) {
		return read_value(dpy, w, A(_NET_WM_DESKTOP), A(CARDINAL), value);
	}

	bool read_wm_state(Display * dpy, Window w, long & value) {
		return read_value(dpy, w, A(WM_STATE), A(WM_STATE), value);
	}

	bool read_wm_transient_for(Display * dpy, Window w, Window & value) {
		return read_value(dpy, w, A(WM_TRANSIENT_FOR), A(WINDOW), value);
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

	void update_wm_hints(window_t * w) {
		if(!w->_wm_hints.is_durty)
			return;
		w->_wm_hints.is_durty = true;

		if(w->_wm_hints.value != 0)
			XFree(w->_wm_hints.value);
		w->_wm_hints.value = XGetWMHints(dpy, w->id);
	}

	void update_wm_normal_hints(window_t * w) {
		if(!w->_wm_normal_hints.is_durty)
			return;
		w->_wm_normal_hints.is_durty = true;

		long size_hints_flags;
		if (!XGetWMNormalHints(dpy, w->id, &w->_wm_normal_hints.value,
				&size_hints_flags)) {
			w->_wm_normal_hints.value.flags = 0;
			w->_wm_normal_hints.has_value = false;
		} else {
			w->_wm_normal_hints.has_value = true;
		}
	}


	void update_wm_name(window_t * w) {
		if(!w->_wm_name.is_durty)
			return;
		w->_wm_name.is_durty = true;

		w->_wm_name.has_value = read_wm_name(dpy, w->id, w->_wm_name.value);
	}

	void update_net_wm_name(window_t * w) {
		if(!w->_net_wm_name.is_durty)
			return;
		w->_net_wm_name.is_durty = true;

		w->_net_wm_name.has_value = read_wm_name(dpy, w->id, w->_net_wm_name.value);
	}


	void update_net_wm_window_type(window_t * w) {
		if (!w->_net_wm_window_type.is_durty)
			return;
		w->_net_wm_window_type.is_durty = true;

		w->_net_wm_window_type.has_value = read_net_wm_window_type(dpy, w->id,
				w->_net_wm_window_type.value);

	}

	void update_net_wm_state(window_t * w) {
		if (!w->_net_wm_state.is_durty)
			return;
		w->_net_wm_state.is_durty = true;

		w->_net_wm_state.has_value = read_net_wm_state(dpy, w->id,
				w->_net_wm_state.value);
	}



	void update_net_wm_protocols(window_t * w) {
		if(!w->_net_wm_protocols.is_durty)
			return;
		w->_net_wm_protocols.is_durty = true;

		w->_net_wm_protocols.has_value = read_net_wm_protocols(dpy, w->id,
				w->_net_wm_protocols.value);
	}

	void update_net_wm_partial_struct(window_t * w) {
		if(!w->_net_wm_partial_struct.is_durty)
			return;
		w->_net_wm_partial_struct.is_durty = true;

		w->_net_wm_partial_struct.has_value = read_net_wm_partial_struct(dpy, w->id,
				w->_net_wm_partial_struct.value);
	}

	void update_net_wm_user_time(window_t * w) {
		if(!w->_net_wm_user_time.is_durty)
			return;
		w->_net_wm_user_time.is_durty = true;

		w->_net_wm_user_time.has_value = read_net_wm_user_time(dpy, w->id,
				w->_net_wm_user_time.value);

	}


	void update_net_wm_desktop(window_t * w) {
		if(!w->_net_wm_desktop.is_durty)
			return;
		w->_net_wm_desktop.is_durty = true;

		w->_net_wm_desktop.has_value = read_net_wm_desktop(dpy, w->id,
				w->_net_wm_desktop.value);

	}

	void update_net_wm_icon(window_t * w) {
		if (!w->_net_wm_icon.is_durty)
			return;
		w->_net_wm_icon.is_durty = true;

		w->_net_wm_icon.has_value = read_net_wm_icon(dpy,
				w->id, w->_net_wm_icon.value);

	}

	void update_wm_state(window_t * w) {
		if(!w->_wm_state.is_durty)
			return;
		w->_wm_state.is_durty = true;

		w->_wm_state.has_value = read_wm_state(dpy, w->id, w->_wm_state.value);

	}

	void update_process_configure_notify_event(window_t * w, XConfigureEvent const & e) {
		assert(e.window == w->id);
		w->_window_attributes.value.x = e.x;
		w->_window_attributes.value.y = e.y;
		w->_window_attributes.value.width = e.width;
		w->_window_attributes.value.height = e.height;
		w->_window_attributes.value.border_width = e.border_width;
		w->_window_attributes.value.override_redirect = e.override_redirect;
	}

	void update_wm_transient_for(window_t * w) {
		if(!w->_wm_transient_for.is_durty)
			return;
		w->_wm_transient_for.is_durty = true;

		w->_wm_transient_for.has_value = read_wm_transient_for(dpy,
				w->id, w->_wm_transient_for.value);
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

	void update_shape_region(window_t * w) {
		if(!w->_shape_region.is_durty)
			return;
		w->_shape_region.is_durty = true;

		int count, ordering;
		XRectangle * recs = XShapeGetRectangles(dpy, w->id, ShapeBounding, &count, &ordering);

		w->_shape_region.value.clear();

		if(recs != NULL) {
			w->_shape_region.has_value = true;
			for(int i = 0; i < count; ++i) {
				w->_shape_region.value = w->_shape_region.value + box_int_t(recs[i]);
			}
			/* In doubt */
			XFree(recs);
		} else {
			w->_shape_region.has_value = false;
			w->_shape_region.value.clear();
		}
	}

	void update_wm_class(window_t * w) {
		if(!w->_wm_class.is_durty)
			return;
		w->_wm_class.is_durty = true;

		XClassHint wm_class_hint;
		if(XGetClassHint(dpy, w->id, &wm_class_hint) != 0) {
			w->_wm_class.has_value = true;
			w->_wm_class.value.res_class = wm_class_hint.res_class;
			w->_wm_class.value.res_name = wm_class_hint.res_name;
		} else {
			w->_wm_class.has_value = false;
			w->_wm_class.value.res_class.clear();
			w->_wm_class.value.res_name.clear();
		}
	}



};

}


#endif /* XCONNECTION_HXX_ */
