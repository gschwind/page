/*
 * managed_window.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "leak_checker.hxx"

#include <cairo.h>
#include <cairo-xlib.h>
#include <cairo-xcb.h>

#include "renderable_floating_outer_gradien.hxx"
#include "client_managed.hxx"
#include "notebook.hxx"
#include "utils.hxx"
#include "grab_handlers.hxx"

namespace page {

using namespace std;

client_managed_t::client_managed_t(page_context_t * ctx, xcb_window_t w, xcb_atom_t net_wm_type) :
				client_base_t{ctx, w},
				_managed_type{MANAGED_FLOATING},
				_net_wm_type{net_wm_type},
				_floating_wished_position{},
				_notebook_wished_position{},
				_wished_position{},
				_orig_position{},
				_base_position{},
				_surf{nullptr},
				_top_buffer{nullptr},
				_bottom_buffer{nullptr},
				_left_buffer{nullptr},
				_right_buffer{nullptr},
				_icon(nullptr),
				_orig_visual{0},
				_orig_depth{-1},
				_deco_visual{0},
				_deco_depth{-1},
				//_orig(props->id()),
				_base{XCB_WINDOW_NONE},
				_deco{XCB_WINDOW_NONE},
				_has_focus{false},
				_is_iconic{true},
				_demands_attention{false},
				_client_view{nullptr}
{

	_update_title();
	rect pos{_client_proxy->position()};

	printf("window default position = %s\n", pos.to_string().c_str());

	if(net_wm_type == A(_NET_WM_WINDOW_TYPE_DOCK))
		_managed_type = MANAGED_DOCK;

	_floating_wished_position = pos;
	_notebook_wished_position = pos;
	_base_position = pos;
	_orig_position = pos;

	_apply_floating_hints_constraint();


	_orig_visual = _client_proxy->wa().visual;
	_orig_depth = _client_proxy->geometry().depth;

	/** if x == 0 then place window at center of the screen **/
	if (_floating_wished_position.x == 0 and not is(MANAGED_DOCK)) {
		_floating_wished_position.x =
				(_client_proxy->geometry().width - _floating_wished_position.w) / 2;
	}

	if(_floating_wished_position.x - _ctx->theme()->floating.margin.left < 0) {
		_floating_wished_position.x = _ctx->theme()->floating.margin.left;
	}

	/**
	 * if y == 0 then place window at center of the screen
	 **/
	if (_floating_wished_position.y == 0 and not is(MANAGED_DOCK)) {
		_floating_wished_position.y = (_client_proxy->geometry().height - _floating_wished_position.h) / 2;
	}

	if(_floating_wished_position.y - _ctx->theme()->floating.margin.top < 0) {
		_floating_wished_position.y = _ctx->theme()->floating.margin.top;
	}

	/**
	 * Create the base window, window that will content managed window
	 **/

	xcb_window_t wbase;
	xcb_window_t wdeco;
	rect b = _floating_wished_position;

	xcb_visualid_t root_visual = cnx()->root_visual()->visual_id;
	int root_depth = cnx()->find_visual_depth(cnx()->root_visual()->visual_id);

	/**
	 * If window visual is 32 bit (have alpha channel, and root do not
	 * have alpha channel, use the window visual, otherwise always prefer
	 * root visual.
	 **/
	if (_orig_depth == 32 and root_depth != 32) {
		_deco_visual = _orig_visual;
		_deco_depth = _orig_depth;

		/** if visual is 32 bits, this values are mandatory **/
		xcb_colormap_t cmap = xcb_generate_id(cnx()->xcb());
		xcb_create_colormap(cnx()->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, cnx()->root(), _deco_visual);

		uint32_t value_mask = 0;
		uint32_t value[4];

		value_mask |= XCB_CW_BACK_PIXEL;
		value[0] = cnx()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_BORDER_PIXEL;
		value[1] = cnx()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_OVERRIDE_REDIRECT;
		value[2] = True;

		value_mask |= XCB_CW_COLORMAP;
		value[3] = cmap;

		wbase = xcb_generate_id(cnx()->xcb());
		xcb_create_window(cnx()->xcb(), _deco_depth, wbase, cnx()->root(), -10, -10, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _deco_visual, value_mask, value);
		wdeco = xcb_generate_id(cnx()->xcb());
		xcb_create_window(cnx()->xcb(), _deco_depth, wdeco, wbase, b.x, b.y, b.w, b.h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _deco_visual, value_mask, value);

	} else {

		/**
		 * Create RGB window for back ground
		 **/

		_deco_visual = cnx()->default_visual_rgba()->visual_id;
		_deco_depth = 32;

		/** if visual is 32 bits, this values are mandatory **/
		xcb_colormap_t cmap = xcb_generate_id(cnx()->xcb());
		xcb_create_colormap(cnx()->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, cnx()->root(), _deco_visual);

		/**
		 * To create RGBA window, the following field MUST bet set, for unknown
		 * reason. i.e. border_pixel, background_pixel and colormap.
		 **/
		uint32_t value_mask = 0;
		uint32_t value[4];

		value_mask |= XCB_CW_BACK_PIXEL;
		value[0] = cnx()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_BORDER_PIXEL;
		value[1] = cnx()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_OVERRIDE_REDIRECT;
		value[2] = True;

		value_mask |= XCB_CW_COLORMAP;
		value[3] = cmap;

		wbase = xcb_generate_id(cnx()->xcb());
		xcb_create_window(cnx()->xcb(), _deco_depth, wbase, cnx()->root(), -10, -10, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _deco_visual, value_mask, value);
		wdeco = xcb_generate_id(cnx()->xcb());
		xcb_create_window(cnx()->xcb(), _deco_depth, wdeco, wbase, b.x, b.y, b.w, b.h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _deco_visual, value_mask, value);

	}

	_base = wbase;
	_deco = wdeco;

	update_floating_areas();

	uint32_t cursor;

	cursor = cnx()->xc_top_side;
	_input_top = cnx()->create_input_only_window(_deco, _area_top, XCB_CW_CURSOR, &cursor);
	cursor = cnx()->xc_bottom_side;
	_input_bottom = cnx()->create_input_only_window(_deco, _area_bottom, XCB_CW_CURSOR, &cursor);
	cursor = cnx()->xc_left_side;
	_input_left = cnx()->create_input_only_window(_deco, _area_left, XCB_CW_CURSOR, &cursor);
	cursor = cnx()->xc_right_side;
	_input_right = cnx()->create_input_only_window(_deco, _area_right, XCB_CW_CURSOR, &cursor);

	cursor = cnx()->xc_top_left_corner;
	_input_top_left = cnx()->create_input_only_window(_deco, _area_top_left, XCB_CW_CURSOR, &cursor);
	cursor = cnx()->xc_top_right_corner;
	_input_top_right = cnx()->create_input_only_window(_deco, _area_top_right, XCB_CW_CURSOR, &cursor);
	cursor = cnx()->xc_bottom_left_corner;
	_input_bottom_left = cnx()->create_input_only_window(_deco, _area_bottom_left, XCB_CW_CURSOR, &cursor);
	cursor = cnx()->xc_bottom_righ_corner;
	_input_bottom_right = cnx()->create_input_only_window(_deco, _area_bottom_right, XCB_CW_CURSOR, &cursor);

	_client_proxy->add_to_save_set();
	select_inputs_unsafe();
	grab_button_unfocused_unsafe();
	_client_proxy->set_border_width(0);
	_client_proxy->reparentwindow(_base, 0, 0);

	_surf = cairo_xcb_surface_create(cnx()->xcb(), _deco, cnx()->find_visual(_deco_visual), b.w, b.h);

	net_wm_state_add(_NET_WM_STATE_FOCUSED);
	grab_button_focused_unsafe();

	update_icon();

}

client_managed_t::~client_managed_t() {

	_ctx->add_global_damage(get_visible_region());

	_client_view = nullptr;

	on_destroy.signal(this);

	unselect_inputs_unsafe();

	if (_surf != nullptr) {
		warn(cairo_surface_get_reference_count(_surf) == 1);
		cairo_surface_destroy(_surf);
		_surf = nullptr;
	}

	destroy_back_buffer();

	xcb_destroy_window(cnx()->xcb(), _input_top);
	xcb_destroy_window(cnx()->xcb(), _input_left);
	xcb_destroy_window(cnx()->xcb(), _input_right);
	xcb_destroy_window(cnx()->xcb(), _input_bottom);
	xcb_destroy_window(cnx()->xcb(), _input_top_left);
	xcb_destroy_window(cnx()->xcb(), _input_top_right);
	xcb_destroy_window(cnx()->xcb(), _input_bottom_left);
	xcb_destroy_window(cnx()->xcb(), _input_bottom_right);

	xcb_destroy_window(cnx()->xcb(), _deco);
	xcb_destroy_window(cnx()->xcb(), _base);

	_client_proxy->delete_net_wm_state();
	_client_proxy->delete_wm_state();

}

auto client_managed_t::shared_from_this() -> shared_ptr<client_managed_t> {
	return dynamic_pointer_cast<client_managed_t>(tree_t::shared_from_this());
}

void client_managed_t::reconfigure() {
	_damage_cache += get_visible_region();

	if (is(MANAGED_FLOATING)) {
		_wished_position = _floating_wished_position;

		if (prefer_window_border()) {
			_base_position.x = _wished_position.x
					- _ctx->theme()->floating.margin.left;
			_base_position.y = _wished_position.y - _ctx->theme()->floating.margin.top;
			_base_position.w = _wished_position.w + _ctx->theme()->floating.margin.left
					+ _ctx->theme()->floating.margin.right;
			_base_position.h = _wished_position.h + _ctx->theme()->floating.margin.top
					+ _ctx->theme()->floating.margin.bottom + _ctx->theme()->floating.title_height;

			_orig_position.x = _ctx->theme()->floating.margin.left;
			_orig_position.y = _ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height;
			_orig_position.w = _wished_position.w;
			_orig_position.h = _wished_position.h;

		} else {
			/* floating window without borders */
			_base_position = _wished_position;

			_orig_position.x = 0;
			_orig_position.y = 0;
			_orig_position.w = _wished_position.w;
			_orig_position.h = _wished_position.h;

		}

		/* avoid to hide title bar of floating windows */
		if(_base_position.y < 0) {
			_base_position.y = 0;
		}

		cairo_xcb_surface_set_size(_surf, _base_position.w, _base_position.h);

	} else if (is(MANAGED_DOCK)) {
		_wished_position = _floating_wished_position;
		_base_position = _wished_position;
		_orig_position.x = 0;
		_orig_position.y = 0;
		_orig_position.w = _wished_position.w;
		_orig_position.h = _wished_position.h;
	} else {
		_wished_position = _notebook_wished_position;
		_base_position = _notebook_wished_position;
		_orig_position = rect(0, 0, _base_position.w, _base_position.h);
	}

	_is_resized = true;
	destroy_back_buffer();
	update_floating_areas();

	if(_is_iconic or not _is_visible) {
		/* if iconic move outside visible area */
		cnx()->move_resize(_base, rect{_ctx->left_most_border()-1-_base_position.w, _ctx->top_most_border(), _base_position.w, _base_position.h});
	} else {
		cnx()->move_resize(_base, _base_position);
	}
	cnx()->move_resize(_deco,
			rect{0, 0, _base_position.w, _base_position.h});
	_client_proxy->move_resize(_orig_position);

	cnx()->move_resize(_input_top, _area_top);
	cnx()->move_resize(_input_bottom, _area_bottom);
	cnx()->move_resize(_input_right, _area_right);
	cnx()->move_resize(_input_left, _area_left);

	cnx()->move_resize(_input_top_left, _area_top_left);
	cnx()->move_resize(_input_top_right, _area_top_right);
	cnx()->move_resize(_input_bottom_left, _area_bottom_left);
	cnx()->move_resize(_input_bottom_right, _area_bottom_right);

	fake_configure_unsafe();

	_update_opaque_region();
	_update_visible_region();
	_damage_cache += get_visible_region();
}

void client_managed_t::fake_configure_unsafe() {
	//printf("fake_reconfigure = %dx%d+%d+%d\n", _wished_position.w,
	//		_wished_position.h, _wished_position.x, _wished_position.y);
	_client_proxy->fake_configure(_wished_position, 0);
}

void client_managed_t::delete_window(xcb_timestamp_t t) {
	printf("request close for '%s'\n", title().c_str());
	_client_proxy->delete_window(t);
}

void client_managed_t::set_managed_type(managed_window_type_e type) {
	if(_managed_type == MANAGED_DOCK) {
		std::list<atom_e> net_wm_allowed_actions;
		_client_proxy->net_wm_allowed_actions_set(net_wm_allowed_actions);
		reconfigure();
	} else {

		std::list<atom_e> net_wm_allowed_actions;
		net_wm_allowed_actions.push_back(_NET_WM_ACTION_CLOSE);
		net_wm_allowed_actions.push_back(_NET_WM_ACTION_FULLSCREEN);
		_client_proxy->net_wm_allowed_actions_set(net_wm_allowed_actions);

		_managed_type = type;

		reconfigure();
	}
}

void client_managed_t::focus(xcb_timestamp_t t) {
	icccm_focus_unsafe(t);
}

rect client_managed_t::get_base_position() const {
	return _base_position;
}

managed_window_type_e client_managed_t::get_type() {
	return _managed_type;
}

bool client_managed_t::is(managed_window_type_e type) {
	return _managed_type == type;
}

void client_managed_t::icccm_focus_unsafe(xcb_timestamp_t t) {

	if(_demands_attention) {
		_demands_attention = false;
		_client_proxy->net_wm_state_remove(_NET_WM_STATE_DEMANDS_ATTENTION);
	}

	if (_client_proxy->wm_hints() != nullptr) {
		if (_client_proxy->wm_hints()->input != False) {
			_client_proxy->set_input_focus(XCB_INPUT_FOCUS_NONE, t);
		}
	} else {
		/** no WM_HINTS, guess hints.input == True **/
		_client_proxy->set_input_focus(XCB_INPUT_FOCUS_NONE, t);
	}

	if (_client_proxy->wm_protocols() != nullptr) {
		if (has_key(*(_client_proxy->wm_protocols()),
				static_cast<xcb_atom_t>(A(WM_TAKE_FOCUS)))) {

			xcb_client_message_event_t ev;
			ev.response_type = XCB_CLIENT_MESSAGE;
			ev.format = 32;
			ev.type = A(WM_PROTOCOLS);
			ev.window = _client_proxy->id();
			ev.data.data32[0] = A(WM_TAKE_FOCUS);
			ev.data.data32[1] = t;

			_client_proxy->send_event(0, XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<char*>(&ev));

		}
	}

}

void client_managed_t::compute_floating_areas() {

	rect position{0, 0, _base_position.w, _base_position.h};

	_floating_area.close_button = compute_floating_close_position(position);
	_floating_area.bind_button = compute_floating_bind_position(position);

	int x0 = _ctx->theme()->floating.margin.left;
	int x1 = position.w - _ctx->theme()->floating.margin.right;

	int y0 = _ctx->theme()->floating.margin.bottom;
	int y1 = position.h - _ctx->theme()->floating.margin.bottom;

	int w0 = position.w - _ctx->theme()->floating.margin.left
			- _ctx->theme()->floating.margin.right;
	int h0 = position.h - _ctx->theme()->floating.margin.bottom
			- _ctx->theme()->floating.margin.bottom;

	_floating_area.title_button = rect(x0, y0, w0, _ctx->theme()->floating.title_height);

	_floating_area.grip_top = rect(x0, 0, w0, _ctx->theme()->floating.margin.top);
	_floating_area.grip_bottom = rect(x0, y1, w0, _ctx->theme()->floating.margin.bottom);
	_floating_area.grip_left = rect(0, y0, _ctx->theme()->floating.margin.left, h0);
	_floating_area.grip_right = rect(x1, y0, _ctx->theme()->floating.margin.right, h0);
	_floating_area.grip_top_left = rect(0, 0, _ctx->theme()->floating.margin.left, _ctx->theme()->floating.margin.top);
	_floating_area.grip_top_right = rect(x1, 0, _ctx->theme()->floating.margin.right, _ctx->theme()->floating.margin.top);
	_floating_area.grip_bottom_left = rect(0, y1, _ctx->theme()->floating.margin.left, _ctx->theme()->floating.margin.bottom);
	_floating_area.grip_bottom_right = rect(x1, y1, _ctx->theme()->floating.margin.right, _ctx->theme()->floating.margin.bottom);

}

rect client_managed_t::compute_floating_close_position(rect const & allocation) const {

	rect position;
	position.x = allocation.w - _ctx->theme()->floating.close_width;
	position.y = 0.0;
	position.w = _ctx->theme()->floating.close_width;
	position.h = _ctx->theme()->floating.title_height;

	return position;
}

rect client_managed_t::compute_floating_bind_position(rect const & allocation) const {

	rect position;
	position.x = allocation.w - _ctx->theme()->floating.bind_width - _ctx->theme()->floating.close_width;
	position.y = 0.0;
	position.w = _ctx->theme()->floating.bind_width;
	position.h = _ctx->theme()->floating.title_height;

	return position;
}

/**
 * set usual passive button grab for a focused client.
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void client_managed_t::grab_button_focused_unsafe() {
	/** First ungrab all **/
	ungrab_all_button_unsafe();
	/** for decoration, grab all **/
	xcb_grab_button(cnx()->xcb(), false, _deco, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_1, XCB_MOD_MASK_ANY);
	xcb_grab_button(cnx()->xcb(), false, _deco, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_2, XCB_MOD_MASK_ANY);
	xcb_grab_button(cnx()->xcb(), false, _deco, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_3, XCB_MOD_MASK_ANY);

	/** grab alt-button1 move **/
	xcb_grab_button(cnx()->xcb(), false, _base, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_1, XCB_MOD_MASK_1/*ALT*/);

	/** grab alt-button3 resize **/
	xcb_grab_button(cnx()->xcb(), false, _base, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_3, XCB_MOD_MASK_1/*ALT*/);

}

/**
 * set usual passive button grab for a not focused client.
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void client_managed_t::grab_button_unfocused_unsafe() {
	/** First ungrab all **/
	ungrab_all_button_unsafe();

	xcb_grab_button(cnx()->xcb(), false, _base, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_1, XCB_MOD_MASK_ANY);

	xcb_grab_button(cnx()->xcb(), false, _base, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_2, XCB_MOD_MASK_ANY);

	xcb_grab_button(cnx()->xcb(), false, _base, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_3, XCB_MOD_MASK_ANY);

	/** for decoration, grab all **/
	xcb_grab_button(cnx()->xcb(), false, _deco, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_1, XCB_MOD_MASK_ANY);

	xcb_grab_button(cnx()->xcb(), false, _deco, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_2, XCB_MOD_MASK_ANY);

	xcb_grab_button(cnx()->xcb(), false, _deco, DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE,
			XCB_NONE, XCB_BUTTON_INDEX_3, XCB_MOD_MASK_ANY);
}


/**
 * Remove all passive grab on windows
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void client_managed_t::ungrab_all_button_unsafe() {
	xcb_ungrab_button(cnx()->xcb(), XCB_BUTTON_INDEX_ANY, _base, XCB_MOD_MASK_ANY);
	xcb_ungrab_button(cnx()->xcb(), XCB_BUTTON_INDEX_ANY, _deco, XCB_MOD_MASK_ANY);
	_client_proxy->ungrab_button(XCB_BUTTON_INDEX_ANY, XCB_MOD_MASK_ANY);
}

/**
 * select usual input events
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void client_managed_t::select_inputs_unsafe() {
	cnx()->select_input(_base, MANAGED_BASE_WINDOW_EVENT_MASK);
	cnx()->select_input(_deco, MANAGED_DECO_WINDOW_EVENT_MASK);
	_client_proxy->select_input(MANAGED_ORIG_WINDOW_EVENT_MASK);
	_client_proxy->select_input_shape(true);
}

/**
 * Remove all selected input event
 *
 * unsafe: need to lock the _orig window to use it.
 **/
void client_managed_t::unselect_inputs_unsafe() {
	cnx()->select_input(_base, XCB_EVENT_MASK_NO_EVENT);
	cnx()->select_input(_deco, XCB_EVENT_MASK_NO_EVENT);
	_client_proxy->select_input(XCB_EVENT_MASK_NO_EVENT);
	_client_proxy->select_input_shape(false);
}

bool client_managed_t::is_fullscreen() {
	if (_client_proxy->net_wm_state() != nullptr) {
		return has_key(*(_client_proxy->net_wm_state()),
				static_cast<xcb_atom_t>(A(_NET_WM_STATE_FULLSCREEN)));
	}
	return false;
}

bool client_managed_t::skip_task_bar() {
	if (_client_proxy->net_wm_state() != nullptr) {
		return has_key(*(_client_proxy->net_wm_state()),
				static_cast<xcb_atom_t>(A(_NET_WM_STATE_SKIP_TASKBAR)));
	}
	return false;
}

xcb_atom_t client_managed_t::net_wm_type() {
	return _net_wm_type;
}

bool client_managed_t::get_wm_normal_hints(XSizeHints * size_hints) {
	if(_client_proxy->wm_normal_hints() != nullptr) {
		*size_hints = *(_client_proxy->wm_normal_hints());
		return true;
	} else {
		return false;
	}
}

void client_managed_t::net_wm_state_add(atom_e atom) {
	_client_proxy->net_wm_state_add(atom);
}

void client_managed_t::net_wm_state_remove(atom_e atom) {
	_client_proxy->net_wm_state_remove(atom);
}

void client_managed_t::net_wm_state_delete() {
	/**
	 * This one is for removing the window manager tag, thus only check if the window
	 * still exist. (don't need lock);
	 **/
	_client_proxy->delete_net_wm_state();
}

void client_managed_t::normalize() {
	if(not _is_iconic)
		return;
	_is_iconic = false;
	_client_proxy->set_wm_state(NormalState);
	for (auto c : filter_class<client_managed_t>(_children)) {
		c->normalize();
	}
}

void client_managed_t::iconify() {
	if(_is_iconic)
		return;
	_is_iconic = true;
	_client_proxy->set_wm_state(IconicState);
	for (auto c : filter_class<client_managed_t>(_children)) {
		c->iconify();
	}

}

void client_managed_t::wm_state_delete() {
	/**
	 * This one is for removing the window manager tag, thus only check if the window
	 * still exist. (don't need lock);
	 **/

	_client_proxy->delete_wm_state();
}

void client_managed_t::set_floating_wished_position(rect const & pos) {
	_floating_wished_position = pos;
}

void client_managed_t::set_notebook_wished_position(rect const & pos) {
	_notebook_wished_position = pos;
}

rect const & client_managed_t::get_wished_position() {
	return _wished_position;
}

rect const & client_managed_t::get_floating_wished_position() {
	return _floating_wished_position;
}

void client_managed_t::destroy_back_buffer() {
	_top_buffer = nullptr;
	_bottom_buffer = nullptr;
	_left_buffer = nullptr;
	_right_buffer = nullptr;
}

void client_managed_t::create_back_buffer() {

	if (not is(MANAGED_FLOATING) or not prefer_window_border()) {
		destroy_back_buffer();
		return;
	}

	{
		int w = _base_position.w;
		int h = _ctx->theme()->floating.margin.top
				+ _ctx->theme()->floating.title_height;
		if(w > 0 and h > 0) {
			_top_buffer = make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGBA, w, h);
		}
	}

	{
		int w = _base_position.w;
		int h = _ctx->theme()->floating.margin.bottom;
		if(w > 0 and h > 0) {
			_bottom_buffer =
					make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGBA, w, h);
		}
	}

	{
		int w = _ctx->theme()->floating.margin.left;
		int h = _base_position.h - _ctx->theme()->floating.margin.top
				- _ctx->theme()->floating.margin.bottom;
		if(w > 0 and h > 0) {
			_left_buffer =
					make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGBA, w, h);
		}
	}

	{
		int w = _ctx->theme()->floating.margin.right;
		int h = _base_position.h - _ctx->theme()->floating.margin.top
				- _ctx->theme()->floating.margin.bottom;
		if(w > 0 and h > 0) {
			_right_buffer =
					make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGBA, w, h);
		}
	}
}

void client_managed_t::update_floating_areas() {
	theme_managed_window_t tm;
	tm.position = _base_position;
	tm.title = _title;

	int x0 = _ctx->theme()->floating.margin.left;
	int x1 = _base_position.w - _ctx->theme()->floating.margin.right;

	int y0 = _ctx->theme()->floating.margin.bottom;
	int y1 = _base_position.h - _ctx->theme()->floating.margin.bottom;

	int w0 = _base_position.w - _ctx->theme()->floating.margin.left
			- _ctx->theme()->floating.margin.right;
	int h0 = _base_position.h - _ctx->theme()->floating.margin.bottom
			- _ctx->theme()->floating.margin.bottom;

	_area_top = rect(x0, 0, w0, _ctx->theme()->floating.margin.bottom);
	_area_bottom = rect(x0, y1, w0, _ctx->theme()->floating.margin.bottom);
	_area_left = rect(0, y0, _ctx->theme()->floating.margin.left, h0);
	_area_right = rect(x1, y0, _ctx->theme()->floating.margin.right, h0);

	_area_top_left = rect(0, 0, _ctx->theme()->floating.margin.left,
			_ctx->theme()->floating.margin.bottom);
	_area_top_right = rect(x1, 0, _ctx->theme()->floating.margin.right,
			_ctx->theme()->floating.margin.bottom);
	_area_bottom_left = rect(0, y1, _ctx->theme()->floating.margin.left,
			_ctx->theme()->floating.margin.bottom);
	_area_bottom_right = rect(x1, y1, _ctx->theme()->floating.margin.right,
			_ctx->theme()->floating.margin.bottom);

	compute_floating_areas();

}

bool client_managed_t::has_window(xcb_window_t w) const {
	return w == _client_proxy->id() or w == _base or w == _deco;
}

string client_managed_t::get_node_name() const {
	string s = _get_node_name<'M'>();
	ostringstream oss;
	oss << s << " " << orig() << " " << title();

	oss << " " << _client_proxy->geometry().width << "x" <<
			_client_proxy->geometry().height << "+" <<
			_client_proxy->geometry().x << "+" << _client_proxy->geometry().y;

	return oss.str();
}

display_t * client_managed_t::cnx() {
	return _client_proxy->cnx();
}

rect const & client_managed_t::base_position() const {
	return _base_position;
}

rect const & client_managed_t::orig_position() const {
	return _orig_position;
}

region client_managed_t::get_visible_region() {
	return _visible_region_cache;
}

region client_managed_t::get_opaque_region() {
	return _opaque_region_cache;
}

region client_managed_t::get_damaged() {
	return _damage_cache;
}

void client_managed_t::update_layout(time64_t const time) {
	if(not _is_visible)
		return;

	/** update damage_cache **/
	region dmg = _client_view->get_damaged();
	dmg.translate(_base_position.x, _base_position.y);
	_damage_cache += dmg;
	_client_view->clear_damaged();

}

void client_managed_t::render_finished() {
	_damage_cache.clear();
}


void client_managed_t::set_focus_state(bool is_focused) {
	_has_change = true;

	_has_focus = is_focused;
	if (_has_focus) {
		net_wm_state_add(_NET_WM_STATE_FOCUSED);
	} else {
		net_wm_state_remove(_NET_WM_STATE_FOCUSED);
	}

	on_focus_change.signal(shared_from_this());
	queue_redraw();
}

void client_managed_t::net_wm_allowed_actions_add(atom_e atom) {
	_client_proxy->net_wm_allowed_actions_add(atom);
}

void client_managed_t::map_unsafe() {
	_client_proxy->xmap();
	cnx()->map(_deco);
	cnx()->map(_base);

	cnx()->map(_input_top);
	cnx()->map(_input_left);
	cnx()->map(_input_right);
	cnx()->map(_input_bottom);

	cnx()->map(_input_top_left);
	cnx()->map(_input_top_right);
	cnx()->map(_input_bottom_left);
	cnx()->map(_input_bottom_right);
}

void client_managed_t::unmap_unsafe() {
	cnx()->unmap(_base);
	cnx()->unmap(_deco);

	//_client_proxy->unmap();

	cnx()->unmap(_input_top);
	cnx()->unmap(_input_left);
	cnx()->unmap(_input_right);
	cnx()->unmap(_input_bottom);

	cnx()->unmap(_input_top_left);
	cnx()->unmap(_input_top_right);
	cnx()->unmap(_input_bottom_left);
	cnx()->unmap(_input_bottom_right);
}

void client_managed_t::hide() {
	for(auto x: _children) {
		x->hide();
	}

	_ctx->add_global_damage(get_visible_region());

	_is_visible = false;
	// do not unmap, just put it outside the screen.
	//unmap();
	reconfigure();

	/* we no not need the view anymore */
	_client_view = nullptr;

}

void client_managed_t::show() {
	_is_visible = true;
	reconfigure();
	map_unsafe();
	for(auto x: _children) {
		x->show();
	}

	if(_client_view == nullptr)
		_client_view = create_view();
}

bool client_managed_t::is_iconic() {
	return _is_iconic;
}

bool client_managed_t::is_stiky() {
	if(_client_proxy->net_wm_state() != nullptr) {
		return has_key(*_client_proxy->net_wm_state(), A(_NET_WM_STATE_STICKY));
	}
	return false;
}

bool client_managed_t::is_modal() {
	if(_client_proxy->net_wm_state() != nullptr) {
		return has_key(*_client_proxy->net_wm_state(), A(_NET_WM_STATE_MODAL));
	}
	return false;
}

void client_managed_t::activate() {

	if(_parent != nullptr) {
		_parent->activate(shared_from_this());
	}

	if(is_iconic()) {
		normalize();
		queue_redraw();
	}

}

bool client_managed_t::button_press(xcb_button_press_event_t const * e) {

	if (not has_window(e->event)) {
		return false;
	}

	if (is(MANAGED_FLOATING)
			and e->detail == XCB_BUTTON_INDEX_1
			and (e->state & XCB_MOD_MASK_1)) {
		_ctx->grab_start(new grab_floating_move_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y});
		return true;
	} else if (is(MANAGED_FLOATING)
			and e->detail == XCB_BUTTON_INDEX_3
			and (e->state & XCB_MOD_MASK_1)) {
		_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_BOTTOM_RIGHT});
		return true;
	} else if (is(MANAGED_FLOATING)
			and e->detail == XCB_BUTTON_INDEX_1
			and e->child != orig()
			and e->event == deco()) {

		if (_floating_area.close_button.is_inside(e->event_x, e->event_y)) {
			delete_window(e->time);
		} else if (_floating_area.bind_button.is_inside(e->event_x, e->event_y)) {
			rect absolute_position = _floating_area.bind_button;
			absolute_position.x += base_position().x;
			absolute_position.y += base_position().y;
			_ctx->grab_start(new grab_bind_client_t{_ctx, shared_from_this(), e->detail, absolute_position});
		} else if (_floating_area.title_button.is_inside(e->event_x, e->event_y)) {
			_ctx->grab_start(new grab_floating_move_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y});
		} else {
			if (_floating_area.grip_top.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_TOP});
			} else if (_floating_area.grip_bottom.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_BOTTOM});
			} else if (_floating_area.grip_left.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_LEFT});
			} else if (_floating_area.grip_right.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_RIGHT});
			} else if (_floating_area.grip_top_left.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_TOP_LEFT});
			} else if (_floating_area.grip_top_right.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_TOP_RIGHT});
			} else if (_floating_area.grip_bottom_left.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_BOTTOM_LEFT});
			} else if (_floating_area.grip_bottom_right.is_inside(e->event_x, e->event_y)) {
				_ctx->grab_start(new grab_floating_resize_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y, RESIZE_BOTTOM_RIGHT});
			} else {
				_ctx->grab_start(new grab_floating_move_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y});
			}
		}

		return true;

	} else if (is(MANAGED_FULLSCREEN)
			and e->detail == (XCB_BUTTON_INDEX_1)
			and (e->state & XCB_MOD_MASK_1)) {
		/** start moving fullscreen window **/
		_ctx->grab_start(new grab_fullscreen_client_t{_ctx, shared_from_this(), e->detail, e->root_x, e->root_y});
		return true;
	} else if (is(MANAGED_NOTEBOOK) and e->detail == (XCB_BUTTON_INDEX_3)
			and (e->state & (XCB_MOD_MASK_1))) {
		_ctx->grab_start(new grab_bind_client_t{_ctx, shared_from_this(), e->detail, rect{e->root_x-10, e->root_y-10, 20, 20}});
		return true;
	}

	return false;
}

void client_managed_t::queue_redraw() {
	if(is(MANAGED_FLOATING)) {
		_is_resized = true;
	} else {
		tree_t::queue_redraw();
	}
}

void client_managed_t::_update_backbuffers() {
	if(not is(MANAGED_FLOATING) or not prefer_window_border())
		return;

	theme_managed_window_t fw;

	if (_bottom_buffer != nullptr) {
		fw.cairo_bottom = cairo_create(_bottom_buffer->get_cairo_surface());
	} else {
		fw.cairo_bottom = nullptr;
	}

	if (_top_buffer != nullptr) {
		fw.cairo_top = cairo_create(_top_buffer->get_cairo_surface());
	} else {
		fw.cairo_top = nullptr;
	}

	if (_right_buffer != nullptr) {
		fw.cairo_right = cairo_create(_right_buffer->get_cairo_surface());
	} else {
		fw.cairo_right = nullptr;
	}

	if (_left_buffer != nullptr) {
		fw.cairo_left = cairo_create(_left_buffer->get_cairo_surface());
	} else {
		fw.cairo_left = nullptr;
	}

	fw.focuced = has_focus();
	fw.position = base_position();
	fw.icon = icon();
	fw.title = title();
	fw.demand_attention = _demands_attention;

	_ctx->theme()->render_floating(&fw);
}

void client_managed_t::trigger_redraw() {

	if(_is_resized) {
		_is_resized = false;
		_has_change = true;
		create_back_buffer();
	}

	if(_has_change) {
		_has_change = false;
		_is_exposed = true;
		_update_backbuffers();
	}

	if(_is_exposed) {
		_is_exposed = false;
		_paint_exposed();
	}

}

void client_managed_t::_update_title() {
	_has_change = true;

	string name;
	if (_client_proxy->net_wm_name() != nullptr) {
		_title = *(_client_proxy->net_wm_name());
	} else if (_client_proxy->wm_name() != nullptr) {
		_title = *(_client_proxy->wm_name());
	} else {
		stringstream s(stringstream::in | stringstream::out);
		s << "#" << (_client_proxy->id()) << " (noname)";
		_title = s.str();
	}
}

void client_managed_t::update_title() {
	_update_title();
	on_title_change.signal(shared_from_this());
}

bool client_managed_t::prefer_window_border() const {
	if (_client_proxy->motif_hints() != nullptr) {
		if(not (_client_proxy->motif_hints()->flags & MWM_HINTS_DECORATIONS))
			return true;
		if(_client_proxy->motif_hints()->decorations != 0x00)
			return true;
		return false;
	}
	return true;
}

shared_ptr<icon16> client_managed_t::icon() const {
	return _icon;
}

void client_managed_t::update_icon() {
	_icon = make_shared<icon16>(this);
}

xcb_window_t client_managed_t::base() const {
	return _base;
}

xcb_window_t client_managed_t::deco() const {
	return _deco;
}

xcb_window_t client_managed_t::orig() const {
	return _client_proxy->id();
}

xcb_atom_t client_managed_t::A(atom_e atom) {
	return cnx()->A(atom);
}


bool client_managed_t::has_focus() const {
	return _has_focus;
}

shared_ptr<client_view_t> client_managed_t::create_view() {
	return _ctx->create_view(_base);
}

void client_managed_t::set_current_desktop(unsigned int n) {
	_client_proxy->set_net_wm_desktop(n);
}

void client_managed_t::set_demands_attention(bool x) {
	if (x) {
		_client_proxy->net_wm_state_add(_NET_WM_STATE_DEMANDS_ATTENTION);
	} else {
		_client_proxy->net_wm_state_remove(_NET_WM_STATE_DEMANDS_ATTENTION);
	}
	_demands_attention = x;
}

bool client_managed_t::demands_attention() {
	return _demands_attention;
}

string const & client_managed_t::title() const {
	return _title;
}

void client_managed_t::render(cairo_t * cr, region const & area) {
	auto pix = _client_view->get_pixmap();

	if (pix != nullptr) {
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(cr, pix->get_cairo_surface(),
				_base_position.x, _base_position.y);
		region r = get_visible_region() & area;
		for (auto &i : r.rects()) {
			cairo_clip(cr, i);
			cairo_mask_surface(cr, pix->get_cairo_surface(),
					_base_position.x, _base_position.y);
		}
		cairo_restore(cr);
	}
}

void client_managed_t::_update_visible_region() {
	/** update visible cache **/
	rect vis{base_position()};

	/* add the shadow */
	if(_managed_type == MANAGED_FLOATING) {
		vis.x -= 32;
		vis.y -= 32;
		vis.w += 64;
		vis.h += 64;
	}

	_visible_region_cache = region{vis};
}

void client_managed_t::_update_opaque_region() {
	/** update opaque region cache **/
	_opaque_region_cache.clear();

	if (net_wm_opaque_region() != nullptr) {
		_opaque_region_cache = region{*(net_wm_opaque_region())};
		_opaque_region_cache &= rect{0, 0, _orig_position.w, _orig_position.h};
	} else {
		if (_client_proxy->geometry().depth != 32) {
			_opaque_region_cache = rect{0, 0, _orig_position.w, _orig_position.h};
		}
	}

	if(shape() != nullptr) {
		_opaque_region_cache &= *shape();
		_opaque_region_cache &= rect{0, 0, _orig_position.w, _orig_position.h};
	}

	_opaque_region_cache.translate(_base_position.x+_orig_position.x, _base_position.y+_orig_position.y);
}

void client_managed_t::on_property_notify(xcb_property_notify_event_t const * e) {
	if (e->atom == A(_NET_WM_NAME) or e->atom == A(WM_NAME)) {
		update_title();
		queue_redraw();
	} else if (e->atom == A(_NET_WM_ICON)) {
		update_icon();
		queue_redraw();
	} if (e->atom == A(_NET_WM_OPAQUE_REGION)) {
		_update_opaque_region();
	}

}

void client_managed_t::_apply_floating_hints_constraint() {

	if(_client_proxy->wm_normal_hints()!= nullptr) {
		XSizeHints const * s = _client_proxy->wm_normal_hints();

		if (s->flags & PBaseSize) {
			if (_floating_wished_position.w < s->base_width)
				_floating_wished_position.w = s->base_width;
			if (_floating_wished_position.h < s->base_height)
				_floating_wished_position.h = s->base_height;
		} else if (s->flags & PMinSize) {
			if (_floating_wished_position.w < s->min_width)
				_floating_wished_position.w = s->min_width;
			if (_floating_wished_position.h < s->min_height)
				_floating_wished_position.h = s->min_height;
		}

		if (s->flags & PMaxSize) {
			if (_floating_wished_position.w > s->max_width)
				_floating_wished_position.w = s->max_width;
			if (_floating_wished_position.h > s->max_height)
				_floating_wished_position.h = s->max_height;
		}

	}
}

void client_managed_t::expose(xcb_expose_event_t const * ev) {
	_is_exposed = true;
}

void client_managed_t::_paint_exposed() {
	if (is(MANAGED_FLOATING) and prefer_window_border()) {

		cairo_xcb_surface_set_size(_surf, _base_position.w, _base_position.h);

		cairo_t * _cr = cairo_create(_surf);

		/** top **/
		if (_top_buffer != nullptr) {
			cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(_cr, 0, 0, _base_position.w,
					_ctx->theme()->floating.margin.top+_ctx->theme()->floating.title_height);
			cairo_set_source_surface(_cr, _top_buffer->get_cairo_surface(), 0, 0);
			cairo_fill(_cr);
		}

		/** bottom **/
		if (_bottom_buffer != nullptr) {
			cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(_cr, 0,
					_base_position.h - _ctx->theme()->floating.margin.bottom,
					_base_position.w, _ctx->theme()->floating.margin.bottom);
			cairo_set_source_surface(_cr, _bottom_buffer->get_cairo_surface(), 0,
					_base_position.h - _ctx->theme()->floating.margin.bottom);
			cairo_fill(_cr);
		}

		/** left **/
		if (_left_buffer != nullptr) {
			cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(_cr, 0.0, _ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height,
					_ctx->theme()->floating.margin.left,
					_base_position.h - _ctx->theme()->floating.margin.top
							- _ctx->theme()->floating.margin.bottom - _ctx->theme()->floating.title_height);
			cairo_set_source_surface(_cr, _left_buffer->get_cairo_surface(), 0.0,
					_ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height);
			cairo_fill(_cr);
		}

		/** right **/
		if (_right_buffer != nullptr) {
			cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(_cr,
					_base_position.w - _ctx->theme()->floating.margin.right,
					_ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height, _ctx->theme()->floating.margin.right,
					_base_position.h - _ctx->theme()->floating.margin.top
							- _ctx->theme()->floating.margin.bottom - _ctx->theme()->floating.title_height);
			cairo_set_source_surface(_cr, _right_buffer->get_cairo_surface(),
					_base_position.w - _ctx->theme()->floating.margin.right,
					_ctx->theme()->floating.margin.top + _ctx->theme()->floating.title_height);
			cairo_fill(_cr);
		}

		cairo_surface_flush(_surf);

		warn(cairo_get_reference_count(_cr) == 1);
		cairo_destroy(_cr);
	}
}

}

