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
#include "window.hxx"

namespace page {

struct client_t : public renderable_t {
	window_t & w;
	/* the desired width/heigth */
	int width;
	int height;
	/* icon data */
	icon_t icon;
	/* icon surface */
	cairo_surface_t * icon_surf;

	explicit client_t(window_t & w);
	client_t(xconnection_t &cnx, Window w, XWindowAttributes &wa,
			long wm_state);
	virtual ~client_t();

	void update_size(int w, int h);
	void init_icon();
	void set_fullscreen();
	void unset_fullscreen();

	bool is_fullscreen() {
		return w.is_fullscreen();
	}

	bool is_xwin(Window x) {
		return w.is_window(x);
	}

	void withdraw_to_X();

	void write_wm_state(long int state) {
		w.write_wm_state(state);
	}

	window_t * get_window() {
		return &w;
	}

	void set_focused() {
		w.set_focused();
	}

	void unset_focused() {
		w.unset_focused();
	}

	void delete_window(Time t) {
		w.delete_window(t);
	}

	std::string const & get_name() {
		return w.read_title();
	}

	void focus() {
		w.focus();
	}

	bool is_window(Window x) {
		return w.is_window(x);
	}

	long get_wm_state() {
		return w.get_wm_state();
	}

	void map() {
		w.map();
	}

	void unmap() {
		w.unmap();
	}

	std::string const & get_title() {
		return w.get_title();
	}

	virtual void repair1(cairo_t * cr, box_int_t const & area);
	virtual box_int_t get_absolute_extend();
	virtual void reconfigure(box_int_t const & area);
	virtual void mark_dirty();
	virtual void mark_dirty_retangle(box_int_t const & area);

};

typedef std::list<client_t *> client_list_t;

}

#endif /* CLIENT_HXX_ */
