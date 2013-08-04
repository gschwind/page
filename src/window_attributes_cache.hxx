/*
 * window_attributes_cache.hxx
 *
 *  Created on: 4 août 2013
 *      Author: bg
 */

#ifndef WINDOW_ATTRIBUTES_CACHE_HXX_
#define WINDOW_ATTRIBUTES_CACHE_HXX_

#include <X11/X.h>
#include <map>

using namespace std;

namespace page {

class window_attributes_cache_t {

	map<Window, XWindowAttributes> _attributes_cache;

public:

	XWindowAttributes * get_window_attributes(Display * dpy, Window w) {
		map<Window, XWindowAttributes>::iterator x = _attributes_cache.find(w);
		if(x != _attributes_cache.end()) {
			return &(x->second);
		} else {
			XWindowAttributes wa;
			if(XGetWindowAttributes(dpy, w, &wa)) {
				_attributes_cache[w] = wa;
				return &_attributes_cache[w];
			} else {
				return 0;
			}
		}
	}

	void mark_durty(Window w) {
		_attributes_cache.erase(w);
	}


};

}


#endif /* WINDOW_ATTRIBUTES_CACHE_HXX_ */
