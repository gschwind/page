/*
 * atoms.h
 *
 *  Created on: 6 nov. 2010
 *      Author: gschwind
 */

#ifndef ATOMS_H_
#define ATOMS_H_

#include <X11/Xlib.h>

typedef struct _atoms_t atoms_t;

struct _atoms_t {
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



#endif /* ATOMS_H_ */
