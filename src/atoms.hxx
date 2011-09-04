/*
 * atoms.hxx
 *
 *  Created on: 26 févr. 2011
 *      Author: gschwind
 */

#ifndef ATOMS_HXX_
#define ATOMS_HXX_

#include <X11/Xatom.h>

struct atoms_t {
	Atom CARDINAL;
	Atom ATOM;
	Atom WINDOW;

	Atom WM_STATE;
	Atom WM_NAME;

	Atom _NET_SUPPORTED;
	Atom _NET_WM_NAME;
	Atom _NET_WM_STATE;
	Atom _NET_WM_STATE_FULLSCREEN;
	Atom _NET_WM_STRUT_PARTIAL;

	Atom _NET_WM_WINDOW_TYPE;
	Atom _NET_WM_WINDOW_TYPE_DOCK;

	Atom _NET_WM_USER_TIME;

	Atom _NET_CLIENT_LIST;
	Atom _NET_CLIENT_LIST_STACKING;



	Atom _NET_NUMBER_OF_DESKTOPS;

	/*TODO Atoms for root window */
	Atom _NET_DESKTOP_GEOMETRY;
	Atom _NET_DESKTOP_VIEWPORT;
	Atom _NET_CURRENT_DESKTOP;
	Atom _NET_DESKTOP_NAMES;
	Atom _NET_ACTIVE_WINDOW;
	Atom _NET_WORKAREA;
	Atom _NET_SUPPORTING_WM_CHECK;
	Atom _NET_VIRTUAL_ROOTS;
	Atom _NET_DESKTOP_LAYOUT;
	Atom _NET_SHOWING_DESKTOP;


};

#endif /* ATOMS_HXX_ */
