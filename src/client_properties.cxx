/*
 * client_properties.cxx
 *
 *  Created on: 25 août 2015
 *      Author: gschwind
 */

#include "client_properties.hxx"

namespace page {

void client_proxy_t::select_input(uint32_t mask) {
	_dpy->select_input(_id, mask|XCB_EVENT_MASK_PROPERTY_CHANGE|XCB_EVENT_MASK_STRUCTURE_NOTIFY);
}

void client_proxy_t::set_focus_state(bool is_focused) {
	if (is_focused) {
		net_wm_state_add(_NET_WM_STATE_FOCUSED);
	} else {
		net_wm_state_remove(_NET_WM_STATE_FOCUSED);
	}
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
	xcb_set_input_focus(_dpy->xcb(), revert_to, _id, time);
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
	__net_wm_state.clear(_dpy->xcb(), _dpy->_A, _id);
}

void client_proxy_t::delete_wm_state() {
	_wm_state.clear(_dpy->xcb(), _dpy->_A, _id);
}

void client_proxy_t::add_to_save_set() {
	xcb_change_save_set(_dpy->xcb(), XCB_SET_MODE_INSERT, _id);
}

}


