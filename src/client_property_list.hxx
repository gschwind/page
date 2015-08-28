/*
 * client_property_list.hxx
 *
 *  Created on: 28 août 2015
 *      Author: gschwind
 */

/*          C++ name                   EWMH/X11 name               EWMH/X11 type       C++ type */

RO_PROPERTY(wm_name,                   WM_NAME,                    STRING,             string) // 8
RO_PROPERTY(wm_icon_name,              WM_ICON_NAME,               STRING,             string) // 8
RO_PROPERTY(wm_normal_hints,           WM_NORMAL_HINTS,            WM_SIZE_HINTS,      XSizeHints) // 32
RO_PROPERTY(wm_hints,                  WM_HINTS,                   WM_HINTS,           XWMHints) // 32
RO_PROPERTY(wm_class,                  WM_CLASS,                   STRING,             vector<string>) // 8
RO_PROPERTY(wm_transient_for,          WM_TRANSIENT_FOR,           WINDOW,             xcb_window_t) // 32
RO_PROPERTY(wm_protocols,              WM_PROTOCOLS,               ATOM,               list<xcb_atom_t>) // 32
RO_PROPERTY(wm_colormap_windows,       WM_COLORMAP_WINDOWS,        WINDOW,             vector<xcb_window_t>) // 32
RO_PROPERTY(wm_client_machine,         WM_CLIENT_MACHINE,          STRING,             string) // 8

RO_PROPERTY(net_wm_name,               _NET_WM_NAME,               UTF8_STRING,        string) // 8
RO_PROPERTY(net_wm_visible_name,       _NET_WM_VISIBLE_NAME,       UTF8_STRING,        string) // 8
RO_PROPERTY(net_wm_icon_name,          _NET_WM_ICON_NAME,          UTF8_STRING,        string) // 8
RO_PROPERTY(net_wm_visible_icon_name,  _NET_WM_VISIBLE_ICON_NAME,  UTF8_STRING,        string) // 8
RO_PROPERTY(net_wm_desktop,            _NET_WM_DESKTOP,            CARDINAL,           unsigned int) // 32
RO_PROPERTY(net_wm_window_type,        _NET_WM_WINDOW_TYPE,        ATOM,               list<xcb_atom_t>) // 32
RO_PROPERTY(net_wm_state,              _NET_WM_STATE,              ATOM,               list<xcb_atom_t>) // 32
RO_PROPERTY(net_wm_allowed_actions,    _NET_WM_ALLOWED_ACTIONS,    ATOM,               list<xcb_atom_t>) // 32
RO_PROPERTY(net_wm_strut,              _NET_WM_STRUT,              CARDINAL,           vector<int>) // 32
RO_PROPERTY(net_wm_strut_partial,      _NET_WM_STRUT_PARTIAL,      CARDINAL,           vector<int>) // 32
RO_PROPERTY(net_wm_icon_geometry,      _NET_WM_ICON_GEOMETRY,      CARDINAL,           vector<int>) // 32
RO_PROPERTY(net_wm_icon,               _NET_WM_ICON,               CARDINAL,           vector<uint32_t>) // 32
RO_PROPERTY(net_wm_pid,                _NET_WM_PID,                CARDINAL,           unsigned int) // 32
RO_PROPERTY(net_wm_user_time,          _NET_WM_USER_TIME,          CARDINAL,           unsigned int) // 32
RO_PROPERTY(net_wm_user_time_window,   _NET_WM_USER_TIME_WINDOW,   WINDOW,             xcb_window_t) // 32
RO_PROPERTY(net_frame_extents,         _NET_FRAME_EXTENTS,         CARDINAL,           vector<int>) // 32
RO_PROPERTY(net_wm_opaque_region,      _NET_WM_OPAQUE_REGION,      CARDINAL,           vector<int>) // 32
RO_PROPERTY(net_wm_bypass_compositor,  _NET_WM_BYPASS_COMPOSITOR,  CARDINAL,           unsigned int) // 32
RO_PROPERTY(motif_hints,               _MOTIF_WM_HINTS,            _MOTIF_WM_HINTS,    motif_wm_hints_t) // 8

RW_PROPERTY(wm_state,                  WM_STATE,                   WM_STATE,           wm_state_data_t) // 32
