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

#include "page_context.hxx"
#include "page_component.hxx"
#include "client_managed.hxx"
#include "renderable_thumbnail.hxx"
#include "renderable_unmanaged_gaussian_shadow.hxx"
#include "dropdown_menu.hxx"

namespace page {

using namespace std;

class client_managed_t;
class grab_bind_client_t;

class notebook_t : public page_component_t {
	time64_t const animation_duration;

	page_context_t * _ctx;

	rect _allocation;

	time64_t _swap_start;

	shared_ptr<renderable_thumbnail_t> tooltips;
	shared_ptr<renderable_notebook_fading_t> fading_notebook;
	vector<renderable_thumbnail_p> _exposay_thumbnail;


	theme_notebook_t _theme_notebook;

	int _theme_client_tabs_offset;
	vector<theme_tab_t> _theme_client_tabs;
	rect _theme_client_tabs_area;

	bool _is_default;
	bool _exposay;

	bool _can_hsplit;
	bool _can_vsplit;
	bool _has_scroll_arrow;
	bool _layout_is_durty;
	bool _has_mouse_change;

	struct {
		int event_x;
		int event_y;
		tuple<rect, weak_ptr<client_managed_t>, theme_tab_t *> * tab;
		tuple<rect, weak_ptr<client_managed_t>, int> * exposay;
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
		client_managed_p client;
		signal_handler_t  title_change_func;
		signal_handler_t  destoy_func;
		signal_handler_t  focus_change_func;

		_client_context_t() = delete;
		_client_context_t(_client_context_t const & x) = default;
		_client_context_t(notebook_t * nbk, client_managed_p client);
		~_client_context_t();

		bool operator==(client_managed_p client) const;

	};

	using _client_context_p = shared_ptr<_client_context_t>;
	using _client_context_w = weak_ptr<_client_context_t>;

	// list to maintain the client order
	list<_client_context_t> _clients_tab_order;

	shared_ptr<client_managed_t> _selected;

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
	vector<tuple<rect, weak_ptr<client_managed_t>, theme_tab_t *>> _client_buttons;
	vector<tuple<rect, weak_ptr<client_managed_t>, int>> _exposay_buttons;
	shared_ptr<renderable_unmanaged_gaussian_shadow_t<16>> _exposay_mouse_over;

	void _set_selected(shared_ptr<client_managed_t> c);


	void _start_fading();

	void _update_notebook_areas();
	void _update_theme_notebook(theme_notebook_t & theme_notebook);
	void _update_layout();
	void _update_mouse_over();

	void _process_notebook_client_menu(dropdown_menu_t<int> * ths, shared_ptr<client_managed_t> c, int selected);

	void _mouse_over_reset();
	void _mouse_over_set();

	rect _compute_notebook_close_window_position(int number_of_client, int selected_client_index) const;
	rect _compute_notebook_unbind_window_position(int number_of_client, int selected_client_index) const;
	rect _compute_notebook_bookmark_position() const;
	rect _compute_notebook_vsplit_position() const;
	rect _compute_notebook_hsplit_position() const;
	rect _compute_notebook_close_position() const;
	rect _compute_notebook_menu_position() const;

	void _client_title_change(shared_ptr<client_managed_t> c);
	void _client_destroy(client_managed_t * c);
	void _client_focus_change(shared_ptr<client_managed_t> c);

	void _update_allocation(rect & allocation);

	void _remove_client(shared_ptr<client_managed_t> c);

	void _activate_client(shared_ptr<client_managed_t> x);


	rect _get_new_client_size();

	void _select_next();

	rect _compute_client_size(shared_ptr<client_managed_t> c);

	auto clients() const -> list<shared_ptr<client_managed_t>>;
	auto selected() const -> shared_ptr<client_managed_t>;
	bool is_default() const;

	bool _has_client(shared_ptr<client_managed_t> c);
	list<_client_context_t>::iterator _find_client_context(client_managed_p client);

	void _update_exposay();
	void _stop_exposay();
	void _start_client_menu(shared_ptr<client_managed_t> c, xcb_button_t button, uint16_t x, uint16_t y);

	void _scroll_left(int x);
	void _scroll_right(int x);

	void _set_theme_tab_offset(int x);

	shared_ptr<notebook_t> shared_from_this();


public:

	notebook_t(page_context_t * ctx);
	virtual ~notebook_t();

	/**
	 * tree_t interface
	 **/
	virtual auto get_node_name() const -> string;
	virtual void remove(shared_ptr<tree_t> src);
	virtual void append_children(vector<shared_ptr<tree_t>> & out) const;
	virtual void hide();
	virtual void show();
	virtual void update_layout(time64_t const time);

	virtual void activate();
	virtual void activate(shared_ptr<tree_t> t);

	virtual bool button_press(xcb_button_press_event_t const * ev);
	virtual bool button_motion(xcb_motion_notify_event_t const * ev);
	virtual bool leave(xcb_leave_notify_event_t const * ev);
	virtual void render(cairo_t * cr, region const & area);
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
	void update_client_position(shared_ptr<client_managed_t> c);
	void iconify_client(shared_ptr<client_managed_t> x);
	bool add_client(shared_ptr<client_managed_t> c, bool prefer_activate);

	/* TODO : remove it */
	friend grab_bind_client_t;

};

using notebook_p = shared_ptr<notebook_t>;
using notebook_w = weak_ptr<notebook_t>;

}

#endif /* NOTEBOOK_HXX_ */
