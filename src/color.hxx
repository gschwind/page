/*
 * color.hxx
 *
 *  Created on: 24 mars 2013
 *      Author: gschwind
 */

#ifndef COLOR_HXX_
#define COLOR_HXX_

struct color_t {
	double r, g, b;

	color_t(unsigned color) {
		r = ((color >> 16) & 0x000000ff) / 255.0;
		g = ((color >> 8) & 0x000000ff) / 255.0;
		b = ((color >> 0) & 0x000000ff) / 255.0;
	}

};



#endif /* COLOR_HXX_ */
