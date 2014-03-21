/*
 * icon.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef ICON_HXX_
#define ICON_HXX_

namespace page {

struct icon_t {
	int width;
	int height;
	unsigned char * data;

	icon_t() : width(0), height(0), data(0) {

	}

};

}


#endif /* ICON_HXX_ */
