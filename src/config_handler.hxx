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

#include <utility>
#include <map>
#include <string>

namespace page {

class config_handler_t {

	typedef std::pair<std::string, std::string> _key_t;
	std::map<_key_t, std::string> _data;

	std::string const & find(char const * group, char const * key) const;

public:
	config_handler_t();
	~config_handler_t();

	void merge_from_file_if_exist(std::string const & filename);

	bool has_key(char const * groups, char const * key) const;

	std::string get_string(char const * groups, char const * key) const;
	double get_double(char const * groups, char const * key) const;
	long get_long(char const * group, char const * key) const;
	std::string get_value(char const * group, char const * key) const;

};


}



#endif /* CONFIG_HANDLER_HXX_ */
