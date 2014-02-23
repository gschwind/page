/*
 * config_handler.cxx
 *
 *  Created on: 30 avr. 2013
 *      Author: gschwind
 */

#include "config_handler.hxx"
#include <stdexcept>
#include <sstream>
#include <string>

namespace page {

config_handler_t::config_handler_t() {

}

config_handler_t::~config_handler_t() {
	_data.clear();
}

/* this function merge config file, the last one override previous loaded files */
void config_handler_t::merge_from_file_if_exist(string const & f) {

	/* check if file exist and is readable */
	if(access(f.c_str(), R_OK) != 0)
		return;

	GKeyFile * kf = g_key_file_new();
	if (kf == 0)
		throw runtime_error("could not allocate memory");

	if (!g_key_file_load_from_file(kf, f.c_str(), G_KEY_FILE_NONE, 0)) {
		throw runtime_error("could not load configuration file");
	}

	gchar ** groups = 0;
	gsize groups_length;

	groups = g_key_file_get_groups(kf, &groups_length);

	if (groups != 0) {
		for (unsigned g = 0; g < groups_length; ++g) {

			gchar ** keys = 0;
			gsize keys_length;

			keys = g_key_file_get_keys(kf, groups[g], &keys_length, 0);

			if (keys != 0) {
				for (unsigned k = 0; k < keys_length; ++k) {
					gchar * value = g_key_file_get_value(kf, groups[g], keys[k],
							0);
					if (value != 0) {
						string svalue(value);
						_data[pair<string, string>(groups[g], keys[k])] =
								svalue;
						g_free(value);
					}
				}
				g_strfreev(keys);
			}
		}
		g_strfreev(groups);
	}

	g_key_file_free(kf);

}

string config_handler_t::get_string(char const * group, char const * key) {
	map<pair<string, string>, string >::iterator i = _data.find(pair<string, string>(group, key));
	if(i != _data.end()) {
		char * x = g_strcompress(i->second.c_str());
		string ret = x;
		g_free(x);
		return ret;
	} else {
		throw runtime_error("group:key not found");
	}
}

double config_handler_t::get_double(char const * group, char const * key) {
	map<pair<string, string>, string >::iterator i = _data.find(pair<string, string>(group, key));
	if(i != _data.end()) {
		return g_strtod(i->second.c_str(), NULL);
	} else {
		throw runtime_error("group:key not found");
	}
}

long config_handler_t::get_long(char const * group, char const * key) {
	long ret = 0;
	map<pair<string, string>, string >::iterator i = _data.find(pair<string, string>(group, key));
	if(i != _data.end()) {
		std::istringstream ( i->second ) >> ret;
	} else {
		throw runtime_error("group:key not found");
	}

	return ret;
}

bool config_handler_t::has_key(char const * group, char const * key) {
	map<pair<string, string>, string >::iterator i = _data.find(pair<string, string>(group, key));
	return (i != _data.end());
}

}


