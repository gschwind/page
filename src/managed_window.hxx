/*
 * managed_window.hxx
 *
 *  Created on: 16 mars 2013
 *      Author: gschwind
 */

#ifndef MANAGED_WINDOW_HXX_
#define MANAGED_WINDOW_HXX_

#include "theme.hxx"
#include "managed_window_base.hxx"
#include "xconnection.hxx"

namespace page {

enum managed_window_type_e {
	MANAGED_FLOATING,
	MANAGED_NOTEBOOK,
	MANAGED_FULLSCREEN
};

class managed_window_t : public managed_window_base_t {
private:

	static long const MANAGED_BASE_WINDOW_EVENT_MASK = SubstructureRedirectMask;
	static long const MANAGED_DECO_WINDOW_EVENT_MASK = ExposureMask;
	static long const MANAGED_ORIG_WINDOW_EVENT_MASK = (StructureNotifyMask)
			| (PropertyChangeMask);

	theme_t const * _theme;

	managed_window_type_e _type;
	Atom _net_wm_type;

	/** hold floating possition **/
	box_int_t _floating_wished_position;

	/** hold notebook possition **/
	box_int_t _notebook_wished_position;

	box_int_t _wished_position;
	box_int_t _orig_position;
	box_int_t _base_position;

	cairo_surface_t * _surf;

	cairo_surface_t * _top_buffer;
	cairo_surface_t * _bottom_buffer;
	cairo_surface_t * _left_buffer;
	cairo_surface_t * _right_buffer;

	mutable window_icon_handler_t * _icon;
	mutable string * _title;

	/* avoid copy */
	managed_window_t(managed_window_t const &);
	managed_window_t & operator=(managed_window_t const &);

	void init_managed_type(managed_window_type_e type);

	xconnection_t * cnx;

	Visual * _orig_visual;
	int _orig_depth;

	Visual * _deco_visual;
	int _deco_depth;

	Window _orig;
	Window _base;
	Window _deco;

	list<box_any_t *> _floating_area;

	bool _is_durty;

	bool _is_focused;

public:

	managed_window_t(xconnection_t * cnx, managed_window_type_e initial_type,
			Atom net_wm_type, Window orig, XWindowAttributes const & wa, theme_t const * theme);
	virtual ~managed_window_t();

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

	window_icon_handler_t * icon() const {
		if (_icon == 0) {
			_icon = new window_icon_handler_t(cnx, _orig, 16, 16);
		}
		return _icon;
	}

	void mark_icon_durty() {
		mark_durty();
		if (_icon != 0) {
			delete _icon;
			_icon = 0;
		}
	}

	void set_theme(theme_t const * theme);

	cairo_t * cairo_top() const {
		return cairo_create(_top_buffer);
	}

	cairo_t * cairo_bottom() const {
		return cairo_create(_bottom_buffer);
	}

	cairo_t * cairo_left() const {
		return cairo_create(_left_buffer);
	}

	cairo_t * cairo_right() const {
		return cairo_create(_right_buffer);
	}


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

	void mark_durty() {
		_is_durty = true;
	}

	void mark_clean() {
		_is_durty = false;
	}

	bool is_durty() {
		return _is_durty;
	}

	bool is_focused() const {
		return _is_focused;
	}

//	void set_default_action() {
//		list<Atom> _net_wm_allowed_actions;
//		_net_wm_allowed_actions.push_back(A(_NET_WM_ACTION_CLOSE));
//		_net_wm_allowed_actions.push_back(A(_NET_WM_ACTION_FULLSCREEN));
//		::page::write_net_wm_allowed_actions(cnx->dpy, _orig,
//				_net_wm_allowed_actions);
//	}


	void net_wm_allowed_actions_add(atom_e atom) {
		list<Atom> net_allowed_actions;
		cnx->read_net_wm_allowed_actions(_orig, &net_allowed_actions);
		net_allowed_actions.remove(A(atom));
		net_allowed_actions.push_back(A(atom));
		cnx->write_net_wm_allowed_actions(_orig, net_allowed_actions);
	}

	void set_focus_state(bool is_focused) {
		_is_focused = is_focused;

		if(_is_focused) {
			net_wm_state_add(_NET_WM_STATE_FOCUSED);
			grab_button_focused();
		} else {
			net_wm_state_remove(_NET_WM_STATE_FOCUSED);
			grab_button_unfocused();
		}

		mark_durty();
		expose();
	}

	char const * title() const {
		if (_title == 0) {
			string name;
			if (cnx->read_net_wm_name(_orig, &name)) {
				_title = new string(name);
			} else if (cnx->read_wm_name(_orig, name)) {
				_title = new string(name);
			} else {
				std::stringstream s(
						std::stringstream::in | std::stringstream::out);
				s << "#" << (_orig) << " (noname)";
				_title = new string(s.str());
			}
		}

		return _title->c_str();
	}

	void mark_title_durty() {
		mark_durty();
		if(_title != 0) {
			delete _title;
			_title = 0;
		}
	}

	box_t<int> const & base_position() const {
		return _base_position;
	}

public:
	void grab_button_focused() {
		/** First ungrab all **/
		XUngrabButton(cnx->dpy, AnyButton, AnyModifier, _orig);
		XUngrabButton(cnx->dpy, AnyButton, AnyModifier, _base);
		XUngrabButton(cnx->dpy, AnyButton, AnyModifier, _deco);

		/** for decoration, grab all **/
		XGrabButton(cnx->dpy, (Button1), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx->dpy, (Button2), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx->dpy, (Button3), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		/** for base, just grab some modified buttons **/
		XGrabButton(cnx->dpy, Button1, (Mod1Mask), _base, False,
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		/** for base, just grab some modified buttons **/
		XGrabButton(cnx->dpy, Button1, (ControlMask), _base, False,
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

	}

	void grab_button_unfocused() {
		/** First ungrab all **/
		XUngrabButton(cnx->dpy, AnyButton, AnyModifier, _orig);
		XUngrabButton(cnx->dpy, AnyButton, AnyModifier, _base);
		XUngrabButton(cnx->dpy, AnyButton, AnyModifier, _deco);

		XGrabButton(cnx->dpy, (Button1), (AnyModifier), _base, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx->dpy, (Button2), (AnyModifier), _base, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx->dpy, (Button3), (AnyModifier), _base, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		/** for decoration, grab all **/
		XGrabButton(cnx->dpy, (Button1), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx->dpy, (Button2), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx->dpy, (Button3), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

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

	void net_wm_state_add(atom_e atom) {
		list<Atom> net_wm_state;
		cnx->read_net_wm_state(_orig, &net_wm_state);
		/** remove it if alredy focused **/
		net_wm_state.remove(A(atom));
		/** add it **/
		net_wm_state.push_back(A(atom));
		cnx->write_net_wm_state(_orig, net_wm_state);
	}

	void net_wm_state_remove(atom_e atom) {
		list<Atom> net_wm_state;
		cnx->read_net_wm_state(_orig, &net_wm_state);
		net_wm_state.remove(A(atom));
		cnx->write_net_wm_state(_orig, net_wm_state);
	}


	void normalize() {
		cnx->write_wm_state(_orig, NormalState, None);
		net_wm_state_remove(_NET_WM_STATE_HIDDEN);
		cnx->map_window(_orig);
		cnx->map_window(_deco);
		cnx->map_window(_base);
	}

	void iconify() {
		net_wm_state_add(_NET_WM_STATE_HIDDEN);
		cnx->write_wm_state(_orig, IconicState, None);
		cnx->unmap(_base);
		cnx->unmap(_deco);
		cnx->unmap(_orig);
	}

	void set_floating_wished_position(box_t<int> & pos) {
		_floating_wished_position = pos;
		if(is(MANAGED_FLOATING)) {
			reconfigure();
		}
	}

	void set_notebook_wished_position(box_t<int> & pos) {
		_notebook_wished_position = pos;
		if(!is(MANAGED_FLOATING)) {
			reconfigure();
		}
	}

	box_int_t const & get_wished_position() {
		return _wished_position;
	}

	box_int_t const & get_floating_wished_position() {
		return _floating_wished_position;
	}

	void destroy_back_buffer() {

		if(_top_buffer != 0) {
			cairo_surface_destroy(_top_buffer);
			_top_buffer = 0;
		}

		if(_bottom_buffer != 0) {
			cairo_surface_destroy(_bottom_buffer);
			_bottom_buffer = 0;
		}

		if(_left_buffer != 0) {
			cairo_surface_destroy(_left_buffer);
			_left_buffer = 0;
		}

		if(_right_buffer != 0) {
			cairo_surface_destroy(_right_buffer);
			_right_buffer = 0;
		}

	}

	void create_back_buffer() {

		_top_buffer = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
				_base_position.w, _theme->floating_margin.top);
		_bottom_buffer = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
				_base_position.w, _theme->floating_margin.bottom);
		_left_buffer = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
				_theme->floating_margin.left,
				_base_position.h - _theme->floating_margin.top
						- _theme->floating_margin.bottom);
		_right_buffer = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
				_theme->floating_margin.right,
				_base_position.h - _theme->floating_margin.top
						- _theme->floating_margin.bottom);

	}

	list<box_any_t *> const & floating_areas() {
		return _floating_area;
	}

	void update_floating_areas() {
		for (list<box_any_t *>::iterator i = _floating_area.begin();
				i != _floating_area.end(); ++i) {
			delete *i;
		}

		_floating_area = _theme->compute_floating_areas(this);
	}

};

}


#endif /* MANAGED_WINDOW_HXX_ */
