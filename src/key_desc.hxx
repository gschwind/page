/*
 * key_desc.hxx
 *
 *  Created on: 12 juil. 2014
 *      Author: gschwind
 */

#ifndef KEY_DESC_HXX_
#define KEY_DESC_HXX_

#include <string>

#include "exception.hxx"

namespace page {

using namespace std;

struct key_desc_t {
	KeySym ks;
	unsigned int mod;

	bool operator==(key_desc_t const & x) {
		return (ks == x.ks) and (mod == x.mod);
	}

	key_desc_t & operator=(key_desc_t const & x) {
		ks = x.ks;
		mod = x.mod;
		return *this;
	}

	key_desc_t & operator=(string const & desc) {
		_find_key_from_string(desc);
		return *this;
	}

	key_desc_t() {
		ks = XK_VoidSymbol;
		mod = 0;
	}

	key_desc_t(string const & desc) {
		_find_key_from_string(desc);
	}

	/**
	 * Parse std::string like "mod4 f" to modifier mask (mod) and keysym (ks)
	 **/
	void _find_key_from_string(string const & desc) {

		/* no binding is set */
		ks = XK_VoidSymbol;
		mod = 0;

		if(desc == "null")
			return;

		/* find all modifier */
		std::size_t bos = 0;
		std::size_t eos = desc.find(" ", bos);
		while(eos != std::string::npos) {
			std::string modifier = desc.substr(bos, eos-bos);

			/* check for supported modifier */
			if(modifier == "shift") {
				mod |= ShiftMask;
			} else if (modifier == "lock") {
				mod |= LockMask;
			} else if (modifier == "control") {
				mod |= ControlMask;
			} else if (modifier == "mod1") {
				mod |= Mod1Mask;
			} else if (modifier == "mod2") {
				mod |= Mod2Mask;
			} else if (modifier == "mod3") {
				mod |= Mod3Mask;
			} else if (modifier == "mod4") {
				mod |= Mod4Mask;
			} else if (modifier == "mod5") {
				mod |= Mod5Mask;
			} else {
				throw exception_t("invalid modifier '%s' for key binding", modifier.c_str());
			}

			bos = eos+1; /* next char of char space */
			eos = desc.find(" ", bos);
		}

		string key = desc.substr(bos);
		ks = XStringToKeysym(key.c_str());
		if(ks == NoSymbol) {
			throw exception_t("key binding not found");
		}
	}

};


}


#endif /* KEY_DESC_HXX_ */
