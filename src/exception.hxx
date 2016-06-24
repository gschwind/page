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
		int n = vsnprintf(nullptr, 0, fmt, l);
		va_end(l);
		str = new char[n+1];
		va_start(l, fmt);
		vsnprintf(str, n+1, fmt, l);
		va_end(l);
	}

	~exception_t() noexcept { delete[] str; }

	char const * what() const noexcept {
		return str;
	}

};



///** not define **/
//template <typename T>
//class exception_t<T>;
//
//template <>
//class exception_t<char const *> : public exception {
//	char const * data;
//public:
//	exception_t(char const * data) : data(data) { }
//
//	char const * what() {
//		static char error_buffer[4096];
//		snprintf(error_buffer, 4096, fmt, key);
//		return error_buffer;
//	}
//
//};

//
//class configuration_error_t : public exception {
//	char const * fmt;
//	char const * key;
//public:
//	configuration_error_t(char const * fmt, char const * key) : fmt(fmt), key(key) { }
//
//	char const * what() {
//		static char error_buffer[4096];
//		snprintf(error_buffer, 4096, fmt, key);
//		return error_buffer;
//	}
//
//};

}



#endif /* EXCEPTION_HXX_ */
