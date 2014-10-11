/*
 * managed_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef CLIENT_MANAGED_HXX_
#define CLIENT_MANAGED_HXX_

#include "utils.hxx"
#include "leak_checker.hxx"
#include "theme.hxx"
#include "display.hxx"
#include "composite_surface_manager.hxx"
#include "renderable_pixmap.hxx"
#include "renderable_floating_outer_gradien.hxx"

#include <stdexcept>
#include <exception>
#include <cairo/cairo-xcb.h>

namespace page {

enum managed_window_type_e {
	MANAGED_FLOATING,
	MANAGED_NOTEBOOK,
	MANAGED_FULLSCREEN
};

class client_managed_t : public client_base_t {
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
	i_rect _floating_wished_position;

	/** hold notebook position **/
	i_rect _notebook_wished_position;

	i_rect _wished_position;
	i_rect _orig_position;
	i_rect _base_position;

	// the output surface (i.e. surface where we write things)
	cairo_surface_t * _surf;

	// border surface of floating window
	cairo_surface_t * _top_buffer;
	cairo_surface_t * _bottom_buffer;
	cairo_surface_t * _left_buffer;
	cairo_surface_t * _right_buffer;

	// icon cache
	ptr<icon16> _icon;

	/* private to avoid copy */
	client_managed_t(client_managed_t const &);
	client_managed_t & operator=(client_managed_t const &);

	void init_managed_type(managed_window_type_e type);

	xcb_visualid_t _orig_visual;
	int _orig_depth;

	xcb_visualid_t _deco_visual;
	int _deco_depth;

	xcb_window_t _orig;
	xcb_window_t _base;
	xcb_window_t _deco;

	vector<floating_event_t> * _floating_area;

	bool _is_focused;

	bool _motif_has_border;

	/** input surface, surface from we get data **/
	ptr<composite_surface_t> _composite_surf;

public:

	client_managed_t(Atom net_wm_type, ptr<client_properties_t> c,
			theme_t const * theme);
	virtual ~client_managed_t();

	void reconfigure();
	void fake_configure();

	void set_wished_position(i_rect const & position);
	i_rect const & get_wished_position() const;

	void delete_window(Time t);

	bool check_orig_position(i_rect const & position);
	bool check_base_position(i_rect const & position);


	i_rect get_base_position() const;

	void set_managed_type(managed_window_type_e type);

	cairo_t * get_cairo_context();

	void focus(Time t);

	managed_window_type_e get_type();

	ptr<icon16> icon() const {
		return _icon;
	}

	void update_icon() {
		_icon = ptr<icon16>{new icon16(*this)};
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
		return _properties->id();
	}

	Window base() const {
		return _base;
	}

	Window deco() const {
		return _deco;
	}

	Atom A(atom_e atom) {
		return cnx()->A(atom);
	}

	void icccm_focus(Time t);

	bool is_focused() const {
		return _is_focused;
	}

	bool motif_has_border() {
		return _motif_has_border;
	}

	shared_ptr<composite_surface_t> surf() {
		return _composite_surf;
	}

	void net_wm_allowed_actions_add(atom_e atom) {
		_properties->net_wm_allowed_actions_add(atom);
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

		expose();
	}

public:
	void grab_button_focused() {
		/** First ungrab all **/
		ungrab_all_button();

		/** for decoration, grab all **/
		XGrabButton(cnx()->dpy(), (Button1), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx()->dpy(), (Button2), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx()->dpy(), (Button3), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		/** for base, just grab some modified buttons **/
		XGrabButton(cnx()->dpy(), Button1, (Mod1Mask), _base, False,
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		/** for base, just grab some modified buttons **/
		XGrabButton(cnx()->dpy(), Button1, (ControlMask), _base, False,
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

	}

	void grab_button_unfocused() {
		/** First ungrab all **/
		ungrab_all_button();

		XGrabButton(cnx()->dpy(), (Button1), (AnyModifier), _base, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx()->dpy(), (Button2), (AnyModifier), _base, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx()->dpy(), (Button3), (AnyModifier), _base, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		/** for decoration, grab all **/
		XGrabButton(cnx()->dpy(), (Button1), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx()->dpy(), (Button2), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx()->dpy(), (Button3), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

	}


	void ungrab_all_button() {
		XUngrabButton(cnx()->dpy(), AnyButton, AnyModifier, _orig);
		XUngrabButton(cnx()->dpy(), AnyButton, AnyModifier, _base);
		XUngrabButton(cnx()->dpy(), AnyButton, AnyModifier, _deco);
	}

	void select_inputs() {
		XSelectInput(cnx()->dpy(), _base, MANAGED_BASE_WINDOW_EVENT_MASK);
		XSelectInput(cnx()->dpy(), _deco, MANAGED_DECO_WINDOW_EVENT_MASK);
		XSelectInput(cnx()->dpy(), _orig, MANAGED_ORIG_WINDOW_EVENT_MASK);
		XShapeSelectInput(cnx()->dpy(), _orig, ShapeNotifyMask);
	}

	void unselect_inputs() {
		XSelectInput(cnx()->dpy(), _base, NoEventMask);
		XSelectInput(cnx()->dpy(), _deco, NoEventMask);
		XSelectInput(cnx()->dpy(), _orig, NoEventMask);
		XShapeSelectInput(cnx()->dpy(), _orig, 0);
	}

	bool is_fullscreen() {
		if (_properties->net_wm_state() != nullptr) {
			return has_key(*(_properties->net_wm_state()), static_cast<xcb_atom_t>(A(_NET_WM_STATE_FULLSCREEN)));
		}
		return false;
	}

	Atom net_wm_type() {
		return _net_wm_type;
	}

	bool get_wm_normal_hints(XSizeHints * size_hints) {
		if(_properties->wm_normal_hints() != nullptr) {
			*size_hints = *(_properties->wm_normal_hints());
			return true;
		} else {
			return false;
		}
	}

	void net_wm_state_add(atom_e atom) {
		_properties->net_wm_state_add(atom);
	}

	void net_wm_state_remove(atom_e atom) {
		_properties->net_wm_state_remove(atom);
	}

	void net_wm_state_delete() {
		cnx()->delete_property(_orig, _NET_WM_STATE);
	}

	void normalize() {
		cnx()->write_wm_state(_orig, NormalState, None);
		net_wm_state_remove(_NET_WM_STATE_HIDDEN);
		cnx()->map_window(_orig);
		cnx()->map_window(_deco);
		cnx()->map_window(_base);

		try {
			_composite_surf = composite_surface_manager_t::get(cnx()->dpy(), base());
		} catch(...) {
			printf("Error while creating composite surface\n");
		}

		for(auto c: _children) {
			client_managed_t * mw = dynamic_cast<client_managed_t *>(c);
			if(mw != nullptr) {
				mw->normalize();
			}
		}

	}

	void iconify() {
		net_wm_state_add(_NET_WM_STATE_HIDDEN);
		cnx()->write_wm_state(_orig, IconicState, None);
		cnx()->unmap(_base);
		cnx()->unmap(_deco);
		cnx()->unmap(_orig);

		_composite_surf.reset();

		for(auto c: _children) {
			client_managed_t * mw = dynamic_cast<client_managed_t *>(c);
			if(mw != nullptr) {
				mw->iconify();
			}
		}

	}

	void wm_state_delete() {
		cnx()->delete_property(_orig, WM_STATE);
	}

	void set_floating_wished_position(i_rect & pos) {
		_floating_wished_position = pos;
	}

	void set_notebook_wished_position(i_rect & pos) {
		_notebook_wished_position = pos;
	}

	i_rect const & get_wished_position() {
		return _wished_position;
	}

	i_rect const & get_floating_wished_position() {
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

		if (_theme->floating.margin.top > 0) {
			_top_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					_base_position.w, _theme->floating.margin.top);
		} else {
			_top_buffer = nullptr;
		}

		if (_theme->floating.margin.bottom > 0) {
			_bottom_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					_base_position.w, _theme->floating.margin.bottom);
		} else {
			_bottom_buffer = nullptr;
		}

		if (_theme->floating.margin.left > 0) {
			_left_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					_theme->floating.margin.left,
					_base_position.h - _theme->floating.margin.top
							- _theme->floating.margin.bottom);
		} else {
			_left_buffer = nullptr;
		}

		if (_theme->floating.margin.right > 0) {
			_right_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					_theme->floating.margin.right,
					_base_position.h - _theme->floating.margin.top
							- _theme->floating.margin.bottom);
		} else {
			_right_buffer = nullptr;
		}

	}

	vector<floating_event_t> const * floating_areas() {
		return _floating_area;
	}

	void update_floating_areas() {

		if (_floating_area != 0) {
			delete _floating_area;
		}

		theme_managed_window_t tm;
		tm.position = _base_position;
		tm.title = _title;

		_floating_area = compute_floating_areas(&tm);
	}

	void set_opaque_region(Window w, region & region);

	virtual bool has_window(Window w) {
		return w == _properties->id() or w == _base or w == _deco;
	}

	virtual string get_node_name() const {
		string s = _get_node_name<'M'>();
		ostringstream oss;
		oss << s << " " << orig() << " " << title();
		return oss.str();
	}

	display_t * cnx() {
		return _properties->cnx();
	}

	virtual void prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time) {

		if (_composite_surf != nullptr) {

			i_rect loc{base_position()};

			if (_motif_has_border) {
				auto x = new renderable_floating_outer_gradien_t(loc, 8.0, 6.0);
				out += ptr<renderable_t> { x };
			}

			ptr<renderable_t> x { get_base_renderable() };
			if (x != nullptr) {
				out += x;
			}
		}

		tree_t::_prepare_render(out, time);

	}

	ptr<renderable_t> get_base_renderable() {
		if (_composite_surf != nullptr) {

			region vis;
			region opa;

			if(_type == MANAGED_FULLSCREEN) {
				vis = _base_position;
				opa = _base_position;
			} else {

				vis = _base_position;

				region shp;
				if (shape() != nullptr) {
					shp = *shape();
				} else {
					shp = i_rect{0, 0, _orig_position.w, _orig_position.h};
				}

				region xopac;
				if (net_wm_opaque_region() != nullptr) {
					xopac = region { *(net_wm_opaque_region()) };
				} else {
					if (geometry()->depth == 24) {
						xopac = i_rect{0, 0, _orig_position.w, _orig_position.h};
					}
				}

				opa = shp & xopac;
				opa.translate(_base_position.x + _orig_position.x,
						_base_position.y + _orig_position.y);

			}

			region dmg { _composite_surf->get_damaged() };
			_composite_surf->clear_damaged();
			dmg.translate(_base_position.x, _base_position.y);
			auto x = new renderable_pixmap_t(_composite_surf->get_pixmap(),
					_base_position, dmg);
			x->set_opaque_region(opa);
			x->set_visible_region(vis);
			return ptr<renderable_t>{x};

		} else {
			/* return null */
			return ptr<renderable_t>{};
		}
	}

	virtual i_rect const & base_position() const {
		return _base_position;
	}

	virtual i_rect const & orig_position() const {
		return _orig_position;
	}

	virtual bool has_window(Window w) const {
		return w == _properties->id() or w == _base or w == _deco;
	}

	vector<floating_event_t> * compute_floating_areas(
			theme_managed_window_t * mw) const;
	i_rect compute_floating_bind_position(
			i_rect const & allocation) const;
	i_rect compute_floating_close_position(i_rect const & allocation) const;


	region visible_area() const {

		if(_type == MANAGED_FLOATING) {
			i_rect vis{base_position()};
			vis.x -= 8;
			vis.y -= 8;
			vis.w += 16;
			vis.h += 16;
			return vis;
		}

		return base_position();

	}

};

}


#endif /* MANAGED_WINDOW_HXX_ */
