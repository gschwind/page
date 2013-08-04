/*
 * properties_cache.hxx
 *
 *  Created on: 4 août 2013
 *      Author: bg
 */

#ifndef PROPERTIES_CACHE_HXX_
#define PROPERTIES_CACHE_HXX_


#include "utils.hxx"

namespace page {

class properties_cache_t {

	typedef pair<Window, Atom> _key_t;

	struct _value_t {
		Atom type;
		int format;
		unsigned long int nitems;
		void * data;

		_value_t(Atom type = 0, int format = 0, unsigned long int nitems = 0,
				void * data = 0) :
				type(type), format(format), nitems(nitems), data(data) {
		}

	};

	/** stat stuff **/
	unsigned int _hit;
	unsigned int _miss;

	map<_key_t , _value_t> _cache;

	template<typename T>
	bool _unsafe_get_window_property(Display * dpy, Window win, Atom prop,
			Atom type, vector<T> * v = 0) {
		_value_t tmp(type, _format<T>::size);
		if (::page::get_window_property_void<T>(dpy, win, prop, type, &tmp.nitems, &tmp.data)) {
			_cache[_key_t(win, prop)] = tmp;
			if (v != 0) {
				T * val = reinterpret_cast<T *>(tmp.data);
				v->assign(&val[0], &val[tmp.nitems]);
			}
//			printf("cache add key(%d,%d) = (%d,%d,%d,%d)\n", win, prop, tmp.type, tmp.format, tmp.nitems, tmp.data);
//			fflush(stdout);
			return true;
		} else {
			_cache[_key_t(win, prop)] = _value_t(type, _format<T>::size);

//			printf("cache add key(%d,%d) = (%d,%d,%d,%d)\n", win, prop, type, _format<T>::size, 0, 0);
//			fflush(stdout);
			return false;
		}
	}

public:

	properties_cache_t() {
		_miss = 0;
		_hit = 0;
	}

	template<typename T>
	bool get_window_property(Display * dpy, Window win, Atom prop, Atom type,
			vector<T> * v = 0) {
		map<_key_t, _value_t>::iterator x = _cache.find(_key_t(win, prop));
		if (x == _cache.end()) {
			++_miss;
			return _unsafe_get_window_property(dpy, win, prop, type, v);
		} else {
			if (x->second.type == type
					and x->second.format == _format<T>::size) {
//				printf("cache found key(%d,%d) = (%d,%d,%d,%d)\n", x->first.first,
//						x->first.second, x->second.type, x->second.format,
//						x->second.nitems, x->second.data);
//				fflush(stdout);
				++_hit;
				if (x->second.data != 0) {
					if (v != 0) {
						T * val = reinterpret_cast<T *>(x->second.data);
						v->assign(&val[0], &val[x->second.nitems]);
					}
					return true;
				} else {
					return false;
				}
			} else {
				++_miss;
				mark_durty(win, prop);
				return _unsafe_get_window_property(dpy, win, prop, type, v);
			}
		}
	}

	void mark_durty(Window win, Atom prop) {
		map<_key_t, _value_t>::iterator x = _cache.find(_key_t(win, prop));
		if(x != _cache.end()) {
			if(x->second.data != 0) {
				XFree(x->second.data);
			}

//			printf("cache remove key(%d,%d) = (%d,%d,%d,%d)\n", x->first.first,
//					x->first.second, x->second.type, x->second.format,
//					x->second.nitems, x->second.data);
//			fflush(stdout);
			_cache.erase(x);

		}
	}

	void erase(Window win) {
		map<_key_t, _value_t>::iterator x = _cache.begin();
		while(x != _cache.end()) {
			if (x->first.first == win) {
				if (x->second.data != 0) {
					XFree(x->second.data);
				}
//				printf("cache remove key(%d,%d) = (%d,%d,%d,%d)\n",
//						x->first.first, x->first.second, x->second.type,
//						x->second.format, x->second.nitems, x->second.data);
//				fflush(stdout);
				_cache.erase(x++);
			} else {
				++x;
			}
		}
	}

	unsigned int miss() {
		return _miss;
	}

	unsigned int hit() {
		return _hit;
	}

};

}


#endif /* PROPERTIES_CACHE_HXX_ */
