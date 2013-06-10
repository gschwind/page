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

namespace page {

enum page_window_type_e {
	PAGE_UNKNOW_WINDOW_TYPE,
	PAGE_NORMAL_WINDOW_TYPE,
	PAGE_DESKTOP_WINDOW_TYPE,
	PAGE_OVERLAY_WINDOW_TYPE,
	PAGE_FLOATING_WINDOW_TYPE,
	PAGE_INPUT_ONLY_TYPE,
	PAGE_NOTIFY_TYPE,
	PAGE_DOCK_TYPE,
	PAGE_TOOLTIP
};


typedef pair<Display *, Window> window_id_t;

class window_t {

private:
	typedef std::set<Atom> atom_set_t;
	typedef std::list<Atom> atom_list_t;
	typedef Window window_key_t;
	typedef std::map<window_key_t, window_t *> window_map_t;

	xconnection_t & _cnx;

	int _lock_count;
	bool _is_lock;
	bool _is_managed;

	XSizeHints * _wm_normal_hints;
	XWMHints * _wm_hints;

	/* store the ICCCM WM_STATE : WithDraw, Iconic or Normal */
	long _wm_state;

	atom_list_t _net_wm_type;
	atom_set_t _net_wm_state;
	atom_set_t _net_wm_protocols;
	atom_set_t _net_wm_allowed_actions;

	bool _has_wm_name;
	bool _has_net_wm_name;
	bool _has_partial_struct;
	/* store if wm must do input focus */
	bool _wm_input_focus;

	bool _has_wm_normal_hints;
	bool _has_transient_for;
	bool _has_wm_state;
	bool _has_net_wm_desktop;

	int _net_wm_desktop;

	/* the name of window */
	std::string _wm_name;
	std::string _net_wm_name;

	/* window surface */
	long _partial_struct[12];

	Window _transient_for;

	long _net_wm_user_time;

	unsigned int _net_wm_icon_size;
	long * _net_wm_icon_data;

	XWindowAttributes _wa;

    Window _above;

    page_window_type_e _type;



private:
	/* avoid copy */
	window_t(window_t const &);
	window_t & operator=(window_t const &);
public:

	Window const id;

    box_int_t default_position;

	window_t(xconnection_t &cnx, Window w);
	virtual ~window_t();

	long * get_properties32(Atom prop, Atom type, unsigned int *num) {
		return _cnx.get_properties<long, 32>(id, prop, type, num);
	}

	short * get_properties16(Atom prop, Atom type, unsigned int *num) {
		return _cnx.get_properties<short, 16>(id, prop, type, num);
	}

	char * get_properties8(Atom prop, Atom type, unsigned int *num) {
		return _cnx.get_properties<char, 8>(id, prop, type, num);
	}

	/* READING FROM CLIENTS */
	void read_wm_hints();
	void read_wm_normal_hints();
	void read_vm_name();
	void read_net_vm_name();
	void read_net_wm_type();
	void read_net_wm_state();
	void read_net_wm_protocols();
	void read_partial_struct();
	void read_net_wm_user_time();
	void read_wm_state();
	void read_transient_for();
	void read_icon_data();
	void read_net_wm_desktop();

	/* try to read window atrribute
	 * @output: return true on success, return false otherwise.
	 */
	bool read_window_attributes();

	/* WRITING FOR CLIENT */
	void write_net_frame_extents();
	void set_fullscreen();
	void unset_fullscreen();
	void write_net_wm_state() const;
	void write_net_wm_allowed_actions();
	void write_wm_state(long state);
	void set_net_wm_state(Atom x);
	void unset_net_wm_state(Atom x);
	void toggle_net_wm_state(Atom x);
	void set_focused();
	void unset_focused();
	void set_default_action();
	void set_dock_action();

	/* window actions */
	bool try_lock();
	void unlock();
	void map();
	void unmap();
	void focus();
	void icccm_focus();
	void move_resize(box_int_t const & location);

	box_int_t get_size();

	string get_title();

	XWMHints const * get_wm_hints();
	XSizeHints const * get_wm_normal_hints();

	void get_icon_data(long const *& data, int & size);

	void update_partial_struct();
	void read_all();
	void read_when_mapped();

	bool is_dock();
	bool is_fullscreen();
	bool demands_atteniion();
	bool has_wm_type(Atom x);

	bool is_window(Window w);
	long get_wm_state();
	bool is_map();
	void delete_window(Time t);

	/* macro */
	void process_configure_notify_event(XConfigureEvent const & e);

	bool override_redirect();

	Window transient_for();

	void map_notify();

	void unmap_notify();

	void select_input(long int mask);

	void print_net_wm_window_type();

	bool is_input_only();
	void add_to_save_set();
	void remove_from_save_set();
	bool is_visible();
	long get_net_user_time();
	void apply_size_constraint();

	page_window_type_e find_window_type_pass1(xconnection_t const & cnx, Atom wm_window_type);
	page_window_type_e find_window_type(xconnection_t const & cnx, Window w, int c_class);

	page_window_type_e get_window_type();

	void update_vm_hints();

	long const * get_partial_struct();

	bool check_normal_hints_constraint(int width, int height);

	std::list<Atom> get_net_wm_type();

	bool get_has_wm_state();

	void notify_move_resize(box_int_t const & area);

	void iconify();
	void normalize();

	void print_net_wm_state();

	bool is_hidden();
	bool is_notification();

	int get_initial_state();

	void fake_configure(box_int_t location, int border_width);

	bool has_net_wm_desktop();

	void reparent(Window parent, int x, int y);

	cairo_surface_t * create_cairo_surface();

	Display * get_display();
	Visual * get_visual();
	int get_depth();

	void grab_button(int button);

	void set_managed(bool state);
	bool is_managed();

};

typedef std::list<window_t *> window_list_t;
typedef std::set<window_t *> window_set_t;

}

#endif /* WINDOW_HXX_ */
