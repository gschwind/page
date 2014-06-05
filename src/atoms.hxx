/*
 * atoms.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef ATOMS_HXX_
#define ATOMS_HXX_

#include <cstdio>
#include <X11/Xatom.h>

namespace page {

#define _NET_WM_MOVERESIZE_SIZE_TOPLEFT      0
#define _NET_WM_MOVERESIZE_SIZE_TOP          1
#define _NET_WM_MOVERESIZE_SIZE_TOPRIGHT     2
#define _NET_WM_MOVERESIZE_SIZE_RIGHT        3
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT  4
#define _NET_WM_MOVERESIZE_SIZE_BOTTOM       5
#define _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT   6
#define _NET_WM_MOVERESIZE_SIZE_LEFT         7
#define _NET_WM_MOVERESIZE_MOVE              8   /* movement only */
#define _NET_WM_MOVERESIZE_SIZE_KEYBOARD     9   /* size via keyboard */
#define _NET_WM_MOVERESIZE_MOVE_KEYBOARD    10   /* move via keyboard */


enum atom_e {
	ATOM = 0,
	CARDINAL,
	WINDOW,
	UTF8_STRING,
	STRING,

	WM_STATE,
	WM_NAME,
	WM_ICON_NAME,
	WM_DELETE_WINDOW,
	WM_PROTOCOLS,
	WM_TAKE_FOCUS,
	WM_CLASS,
	WM_CHANGE_STATE,
	WM_TRANSIENT_FOR,
	WM_HINTS,
	WM_NORMAL_HINTS,
	WM_SIZE_HINTS,
	WM_COLORMAP_WINDOWS,
	WM_CLIENT_MACHINE,

	_NET_SUPPORTED,
	_NET_WM_NAME,
	_NET_WM_VISIBLE_NAME,
	_NET_WM_ICON_NAME,
	_NET_WM_VISIBLE_ICON_NAME,
	_NET_WM_STATE,
	_NET_WM_STRUT_PARTIAL,
	_NET_WM_STRUT,
	_NET_WM_ICON_GEOMETRY,

	_NET_WM_WINDOW_TYPE,
	_NET_WM_WINDOW_TYPE_DESKTOP,
	_NET_WM_WINDOW_TYPE_DOCK,
	_NET_WM_WINDOW_TYPE_TOOLBAR,
	_NET_WM_WINDOW_TYPE_MENU,
	_NET_WM_WINDOW_TYPE_UTILITY,
	_NET_WM_WINDOW_TYPE_SPLASH,
	_NET_WM_WINDOW_TYPE_DIALOG,
	_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
	_NET_WM_WINDOW_TYPE_POPUP_MENU,
	_NET_WM_WINDOW_TYPE_TOOLTIP,
	_NET_WM_WINDOW_TYPE_NOTIFICATION,
	_NET_WM_WINDOW_TYPE_COMBO,
	_NET_WM_WINDOW_TYPE_DND,
	_NET_WM_WINDOW_TYPE_NORMAL,

	_NET_WM_USER_TIME,
	_NET_WM_USER_TIME_WINDOW,
	_NET_CLIENT_LIST,
	_NET_CLIENT_LIST_STACKING,

	_NET_NUMBER_OF_DESKTOPS,
	_NET_DESKTOP_GEOMETRY,
	_NET_DESKTOP_VIEWPORT,
	_NET_CURRENT_DESKTOP,
	_NET_WM_DESKTOP,

	_NET_SHOWING_DESKTOP,
	_NET_WORKAREA,

	_NET_ACTIVE_WINDOW,

	_NET_WM_STATE_MODAL,
	_NET_WM_STATE_STICKY,
	_NET_WM_STATE_MAXIMIZED_VERT,
	_NET_WM_STATE_MAXIMIZED_HORZ,
	_NET_WM_STATE_SHADED,
	_NET_WM_STATE_SKIP_TASKBAR,
	_NET_WM_STATE_SKIP_PAGER,
	_NET_WM_STATE_HIDDEN,
	_NET_WM_STATE_FULLSCREEN,
	_NET_WM_STATE_ABOVE,
	_NET_WM_STATE_BELOW,
	_NET_WM_STATE_DEMANDS_ATTENTION,
	_NET_WM_STATE_FOCUSED,

	_NET_WM_ALLOWED_ACTIONS,
	_NET_WM_ACTION_MOVE,
	_NET_WM_ACTION_RESIZE,
	_NET_WM_ACTION_MINIMIZE,
	_NET_WM_ACTION_SHADE,
	_NET_WM_ACTION_STICK,
	_NET_WM_ACTION_MAXIMIZE_HORZ,
	_NET_WM_ACTION_MAXIMIZE_VERT,
	_NET_WM_ACTION_FULLSCREEN,
	_NET_WM_ACTION_CHANGE_DESKTOP,
	_NET_WM_ACTION_CLOSE,
	_NET_WM_ACTION_ABOVE,
	_NET_WM_ACTION_BELOW,

	_NET_CLOSE_WINDOW,

	_NET_REQUEST_FRAME_EXTENTS,
	_NET_FRAME_EXTENTS,

	_NET_WM_ICON,

	_NET_WM_PID,

	_NET_SUPPORTING_WM_CHECK,

	_NET_DESKTOP_NAMES,

	_MOTIF_WM_HINTS,

	_NET_WM_OPAQUE_REGION,
	_NET_WM_BYPASS_COMPOSITOR,

	_NET_WM_MOVERESIZE,

	PAGE_QUIT,

	LAST_ATOM
};

/**
 * To avoid miss match order in enum list and this list we use
 * this struct which will make id match the name.
 **/
struct atom_item_t {
	int id;
	char const * name;
};

#define ATOM_ITEM(name) { name, #name },

atom_item_t const atom_name[] = {

ATOM_ITEM(ATOM)
ATOM_ITEM(CARDINAL)
ATOM_ITEM(WINDOW)
ATOM_ITEM(UTF8_STRING)
ATOM_ITEM(STRING)

ATOM_ITEM(WM_STATE)
ATOM_ITEM(WM_NAME)
ATOM_ITEM(WM_ICON_NAME)
ATOM_ITEM(WM_DELETE_WINDOW)
ATOM_ITEM(WM_PROTOCOLS)
ATOM_ITEM(WM_TAKE_FOCUS)
ATOM_ITEM(WM_CLASS)
ATOM_ITEM(WM_CHANGE_STATE)
ATOM_ITEM(WM_TRANSIENT_FOR)
ATOM_ITEM(WM_HINTS)
ATOM_ITEM(WM_NORMAL_HINTS)
ATOM_ITEM(WM_SIZE_HINTS)
ATOM_ITEM(WM_COLORMAP_WINDOWS)
ATOM_ITEM(WM_CLIENT_MACHINE)

ATOM_ITEM(_NET_SUPPORTED)

ATOM_ITEM(_NET_WM_NAME)
ATOM_ITEM(_NET_WM_VISIBLE_NAME)
ATOM_ITEM(_NET_WM_ICON_NAME)
ATOM_ITEM(_NET_WM_VISIBLE_ICON_NAME)
ATOM_ITEM(_NET_WM_STATE)
ATOM_ITEM(_NET_WM_STRUT_PARTIAL)
ATOM_ITEM(_NET_WM_STRUT)
ATOM_ITEM(_NET_WM_ICON_GEOMETRY)

ATOM_ITEM(_NET_WM_WINDOW_TYPE)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_DESKTOP)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_DOCK)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_TOOLBAR)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_MENU)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_UTILITY)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_SPLASH)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_DIALOG)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_POPUP_MENU)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_TOOLTIP)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_NOTIFICATION)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_COMBO)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_DND)
ATOM_ITEM(_NET_WM_WINDOW_TYPE_NORMAL)

ATOM_ITEM(_NET_WM_USER_TIME)
ATOM_ITEM(_NET_WM_USER_TIME_WINDOW)

ATOM_ITEM(_NET_CLIENT_LIST)
ATOM_ITEM(_NET_CLIENT_LIST_STACKING)

ATOM_ITEM(_NET_NUMBER_OF_DESKTOPS)
ATOM_ITEM(_NET_DESKTOP_GEOMETRY)
ATOM_ITEM(_NET_DESKTOP_VIEWPORT)
ATOM_ITEM(_NET_CURRENT_DESKTOP)
ATOM_ITEM(_NET_WM_DESKTOP)

ATOM_ITEM(_NET_SHOWING_DESKTOP)
ATOM_ITEM(_NET_WORKAREA)

ATOM_ITEM(_NET_ACTIVE_WINDOW)

ATOM_ITEM(_NET_WM_STATE_MODAL)
ATOM_ITEM(_NET_WM_STATE_STICKY)
ATOM_ITEM(_NET_WM_STATE_MAXIMIZED_VERT)
ATOM_ITEM(_NET_WM_STATE_MAXIMIZED_HORZ)
ATOM_ITEM(_NET_WM_STATE_SHADED)
ATOM_ITEM(_NET_WM_STATE_SKIP_TASKBAR)
ATOM_ITEM(_NET_WM_STATE_SKIP_PAGER)
ATOM_ITEM(_NET_WM_STATE_HIDDEN)
ATOM_ITEM(_NET_WM_STATE_FULLSCREEN)
ATOM_ITEM(_NET_WM_STATE_ABOVE)
ATOM_ITEM(_NET_WM_STATE_BELOW)
ATOM_ITEM(_NET_WM_STATE_DEMANDS_ATTENTION)
ATOM_ITEM(_NET_WM_STATE_FOCUSED)

ATOM_ITEM(_NET_WM_ALLOWED_ACTIONS)
ATOM_ITEM(_NET_WM_ACTION_MOVE)
ATOM_ITEM(_NET_WM_ACTION_RESIZE)
ATOM_ITEM(_NET_WM_ACTION_MINIMIZE)
ATOM_ITEM(_NET_WM_ACTION_SHADE)
ATOM_ITEM(_NET_WM_ACTION_STICK)
ATOM_ITEM(_NET_WM_ACTION_MAXIMIZE_HORZ)
ATOM_ITEM(_NET_WM_ACTION_MAXIMIZE_VERT)
ATOM_ITEM(_NET_WM_ACTION_FULLSCREEN)
ATOM_ITEM(_NET_WM_ACTION_CHANGE_DESKTOP)
ATOM_ITEM(_NET_WM_ACTION_CLOSE)
ATOM_ITEM(_NET_WM_ACTION_ABOVE)
ATOM_ITEM(_NET_WM_ACTION_BELOW)

ATOM_ITEM(_NET_CLOSE_WINDOW)
ATOM_ITEM(_NET_REQUEST_FRAME_EXTENTS)
ATOM_ITEM(_NET_FRAME_EXTENTS)
ATOM_ITEM(_NET_WM_ICON)
ATOM_ITEM(_NET_WM_PID)
ATOM_ITEM(_NET_SUPPORTING_WM_CHECK)
ATOM_ITEM(_NET_DESKTOP_NAMES)

ATOM_ITEM(_MOTIF_WM_HINTS)

ATOM_ITEM(_NET_WM_OPAQUE_REGION)
ATOM_ITEM(_NET_WM_BYPASS_COMPOSITOR)

ATOM_ITEM(_NET_WM_MOVERESIZE)

ATOM_ITEM(PAGE_QUIT)

};

#undef ATOM_ITEM

/**
 * This is a smart pointer like atom list handler, allow, fast an short
 * atom call.
 **/
class atom_handler_t {
	Atom * _data;
	char const ** _name;

	/** cannot be moved or copied **/
	atom_handler_t(atom_handler_t const & a);
	atom_handler_t(atom_handler_t const && a);
	atom_handler_t & operator=(atom_handler_t const & a);

public:

	atom_handler_t(Display * dpy) {
		unsigned const n_items = sizeof(atom_name) / sizeof(atom_item_t);

		_data = new Atom[LAST_ATOM];
		_name = new char const *[LAST_ATOM];

		for (int i = 0; i < LAST_ATOM; ++i) {
			_data[i] = None;
			_name[i] = nullptr;
		}

		for (unsigned int i = 0; i < n_items; ++i) {
			_data[atom_name[i].id] = XInternAtom(dpy, atom_name[i].name, False);
			_name[atom_name[i].id] = atom_name[i].name;
		}

		for (unsigned int i = 0; i < LAST_ATOM; ++i) {
			if(_data[i] == None)
				printf("FAIL INIT ATOM %d\n", i);
		}
	}

	~atom_handler_t() {
		delete [] _data;
		delete [] _name;
	}

	Atom operator() (int id) {
		return _data[id];
	}

	Atom operator[] (int id) {
		return _data[id];
	}


};

}

#endif /* ATOMS_HXX_ */
