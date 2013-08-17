/*
 * managed_window.cxx
 *
 *  Created on: 16 mars 2013
 *      Author: gschwind
 */

#include <cairo.h>
#include <cairo-xlib.h>
#include "managed_window.hxx"
#include "notebook.hxx"

namespace page {

managed_window_t::managed_window_t(xconnection_t * cnx, managed_window_type_e initial_type,
		Atom net_wm_type, Window orig, theme_layout_t const * theme) : cnx(cnx) {

	_net_wm_type = net_wm_type;

	/**
	 * Create the base window, window that will content managed window
	 **/

	XSetWindowAttributes wa;
	Window wbase;
	Window wdeco;
	box_int_t b = cnx->get_window_position(orig);

	/** Common window properties **/
	unsigned long value_mask = CWOverrideRedirect;
	wa.override_redirect = True;

	Visual * root_visual =
			cnx->get_window_attributes(cnx->get_root_window())->visual;
	int root_depth = cnx->get_window_attributes(cnx->get_root_window())->depth;

	/**
	 * If window visual is 32 bit (have alpha channel, and root do not
	 * have alpha channel, use the window visual, other wise, always prefer
	 * root visual.
	 **/
	if (cnx->get_window_attributes(orig)->depth == 32 && root_depth != 32) {
		/** if visual is 32 bits, this values are mandatory **/
		Visual * v = cnx->get_window_attributes(orig)->visual;
		wa.colormap = XCreateColormap(cnx->dpy, cnx->get_root_window(), v,
				AllocNone);
		wa.background_pixel = BlackPixel(cnx->dpy, cnx->screen());
		wa.border_pixel = BlackPixel(cnx->dpy, cnx->screen());
		value_mask |= CWColormap | CWBackPixel | CWBorderPixel;

		wbase = XCreateWindow(cnx->dpy, cnx->get_root_window(), -10, -10, 1, 1,
				0, 32,
				InputOutput, v, value_mask, &wa);
		wdeco = XCreateWindow(cnx->dpy, wbase, b.x, b.y, b.w, b.h, 0, 32,
		InputOutput, v, value_mask, &wa);
	} else {
		wbase = XCreateWindow(cnx->dpy, cnx->get_root_window(), -10, -10, 1, 1,
				0, root_depth, InputOutput, root_visual, value_mask, &wa);
		wdeco = XCreateWindow(cnx->dpy, wbase, b.x, b.y, b.w, b.h, 0,
				root_depth, InputOutput, root_visual, value_mask, &wa);
	}

	_orig = orig;
	_base = wbase;
	_deco = wdeco;

	/**
	 * Grab and sync the server before reading and setup select_input to not
	 * miss events and to get the valid state of windows
	 **/
	//cnx->grab();

	cnx->select_input(_base, MANAGED_BASE_WINDOW_EVENT_MASK);
	cnx->select_input(_deco, MANAGED_DECO_WINDOW_EVENT_MASK);
	cnx->select_input(_orig, MANAGED_ORIG_WINDOW_EVENT_MASK);

	/* Grab button click */
	grab_button_focused();

	//cnx->ungrab();

	set_theme(theme);
	init_managed_type(initial_type);

	cnx->reparentwindow(_orig, _base, 0, 0);

	_surf = cairo_xlib_surface_create(cnx->dpy, _deco,
			cnx->get_window_attributes(_deco)->visual,
			cnx->get_window_attributes(_deco)->width,
			cnx->get_window_attributes(_deco)->height);
	assert(_surf != 0);
	_cr = cairo_create(_surf);
	assert(_cr != 0);

	if (initial_type == MANAGED_FLOATING) {
		_back_surf = cairo_surface_create_similar(_surf, CAIRO_CONTENT_COLOR,
				cnx->get_window_attributes(_base)->width,
				cnx->get_window_attributes(_base)->height);

		_back_cr = cairo_create(_back_surf);
	} else {
		_back_cr = 0;
		_back_surf = 0;
	}

	_floating_wished_position = cnx->get_window_position(orig);
	set_wished_position(cnx->get_window_position(orig));

	icon = 0;

}

managed_window_t::~managed_window_t() {
	if(_cr != 0) {
		cairo_destroy(_cr);
	}

	if(_surf != 0) {
		cairo_surface_destroy(_surf);
	}
}


void managed_window_t::reconfigure() {

	_base_position.x = _wished_position.x - _margin_left;
	_base_position.y = _wished_position.y - _margin_top;
	_base_position.w = _wished_position.w + _margin_left + _margin_right;
	_base_position.h = _wished_position.h + _margin_top + _margin_bottom;

	if(_base_position.x < 0)
		_base_position.x = 0;
	if(_base_position.y < 0)
		_base_position.y = 0;

	_orig_position.x = _margin_left;
	_orig_position.y = _margin_top;
	_orig_position.w = _wished_position.w;
	_orig_position.h = _wished_position.h;

	cnx->move_resize(_base, _base_position);
	cnx->move_resize(_deco, box_int_t(0, 0, _base_position.w, _base_position.h));
	cnx->move_resize(_orig, _orig_position);

	cairo_xlib_surface_set_size(_surf, _base_position.w, _base_position.h);

	if (_type == MANAGED_FLOATING) {

		/** rebuild back buffer **/
		if (_back_cr != 0)
			cairo_destroy(_back_cr);
		if (_back_surf != 0)
			cairo_surface_destroy(_back_surf);

		_back_surf = cairo_surface_create_similar(_surf, CAIRO_CONTENT_COLOR,
				cnx->get_window_attributes(_base)->width,
				cnx->get_window_attributes(_base)->height);

		_back_cr = cairo_create(_back_surf);
	}

	fake_configure();

}

void managed_window_t::set_wished_position(box_int_t const & position) {
		_wished_position = position;
		reconfigure();
}

box_int_t const & managed_window_t::get_wished_position() const {
	return _wished_position;
}

void managed_window_t::fake_configure() {
	cnx->fake_configure(_orig, _wished_position, 0);
}

void managed_window_t::delete_window(Time t) {
	XEvent ev;
	ev.xclient.display = cnx->dpy;
	ev.xclient.type = ClientMessage;
	ev.xclient.format = 32;
	ev.xclient.message_type = A(WM_PROTOCOLS);
	ev.xclient.window = _orig;
	ev.xclient.data.l[0] = A(WM_DELETE_WINDOW);
	ev.xclient.data.l[1] = t;
	cnx->send_event(_orig, False, NoEventMask, &ev);
}

bool managed_window_t::check_orig_position(box_int_t const & position) {
	return position == _orig_position;
}

bool managed_window_t::check_base_position(box_int_t const & position) {
	return position == _base_position;
}

void managed_window_t::init_managed_type(managed_window_type_e type) {
	switch(type) {
	case MANAGED_FLOATING:
		_margin_top = theme->floating_margin.top;
		_margin_bottom = theme->floating_margin.bottom;
		_margin_left = theme->floating_margin.left;
		_margin_right = theme->floating_margin.right;
		break;
	case MANAGED_NOTEBOOK:
	case MANAGED_FULLSCREEN:
		_margin_top = 0;
		_margin_bottom = 0;
		_margin_left = 0;
		_margin_right = 0;
		break;
	}

	_type = type;

}

void managed_window_t::set_managed_type(managed_window_type_e type) {
	switch(type) {
	case MANAGED_FLOATING:

	{
		list<Atom> net_wm_allowed_actions;
		net_wm_allowed_actions.push_back(A(_NET_WM_ACTION_CLOSE));
		net_wm_allowed_actions.push_back(A(_NET_WM_ACTION_FULLSCREEN));
		cnx->write_net_wm_allowed_actions(_orig, net_wm_allowed_actions);
	}

		if (_back_surf == 0)
			_back_surf = cairo_surface_create_similar(_surf,
					CAIRO_CONTENT_COLOR, cnx->get_window_attributes(_base)->width,
					cnx->get_window_attributes(_base)->height);

		if (_back_cr == 0)
			_back_cr = cairo_create(_back_surf);

		_margin_top = theme->floating_margin.top;
		_margin_bottom = theme->floating_margin.bottom;
		_margin_left = theme->floating_margin.left;
		_margin_right = theme->floating_margin.right;
		_wished_position = _floating_wished_position;
		break;
	case MANAGED_NOTEBOOK:
	case MANAGED_FULLSCREEN:

	{
		list<Atom> net_wm_allowed_actions;
		net_wm_allowed_actions.push_back(A(_NET_WM_ACTION_CLOSE));
		net_wm_allowed_actions.push_back(A(_NET_WM_ACTION_FULLSCREEN));
		cnx->write_net_wm_allowed_actions(_orig, net_wm_allowed_actions);
	}

		/** destroy back buffer **/
		if(_back_cr != 0)
			cairo_destroy(_back_cr);
		if(_back_surf != 0)
			cairo_surface_destroy(_back_surf);

		if(_type == MANAGED_FLOATING)
			_floating_wished_position = _wished_position;
		_margin_top = 0;
		_margin_bottom = 0;
		_margin_left = 0;
		_margin_right = 0;
		break;
	}

	_type = type;
	reconfigure();

}

cairo_t * managed_window_t::get_cairo_context() {
	return _cr;
}

void managed_window_t::focus(Time t) {
	if(cnx->get_window_attributes(_orig)->map_state == IsUnmapped)
		return;

	net_wm_state_add(_NET_WM_STATE_FOCUSED);
	/** when focus a window, disable all button grab **/
	grab_button_focused();
	icccm_focus(t);

}

box_int_t managed_window_t::get_base_position() const {
	p_window_attribute_t wa = cnx->get_window_attributes(_base);
	return box_int_t(wa->x, wa->y, wa->width, wa->height);
}

managed_window_type_e managed_window_t::get_type() {
	return _type;
}

window_icon_handler_t * managed_window_t::get_icon() {
	if(icon == 0) {
		icon = new window_icon_handler_t(cnx, _orig, 16, 16);
	}
	return icon;
}

void managed_window_t::update_icon() {
	if(icon != 0)
		delete icon;
	icon = new window_icon_handler_t(cnx, _orig, 16, 16);
}

void managed_window_t::set_theme(theme_layout_t const * theme) {
	this->theme = theme;
}

cairo_t * managed_window_t::get_cairo() {
	return _back_cr;
}

bool managed_window_t::is(managed_window_type_e type) {
	return _type == type;
}

void managed_window_t::expose() {
	if (is(MANAGED_FLOATING)) {
		cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
		cairo_rectangle(_cr, 0, 0, cnx->get_window_attributes(_deco)->width, cnx->get_window_attributes(_deco)->height);
		cairo_set_source_surface(_cr, _back_surf, 0, 0);
		cairo_paint(_cr);
		cairo_surface_flush(_surf);
	}
}

void managed_window_t::icccm_focus(Time t) {
	fprintf(stderr, "Focus time = %lu\n", t);

	XWMHints hints;

	if (cnx->read_wm_hints(_orig, &hints)) {
		if (hints.input == True) {
			cnx->set_input_focus(_orig, RevertToParent, t);
		}
	} else {
		/** no VM_HINTS, guess hints.input == True **/
		cnx->set_input_focus(_orig, RevertToParent, t);
	}

	list<Atom> protocols;
	if (cnx->read_net_wm_protocols(_orig, &protocols)) {
		if (::std::find(protocols.begin(), protocols.end(), A(WM_TAKE_FOCUS))
				!= protocols.end()) {
			XEvent ev;
			ev.xclient.display = cnx->dpy;
			ev.xclient.type = ClientMessage;
			ev.xclient.format = 32;
			ev.xclient.message_type = A(WM_PROTOCOLS);
			ev.xclient.window = _orig;
			ev.xclient.data.l[0] = A(WM_TAKE_FOCUS);
			ev.xclient.data.l[1] = t;
			cnx->send_event(_orig, False, NoEventMask, &ev);
		}
	}

}


}


