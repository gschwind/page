/*
 * client.hxx
 *
 *  Created on: Feb 25, 2011
 *      Author: gschwind
 */

#ifndef CLIENT_HXX_
#define CLIENT_HXX_

#include <string>

class client_t {
	std::string name;
public:
	client_t(char const * name) :
		name(name) {
	}
	inline std::string const & get_name() {
		return name;
	}
};

#endif /* CLIENT_HXX_ */
