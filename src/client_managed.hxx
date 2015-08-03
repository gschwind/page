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

#include <string>
#include <vector>

#include <xcb/xcb.h>

#include "icon_handler.hxx"
#include "theme.hxx"

#include "composite_surface_manager.hxx"
#include "client_properties.hxx"

#include "floating_event.hxx"
#include "composite_surface.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "renderable_pixmap.hxx"

namespace page {

enum managed_window_type_e {
	MANAGED_FLOATING,
	MANAGED_NOTEBOOK,
	MANAGED_FULLSCREEN,
	MANAGED_DOCK
};

class client_managed_t : public client_base_t {
private:

	static long const MANAGED_BASE_WINDOW_EVENT_MASK = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
	static long const MANAGED_DECO_WINDOW_EVENT_MASK = XCB_EVENT_MASK_EXPOSURE;
	static long const MANAGED_ORIG_WINDOW_EVENT_MASK = XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_ENTER_WINDOW;
	static uint32_t const DEFAULT_BUTTON_EVENT_MASK =  XCB_EVENT_MASK_BUTTON_PRESS|XCB_EVENT_MASK_BUTTON_MOTION|XCB_EVENT_MASK_BUTTON_RELEASE;

	managed_window_type_e _managed_type;
	xcb_atom_t _net_wm_type;

	/** hold floating position **/
	rect _floating_wished_position;

	/** hold notebook position **/
	rect _notebook_wished_position;

	/** the absolute position without border **/
	rect _wished_position;

	rect _orig_position;
	rect _base_position;

	// the output surface (i.e. surface where we write things)
	cairo_surface_t * _surf;

	// border surface of floating window
	cairo_surface_t * _top_buffer;
	cairo_surface_t * _bottom_buffer;
	cairo_surface_t * _left_buffer;
	cairo_surface_t * _right_buffer;


	// window title cache
	std::string _title;

	// icon cache
	std::shared_ptr<icon16> _icon;

	renderable_floating_outer_gradien_t * _shadow;
	renderable_pixmap_t * _base_renderable;

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

	xcb_window_t _input_top;
	xcb_window_t _input_left;
	xcb_window_t _input_right;
	xcb_window_t _input_bottom;
	xcb_window_t _input_top_left;
	xcb_window_t _input_top_right;
	xcb_window_t _input_bottom_left;
	xcb_window_t _input_bottom_right;

	rect _area_top;
	rect _area_left;
	rect _area_right;
	rect _area_bottom;
	rect _area_top_left;
	rect _area_top_right;
	rect _area_bottom_left;
	rect _area_bottom_right;

	std::vector<floating_event_t> * _floating_area;

	bool _is_focused;
	bool _is_iconic;
	bool _demands_attention;
	bool _is_durty;

public:

	signal_t<client_managed_t *> on_title_change;
	signal_t<client_managed_t *> on_destroy;
	signal_t<client_managed_t *> on_activate;
	signal_t<client_managed_t *> on_deactivate;

	client_managed_t(page_context_t * ctx, xcb_atom_t net_wm_type, std::shared_ptr<client_properties_t> props);
	virtual ~client_managed_t();

	void reconfigure();
	void fake_configure();

	void set_wished_position(rect const & position);
	rect const & get_wished_position() const;

	void delete_window(xcb_timestamp_t);

	rect get_base_position() const;

	void set_managed_type(managed_window_type_e type);

	cairo_t * get_cairo_context();

	void focus(xcb_timestamp_t t);

	managed_window_type_e get_type();

	std::shared_ptr<icon16> icon() const {
		return _icon;
	}

	void update_icon() {
		_icon = std::shared_ptr<icon16>{new icon16(*this)};
	}

	void set_theme(theme_t const * theme);

	bool is(managed_window_type_e type);

	void expose();

	xcb_window_t orig() const {
		return _properties->id();
	}

	xcb_window_t base() const {
		return _base;
	}

	xcb_window_t deco() const {
		return _deco;
	}

	xcb_atom_t A(atom_e atom) {
		return cnx()->A(atom);
	}

	void icccm_focus(xcb_timestamp_t t);

	bool is_focused() const {
		return _is_focused;
	}

	std::shared_ptr<pixmap_t> get_last_pixmap() {
		return _ctx->csm()->get_last_pixmap(_base);
	}

	void set_current_desktop(unsigned int n) {
		_properties->set_net_wm_desktop(n);
	}

	void net_wm_allowed_actions_add(atom_e atom);

	bool lock();
	void unlock();
	void set_focus_state(bool is_focused);

	void set_demands_attention(bool x) {
		if (x) {
			_properties->net_wm_state_add(_NET_WM_STATE_DEMANDS_ATTENTION);
		} else {
			_properties->net_wm_state_remove(_NET_WM_STATE_DEMANDS_ATTENTION);
		}
		_demands_attention = x;
	}

	bool demands_attention() {
		return _demands_attention;
	}

private:

	void map();
	void unmap();
	void grab_button_focused();
	void grab_button_unfocused();
	void ungrab_all_button();
	void select_inputs();
	void unselect_inputs();

public:

	bool is_fullscreen();
	bool skip_task_bar();
	xcb_atom_t net_wm_type();
	bool get_wm_normal_hints(XSizeHints * size_hints);
	void net_wm_state_add(atom_e atom);
	void net_wm_state_remove(atom_e atom);
	void net_wm_state_delete();
	void normalize();
	void iconify();
	void wm_state_delete();
	void set_floating_wished_position(rect const & pos);
	void set_notebook_wished_position(rect const & pos);
	rect const & get_wished_position();
	rect const & get_floating_wished_position();
	void destroy_back_buffer();
	void create_back_buffer();
	std::vector<floating_event_t> const * floating_areas();
	void update_floating_areas();
	void set_opaque_region(xcb_window_t w, region & region);
	bool has_window(xcb_window_t w) const;
	virtual std::string get_node_name() const;
	display_t * cnx();
	virtual void udpate_layout(time64_t const time);
	void update_base_renderable();
	virtual rect const & base_position() const;
	virtual rect const & orig_position() const;
	std::vector<floating_event_t> * compute_floating_areas(
			theme_managed_window_t * mw) const;
	rect compute_floating_bind_position(
			rect const & allocation) const;
	rect compute_floating_close_position(rect const & allocation) const;
	region visible_area() const;
	void hide();
	void show();
	void get_visible_children(std::vector<tree_t *> & out);

	bool is_iconic();

	xcb_window_t const * wm_transient_for() {
		return _properties->wm_transient_for();
	}

	bool is_stiky();
	bool is_modal();

	void activate(tree_t * t = nullptr);

	virtual bool button_press(xcb_button_press_event_t const * ev);

	xcb_window_t get_window();

	void queue_redraw();
	void trigger_redraw();

	void update_title();

	std::string const & title() const {
		return _title;
	}

	bool prefer_window_border() const;
	virtual void children(std::vector<tree_t *> & out) const;

};

}


#endif /* MANAGED_WINDOW_HXX_ */
