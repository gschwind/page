/*
 * client.hxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

#ifndef CLIENT_HXX_
#define CLIENT_HXX_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo.h>
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

	std::set<Atom> type;
	std::set<Atom> net_wm_state;
	std::set<Atom> wm_protocols;

	/* the name of window */
	std::string name;

	bool wm_name_is_valid;
	std::string wm_name;
	bool net_wm_name_is_valid;
	std::string net_wm_name;
	Window clipping_window;
	Window page_window;

	Pixmap pix;

	/* store if wm must do input focus */
	bool wm_input_focus;
	/* store the map/unmap stase from the point of view of PAGE */
	bool is_map;
	bool need_focus;
	bool is_lock;
	/* this is used to distinguish if unmap is initiated by client or by PAGE
	 * PAGE unmap mean Normal to Iconic
	 * client unmap mean Normal to WithDrawn */
	int unmap_pending;

	bool as_icon;
	Pixmap pixmap_icon;
	Window w_icon;

	/* the desired width/heigth */
	int width;
	int height;

	Damage damage;

	XSizeHints hints;

	icon_t icon;
	cairo_surface_t * icon_surf;

	bool is_dock;
	bool has_partial_struct;
	int struct_left;
	int struct_right;
	int struct_top;
	int struct_bottom;

	client_t(xconnection_t &cnx, Window page_window, Window w,
			XWindowAttributes &wa) :
			cnx(cnx), xwin(w), wa(wa), is_dock(false), has_partial_struct(
					false), height(wa.height), width(wa.width), page_window(
					page_window), lock_count(0), is_lock(false) {

		/* if the client is mapped, the reparent will unmap the window
		 * The client is mapped if the manage occur on start of
		 * page.
		 */
		if (wa.map_state == IsUnmapped) {
			is_map = false;
			unmap_pending = 0;
		} else {
			is_map = true;
			unmap_pending = 1;
		}

		wm_input_focus = false;

		XWMHints * hints = XGetWMHints(cnx.dpy, xwin);
		if (hints) {
			if ((hints->flags & InputHint) && hints->input == True)
				wm_input_focus = true;
		}

		update_net_vm_name();
		update_vm_name();
		update_title();
		client_update_size_hints();
		update_type();
		read_wm_state();
		read_wm_protocols();
	}

	void map();
	void unmap();
	void update_client_size(int w, int h);

private:
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
	void read_wm_state();
	void read_wm_protocols();
	void write_wm_state();

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

};

}

#endif /* CLIENT_HXX_ */
