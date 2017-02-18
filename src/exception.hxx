/*
 * Copyright (2014) Benoit Gschwind
 *
 * This file is part of page-compositor.
 *
 * page-compositor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * page-compositor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with page-compositor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef EXCEPTION_HXX_
#define EXCEPTION_HXX_

#include <cstdarg>

namespace page {

class exception_t : public std::exception {
	char * str;
public:
	exception_t(char const * fmt, ...) : str(nullptr) {
		va_list l;
		va_start(l, fmt);
		if(vasprintf(&str, fmt, l) < 0)
			str = nullptr;
		va_end(l);
	}

	~exception_t() noexcept { free(str); }

	char const * what() const noexcept {
		return str?str:"nested memory error: could not allocate error message";
	}

};

}



#endif /* EXCEPTION_HXX_ */
