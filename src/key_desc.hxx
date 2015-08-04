/*
 * key_desc.hxx
 *
 *  Created on: 12 juil. 2014
 *      Author: gschwind
 */

#ifndef KEY_DESC_HXX_
#define KEY_DESC_HXX_


namespace page {

struct key_desc_t {
	KeySym ks;
	unsigned int mod;

	bool operator==(key_desc_t const & x) {
		return (ks == x.ks) and (mod == x.mod);
	}
};


}


#endif /* KEY_DESC_HXX_ */
