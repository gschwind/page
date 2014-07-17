/*
 * keymap.hxx
 *
 *  Created on: 3 juil. 2014
 *      Author: gschwind
 */

#ifndef KEYMAP_HXX_
#define KEYMAP_HXX_

#include <cassert>

#include "key_desc.hxx"

namespace page {

class keymap_t {

	int first_keycode;
	int last_keycode;
	int n_mod;

	/** KeySym is an unique key identifier used by X11, it's the equivalent of UNICODE id of a glyph
	 * This table store a mapping between keycode (a key on keyboard) to a KeySym. keycode can be bound to
	 * Several KeySim because key press can be modified by key modifier like shift or alt key.
	 **/
	KeySym * data;
	XModifierKeymap * modmap;

	unsigned int _numlock_mod_mask;

public:

	keymap_t(Display * dpy) {
		XDisplayKeycodes(dpy, &first_keycode, &last_keycode);
		data = XGetKeyboardMapping(dpy, first_keycode, (last_keycode - first_keycode) + 1, &n_mod);
		modmap = XGetModifierMapping(dpy);

		_numlock_mod_mask = 0;

		KeyCode numlock_kc = find_keysim(XK_Num_Lock);
		if(numlock_kc != 0) {
			for(int i = 0; i < 8 * modmap->max_keypermod; ++i) {
				if(modmap->modifiermap[i] == numlock_kc) {
					_numlock_mod_mask |= 1<<(i / modmap->max_keypermod);
				}
			}
		}

	}

	~keymap_t() {
		if(data != NULL) {
			XFree(data);
		}
	}

	unsigned int numlock_mod_mask() {
		return _numlock_mod_mask;
	}

	KeySym get(KeyCode kc, int mod = 0) {
		assert(kc >= first_keycode and kc <= last_keycode);
		assert(mod >= 0 and mod < n_mod);
		if (data != NULL) {
			return data[(kc - first_keycode) * n_mod + mod];
		} else {
			return NoSymbol;
		}
	}

	KeyCode find_keysim(KeySym ks) {
		for(KeySym i = 0; i <= (last_keycode - first_keycode + 1) * n_mod; ++i) {
			if(ks == data[i]) {
				return (i / n_mod) + first_keycode;
			}
		}
		return 0;
	}


};


}



#endif /* KEYMAP_HXX_ */
