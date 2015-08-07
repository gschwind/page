/*
 * page_exception.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef PAGE_EXCEPTION_HXX_
#define PAGE_EXCEPTION_HXX_

#include <exception>
#include <string>

namespace page {

using namespace std;

class exception : public std::exception {
	string msg;
public:

	template<typename ... Args>
	exception(char const * fmt, Args ... args) {
	    size_t size = std::snprintf(nullptr, 0, fmt, args ... ) + 1;
	    unique_ptr<char[]> buf{new char[size]};
	    std::snprintf( buf.get(), size, fmt, args ... );
	    msg = string( buf.get(), buf.get() + size - 1 );
	}

	~exception() throw () { }

	char const * what() const throw () {
		return msg.c_str();
	}

};

}

#endif /* PAGE_EXCEPTION_HXX_ */
