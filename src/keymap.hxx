/*
 * keymap.hxx
 *
 *  Created on: 3 juil. 2014
 *      Author: gschwind
 */

#ifndef KEYMAP_HXX_
#define KEYMAP_HXX_

#include <cassert>


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

public:

	keymap_t(Display * dpy) {
		XDisplayKeycodes(dpy, &first_keycode, &last_keycode);
		data = XGetKeyboardMapping(dpy, first_keycode, (last_keycode - first_keycode) + 1, &n_mod);
	}

	~keymap_t() {
		if(data != NULL) {
			XFree(data);
		}
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
