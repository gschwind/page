/*
 * managed_window.hxx
 *
 *  Created on: 16 mars 2013
 *      Author: gschwind
 */

#ifndef MANAGED_WINDOW_HXX_
#define MANAGED_WINDOW_HXX_

#include "theme_layout.hxx"
#include "xconnection.hxx"
#include "window_icon_handler.hxx"

namespace page {

enum managed_window_type_e {
	MANAGED_FLOATING,
	MANAGED_NOTEBOOK,
	MANAGED_FULLSCREEN
};

class managed_window_t {
private:

	static long const MANAGED_BASE_WINDOW_EVENT_MASK =
			PropertyChangeMask | SubstructureRedirectMask;
	static long const MANAGED_DECO_WINDOW_EVENT_MASK = ExposureMask;
	static long const MANAGED_ORIG_WINDOW_EVENT_MASK =
			StructureNotifyMask | PropertyChangeMask;

	theme_layout_t const * theme;

	managed_window_type_e _type;
	Atom _net_wm_type;

	unsigned _margin_top;
	unsigned _margin_bottom;
	unsigned _margin_left;
	unsigned _margin_right;

	box_int_t _floating_wished_position;

	box_int_t _wished_position;
	box_int_t _orig_position;
	box_int_t _base_position;

	cairo_t * _cr;
	cairo_surface_t * _surf;

	cairo_t * _back_cr;
	cairo_surface_t * _back_surf;

	window_icon_handler_t * icon;

	/* avoid copy */
	managed_window_t(managed_window_t const &);
	managed_window_t & operator=(managed_window_t const &);

	void init_managed_type(managed_window_type_e type);

	xconnection_t * cnx;

	Window _orig;
	Window _base;
	Window _deco;

public:

	managed_window_t(xconnection_t * cnx, managed_window_type_e initial_type,
			Atom net_wm_type, Window orig, theme_layout_t const * theme);
	virtual ~managed_window_t();

	void normalize();
	void iconify();

	void reconfigure();
	void fake_configure();

	void set_wished_position(box_int_t const & position);
	box_int_t const & get_wished_position() const;

	void delete_window(Time t);

	bool check_orig_position(box_int_t const & position);
	bool check_base_position(box_int_t const & position);


	box_int_t get_base_position() const;

	void set_managed_type(managed_window_type_e type);

	cairo_t * get_cairo_context();

	void focus(Time t);

	managed_window_type_e get_type();

	window_icon_handler_t * get_icon();
	void update_icon();

	void set_theme(theme_layout_t const * theme);

	cairo_t * get_cairo();

	bool is(managed_window_type_e type);

	void expose();

	Window orig() {
		return _orig;
	}

	Window base() {
		return _base;
	}

	Window deco() {
		return _deco;
	}

	Atom A(atom_e atom) {
		return cnx->A(atom);
	}

	void icccm_focus(Time t);

//	void set_default_action() {
//		list<Atom> _net_wm_allowed_actions;
//		_net_wm_allowed_actions.push_back(A(_NET_WM_ACTION_CLOSE));
//		_net_wm_allowed_actions.push_back(A(_NET_WM_ACTION_FULLSCREEN));
//		::page::write_net_wm_allowed_actions(cnx->dpy, _orig,
//				_net_wm_allowed_actions);
//	}

	void set_focused() {
		list<Atom> net_wm_state;
		cnx->read_net_wm_state(_orig, &net_wm_state);

		/** remove it if alredy focused **/
		net_wm_state.remove(A(_NET_WM_STATE_FOCUSED));
		/** add it **/
		net_wm_state.push_back(A(_NET_WM_STATE_FOCUSED));
		cnx->write_net_wm_state(cnx->dpy, _orig, net_wm_state);

		ungrab_all_buttons(_base);

	}

	void unset_focused() {
		list<Atom> net_wm_state;
		cnx->read_net_wm_state(_orig, &net_wm_state);

		net_wm_state.remove(A(_NET_WM_STATE_FOCUSED));
		cnx->write_net_wm_state(cnx->dpy, _orig, net_wm_state);

		ungrab_all_buttons(_base);
		grab_button_unfocused(_base);

	}

	string get_title() {
		std::string name;
		if (cnx->read_net_wm_name(_orig, name)) {
			return name;
		}

		if (cnx->read_wm_name(_orig, name)) {
			return name;
		}

		std::stringstream s(std::stringstream::in | std::stringstream::out);
		s << "#" << (_orig) << " (noname)";
		name = s.str();
		return name;
	}

	void grab_all_buttons(Window w) {
		XGrabButton(cnx->dpy, AnyButton, AnyModifier, w, False,
				ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
				GrabModeSync, GrabModeAsync, None, None);
	}

	void ungrab_all_buttons(Window w) {
		XUngrabButton(cnx->dpy, AnyButton, AnyModifier, w);
	}

	void grab_button_unfocused(Window w) {
		XGrabButton(cnx->dpy, Button1, AnyModifier, w,
				False, ButtonPressMask, GrabModeSync,
				GrabModeAsync, None, None);

		XGrabButton(cnx->dpy, Button2, AnyModifier, w,
				False, ButtonPressMask, GrabModeSync,
				GrabModeAsync, None, None);

		XGrabButton(cnx->dpy, Button3, AnyModifier, w,
				False, ButtonPressMask, GrabModeSync,
				GrabModeAsync, None, None);

	}

	bool is_fullscreen() {
		list<Atom> state;
		cnx->read_net_wm_state(_orig, &state);
		list<Atom>::iterator x = find(state.begin(), state.end(),
				A(_NET_WM_STATE_FULLSCREEN));
		return x != state.end();
	}

	Atom net_wm_type() {
		return _net_wm_type;
	}

	bool get_wm_normal_hints(XSizeHints * size_hints) {
		return cnx->read_wm_normal_hints(_orig, size_hints);
	}

};

}


#endif /* MANAGED_WINDOW_HXX_ */
