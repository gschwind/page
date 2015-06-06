/*
 * color.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef COLOR_HXX_
#define COLOR_HXX_

#include <stdexcept>

namespace page {

struct color_t {
	double r, g, b, a;

	color_t(color_t const & x) : r{x.r}, g{x.g}, b{x.b}, a{x.a} { }

	color_t() : r{0.0}, g{0.0}, b{0.0}, a{0.0} { }

	color_t(double r, double g, double b, double a): r{r}, g{g}, b{b}, a{a} { }

	color_t(unsigned color) {
		set(color);
	}

	color_t(std::string const & scolor) {
		set(scolor);
	}

	void set(std::string const & scolor) {
		if(scolor.size() == 6 || scolor.size() == 8) {
			char const * c = scolor.c_str();
			unsigned x = strtoul(c, 0, 16);
			set(x);
		} else {
			throw std::runtime_error("bad color format");
		}
	}

	void set(unsigned color) {
		/** convert common alpha sense to cairo alpha **/
		a = 1.0 - ((color >> 24) & 0x000000ff) / 255.0;
		r = ((color >> 16) & 0x000000ff) / 255.0;
		g = ((color >> 8) & 0x000000ff) / 255.0;
		b = ((color >> 0) & 0x000000ff) / 255.0;
	}

};

}

#endif /* COLOR_HXX_ */
