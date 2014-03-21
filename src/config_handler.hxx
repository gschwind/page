/*
 * config_handler.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef CONFIG_HANDLER_HXX_
#define CONFIG_HANDLER_HXX_

#include <glib.h>

#include <string>
#include <map>
#include <unistd.h>

using namespace std;

namespace page {

class config_handler_t {

	map<pair<string, string>, string> _data;

public:
	config_handler_t();
	~config_handler_t();
	void merge_from_file_if_exist(string const & f);

	bool has_key(char const * groups, char const * key);

	string get_string(char const * groups, char const * key);
	double get_double(char const * groups, char const * key);
	long get_long(char const * group, char const * key);

};


}



#endif /* CONFIG_HANDLER_HXX_ */
