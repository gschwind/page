/*
 * notebook.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef NOTEBOOK_HXX_
#define NOTEBOOK_HXX_

#include <algorithm>
#include <cmath>
#include <cassert>
#include <memory>

#include "theme.hxx"
#include "pixmap.hxx"
#include "composite_surface.hxx"
#include "renderable_notebook_fading.hxx"
#include "renderable_pixmap.hxx"
#include "renderable_empty.hxx"

#include "page_context.hxx"
#include "page_component.hxx"
#include "client_managed.hxx"
#include "renderable_thumbnail.hxx"
#include "renderable_unmanaged_gaussian_shadow.hxx"

namespace page {

using namespace std;
using time_t = page::time_t;

class client_managed_t;
class grab_bind_client_t;

class notebook_t : public page_component_t {
	time_t const animation_duration{0, 500000000};

	page_context_t * _ctx;
	page_component_t * _parent;

	rect _allocation;

	/* child stack order the first one is the lowest one */
	list<tree_t *> _children;

	time_t swap_start;

	shared_ptr<renderable_notebook_fading_t> fading_notebook;
	vector<shared_ptr<renderable_thumbnail_t>> _exposay_thumbnail;

	theme_notebook_t theme_notebook;

	bool _is_default;
	bool _is_hidden;
	bool _keep_selected;
	bool _exposay;

	struct {
		tuple<rect, client_managed_t *, theme_tab_t *> * tab;
		tuple<rect, client_managed_t *, int> * exposay;
	} _mouse_over;

	enum select_e {
		SELECT_NONE,
		SELECT_TAB,
		SELECT_TOP,
		SELECT_BOTTOM,
		SELECT_LEFT,
		SELECT_RIGHT
	};

	struct _client_context_t {
		decltype(client_managed_t::on_title_change)::signal_func_t  title_change_func;
		decltype(client_managed_t::on_destroy)::signal_func_t       destoy_func;
		decltype(client_managed_t::on_activate)::signal_func_t      activate_func;
		decltype(client_managed_t::on_deactivate)::signal_func_t    deactivate_func;
	};

	// list to maintain the client order
	list<client_managed_t *> _clients;
	map<client_managed_t *, _client_context_t> _clients_context;

	client_managed_t * _selected;


	rect client_position;
	rect client_area;

	rect button_close;
	rect button_vsplit;
	rect button_hsplit;
	rect button_select;
	rect button_exposay;

	rect close_client_area;
	rect undck_client_area;

	rect tab_area;
	rect top_area;
	rect bottom_area;
	rect left_area;
	rect right_area;

	rect popup_top_area;
	rect popup_bottom_area;
	rect popup_left_area;
	rect popup_right_area;
	rect popup_center_area;

	/* list of tabs and exposay buttons */
	vector<tuple<rect, client_managed_t *, theme_tab_t *>> _client_buttons;
	vector<tuple<rect, client_managed_t *, int>> _exposay_buttons;
	shared_ptr<renderable_unmanaged_gaussian_shadow_t<16>> _exposay_mouse_over;

	void set_selected(client_managed_t * c);


	void start_fading();

	void _update_notebook_areas();
	void _update_theme_notebook(theme_notebook_t & theme_notebook) const;
	void _update_layout();

	void process_notebook_client_menu(client_managed_t * c, int selected);

	void _mouse_over_reset();
	void _mouse_over_set();

	rect compute_notebook_close_window_position(int number_of_client, int selected_client_index) const;
	rect compute_notebook_unbind_window_position(int number_of_client, int selected_client_index) const;
	rect compute_notebook_bookmark_position() const;
	rect compute_notebook_vsplit_position() const;
	rect compute_notebook_hsplit_position() const;
	rect compute_notebook_close_position() const;
	rect compute_notebook_menu_position() const;

	void client_title_change(client_managed_t * c);
	void client_destroy(client_managed_t * c);
	void client_activate(client_managed_t * c);
	void client_deactivate(client_managed_t * c);

	void update_allocation(rect & allocation);

	bool process_button_press_event(XEvent const * e);


	void remove_client(client_managed_t * c);

	void activate_client(client_managed_t * x);


	rect get_new_client_size();

	void select_next();
	void delete_all();

	notebook_t * get_nearest_notebook();

	auto find_client_tab(int x, int y) -> client_managed_t *;

	void update_close_area();

	rect compute_client_size(client_managed_t * c);
	rect const & get_allocation();
	void set_theme(theme_t const * theme);
	auto clients() const -> list<client_managed_t const *>;
	auto selected() const -> client_managed_t const *;
	bool is_default() const;

	void raise_child(tree_t * t = nullptr);

	void render(cairo_t * cr, time_t time);
	bool need_render(time_t time);

	bool has_client(client_managed_t * c);
	void set_keep_selected(bool x);
	void _update_exposay();
	void stop_exposay();
	void start_client_menu(client_managed_t * c, xcb_button_t button, uint16_t x, uint16_t y);

public:

	notebook_t(page_context_t * ctx, bool keep_selected);
	~notebook_t();

	/**
	 * tree_t interface
	 **/
	virtual auto parent() const -> page_component_t *;
	virtual auto get_node_name() const -> string;
	virtual void remove(tree_t * src);
	virtual void set_parent(tree_t * t);
	virtual void children(vector<tree_t *> & out) const;
	virtual void get_visible_children(vector<tree_t *> & out);
	virtual void hide();
	virtual void show();
	virtual void prepare_render(vector<shared_ptr<renderable_t>> & out, time_t const & time);
	virtual void activate(tree_t * t = nullptr);
	virtual bool button_press(xcb_button_press_event_t const * ev);
	virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	virtual bool leave(xcb_leave_notify_event_t const * ev);

	/**
	 * page_component_t interface
	 **/
	virtual void set_allocation(rect const & area);
	virtual rect allocation() const;
	virtual void replace(page_component_t * src, page_component_t * by);

	/**
	 * notebook_t interface
	 **/
	void set_default(bool x);
	void render_legacy(cairo_t * cr) const;
	void start_exposay();
	void update_client_position(client_managed_t * c);
	void iconify_client(client_managed_t * x);
	bool add_client(client_managed_t * c, bool prefer_activate);
	auto get_clients() -> list<client_managed_t *> const &;

	/* TODO : remove it */
	friend grab_bind_client_t;

};

}

#endif /* NOTEBOOK_HXX_ */
