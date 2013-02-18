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
	PAGE_OVERLAY_WINDOW_TYPE,
	PAGE_FLOATING_WINDOW_TYPE,
	PAGE_INPUT_ONLY_TYPE,
	PAGE_NOTIFY_TYPE
};

class window_t {

protected:
	typedef std::set<Atom> atom_set_t;
	typedef std::list<Atom> atom_list_t;
	typedef Window window_key_t;
	typedef std::map<window_key_t, window_t *> window_map_t;

	xconnection_t & cnx;
	Window xwin;
	XSizeHints * wm_normal_hints;
	XWMHints * wm_hints;

	int lock_count;

	/* store the ICCCM WM_STATE : WithDraw, Iconic or Normal */
	long wm_state;

	atom_list_t net_wm_type;
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
	bool has_wm_normal_hints;
	bool has_transient_for;
	bool has_wm_state;

	/* the name of window */
	std::string wm_name;
	std::string net_wm_name;

	/* window surface */
	long partial_struct[12];

	Window _transient_for;

	long user_time;

	unsigned int net_wm_icon_size;
	long * net_wm_icon_data;

	std::set<window_t *> sibbling_childs;

	box_int_t position;				/* window position */
    int border_width;				/* border width of window */
    int depth;          			/* depth of window */
    Visual * visual;				/* the associated visual structure */
    Window root;					/* root of screen containing window */
    int c_class;					/* C++ InputOutput, InputOnly*/
    int bit_gravity;				/* one of bit gravity values */
    int win_gravity;				/* one of the window gravity values */
    bool save_under;				/* boolean, should bits under be saved? */
    Colormap colormap;				/* color map to be associated with window */
    bool map_installed;				/* boolean, is color map currently installed*/
    int map_state;					/* IsUnmapped, IsUnviewable, IsViewable */
    Screen *screen;					/* back pointer to correct screen */

    bool _overide_redirect;
    Window above;

    page_window_type_e type;

private:
	/* avoid copy */
	window_t(window_t const &);
	window_t & operator=(window_t const &);
public:
	window_t(xconnection_t &cnx, Window w, XWindowAttributes const &wa);
	virtual ~window_t();

	long * get_properties32(Atom prop, Atom type, unsigned int *num) {
		return cnx.get_properties<long, 32>(xwin, prop, type, num);
	}

	short * get_properties16(Atom prop, Atom type, unsigned int *num) {
		return cnx.get_properties<short, 16>(xwin, prop, type, num);
	}

	char * get_properties8(Atom prop, Atom type, unsigned int *num) {
		return cnx.get_properties<char, 8>(xwin, prop, type, num);
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

	/* WRITING FOR CLIENT */
	void setup_extends();
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
	void move_resize(box_int_t const & location);


	box_int_t get_size();
	std::string get_title();

	XWMHints const * get_wm_hints();
	XSizeHints const * get_wm_normal_hints();

	void get_icon_data(long const *& data, int & size);

	void update_partial_struct();
	void read_all();

	bool is_dock();
	bool is_fullscreen();
	bool demands_atteniion();
	bool has_wm_type(Atom x);

	bool is_window(Window w);
	Window get_xwin();
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

	void add_sibbling_child(window_t * c);
	void remove_sibbling_child(window_t * c);
	std::set<window_t *> get_sibbling_childs();

	bool check_normal_hints_constraint(int width, int height);

	std::list<Atom> get_net_wm_type();

	bool get_has_wm_state();

	void notify_move_resize(box_int_t const & area);

	void iconify();
	void normalize();

	void print_net_wm_state();

	bool is_hidden();

	friend class renderable_window_t;
	friend class floating_window_t;

};

typedef std::list<window_t *> window_list_t;
typedef std::set<window_t *> window_set_t;

}

#endif /* WINDOW_HXX_ */
