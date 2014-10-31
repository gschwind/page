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
#include <cstring>

namespace page {

class exception : public std::exception {
	char const * msg;
	char const * file;
	unsigned line;
public:

	exception(char const * msg, char const * file, unsigned line) :
			msg(msg), file(file), line(line) {

	}

	~exception() throw () { }

	char const * what() const throw () {
		static char buffer[1024] = "";
		snprintf(buffer, 1024, "Error: %s in %s at %u", msg, file, line);
		return buffer;
	}

};

}

/** define my custom assert **/
#define page_assert(test) \
	do { \
		if(not (test)) \
			throw exception(#test, __FILE__, __LINE__); \
	} while(false)



#endif /* PAGE_EXCEPTION_HXX_ */
