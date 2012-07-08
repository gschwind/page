/*
 * icon.hxx
 *
 *  Created on: Oct 29, 2011
 *      Author: gschwind
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
