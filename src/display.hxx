/*
 * display.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef DISPLAY_HXX_
#define DISPLAY_HXX_

#include "config.hxx"

#include <xcb/xcb.h>
#include <xcb/damage.h>
#include <xcb/composite.h>
#include <xcb/xfixes.h>
#include <xcb/randr.h>
#include <xcb/shape.h>

//#include <X11/keysymdef.h>
#include <X11/cursorfont.h>
//#include <X11/keysym.h>
//#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <cstring>
#include <cstdarg>
#include <map>
#include <list>
#include <vector>
#include <memory>
#include <iostream>

#include "box.hxx"
#include "region.hxx"
#include "utils.hxx"
#include "atoms.hxx"
#include "motif_hints.hxx"
#include "properties.hxx"

namespace page {

using namespace std;

class client_proxy_t;
class client_view_t;

static unsigned long const AllEventMask = 0x01ffffff;

/**
 * Structure to handle X connection context.
 **/
class display_t {

	int _fd;
	int _default_screen;

	xcb_connection_t * _xcb;
	xcb_screen_t * _screen;

	uint32_t _xcb_default_visual_depth;
	xcb_visualtype_t * _xcb_default_visual_type;
	xcb_visualtype_t * _xcb_root_visual_type;

	class map<xcb_visualid_t, xcb_visualtype_t*> _xcb_visual_data;
	class map<xcb_visualid_t, uint32_t> _xcb_visual_depth;
	list<xcb_generic_event_t *> pending_event;

	int _grab_count;

	class map<xcb_window_t, shared_ptr<client_proxy_t>> _client_proxies;

	bool _enable_composite;

public:

	char const * event_type_name[128];
	char const * error_type_name[256];

	/*damage event handler */
	int damage_opcode, damage_event, damage_error;

	/* composite event handler */
	int composite_opcode, composite_event, composite_error;
	bool has_composite;

	/* fixes event handler */
	int fixes_opcode, fixes_event, fixes_error;

	/* xshape extension handler */
	int shape_opcode, shape_event, shape_error;

	/* xrandr extension handler */
	int randr_opcode, randr_event, randr_error;

	int dbe_opcode, dbe_event, dbe_error;

	/* overlay composite */
	xcb_window_t composite_overlay;

	shared_ptr<atom_handler_t> _A;

	xcb_font_t cursor_font;

	xcb_cursor_t default_cursor;
	xcb_cursor_t xc_left_ptr;
	xcb_cursor_t xc_fleur;
	xcb_cursor_t xc_bottom_left_corner;
	xcb_cursor_t xc_bottom_righ_corner;
	xcb_cursor_t xc_bottom_side;
	xcb_cursor_t xc_left_side;
	xcb_cursor_t xc_right_side;
	xcb_cursor_t xc_top_right_corner;
	xcb_cursor_t xc_top_left_corner;
	xcb_cursor_t xc_top_side;
	xcb_cursor_t xc_sb_h_double_arrow;
	xcb_cursor_t xc_sb_v_double_arrow;

	xcb_atom_t wm_sn_atom;
	xcb_atom_t cm_sn_atom;

public:

	signal_t<xcb_window_t, bool> on_visibility_change;

	int fd();
	xcb_window_t root();
	xcb_connection_t * xcb();
	xcb_visualtype_t * default_visual_rgba();
	xcb_visualtype_t * root_visual();

	xcb_screen_t * xcb_screen();

	/* Convenient macro to get atom XID */
	xcb_atom_t A(atom_e atom);

	int screen();

	display_t();
	~display_t();

	void set_net_active_window(xcb_window_t w);

	void grab();
	void ungrab();
	bool is_not_grab();
	void unmap(xcb_window_t w);
	void reparentwindow(xcb_window_t w, xcb_window_t parent, int x, int y);
	void map(xcb_window_t w);

	bool register_wm(xcb_window_t w, bool replace);
	bool register_cm(xcb_window_t w);
	void unregister_cm();

	void add_to_save_set(xcb_window_t w);
	void remove_from_save_set(xcb_window_t w);
	void move_resize(xcb_window_t w, rect const & size);
	void set_window_border_width(xcb_window_t w, unsigned int width);
	void raise_window(xcb_window_t w);

	template<typename T>
	void change_property(xcb_window_t w, atom_e property, atom_e type, int format, T data, int nelements) {
		xcb_change_property(_xcb, XCB_PROP_MODE_REPLACE, w, A(property), A(type), format, nelements, reinterpret_cast<unsigned char const *>(data));
	}

	void delete_property(xcb_window_t w, atom_e property);
	void lower_window(xcb_window_t w);
	void set_input_focus(xcb_window_t focus, int revert_to, xcb_timestamp_t time);
	void fake_configure(xcb_window_t w, rect location, int border_width);

	bool check_composite_extension();
	bool check_damage_extension();
	bool check_shape_extension();
	bool check_randr_extension();
	bool check_xfixes_extension();
	bool check_glx_extension();
	bool check_dbe_extension();


	static void create_surf(char const * f, int l);
	static void destroy_surf(char const * f, int l);
	static int get_surf_count();

	static void create_context(char const * f, int l);
	static void destroy_context(char const * f, int l);
	static int get_context_count();

	void update_default_visual();

	static xcb_screen_t * screen_of_display (xcb_connection_t *c, int screen);

	xcb_visualtype_t * find_visual(xcb_visualid_t id);
	uint32_t find_visual_depth(xcb_visualid_t id);

	static void print_visual_type(xcb_visualtype_t * vis);

	bool check_for_reparent_window(xcb_window_t w);
	bool check_for_fake_unmap_window(xcb_window_t w);
	bool check_for_destroyed_window(xcb_window_t w);
	bool check_for_unmap_window(xcb_window_t w);

	void fetch_pending_events();

	xcb_generic_event_t * front_event();
	void pop_event();
	bool has_pending_events();

	void clear_events();

	list<xcb_generic_event_t *> const & get_pending_events_list();

	void load_cursors();
	void unload_cursors();
	xcb_cursor_t _load_cursor(uint16_t cursor_id);

	void set_window_cursor(xcb_window_t w, xcb_cursor_t c);

	xcb_window_t create_input_only_window(xcb_window_t parent, rect const & pos, uint32_t attrs_mask, uint32_t * attrs);

	/** undocumented : http://lists.freedesktop.org/pipermail/xorg/2005-January/005954.html **/
	void allow_input_passthrough(xcb_window_t w);

	void disable_input_passthrough(xcb_window_t w);

	bool query_extension(char const * name, int * opcode, int * event, int * error);

	void select_input(xcb_window_t w, uint32_t mask);
	void set_border_width(xcb_window_t w, uint32_t width);


	int root_depth() {
		return _screen->root_depth;
	}

	region read_damaged_region(xcb_damage_damage_t d);

	int get_visual_depth(xcb_visualid_t vid) {
		return _xcb_visual_depth[vid];
	}

	xcb_visualtype_t * get_visual_type(xcb_visualid_t vid) {
		return _xcb_visual_data[vid];
	}

	void check_x11_extension();

	xcb_atom_t get_atom(char const * name);
	void print_error(xcb_generic_error_t const * err);

	char const * get_event_name(uint8_t response_type);

	string const & get_atom_name(xcb_atom_t a) {
		return _A->name(a);
	}

	auto create_client_proxy(xcb_window_t w) -> shared_ptr<client_proxy_t>;

	void filter_events(xcb_generic_event_t const * e);

	void disable();
	void enable();
	void make_surface_stats(int & size, int & count);

	auto create_view(xcb_window_t w) -> shared_ptr<client_view_t>;
	void destroy_view(client_view_t * v);

};

}


#endif /* XCONNECTION_HXX_ */
