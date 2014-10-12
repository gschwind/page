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

	ptr<composite_surface_t> surf() {
		return _composite_surf;
	}

	void net_wm_allowed_actions_add(atom_e atom);

	bool lock();
	void unlock();
	void set_focus_state(bool is_focused);

public:
	void grab_button_focused();
	void grab_button_unfocused();
	void ungrab_all_button();
	void select_inputs();
	void unselect_inputs();
	bool is_fullscreen();
	Atom net_wm_type();
	bool get_wm_normal_hints(XSizeHints * size_hints);
	void net_wm_state_add(atom_e atom);
	void net_wm_state_remove(atom_e atom);
	void net_wm_state_delete();
	void normalize();
	void iconify();
	void wm_state_delete();
	void set_floating_wished_position(i_rect const & pos);
	void set_notebook_wished_position(i_rect const & pos);
	i_rect const & get_wished_position();
	i_rect const & get_floating_wished_position();
	void destroy_back_buffer();
	void create_back_buffer();
	vector<floating_event_t> const * floating_areas();
	void update_floating_areas();
	void set_opaque_region(Window w, region & region);
	virtual bool has_window(Window w);
	virtual string get_node_name() const;
	display_t * cnx();
	virtual void prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time);
	ptr<renderable_t> get_base_renderable();
	virtual i_rect const & base_position() const;
	virtual i_rect const & orig_position() const;
	virtual bool has_window(Window w) const;
	vector<floating_event_t> * compute_floating_areas(
			theme_managed_window_t * mw) const;
	i_rect compute_floating_bind_position(
			i_rect const & allocation) const;
	i_rect compute_floating_close_position(i_rect const & allocation) const;
	region visible_area() const;

};

}


#endif /* MANAGED_WINDOW_HXX_ */
