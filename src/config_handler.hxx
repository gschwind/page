/*
 * config_handler.hxx
 *
 * Config file based on GkeyFile from glib
 *
 */

#ifndef CONFIG_HANDLER_HXX_
#define CONFIG_HANDLER_HXX_

#include <glib.h>

#include <string>
#include <map>

using namespace std;

namespace page {

class config_handler_t {

	map<pair<string, string>, string> _data;

public:
	config_handler_t();
	void merge_from_file_if_exist(string const & f);

	bool has_key(char const * groups, char const * key);

	string get_string(char const * groups, char const * key);
	double get_double(char const * groups, char const * key);

};


}



#endif /* CONFIG_HANDLER_HXX_ */
