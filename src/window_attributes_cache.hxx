/*
 * window_attributes_cache.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef WINDOW_ATTRIBUTES_CACHE_HXX_
#define WINDOW_ATTRIBUTES_CACHE_HXX_

#include <X11/X.h>
#include <map>

#include "smart_pointer.hxx"

using namespace std;

namespace page {

struct window_attribute_t: public XWindowAttributes {
	bool is_valid;

	window_attribute_t() {
		bzero(static_cast<XWindowAttributes *>(this),
				sizeof(XWindowAttributes));
		is_valid = false;
	}
};

typedef smart_pointer_t<window_attribute_t> p_window_attribute_t;

class window_attributes_cache_t {

	map<Window, p_window_attribute_t> _attributes_cache;

public:

	p_window_attribute_t get_window_attributes(Display * dpy, Window w) {

		map<Window, p_window_attribute_t>::iterator x =
				_attributes_cache.find(w);

		if (x != _attributes_cache.end()) {
			if (!x->second->is_valid) {
				if (!XGetWindowAttributes(dpy, w, x->second.get())) {
					x->second->is_valid = false;
					return x->second;
				}
			}

			x->second->is_valid = true;
			return x->second;

		} else {
			if (XGetWindowAttributes(dpy, w, _attributes_cache[w].get())) {
				_attributes_cache[w]->is_valid = true;
				return _attributes_cache[w];
			} else {
				_attributes_cache[w]->is_valid = false;
				return _attributes_cache[w];
			}
		}
	}

	void mark_durty(Window w) {
		_attributes_cache[w]->is_valid = false;
	}

	void destroy(Window w) {
		map<Window, p_window_attribute_t>::iterator x =
				_attributes_cache.find(w);
		if (x != _attributes_cache.end()) {
			_attributes_cache.erase(x);
		}
	}


};

}


#endif /* WINDOW_ATTRIBUTES_CACHE_HXX_ */
