/*
 * window.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef WINDOW_HXX_
#define WINDOW_HXX_

#include <X11/Xlib.h>
#include <set>
#include <map>
#include "xconnection.hxx"
#include "renderable.hxx"
#include "region.hxx"
#include "icon.hxx"

namespace page {

class window_t: public renderable_t {

protected:
	typedef std::set<Atom> atom_set_t;
	typedef Window window_key_t;
	typedef std::map<window_key_t, window_t *> window_map_t;

	xconnection_t & cnx;

	/* track created windows */
	static window_map_t created_window;

	Window xwin;

	/* as Xlib recommend do not use static data allocation
	 * and use XAllocSizeHints */
	XSizeHints * wm_normal_hints;

	XWMHints * wm_hints;

	Visual * visual;

	int lock_count;

	/* store the ICCCM WM_STATE : WithDraw, Iconic or Normal */
	long wm_state;

	atom_set_t net_wm_type;
	atom_set_t net_wm_state;
	atom_set_t net_wm_protocols;
	atom_set_t net_wm_allowed_actions;

	bool has_wm_name;
	bool has_net_wm_name;
	bool has_partial_struct;
	/* store if wm must do input focus */
	bool wm_input_focus;
	/* store the map/unmap stase from the point of view of PAGE */
	bool _is_map;
	bool is_lock;
	bool _override_redirect;
	bool _is_composite_redirected;

	/* the name of window */
	std::string name;
	std::string wm_name;
	std::string net_wm_name;

	box_int_t requested_size;
	/* the real size */
	box_int_t size;

	Damage damage;

	double opacity;

	/* window surface */
	cairo_surface_t * window_surf;
	long partial_struct[12];

	int w_class;

	Window _transient_for;

public:
	/* icon data */
	icon_t icon;
	/* icon surface */
	cairo_surface_t * icon_surf;

	long user_time;

private:
	/* avoid copy */
	window_t(window_t const &);
	window_t & operator=(window_t const &);
public:
	window_t(xconnection_t &cnx, Window w, XWindowAttributes const &wa);
	virtual ~window_t();

	void setup_extends();

	/* window actions */
	bool try_lock();
	void unlock();
	void map();
	void unmap();
	void focus();
	void move_resize(box_int_t const & location);

	void set_fullscreen();
	void unset_fullscreen();

	/* read window status */
	void update_net_wm_user_time();
	void update_icon();
	long read_wm_state();
	void read_wm_normal_hints();
	std::string const & read_vm_name();
	std::string const & read_net_vm_name();
	std::string const & read_title();
	void read_net_wm_type();
	void read_net_wm_state();
	void read_net_wm_protocols();
	long const * get_partial_struct();
	void update_partial_struct();
	void read_all();
	XWMHints const * read_wm_hints();
	void read_shape_clip();

	void write_net_wm_state() const;
	void write_net_wm_allowed_actions();
	void write_wm_state(long state);

	bool is_dock();
	bool is_fullscreen();
	bool demands_atteniion();
	bool has_wm_type(Atom x);

	void set_net_wm_state(Atom x);
	void unset_net_wm_state(Atom x);
	void toggle_net_wm_state(Atom x);

	inline bool is_window(Window w) {
		return w == xwin;
	}

	cairo_surface_t * get_surf() {
		return window_surf;
	}

	inline Window get_xwin() {
		return xwin;
	}

	long get_wm_state() {
		return wm_state;
	}

	bool is_map() {
		return _is_map;
	}

	void set_focused() {
		net_wm_state.insert(cnx.atoms._NET_WM_STATE_FOCUSED);
		write_net_wm_state();
	}

	void unset_focused() {
		net_wm_state.erase(cnx.atoms._NET_WM_STATE_FOCUSED);
		write_net_wm_state();
	}

	void delete_window(Time t) {
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

	void set_default_action() {
		net_wm_allowed_actions.clear();
		net_wm_allowed_actions.insert(cnx.atoms._NET_WM_ACTION_CLOSE);
		net_wm_allowed_actions.insert(cnx.atoms._NET_WM_ACTION_FULLSCREEN);
		write_net_wm_allowed_actions();
	}

	void set_dock_action() {
		net_wm_allowed_actions.clear();
		net_wm_allowed_actions.insert(cnx.atoms._NET_WM_ACTION_CLOSE);
		write_net_wm_allowed_actions();
	}

	XSizeHints const * get_size_hints() {
		return wm_normal_hints;
	}

	void create_render_context();
	void destroy_render_context();

	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual box_int_t get_absolute_extend();
	box_int_t get_requested_size();
	virtual region_t<int> get_area();
	virtual void reconfigure(box_int_t const & area);
	virtual void mark_dirty();
	virtual void mark_dirty_retangle(box_int_t const & area);

	/* macro */
	long * get_properties32(Atom prop, Atom type, unsigned int *num) {
		return cnx.get_properties<long, 32>(xwin, prop, type, num);
	}

	short * get_properties16(Atom prop, Atom type, unsigned int *num) {
		return cnx.get_properties<short, 16>(xwin, prop, type, num);
	}

	char * get_properties8(Atom prop, Atom type, unsigned int *num) {
		return cnx.get_properties<char, 8>(xwin, prop, type, num);
	}

	long * get_icon_data(unsigned int * n) {
		return get_properties32(cnx.atoms._NET_WM_ICON, cnx.atoms.CARDINAL, n);
	}

	std::string const & get_title() {
		return name;
	}

	bool is_popup() {
		return (net_wm_type.find(cnx.atoms._NET_WM_WINDOW_TYPE_POPUP_MENU)
				!= net_wm_type.end()
				|| net_wm_type.find(cnx.atoms._NET_WM_WINDOW_TYPE_DROPDOWN_MENU)
						!= net_wm_type.end()
				|| net_wm_type.find(cnx.atoms._NET_WM_WINDOW_TYPE_TOOLTIP)
						!= net_wm_type.end()
				|| net_wm_type.find(cnx.atoms._NET_WM_WINDOW_TYPE_SPLASH)
						!= net_wm_type.end()
				|| net_wm_type.find(cnx.atoms._NET_WM_WINDOW_TYPE_NOTIFICATION)
						!= net_wm_type.end());
	}

	bool is_normal() {
		return (net_wm_type.find(cnx.atoms._NET_WM_WINDOW_TYPE_NORMAL)
				!= net_wm_type.end()
				|| net_wm_type.find(cnx.atoms._NET_WM_WINDOW_TYPE_DESKTOP)
						!= net_wm_type.end()
				|| net_wm_type.find(cnx.atoms._NET_WM_WINDOW_TYPE_DIALOG)
						!= net_wm_type.end()
				|| net_wm_type.find(cnx.atoms._NET_WM_WINDOW_TYPE_UTILITY)
						!= net_wm_type.end());
	}

	void process_configure_notify_event(XConfigureEvent const & e);
	void process_configure_request_event(XConfigureRequestEvent const & e);

	bool override_redirect() {
		return _override_redirect;
	}

	Window transient_for() {
		return _transient_for;
	}

	void map_notify();

	void unmap_notify();

	static window_t * find_window(xconnection_t * cnx, Window w) {
		window_map_t::iterator i = created_window.find(w);
		if (i != created_window.end())
			return i->second;
		else
			return 0;
	}

	void select_input(long int mask) {
		cnx.select_input(xwin, mask);
	}

	void print_net_wm_window_type() {
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

	bool is_input_only() {
		return w_class == InputOnly;
	}

	void set_opacity(double x) {
		opacity = x;
		if (x > 1.0)
			x = 1.0;
		if (x < 0.0)
			x = 0.0;
	}

	void add_to_save_set() {
		cnx.add_to_save_set(xwin);
	}

	void remove_from_save_set() {
		cnx.remove_from_save_set(xwin);
	}

	bool is_visible() {
		return is_map() && !is_input_only();
	}

	long get_net_user_time() {
		return user_time;
	}

	Window update_transient_for();

};

typedef std::list<window_t *> window_list_t;
typedef std::set<window_t *> window_set_t;

}

#endif /* WINDOW_HXX_ */
