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
#include "renderable_notebook_fading.hxx"
#include "renderable_pixmap.hxx"
#include "renderable_empty.hxx"

#include "page_component.hxx"
#include "client_managed.hxx"
#include "renderable_thumbnail.hxx"
#include "renderable_unmanaged_gaussian_shadow.hxx"
#include "dropdown_menu.hxx"
#include "page-types.hxx"

namespace page {

using namespace std;

class client_managed_t;
class grab_bind_view_notebook_t;

class notebook_t : public page_component_t {
	time64_t const animation_duration;

	page_t * _ctx;

	rect _allocation;

	time64_t _swap_start;

	shared_ptr<renderable_thumbnail_t> tooltips;
	shared_ptr<renderable_notebook_fading_t> fading_notebook;
	vector<renderable_thumbnail_p> _exposay_thumbnail;

	tree_p _notebook_view_layer;
	tree_p _fading_notebook_layer;
	tree_p _tooltips_layer;


	theme_notebook_t _theme_notebook;

	int _theme_client_tabs_offset;
	vector<theme_tab_t> _theme_client_tabs;
	rect _theme_client_tabs_area;


	shared_ptr<timeout_t> _fading_timeout;

	bool _is_default;
	bool _exposay;

	bool _can_hsplit;
	bool _can_vsplit;
	bool _has_scroll_arrow;
	bool _has_pending_fading_timeout;

	struct {
		tuple<rect, view_notebook_w, theme_tab_t *> * tab;
		tuple<rect, view_notebook_w, int> * exposay;
	} _mouse_over;

	enum select_e {
		SELECT_NONE,
		SELECT_TAB,
		SELECT_TOP,
		SELECT_BOTTOM,
		SELECT_LEFT,
		SELECT_RIGHT
	};

	// list to maintain the client order
	list<view_notebook_p> _clients_tab_order;

	view_notebook_p _selected;

	rect _client_area;
	rect _client_position;

	struct {
		rect button_close;
		rect button_vsplit;
		rect button_hsplit;
		rect button_select;
		rect button_exposay;

		rect left_scroll_arrow;
		rect right_scroll_arrow;

		rect close_client;
		rect undck_client;

		rect tab;
		rect top;
		rect bottom;
		rect left;
		rect right;

		rect popup_top;
		rect popup_bottom;
		rect popup_left;
		rect popup_right;
		rect popup_center;

	} _area;

	/* list of tabs and exposay buttons */
	vector<tuple<rect, view_notebook_w, theme_tab_t *>> _client_buttons;
	vector<tuple<rect, view_notebook_w, int>> _exposay_buttons;
	shared_ptr<renderable_unmanaged_gaussian_shadow_t<16>> _exposay_mouse_over;

	void _start_fading();

	void _update_notebook_buttons_area();
	void _update_theme_notebook(theme_notebook_t & theme_notebook);
	void _update_all_layout();
	void _update_mouse_over(int x, int y);

	void _mouse_over_reset();
	void _mouse_over_set();

	rect _compute_notebook_close_window_position(int number_of_client, int selected_client_index) const;
	rect _compute_notebook_unbind_window_position(int number_of_client, int selected_client_index) const;
	rect _compute_notebook_bookmark_position() const;
	rect _compute_notebook_vsplit_position() const;
	rect _compute_notebook_hsplit_position() const;
	rect _compute_notebook_close_position() const;
	rect _compute_notebook_menu_position() const;

	void _client_title_change(client_managed_t * c);
	void _client_destroy(client_managed_t * c);
	void _client_focus_change(client_managed_t * c);

	void _update_allocation(rect & allocation);

	rect _get_new_client_size();

	void _select_next();

	rect _compute_client_size(shared_ptr<client_managed_t> c);

	auto clients() const -> list<shared_ptr<client_managed_t>>;
	auto selected() const -> view_notebook_p;
	bool is_default() const;

	bool _has_client(client_managed_p c);

	void _update_exposay();
	void _stop_exposay();
	void _start_client_menu(view_notebook_p c, xcb_button_t button, uint16_t x, uint16_t y, xcb_timestamp_t time);

	void _scroll_left(int x);
	void _scroll_right(int x);

	void _close_view_notebook(view_notebook_p w, xcb_timestamp_t time);

	void _set_theme_tab_offset(int x);
	void _set_selected(view_notebook_p c);
	void _add_client_view(view_notebook_p vn, xcb_timestamp_t time);

	void activate(view_notebook_p c, xcb_timestamp_t time);

	auto shared_from_this() -> notebook_p;

	shared_ptr<pixmap_t> _render_to_pixmap();

public:

	notebook_t(tree_t * ref);
	virtual ~notebook_t();

	void remove_view_notebook(view_notebook_p vn);

	/**
	 * tree_t interface
	 **/
	virtual auto get_node_name() const -> string;
	using tree_t::remove;
	using tree_t::hide;
	virtual void show() override;
	virtual void update_layout(time64_t const time);

	virtual auto button_press(xcb_button_press_event_t const * ev)  -> button_action_e;
	virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	virtual bool leave(xcb_leave_notify_event_t const * ev);
	virtual void render(cairo_t * cr, region const & area);
	virtual void render_finished();
	using tree_t::reconfigure;
	virtual void on_workspace_enable() override;
	virtual void on_workspace_disable() override;

	virtual region get_opaque_region();
	virtual region get_visible_region();
	virtual region get_damaged();
	virtual void queue_redraw();

	/**
	 * page_component_t interface
	 **/
	virtual void set_allocation(rect const & area);
	virtual rect allocation() const;
	virtual void replace(shared_ptr<page_component_t> src, shared_ptr<page_component_t> by);
	virtual void get_min_allocation(int & width, int & height) const;

	/**
	 * notebook_t interface
	 **/
	void set_default(bool x);
	void render_legacy(cairo_t * cr);
	void start_exposay();
	void update_client_position(view_notebook_p c);
	void iconify_client(view_notebook_p x);
	bool add_client(client_managed_p c, xcb_timestamp_t time);
	void add_client_from_view(view_rebased_p c, xcb_timestamp_t time);

	/* TODO : remove it */
	friend struct grab_bind_view_notebook_t;
	friend struct grab_bind_view_floating_t;
	friend struct view_notebook_t;

};

using notebook_p = shared_ptr<notebook_t>;
using notebook_w = weak_ptr<notebook_t>;

}

#endif /* NOTEBOOK_HXX_ */
