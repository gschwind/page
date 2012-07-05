/*
 * client.hxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

#ifndef CLIENT_HXX_
#define CLIENT_HXX_

#include <X11/Xmd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <cstring>
#include <string>
#include <list>
#include <set>
#include "atoms.hxx"
#include "icon.hxx"
#include "xconnection.hxx"

namespace page_next {

struct client_t {
	xconnection_t &cnx;
	Window xwin;
	XWindowAttributes wa;

	int lock_count;

	/* store the ICCCM WM_STATE : WithDraw, Iconic or Normal */
	long wm_state;

	std::set<Atom> type;
	std::set<Atom> net_wm_state;
	std::set<Atom> wm_protocols;
	std::set<Atom> net_wm_allowed_actions;

	/* the name of window */
	std::string name;

	bool wm_name_is_valid;
	std::string wm_name;
	bool net_wm_name_is_valid;
	std::string net_wm_name;
	//Window clipping_window;
	//Window page_window;

	Pixmap pix;

	/* store if wm must do input focus */
	bool wm_input_focus;
	/* store the map/unmap stase from the point of view of PAGE */
	bool is_map;
	bool is_lock;
	/* this is used to distinguish if unmap is initiated by client or by PAGE
	 * PAGE unmap mean Normal to Iconic
	 * client unmap mean Normal to WithDrawn */
	//int unmap_pending;
	Pixmap pixmap_icon;
	Window w_icon;

	/* the desired width/heigth */
	//int offset_x;
	//int offset_y;
	int width;
	int height;

	Damage damage;

	XSizeHints hints;

	icon_t icon;
	cairo_surface_t * icon_surf;

	cairo_surface_t * window_surf;

	box_int_t size;

	bool is_dock;
	bool has_partial_struct;
	long partial_struct[12];

	client_t(xconnection_t &cnx, Window w, XWindowAttributes &wa,
			long wm_state);
	~client_t();

	void map();
	void unmap();
	void update_client_size(int w, int h);

private:
	long get_window_state();

	bool try_lock_client();
	void unlock_client();
public:
	void focus();
	void fullscreen(int w, int h);
	void client_update_size_hints();
	void update_vm_name();
	void update_net_vm_name();
	void update_title();
	void init_icon();
	void update_type();
	void read_net_wm_state();
	void read_wm_protocols();
	void write_net_wm_state();

	void update_net_wm_allowed_actions();


	/* NOTE : ICCCM CARD32 mean "long" C type */
	void set_wm_state(long state) {
		wm_state = state;
		struct {
			long state;
			Window icon;
		} data;

		data.state = state;
		data.icon = None;
		XChangeProperty(cnx.dpy, xwin, cnx.atoms.WM_STATE, cnx.atoms.WM_STATE,
				32, PropModeReplace, reinterpret_cast<unsigned char *>(&data),
				2);
	}

	bool client_is_dock() {
		return type.find(cnx.atoms._NET_WM_WINDOW_TYPE_DOCK) != type.end();
	}

	bool is_fullscreen() {
		return net_wm_state.find(cnx.atoms._NET_WM_STATE_FULLSCREEN)
				!= net_wm_state.end();
	}

	void set_fullscreen();
	void unset_fullscreen();

	long * get_properties32(Atom prop, Atom type, unsigned int *num) {
		return cnx.get_properties<long, 32>(xwin, prop, type, num);
	}

	short * get_properties16(Atom prop, Atom type, unsigned int *num) {
		return cnx.get_properties<short, 16>(xwin, prop, type, num);
	}

	char * get_properties8(Atom prop, Atom type, unsigned int *num) {
		return cnx.get_properties<char, 8>(xwin, prop, type, num);
	}



	void update_all();

	void move_resize(box_int_t const & location) {
		size = location;
		XMoveResizeWindow(cnx.dpy, xwin, size.x, size.y, size.w, size.h);
		cairo_surface_flush(window_surf);
		cairo_xlib_surface_set_size(window_surf, size.w, size.h);
		cairo_surface_flush(window_surf);
		cairo_surface_mark_dirty(window_surf);
	}

};

typedef std::list<client_t *> client_list_t;

}

#endif /* CLIENT_HXX_ */
