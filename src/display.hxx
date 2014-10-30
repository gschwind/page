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

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include <xcb/xcb.h>

#include <X11/keysymdef.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <X11/extensions/shape.h>
#include <X11/extensions/Xdbe.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xcomposite.h>

#include <cstring>
#include <cstdarg>
#include <map>
#include <list>
#include <vector>
#include <memory>

#include "box.hxx"
#include "atoms.hxx"
#include "motif_hints.hxx"

#include "properties.hxx"

namespace page {

static unsigned long const AllEventMask = 0x01ffffff;

/**
 * Structure to handle X connection context.
 **/
class display_t {

	int _fd;
	Display * _dpy;
	xcb_connection_t * _xcb;
	xcb_screen_t * _screen;

	uint32_t _xcb_default_visual_depth;
	xcb_visualtype_t * _xcb_default_visual_type;

	uint32_t _xcb_root_visual_depth;
	xcb_visualtype_t * _xcb_root_visual_type;

	std::map<xcb_visualid_t, xcb_visualtype_t*> _xcb_visual_data;
	std::map<xcb_visualid_t, uint32_t> _xcb_visual_depth;

	std::list<XEvent> pending_event;

public:

	/* overlay composite */
	Window composite_overlay;

	std::shared_ptr<atom_handler_t> _A;


	int grab_count;
	int (*old_error_handler)(Display * _dpy, XErrorEvent * ev);

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

public:

	unsigned long int last_serial;

	int fd();
	Window root();
	Display * dpy();
	xcb_connection_t * xcb();
	xcb_visualtype_t * default_visual();
	xcb_visualtype_t * root_visual();

	xcb_screen_t * xcb_screen();

	/* conveniant macro to get atom XID */
	Atom A(atom_e atom);

	int screen();

public:


	display_t();
	~display_t();

	void set_net_active_window(xcb_window_t w);

	int move_window(Window w, int x, int y);
	void grab();
	void ungrab();
	bool is_not_grab();
	void unmap(Window w);
	void reparentwindow(xcb_window_t w, xcb_window_t parent, int x, int y);
	void map(xcb_window_t w);

	void xnextevent(XEvent * ev);

	bool register_wm(bool replace, Window w);
	bool register_cm(Window w);

	void add_to_save_set(Window w);
	void remove_from_save_set(Window w);
	void move_resize(Window w, i_rect const & size);
	void set_window_border_width(Window w, unsigned int width);
	void raise_window(Window w);

	template<typename T>
	int change_property(xcb_window_t w, atom_e property, atom_e type, int format, T data, int nelements) {
		cnx_printf("XChangeProperty: win = %lu\n", w);
		xcb_change_property(_xcb, XCB_PROP_MODE_REPLACE, w, A(property), A(type), format, nelements, reinterpret_cast<unsigned char const *>(data));
	}

	void delete_property(Window w, atom_e property);

	Status get_window_attributes(Window w,
			XWindowAttributes * window_attributes_return);

	Status get_text_property(Window w,
			XTextProperty * text_prop_return, atom_e property);
	int lower_window(Window w);
	int configure_window(Window w, unsigned int value_mask,
			XWindowChanges * values);

	/* used for debuging, do not optimize with some cache */
	std::shared_ptr<char> get_atom_name(Atom atom);

	Status send_event(Window w, Bool propagate, long event_mask,
			XEvent* event_send);
	int set_input_focus(Window focus, int revert_to, Time time);
	void fake_configure(Window w, i_rect location, int border_width);

	void cnx_printf(char const * str, ...) {
		if (false) {
			va_list args;
			va_start(args, str);
			char buffer[1024];
			unsigned long serial = XNextRequest(_dpy);
			snprintf(buffer, 1024, ">%08lu ", serial);
			unsigned int len = strnlen(buffer, 1024);
			vsnprintf(&buffer[len], 1024 - len, str, args);
			printf("%s", buffer);
		}
	}


	bool check_composite_extension(int * opcode, int * event, int * error);
	bool check_damage_extension(int * opcode, int * event, int * error);
	bool check_shape_extension(int * opcode, int * event, int * error);
	bool check_randr_extension(int * opcode, int * event, int * error);
	bool check_glx_extension(int * opcode, int * event, int * error);
	bool check_dbe_extension(int * opcode, int * event, int * error);


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

	bool check_for_reparent_window(Window w);
	bool check_for_fake_unmap_window(Window w);
	bool check_for_destroyed_window(Window w);
	bool check_for_unmap_window(Window w);

	void fetch_pending_events();

	XEvent * front_event();
	void pop_event();
	bool has_pending_events();

	void clear_events();

	void load_cursors();
	void unload_cursors();
	xcb_cursor_t _load_cursor(uint16_t cursor_id);

	void set_window_cursor(xcb_window_t w, xcb_cursor_t c);

	xcb_window_t create_input_only_window(xcb_window_t parent, i_rect const & pos, uint32_t attrs_mask, uint32_t * attrs);

	/** undocumented : http://lists.freedesktop.org/pipermail/xorg/2005-January/005954.html **/
	void allow_input_passthrough(Display * dpy, Window w);

	void disable_input_passthrough(Display * dpy, Window w);

	static int error_handler(Display * dpy, XErrorEvent * ev);

	template<atom_e name, atom_e type, typename xT >
	struct properties_fetcher_t {

		property_t<name, type, xT> & p;
		xcb_get_property_cookie_t ck;
		properties_fetcher_t(property_t<name, type, xT> & p, display_t * cnx, xcb_window_t w) : p(p) {
			ck = xcb_get_property(cnx->xcb(), 0, static_cast<xcb_window_t>(w), cnx->A(name), cnx->A(type), 0, std::numeric_limits<uint32_t>::max());
		}

		void update(display_t * cnx) {
			xcb_generic_error_t * err;
			xcb_get_property_reply_t * r = xcb_get_property_reply(cnx->xcb(), ck, &err);

			if(err != nullptr or r == nullptr) {
				if(r != nullptr)
					free(r);
				p = nullptr;
			} else if(r->length == 0 or r->format != property_helper_t<xT>::format) {
				if(r != nullptr)
					free(r);
				p = nullptr;
			} else {
				int length = xcb_get_property_value_length(r) /  (property_helper_t<xT>::format / 8);
				void * tmp = (xcb_get_property_value(r));
				xT * ret = property_helper_t<xT>::marshal(tmp, length);
				free(r);
				p = ret;
			}
		}

	};

	template<atom_e name, atom_e type, typename xT >
	void write_property(xcb_window_t w, property_t<name, type, xT> & p) {
		char * data;
		int length;
		property_helper_t<xT>::serialize(p.data, data, length);
		xcb_change_property(_xcb, XCB_PROP_MODE_REPLACE, w, name, type, property_helper_t<xT>::format, length, data);
		delete[] data;
	}

	template<atom_e name, atom_e type, typename xT>
	static properties_fetcher_t<name, type, xT> make_property_fetcher_t(property_t<name, type, xT> & p, display_t * cnx, xcb_window_t w) {
		return properties_fetcher_t<name, type, xT>{p, cnx, w};
	}


};

}


#endif /* XCONNECTION_HXX_ */
