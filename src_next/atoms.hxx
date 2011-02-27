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

	Atom WM_STATE;

	Atom _NET_SUPPORTED;
	Atom _NET_WM_NAME;
	Atom _NET_WM_STATE;
	Atom _NET_WM_STATE_FULLSCREEN;
	Atom _NET_WM_STRUT_PARTIAL;

	Atom _NET_WM_WINDOW_TYPE;
	Atom _NET_WM_WINDOW_TYPE_DOCK;

};

#endif /* ATOMS_HXX_ */
