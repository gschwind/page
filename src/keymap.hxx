/*
 * keymap.hxx
 *
 *  Created on: 3 juil. 2014
 *      Author: gschwind
 */

#ifndef KEYMAP_HXX_
#define KEYMAP_HXX_

#include <X11/Xlib.h>
#include <X11/keysymdef.h>

#include <cassert>

namespace page {

using namespace std;

class keymap_t {

	int first_keycode;
	int last_keycode;
	int keycodes_per_modifier;
	int keysyms_per_keycode;

	/**
	 * KeySym is an unique key identifier used by X11, it's the equivalent of UNICODE id of a glyph
	 * This table store a mapping between keycode (a key on keyboard) to a KeySym. keycode can be bound to
	 * Several KeySim because key press can be modified by key modifier like shift or alt key.
	 **/
	vector<xcb_keysym_t> _keydata;
	vector<xcb_keycode_t> _moddata;

	unsigned int _numlock_mod_mask;

public:

	keymap_t(xcb_connection_t * dpy) {
		first_keycode = xcb_get_setup(dpy)->min_keycode;
		last_keycode = xcb_get_setup(dpy)->max_keycode;

		xcb_get_keyboard_mapping_cookie_t ck0 = xcb_get_keyboard_mapping(dpy, first_keycode, (last_keycode - first_keycode) + 1);
		xcb_get_modifier_mapping_cookie_t ck1 = xcb_get_modifier_mapping(dpy);

		xcb_get_keyboard_mapping_reply_t * keymap = xcb_get_keyboard_mapping_reply(dpy, ck0, 0);
		xcb_get_modifier_mapping_reply_t * modmap = xcb_get_modifier_mapping_reply(dpy, ck1, 0);

		xcb_keysym_t * keydata = xcb_get_keyboard_mapping_keysyms(keymap);

		_keydata.clear();
		_keydata.insert(_keydata.end(), &keydata[0], &keydata[xcb_get_keyboard_mapping_keysyms_length(keymap)]);

		xcb_keycode_t * moddata = xcb_get_modifier_mapping_keycodes(modmap);
		keycodes_per_modifier = modmap->keycodes_per_modifier;
		keysyms_per_keycode = keymap->keysyms_per_keycode;

		_moddata.clear();
		_moddata.insert(_moddata.end(), &moddata[0], &moddata[xcb_get_modifier_mapping_keycodes_length(modmap)]);

		_numlock_mod_mask = 0;

		xcb_keycode_t numlock_kc = find_keysim(XK_Num_Lock);
		if(numlock_kc != 0) {
			for(int i = 0; i < 8 * keycodes_per_modifier; ++i) {
				if(_moddata[i] == numlock_kc) {
					_numlock_mod_mask |= 1<<(i / keycodes_per_modifier);
				}
			}
		}

		free(keymap);
		free(modmap);

	}

	~keymap_t() {

	}

	unsigned int numlock_mod_mask() const {
		return _numlock_mod_mask;
	}

	xcb_keysym_t get(xcb_keycode_t kc, int mod = 0) const {
		assert(kc >= first_keycode and kc <= last_keycode);
		assert(mod >= 0 and mod < keysyms_per_keycode);
		if (_keydata.size() > 0) {
			return _keydata[(kc - first_keycode) * keysyms_per_keycode + mod];
		} else {
			return XCB_NO_SYMBOL;
		}
	}

	xcb_keycode_t find_keysim(xcb_keysym_t ks) const {
		for(xcb_keysym_t i = 0; i < (last_keycode - first_keycode + 1) * keysyms_per_keycode; ++i) {
			if(ks == _keydata[i]) {
				return (i / keysyms_per_keycode) + first_keycode;
			}
		}
		return 0;
	}


};


}



#endif /* KEYMAP_HXX_ */
