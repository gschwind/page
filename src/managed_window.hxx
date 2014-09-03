/*
 * managed_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef MANAGED_WINDOW_HXX_
#define MANAGED_WINDOW_HXX_

#include "theme.hxx"
#include "managed_window_base.hxx"
#include "display.hxx"
#include "composite_surface_manager.hxx"

namespace page {

enum managed_window_type_e {
	MANAGED_FLOATING,
	MANAGED_NOTEBOOK,
	MANAGED_FULLSCREEN
};

class managed_window_t : public managed_window_base_t {
private:

	static long const MANAGED_BASE_WINDOW_EVENT_MASK = SubstructureRedirectMask | StructureNotifyMask;
	static long const MANAGED_DECO_WINDOW_EVENT_MASK = ExposureMask;
	static long const MANAGED_ORIG_WINDOW_EVENT_MASK = (StructureNotifyMask)
			| (PropertyChangeMask) | (FocusChangeMask);

	// theme used for window decoration
	theme_t const * _theme;

	managed_window_type_e _type;
	Atom _net_wm_type;

	/** hold floating position **/
	rectangle _floating_wished_position;

	/** hold notebook position **/
	rectangle _notebook_wished_position;

	rectangle _wished_position;
	rectangle _orig_position;
	rectangle _base_position;

	// the window surface
	cairo_surface_t * _surf;

	// border surface of floating window
	cairo_surface_t * _top_buffer;
	cairo_surface_t * _bottom_buffer;
	cairo_surface_t * _left_buffer;
	cairo_surface_t * _right_buffer;

	// icon cache
	mutable window_icon_handler_t * _icon;


	/* private to avoid copy */
	managed_window_t(managed_window_t const &);
	managed_window_t & operator=(managed_window_t const &);

	void init_managed_type(managed_window_type_e type);

	Visual * _orig_visual;
	int _orig_depth;

	Visual * _deco_visual;
	int _deco_depth;

	Window _orig;
	Window _base;
	Window _deco;

	vector<floating_event_t> * _floating_area;

	bool _is_durty;

	bool _is_focused;

	bool _motif_has_border;

	composite_surface_handler_t _composite_surf;

public:

	managed_window_t(Atom net_wm_type, client_base_t * c,
			theme_t const * theme);
	virtual ~managed_window_t();

	void reconfigure();
	void fake_configure();

	void set_wished_position(rectangle const & position);
	rectangle const & get_wished_position() const;

	void delete_window(Time t);

	bool check_orig_position(rectangle const & position);
	bool check_base_position(rectangle const & position);


	rectangle get_base_position() const;

	void set_managed_type(managed_window_type_e type);

	cairo_t * get_cairo_context();

	void focus(Time t);

	managed_window_type_e get_type();

	window_icon_handler_t * icon() const {
		if (_icon == nullptr) {
			_icon = new window_icon_handler_t(this, 16, 16);
		}
		return _icon;
	}

	void mark_icon_durty() {
		mark_durty();
		if (_icon != nullptr) {
			delete _icon;
			_icon = nullptr;
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

	Window orig() const {
		return _orig;
	}

	Window base() const {
		return _base;
	}

	Window deco() const {
		return _deco;
	}

	Atom A(atom_e atom) {
		return _cnx->A(atom);
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

	bool motif_has_border() {
		return _motif_has_border;
	}

	composite_surface_handler_t surf() {
		return _composite_surf;
	}

	void net_wm_allowed_actions_add(atom_e atom) {
		if(_net_wm_allowed_actions == 0) {
			_net_wm_allowed_actions = new list<Atom>;
		}

		_net_wm_allowed_actions->remove(A(atom));
		_net_wm_allowed_actions->push_back(A(atom));
		_cnx->write_net_wm_allowed_actions(_orig, *(_net_wm_allowed_actions));
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

	rectangle const & base_position() const {
		return _base_position;
	}

public:
	void grab_button_focused() {
		/** First ungrab all **/
		ungrab_all_button();

		/** for decoration, grab all **/
		XGrabButton(_cnx->dpy(), (Button1), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(_cnx->dpy(), (Button2), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(_cnx->dpy(), (Button3), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		/** for base, just grab some modified buttons **/
		XGrabButton(_cnx->dpy(), Button1, (Mod1Mask), _base, False,
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		/** for base, just grab some modified buttons **/
		XGrabButton(_cnx->dpy(), Button1, (ControlMask), _base, False,
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

	}

	void grab_button_unfocused() {
		/** First ungrab all **/
		ungrab_all_button();

		XGrabButton(_cnx->dpy(), (Button1), (AnyModifier), _base, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(_cnx->dpy(), (Button2), (AnyModifier), _base, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(_cnx->dpy(), (Button3), (AnyModifier), _base, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		/** for decoration, grab all **/
		XGrabButton(_cnx->dpy(), (Button1), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(_cnx->dpy(), (Button2), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(_cnx->dpy(), (Button3), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

	}


	void ungrab_all_button() {
		XUngrabButton(_cnx->dpy(), AnyButton, AnyModifier, _orig);
		XUngrabButton(_cnx->dpy(), AnyButton, AnyModifier, _base);
		XUngrabButton(_cnx->dpy(), AnyButton, AnyModifier, _deco);
	}

	void select_inputs() {
		XSelectInput(_cnx->dpy(), _base, MANAGED_BASE_WINDOW_EVENT_MASK);
		XSelectInput(_cnx->dpy(), _deco, MANAGED_DECO_WINDOW_EVENT_MASK);
		XSelectInput(_cnx->dpy(), _orig, MANAGED_ORIG_WINDOW_EVENT_MASK);
	}

	void unselect_inputs() {
		XSelectInput(_cnx->dpy(), _base, NoEventMask);
		XSelectInput(_cnx->dpy(), _deco, NoEventMask);
		XSelectInput(_cnx->dpy(), _orig, NoEventMask);
	}

	bool is_fullscreen() {
		if (_net_wm_state != 0) {
			list<Atom>::iterator x = find(_net_wm_state->begin(),
					_net_wm_state->end(), A(_NET_WM_STATE_FULLSCREEN));
			return x != _net_wm_state->end();
		}
		return false;
	}

	Atom net_wm_type() {
		return _net_wm_type;
	}

	bool get_wm_normal_hints(XSizeHints * size_hints) {
		if(wm_normal_hints != 0) {
			*size_hints = *(wm_normal_hints);
			return true;
		} else {
			return false;
		}
	}

	void net_wm_state_add(atom_e atom) {
		if(_net_wm_state == 0) {
			_net_wm_state = new list<Atom>;
		}
		/** remove it if alredy focused **/
		_net_wm_state->remove(A(atom));
		/** add it **/
		_net_wm_state->push_back(A(atom));
		_cnx->write_net_wm_state(_orig, *(_net_wm_state));
	}

	void net_wm_state_remove(atom_e atom) {
		if(_net_wm_state == 0) {
			_net_wm_state = new list<Atom>;
		}

		_net_wm_state->remove(A(atom));
		_cnx->write_net_wm_state(_orig, *(_net_wm_state));
	}

	void net_wm_state_delete() {
		_cnx->delete_property(_orig, _NET_WM_STATE);
	}

	void normalize() {
		_cnx->write_wm_state(_orig, NormalState, None);
		net_wm_state_remove(_NET_WM_STATE_HIDDEN);
		_cnx->map_window(_orig);
		_cnx->map_window(_deco);
		_cnx->map_window(_base);

		_composite_surf = composite_surface_manager_t::get(_cnx->dpy(), base());

		for(auto c: _childen) {
			managed_window_t * mw = dynamic_cast<managed_window_t *>(c);
			if(mw != nullptr) {
				mw->normalize();
			}
		}

	}

	void iconify() {
		net_wm_state_add(_NET_WM_STATE_HIDDEN);
		_cnx->write_wm_state(_orig, IconicState, None);
		_cnx->unmap(_base);
		_cnx->unmap(_deco);
		_cnx->unmap(_orig);

		_composite_surf.reset();

		for(auto c: _childen) {
			managed_window_t * mw = dynamic_cast<managed_window_t *>(c);
			if(mw != nullptr) {
				mw->iconify();
			}
		}

	}

	void wm_state_delete() {
		_cnx->delete_property(_orig, WM_STATE);
	}

	void set_floating_wished_position(rectangle & pos) {
		_floating_wished_position = pos;
	}

	void set_notebook_wished_position(rectangle & pos) {
		_notebook_wished_position = pos;
	}

	rectangle const & get_wished_position() {
		return _wished_position;
	}

	rectangle const & get_floating_wished_position() {
		return _floating_wished_position;
	}

	void destroy_back_buffer() {

		if(_top_buffer != nullptr) {
			warn(cairo_surface_get_reference_count(_top_buffer) == 1);
			cairo_surface_destroy(_top_buffer);
			_top_buffer = nullptr;
		}

		if(_bottom_buffer != nullptr) {
			warn(cairo_surface_get_reference_count(_bottom_buffer) == 1);
			cairo_surface_destroy(_bottom_buffer);
			_bottom_buffer = nullptr;
		}

		if(_left_buffer != nullptr) {
			warn(cairo_surface_get_reference_count(_left_buffer) == 1);
			cairo_surface_destroy(_left_buffer);
			_left_buffer = nullptr;
		}

		if(_right_buffer != nullptr) {
			warn(cairo_surface_get_reference_count(_right_buffer) == 1);
			cairo_surface_destroy(_right_buffer);
			_right_buffer = nullptr;
		}

	}

	void create_back_buffer() {

		if (_theme->floating_margin.top > 0) {
			_top_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					_base_position.w, _theme->floating_margin.top);
		} else {
			_top_buffer = 0;
		}

		if (_theme->floating_margin.bottom > 0) {
			_bottom_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					_base_position.w, _theme->floating_margin.bottom);
		} else {
			_bottom_buffer = 0;
		}

		if (_theme->floating_margin.left > 0) {
			_left_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					_theme->floating_margin.left,
					_base_position.h - _theme->floating_margin.top
							- _theme->floating_margin.bottom);
		} else {
			_left_buffer = 0;
		}

		if (_theme->floating_margin.right > 0) {
			_right_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					_theme->floating_margin.right,
					_base_position.h - _theme->floating_margin.top
							- _theme->floating_margin.bottom);
		} else {
			_right_buffer = 0;
		}

	}

	vector<floating_event_t> const * floating_areas() {
		return _floating_area;
	}

	void update_floating_areas() {

		if (_floating_area != 0) {
			delete _floating_area;
		}

		_floating_area = _theme->compute_floating_areas(this);
	}

	void set_opaque_region(Window w, region & region);

	virtual bool has_window(Window w) {
		return w == _id or w == _base or w == _deco;
	}

	virtual string get_node_name() const {
		string s = _get_node_name<'M'>();
		ostringstream oss;
		oss << s << " " << orig() << " " << title();
		return oss.str();
	}

	virtual void render(cairo_t * cr, time_t time) {

		if (_composite_surf != nullptr) {
			shared_ptr<pixmap_t> p = _composite_surf->get_pixmap();
			if (p != nullptr) {
				cairo_surface_t * s = p->get_cairo_surface();
				rectangle loc = base_position();

				if(_motif_has_border) {
					draw_outer_graddien2(cr, rectangle(loc.x,loc.y,loc.w,loc.h), 8.0, 6.0);
				}


				cairo_save(cr);
				cairo_set_source_surface(cr, s, loc.x, loc.y);
				cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
				cairo_rectangle(cr, loc.x, loc.y, loc.w, loc.h);
				cairo_fill(cr);

				cairo_restore(cr);
			}
		}

		for(auto i: childs()) {
			i->render(cr, time);
		}
	}

	bool need_render(time_t time) {

		for(auto i: childs()) {
			if(i->need_render(time)) {
				return true;
			}
		}
		return false;
	}


};

}


#endif /* MANAGED_WINDOW_HXX_ */
