/*
 * client_properties.cxx
 *
 *  Created on: 25 ao��t 2015
 *      Author: gschwind
 */

#include "client_proxy.hxx"

#include "pixmap.hxx"

namespace page {


void client_proxy_t::select_input(uint32_t mask) {
	_wa.your_event_mask |= mask;
	_wa.your_event_mask |= XCB_EVENT_MASK_PROPERTY_CHANGE|XCB_EVENT_MASK_STRUCTURE_NOTIFY;
	_dpy->select_input(_id, _wa.your_event_mask);
}

void client_proxy_t::select_input_shape(bool x) {
	xcb_shape_select_input(cnx()->xcb(), _id, x?1:0);
}

void client_proxy_t::grab_button (
                 uint8_t           owner_events,
                 uint16_t          event_mask,
                 uint8_t           pointer_mode,
                 uint8_t           keyboard_mode,
                 xcb_window_t      confine_to,
                 xcb_cursor_t      cursor,
                 uint8_t           button,
                 uint16_t          modifiers) {
	xcb_grab_button(_dpy->xcb(), owner_events, _id, event_mask, pointer_mode, keyboard_mode, confine_to, cursor, button, modifiers);
}

void client_proxy_t::ungrab_button(uint8_t button, uint16_t modifiers) {
	xcb_ungrab_button(_dpy->xcb(), button, _id, modifiers);
}

void client_proxy_t::send_event (
                uint8_t           propagate  /**< */,
                uint32_t          event_mask  /**< */,
                const char       *event  /**< */) {
	xcb_send_event(_dpy->xcb(), propagate, _id, event_mask, event);
}

void client_proxy_t::set_input_focus(int revert_to, xcb_timestamp_t time) {
	_dpy->set_input_focus(_id, revert_to, time);
}

void client_proxy_t::move_resize(rect const & size) {
	_dpy->move_resize(_id, size);
}

void client_proxy_t::fake_configure(rect const & location, int border_width) {
	xcb_configure_notify_event_t xev;
	xev.response_type = XCB_CONFIGURE_NOTIFY;
	xev.event = _id;
	xev.window = _id;

	/* if we need fake configure, override_redirect is False */
	xev.override_redirect = 0;
	xev.border_width = border_width;
	xev.above_sibling = None;

	/* send mandatory fake event */
	xev.x = location.x;
	xev.y = location.y;
	xev.width = location.w;
	xev.height = location.h;

	xcb_send_event(_dpy->xcb(), false, _id, XCB_EVENT_MASK_STRUCTURE_NOTIFY, reinterpret_cast<char*>(&xev));

}

void client_proxy_t::delete_window(xcb_timestamp_t t) {
	xcb_client_message_event_t xev;
	xev.response_type = XCB_CLIENT_MESSAGE;
	xev.type = A(WM_PROTOCOLS);
	xev.format = 32;
	xev.window = _id;
	xev.data.data32[0] = A(WM_DELETE_WINDOW);
	xev.data.data32[1] = t;

	xcb_send_event(_dpy->xcb(), 0, _id, XCB_EVENT_MASK_NO_EVENT,
			reinterpret_cast<char*>(&xev));
}

void client_proxy_t::set_border_width(uint32_t width) {
	xcb_configure_window(_dpy->xcb(), _id, XCB_CONFIG_WINDOW_BORDER_WIDTH, &width);
}

void client_proxy_t::unmap() {
	xcb_unmap_window(_dpy->xcb(), _id);
}

void client_proxy_t::reparentwindow(xcb_window_t parent, int x, int y) {
	xcb_reparent_window(_dpy->xcb(), _id, parent, x, y);
}

void client_proxy_t::xmap() {
	xcb_map_window(_dpy->xcb(), _id);
}

void client_proxy_t::delete_net_wm_state() {
	del<p_net_wm_state>();
}

void client_proxy_t::delete_wm_state() {
	del<p_wm_state>();
}

void client_proxy_t::add_to_save_set() {
	xcb_change_save_set(_dpy->xcb(), XCB_SET_MODE_INSERT, _id);
}

void client_proxy_t::remove_from_save_set()
{
	xcb_change_save_set(_dpy->xcb(), XCB_SET_MODE_DELETE, _id);
}

/** short cut **/
xcb_atom_t client_proxy_t::A(atom_e atom) {
	return _dpy->A(atom);
}

xcb_atom_t client_proxy_t::B(atom_e atom) {
	return static_cast<xcb_atom_t>(A(atom));
}

xcb_window_t client_proxy_t::xid() {
	return static_cast<xcb_window_t>(_id);
}

client_proxy_t::client_proxy_t(display_t * dpy, xcb_window_t id) :
	wm_properties_t{dpy->xcb(), dpy->_A, id},
	_dpy{dpy},
	_id{id}
{
	_destroyed = false;
	_shape = nullptr;
	_need_update_type = true;
	_is_redirected = false;
	_wm_type = A(_NET_WM_WINDOW_TYPE_NORMAL);
	_damage = XCB_NONE;


	/** following request are mandatory to create a client_proxy **/
	auto ck1 = xcb_get_window_attributes(_dpy->xcb(), xid());
	auto ck2 = xcb_get_geometry(_dpy->xcb(), xid());

	xcb_get_geometry_reply_t * geometry = nullptr;
	xcb_get_window_attributes_reply_t * wa = nullptr;
	xcb_generic_error_t * err;

	try {
		wa = xcb_get_window_attributes_reply(_dpy->xcb(), ck1, &err);
		if(err != nullptr)
			throw invalid_client_t{};
		geometry = xcb_get_geometry_reply(_dpy->xcb(), ck2, &err);
		if(err != nullptr)
			throw invalid_client_t{};

		_wa = *wa;
		_geometry = *geometry;
		_vis = _dpy->get_visual_type(_wa.visual);
		_need_pixmap_update= true;

		/**
		 * select needed default inputs.
		 **/
		_wa.your_event_mask |= XCB_EVENT_MASK_PROPERTY_CHANGE|XCB_EVENT_MASK_STRUCTURE_NOTIFY;
		auto ck0 = xcb_change_window_attributes_checked(_dpy->xcb(), id, XCB_CW_EVENT_MASK, &_wa.your_event_mask);
		err = xcb_request_check(_dpy->xcb(), ck0);
		if(err != nullptr)
			throw invalid_client_t{};
		xcb_discard_reply(_dpy->xcb(), ck0.sequence);

		free(wa);
		free(geometry);

	} catch(invalid_client_t const & e) {
		free(err);
		if(wa != nullptr)
			free(wa);
		else
			xcb_discard_reply(_dpy->xcb(), ck1.sequence);
		if(geometry != nullptr)
			free(geometry);
		else
			xcb_discard_reply(_dpy->xcb(), ck2.sequence);
		throw;
	}

	read_all_properties();

}

client_proxy_t::~client_proxy_t()
{
	if(not _views.empty()) {
		cout << "Warning: destroying client_proxy with views" << endl;
	}

	delete_all_properties();
	if(_is_redirected)
		xcb_composite_unredirect_window(_dpy->xcb(), _id, XCB_COMPOSITE_REDIRECT_MANUAL);
	destroy_damage();
}

void client_proxy_t::read_all_properties()
{
	_wm_transiant_for = get<p_wm_transient_for>();

	fetch_all();
	update_shape();
	update_type();
}

void client_proxy_t::delete_all_properties()
{
	safe_delete(_shape);
}

void client_proxy_t::update_shape() {
	delete _shape;
	_shape = nullptr;

	int count;
	int ordering;

	/** request extent to check if shape has been set **/
	xcb_shape_query_extents_cookie_t ck0 = xcb_shape_query_extents(_dpy->xcb(), _id);

	/** in the same time we make the request for rectangle (even if this request isn't needed) **/
	xcb_shape_get_rectangles_cookie_t ck1 = xcb_shape_get_rectangles(_dpy->xcb(), _id, XCB_SHAPE_SK_BOUNDING);

	xcb_shape_query_extents_reply_t * r0 = xcb_shape_query_extents_reply(_dpy->xcb(), ck0, 0);
	xcb_shape_get_rectangles_reply_t * r1 = xcb_shape_get_rectangles_reply(_dpy->xcb(), ck1, 0);

	if (r0 != nullptr) {

		if (r0->bounding_shaped and r1 != nullptr) {

			_shape = new region;

			xcb_rectangle_iterator_t i =
					xcb_shape_get_rectangles_rectangles_iterator(r1);
			while (i.rem > 0) {
				*_shape += rect { i.data->x, i.data->y, i.data->width,
						i.data->height };
				xcb_rectangle_next(&i);
			}

		}
		free(r0);
	}

	if(r1 != nullptr) {
		free(r1);
	}

}

void client_proxy_t::set_net_wm_desktop(unsigned int n) {
	auto new_net_wm_desktop = make_shared<unsigned int>(n);
	set<p_net_wm_desktop>(new_net_wm_desktop);
}

void client_proxy_t::print_window_attributes() {
	printf(">>> window xid: #%u\n", _id);
	printf("> size: %dx%d+%d+%d\n", _geometry.width, _geometry.height, _geometry.x, _geometry.y);
	printf("> border_width: %d\n", _geometry.border_width);
	printf("> depth: %d\n", _geometry.depth);
	printf("> visual #%u\n", _wa.visual);
	printf("> root: #%u\n", _geometry.root);
	if (_wa._class == CopyFromParent) {
		printf("> class: CopyFromParent\n");
	} else if (_wa._class == InputOutput) {
		printf("> class: InputOutput\n");
	} else if (_wa._class == InputOnly) {
		printf("> class: InputOnly\n");
	} else {
		printf("> class: Unknown\n");
	}

	if (_wa.map_state == IsViewable) {
		printf("> map_state: IsViewable\n");
	} else if (_wa.map_state == IsUnviewable) {
		printf("> map_state: IsUnviewable\n");
	} else if (_wa.map_state == IsUnmapped) {
		printf("> map_state: IsUnmapped\n");
	} else {
		printf("> map_state: Unknown\n");
	}

	printf("> bit_gravity: %d\n", _wa.bit_gravity);
	printf("> win_gravity: %d\n", _wa.win_gravity);
	printf("> backing_store: %dlx\n", _wa.backing_store);
	printf("> backing_planes: %x\n", _wa.backing_planes);
	printf("> backing_pixel: %x\n", _wa.backing_pixel);
	printf("> save_under: %d\n", _wa.save_under);
	printf("> colormap: <Not Implemented>\n");
	printf("> all_event_masks: %08x\n", _wa.all_event_masks);
	printf("> your_event_mask: %08x\n", _wa.your_event_mask);
	printf("> do_not_propagate_mask: %08x\n", _wa.do_not_propagate_mask);
	printf("> override_redirect: %d\n", _wa.override_redirect);
}


void client_proxy_t::print_properties() {
//	/* ICCCM */
//	if(get<p_wm_name>() != nullptr)
//		cout << "* WM_NAME = " << *get<p_wm_name>() << endl;
//
//	if(get<p_wm_icon_name>() != nullptr)
//		cout << "* WM_ICON_NAME = " << *get<p_wm_icon_name>() << endl;
//
//	//if(wm_normal_hints != nullptr)
//	//	cout << "WM_NORMAL_HINTS = " << *wm_normal_hints << endl;
//
//	//if(wm_hints != nullptr)
//	//	cout << "WM_HINTS = " << *wm_hints << endl;
//
//	if(get<p_wm_class>() != nullptr)
//		cout << "* WM_CLASS = " << (*get<p_wm_class>())[0] << "," << (*get<p_wm_class>())[1] << endl;
//
//	if(get<p_wm_transient_for>() != nullptr)
//		cout << "* WM_TRANSIENT_FOR = " << *get<p_wm_transient_for>() << endl;
//
//	if(get<p_wm_protocols>() != nullptr) {
//		cout << "* WM_PROTOCOLS = ";
//		for(auto x: *get<p_wm_protocols>()) {
//			cout << _dpy->get_atom_name(x) << " ";
//		}
//		cout << endl;
//	}
//
//	if(get<p_wm_colormap_windows>() != nullptr)
//		cout << "WM_COLORMAP_WINDOWS = " << (*get<p_wm_colormap_windows>())[0] << endl;
//
//	if(get<p_wm_client_machine>() != nullptr)
//		cout << "* WM_CLIENT_MACHINE = " << *get<p_wm_client_machine>() << endl;
//
//	if(get<p_wm_state>() != nullptr) {
//		cout << "* WM_STATE = " << get<p_wm_state>()->state << endl;
//	}
//
//
//	/* EWMH */
//	if(get<p_net_wm_name>() != nullptr)
//		cout << "* _NET_WM_NAME = " << *get<p_net_wm_name>() << endl;
//
//	if(get<p_net_wm_visible_name>() != nullptr)
//		cout << "* _NET_WM_VISIBLE_NAME = " << *get<p_net_wm_visible_name>() << endl;
//
//	if(get<p_net_wm_icon_name>() != nullptr)
//		cout << "* _NET_WM_ICON_NAME = " << *get<p_net_wm_icon_name>() << endl;
//
//	if(get<p_net_wm_visible_icon_name>() != nullptr)
//		cout << "* _NET_WM_VISIBLE_ICON_NAME = " << *get<p_net_wm_visible_icon_name>() << endl;
//
//	if(get<p_net_wm_desktop>() != nullptr)
//		cout << "* _NET_WM_DESKTOP = " << *get<p_net_wm_desktop>() << endl;
//
//	if(get<p_net_wm_window_type>() != nullptr) {
//		cout << "* _NET_WM_WINDOW_TYPE = ";
//		for(auto x: *get<p_net_wm_window_type>()) {
//			cout << _dpy->get_atom_name(x) << " ";
//		}
//		cout << endl;
//	}
//
//	if(get<p_net_wm_state>() != nullptr) {
//		cout << "* _NET_WM_STATE = ";
//		for(auto x: *get<p_net_wm_state>()) {
//			cout << _dpy->get_atom_name(x) << " ";
//		}
//		cout << endl;
//	}
//
//	if(get<p_net_wm_allowed_actions>() != nullptr) {
//		cout << "* _NET_WM_ALLOWED_ACTIONS = ";
//		for(auto x: *get<p_net_wm_allowed_actions>()) {
//			cout << _dpy->get_atom_name(x) << " ";
//		}
//		cout << endl;
//	}
//
//	if(get<p_net_wm_strut>() != nullptr) {
//		cout << "* _NET_WM_STRUCT = ";
//		for(auto x: *get<p_net_wm_strut>()) {
//			cout << x << " ";
//		}
//		cout << endl;
//	}
//
//	if(get<p_net_wm_strut_partial>() != nullptr) {
//		cout << "* _NET_WM_PARTIAL_STRUCT = ";
//		for(auto x: *get<p_net_wm_strut_partial>()) {
//			cout << x << " ";
//		}
//		cout << endl;
//	}
//
//	if(get<p_net_wm_icon_geometry>() != nullptr) {
//		cout << "* _NET_WM_ICON_GEOMETRY = ";
//		for(auto x: *get<p_net_wm_icon_geometry>()) {
//			cout << x << " ";
//		}
//		cout << endl;
//	}
//
//	if(get<p_net_wm_icon>() != nullptr)
//		cout << "* _NET_WM_ICON = " << "TODO" << endl;
//
//	if(get<p_net_wm_pid>() != nullptr)
//		cout << "* _NET_WM_PID = " << *get<p_net_wm_pid>() << endl;
//
//	//if(_net_wm_handled_icons != false)
//	//	;
//
//	if(get<p_net_wm_user_time>() != nullptr)
//		cout << "* _NET_WM_USER_TIME = " << *get<p_net_wm_user_time>() << endl;
//
//	if(get<p_net_wm_user_time_window>() != nullptr)
//		cout << "* _NET_WM_USER_TIME_WINDOW = " << *get<p_net_wm_user_time_window>() << endl;
//
//	if(get<p_net_frame_extents>() != nullptr) {
//		cout << "* _NET_FRAME_EXTENTS = ";
//		for(auto x: *get<p_net_frame_extents>()) {
//			cout << x << " ";
//		}
//		cout << endl;
//	}
//
//	//_net_wm_opaque_region = nullptr;
//	//_net_wm_bypass_compositor = nullptr;
//	//motif_hints = nullptr;
}

void client_proxy_t::update_type() {
	_wm_type = XCB_NONE;

	list<xcb_atom_t> net_wm_window_type;
	bool override_redirect = (_wa.override_redirect == True)?true:false;
	auto px_net_wm_window_type = this->get<p_net_wm_window_type>();
	if(px_net_wm_window_type == nullptr) {
		/**
		 * Fallback from ICCCM.
		 **/

		if(!override_redirect) {
			/* Managed windows */
			if(_wm_transiant_for == nullptr) {
				/**
				 * Extended ICCCM:
				 * _NET_WM_WINDOW_TYPE_NORMAL [...] Managed windows with neither
				 * _NET_WM_WINDOW_TYPE nor WM_TRANSIENT_FOR set MUST be taken
				 * as this type.
				 **/
				net_wm_window_type.push_back(A(_NET_WM_WINDOW_TYPE_NORMAL));
			} else {
				/**
				 * Extended ICCCM:
				 * _NET_WM_WINDOW_TYPE_DIALOG [...] If _NET_WM_WINDOW_TYPE is
				 * not set, then managed windows with WM_TRANSIENT_FOR set MUST
				 * be taken as this type.
				 **/
				net_wm_window_type.push_back(A(_NET_WM_WINDOW_TYPE_DIALOG));
			}

		} else {
			/**
			 * Override-redirected windows.
			 *
			 * Extended ICCCM:
			 * _NET_WM_WINDOW_TYPE_NORMAL [...] Override-redirect windows
			 * without _NET_WM_WINDOW_TYPE, must be taken as this type, whether
			 * or not they have WM_TRANSIENT_FOR set.
			 **/
			net_wm_window_type.push_back(A(_NET_WM_WINDOW_TYPE_NORMAL));
		}
	} else {
		net_wm_window_type = *(px_net_wm_window_type);
	}

	/* always fall back to normal */
	net_wm_window_type.push_back(A(_NET_WM_WINDOW_TYPE_NORMAL));

	/* TODO: make this ones */
	static std::set<xcb_atom_t> known_type;
	if (known_type.size() == 0) {
		known_type.insert(A(_NET_WM_WINDOW_TYPE_DESKTOP));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_DOCK));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_TOOLBAR));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_MENU));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_UTILITY));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_SPLASH));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_DIALOG));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_POPUP_MENU));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_TOOLTIP));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_NOTIFICATION));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_COMBO));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_DND));
		known_type.insert(A(_NET_WM_WINDOW_TYPE_NORMAL));
	}

	/** find the first known window type **/
	for (auto i : net_wm_window_type) {
		//printf("Check for %s\n", cnx->get_atom_name(*i).c_str());
		if (has_key(known_type, i)) {
			_wm_type = i;
			break;
		}
	}
}

xcb_atom_t client_proxy_t::wm_type() {
	if(_need_update_type) {
		_need_update_type = false;
		update_type();
	}
	return _wm_type;
}

display_t *          client_proxy_t::cnx() const { return _dpy; }
xcb_window_t         client_proxy_t::id() const { return _id; }

auto client_proxy_t::wa() const -> xcb_get_window_attributes_reply_t const & { return _wa; }
auto client_proxy_t::geometry() const -> xcb_get_geometry_reply_t const & { return _geometry; }
auto client_proxy_t::visualid() const -> xcb_visualid_t { return _wa.visual; }
auto client_proxy_t::visual_depth() const -> uint8_t { return _geometry.depth; }

/* OTHERs */
region const *                     client_proxy_t::shape() const { return _shape; }

void client_proxy_t::net_wm_allowed_actions_add(atom_e atom) {
	auto new_net_wm_allowed_actions = make_shared<list<xcb_atom_t>>();

	auto net_wm_allowed_actions = get<p_net_wm_allowed_actions>();
	if(net_wm_allowed_actions != nullptr) {
		new_net_wm_allowed_actions->insert(new_net_wm_allowed_actions->end(), net_wm_allowed_actions->begin(), net_wm_allowed_actions->end());
	}

	new_net_wm_allowed_actions->remove(A(atom));
	new_net_wm_allowed_actions->push_back(A(atom));
	set<p_net_wm_allowed_actions>(new_net_wm_allowed_actions);
}

void client_proxy_t::net_wm_allowed_actions_set(list<atom_e> atom_list) {
	auto new_net_wm_allowed_actions = make_shared<list<xcb_atom_t>>();
	for(auto i: atom_list) {
		new_net_wm_allowed_actions->push_back(A(i));
	}
	set<p_net_wm_allowed_actions>(new_net_wm_allowed_actions);
}

void client_proxy_t::set_wm_state(int state) {
	set<p_wm_state>(make_shared<wm_state_data_t>(state, None));
}

void client_proxy_t::process_event(xcb_configure_notify_event_t const * e)
{
	if(_wa.override_redirect != e->override_redirect) {
		_wa.override_redirect = e->override_redirect;
		update_type();
	}

	if (e->width != _geometry.width or e->height != _geometry.height) {
		_need_pixmap_update = true;
		_geometry.width = e->width;
		_geometry.height = e->height;
	}

	_geometry.x = e->x;
	_geometry.y = e->y;
	_geometry.border_width = e->border_width;
}

rect client_proxy_t::position() const
{
	return rect{_geometry.x, _geometry.y, _geometry.width, _geometry.height};
}

client_view_t::client_view_t(client_proxy_t * parent) :
	_parent{parent}
{
	_damaged += region(0, 0, parent->_geometry.width, parent->_geometry.height);
}

client_view_t::~client_view_t()
{
	on_destroy.signal(this);
}

auto client_view_t::get_pixmap() -> shared_ptr<pixmap_t> {
	return _parent->get_pixmap();
}

void client_view_t::clear_damaged() {
	_damaged.clear();
}

auto client_view_t::get_damaged() -> region const & {
	return _damaged;
}

bool client_view_t::has_damage() {
	return not _damaged.empty();
}

void client_proxy_t::create_damage() {
	if (_damage == XCB_NONE) {
		_damage = xcb_generate_id(_dpy->xcb());
		xcb_damage_create(_dpy->xcb(), _damage, _id, XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);
		add_damaged(region(0, 0, _geometry.width, _geometry.height));
	}
}

void client_proxy_t::destroy_damage() {
	if (_damage != XCB_NONE) {
		xcb_damage_destroy(_dpy->xcb(), _damage);
		_damage = XCB_NONE;
	}
}

void client_proxy_t::enable_redirect() {
	_need_pixmap_update = true;
	if(_is_redirected)
		return;
	_is_redirected = true;
	xcb_composite_redirect_window(_dpy->xcb(), _id, XCB_COMPOSITE_REDIRECT_MANUAL);
	create_damage();
}

void client_proxy_t::disable_redirect() {
	_pixmap = nullptr;
	if(not _is_redirected)
		return;
	_is_redirected = false;
	xcb_composite_unredirect_window(_dpy->xcb(), _id, XCB_COMPOSITE_REDIRECT_MANUAL);
	destroy_damage();
}

void client_proxy_t::on_map() {
	_need_pixmap_update = true;
}

void client_proxy_t::process_event(xcb_damage_notify_event_t const * ev) {
	for(auto x: _views) {
		x->_damaged += region_t{ev->area};
	}
}

void client_proxy_t::add_damaged(region const & r) {
	for(auto x: _views) {
		x->_damaged += r;
	}
}

int client_proxy_t::depth() {
	return _geometry.depth;
}

bool client_proxy_t::_safe_pixmap_update() {
	xcb_pixmap_t pixmap_id = xcb_generate_id(_dpy->xcb());
	xcb_void_cookie_t ck = xcb_composite_name_window_pixmap_checked(_dpy->xcb(), _id, pixmap_id);
	auto err = xcb_request_check(_dpy->xcb(), ck);
	if(err != nullptr) {
		cout << "INFO: could not get pixmap : " << xcb_event_get_error_label(err->error_code) << endl;
		free(err);
		return false;
	} else {
		_pixmap = make_shared<pixmap_t>(_dpy, _vis, pixmap_id, _geometry.width, _geometry.height);
		xcb_discard_reply(_dpy->xcb(), ck.sequence);
		return true;
	}
}

void client_proxy_t::freeze(bool x) {
//	_is_freezed = x;
//	if(_pixmap != nullptr and _is_freezed) {
//		xcb_pixmap_t pix = xcb_generate_id(_dpy->xcb());
//		xcb_create_pixmap(_dpy->xcb(), _depth, pix, _dpy->root(), _width, _height);
//		auto xpix = std::make_shared<pixmap_t>(_dpy, _vis, pix, _width, _height);
//
//		cairo_surface_t * s = xpix->get_cairo_surface();
//		cairo_t * cr = cairo_create(s);
//		cairo_set_source_surface(cr, _pixmap->get_cairo_surface(), 0, 0);
//		cairo_paint(cr);
//		cairo_destroy(cr);
//
//		_pixmap = xpix;
//	}
}

shared_ptr<pixmap_t> client_proxy_t::get_pixmap() {

	if(_need_pixmap_update) {
		_need_pixmap_update = false;
		_safe_pixmap_update();
	}

	return _pixmap;
}

void client_proxy_t::process_event(xcb_property_notify_event_t const * e) {

	update_all(e->atom);

	/* custom case */
	if (e->atom == A(WM_TRANSIENT_FOR)) {
		_need_update_type = true;
	} else if (e->atom == A(_NET_WM_WINDOW_TYPE)) {
		_need_update_type = true;
	} else if (e->atom == A(_MOTIF_WM_HINTS)) {
		_need_update_type = true;
	} else if (e->atom == A(WM_TRANSIENT_FOR)) {
		_wm_transiant_for = get<p_wm_transient_for>();
	}

}

bool client_proxy_t::destroyed() {
	return _destroyed;
}

bool client_proxy_t::destroyed(bool x) {
	return _destroyed = x;
}

auto client_proxy_t::wm_transiant_for() -> shared_ptr<xcb_window_t>
{
	return _wm_transiant_for;
}

auto client_proxy_t::create_view(xcb_window_t base) -> client_view_p
{
	return _dpy->create_view(base);
}

}




