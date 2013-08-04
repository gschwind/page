/*
 * window.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef WINDOW_HXX_
#define WINDOW_HXX_

#include <X11/X.h>
#include <X11/Xutil.h>
#include <set>
#include <map>
#include <string>
#include <vector>

#include "atoms.hxx"
#include "region.hxx"

using namespace std;

namespace page {

class window_t {

	/* avoid copy */
	window_t(window_t const &);
	window_t & operator=(window_t const &);

public:

	window_t(Window w) :
			id(w) {

	}
	~window_t();


	typedef std::set<Atom> atom_set_t;
	typedef std::list<Atom> atom_list_t;
	typedef Window window_key_t;
	typedef std::map<window_key_t, window_t *> window_map_t;

	Window const id;

	template<typename T> struct _cache_data_t {
		bool is_durty;
		bool has_value;
		T value;

		_cache_data_t() {
			is_durty = true;
			has_value = false;
		}

	};

	_cache_data_t<XWindowAttributes> _window_attributes;

	void mark_durty_window_attributes() {
		_window_attributes.is_durty = true;
	}

	void mark_durty_all() {
		mark_durty_window_attributes();
	}

	XWindowAttributes const & get_window_attributes() {
		return _window_attributes.value;
	}

	bool has_window_attributes() {
		return _window_attributes.has_value;
	}

	/* update windows attributes */
	void process_configure_notify_event(XConfigureEvent const & e);

	box_int_t get_size();

};

typedef std::list<window_t *> window_list_t;
typedef std::set<window_t *> window_set_t;

}

#endif /* WINDOW_HXX_ */
