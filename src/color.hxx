/*
 * color.hxx
 *
 *  Created on: 24 mars 2013
 *      Author: gschwind
 */

#ifndef COLOR_HXX_
#define COLOR_HXX_

#include <string>
#include <stdexcept>

using namespace std;

struct color_t {
	double r, g, b;

	color_t() {
		r = 0;
		g = 0;
		b = 0;
	}

	color_t(unsigned color) {
		set(color);
	}

	color_t(string const & scolor) {
		set(scolor);
	}

	void set(string const & scolor) {
		if(scolor.size() != 6) {
			throw runtime_error("bad color format");
		}

		char const * c = scolor.c_str();
		unsigned x = strtoul(c, 0, 16);

		set(x);

	}

	void set(unsigned color) {
		r = ((color >> 16) & 0x000000ff) / 255.0;
		g = ((color >> 8) & 0x000000ff) / 255.0;
		b = ((color >> 0) & 0x000000ff) / 255.0;
	}

};



#endif /* COLOR_HXX_ */
