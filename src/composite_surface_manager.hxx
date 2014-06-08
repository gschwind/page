/*
 * composite_surface_manager.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 *
 * Composite manager manage composite surfaces. Composite surface may be handled after the destruction of
 * the relate window. This manager keep those surfaces up-to-date and enable the access after the destruction
 * of the related window.
 *
 * This manager track the number of use of a given surface using a smart pointer like structure, when
 * all client release the handle the surface is destroyed.
 *
 * Why not simply use smart pointer ?
 *
 * Because you may want recover a released handler while it hasn't been released yet.
 *
 * For example:
 *
 * Client 1 handle (display, window N)
 * Client 2 handle (display, window N)
 * Client 2 release (display, window N)
 * Client 2 request new handle to (display, window N)
 *
 * The manager will give back (display, window N) that Client 1 also handle.
 *
 */

#ifndef COMPOSITE_SURFACE_MANAGER_HXX_
#define COMPOSITE_SURFACE_MANAGER_HXX_

#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <cairo/cairo-xlib.h>
#include <map>
#include <iostream>

using namespace std;

namespace page {

class composite_surface_manager_t {

	typedef pair<Display *, Window> _key_t;

	class _composite_surface_t {

		Display * _dpy;
		Visual * _vis;
		Window _window_id;
		Pixmap _pixmap_id;
		cairo_surface_t * _surf;
		int _width, _height;

		composite_surface_manager_t * _mngr;
		int _nref;

		_composite_surface_t(composite_surface_manager_t * mngr, Display * dpy, Window w) {
			XWindowAttributes wa;
			XGetWindowAttributes(dpy, w, &wa);

			_window_id = w;
			_dpy = dpy;
			_vis = wa.visual;
			_width = wa.width;
			_height = wa.height;
			_surf = nullptr;
			_pixmap_id = None;

			_nref = 0;
			_mngr = mngr;

			cout << "create : (" << _dpy << "," << _window_id << ") nref = " << _nref << endl;
		}

		~_composite_surface_t() {
			cout << "destroy : (" << _dpy << "," << _window_id << ") nref = " << _nref << endl;
			destroy_cache();
		}

		void destroy_cache() {
			if (_surf != nullptr) {
				cairo_surface_destroy(_surf);
				_surf = nullptr;
			}

			if(_pixmap_id != None) {
				XFreePixmap(_dpy, _pixmap_id);
				_pixmap_id = None;
			}
		}

		_key_t get_key() {
			return _key_t(_dpy, _window_id);
		}

		void decr_ref() {
			_nref -= 1;
			cout << "delete : (" << _dpy << "," << _window_id << ") nref = " << _nref << endl;
			if(_nref == 0) {
				_mngr->erase(this);
			}
		}

		void incr_ref() {
			_nref += 1;
			cout << "copy : (" << _dpy << "," << _window_id << ") nref = " << _nref << endl;
		}

	public:
		void onmap() {
			if(_pixmap_id != None) {
				XFreePixmap(_dpy, _pixmap_id);
				_pixmap_id = None;
			}
			_pixmap_id = XCompositeNameWindowPixmap(_dpy, _window_id);

		}

		void onresize(int width, int height) {
			if (width != _width or height != _height) {
				_width = width;
				_height = height;
				onmap();
			}
		}

		Window wid() {
			return _window_id;
		}

		cairo_surface_t * get_surf() {
			if (_surf != nullptr) {
				cairo_surface_destroy(_surf);
				_surf = nullptr;
			}
			_surf = cairo_xlib_surface_create(_dpy, _pixmap_id, _vis, _width, _height);
			return _surf;
		}

		friend composite_surface_manager_t;

	};

	typedef map<_key_t, _composite_surface_t *> _data_map_t;
	typedef typename _data_map_t::iterator _map_iter_t;

	_data_map_t _data;

	void erase(_composite_surface_t * c) {
		cout << "call " << __PRETTY_FUNCTION__ << endl;
		cout << "count = " << _data.size() << endl;
		_key_t key = c->get_key();
		delete c;
		_data.erase(key);
		cout << "count = " << _data.size() << endl;
		cout << "exit " << __PRETTY_FUNCTION__ << endl;
	}

public:
	class ptr_t {
		_composite_surface_t * v;
		ptr_t(_composite_surface_t * x) {
			cout << "call " << __PRETTY_FUNCTION__ << endl;
			if(x != nullptr)
				x->incr_ref();
			v = x;
			cout << "exit " << __PRETTY_FUNCTION__ << endl;
		}

	public:
		ptr_t() {
			cout << "call " << __PRETTY_FUNCTION__ << endl;
			v = nullptr;
			cout << "exit " << __PRETTY_FUNCTION__ << endl;
		}

		ptr_t(ptr_t const & p) {
			cout << "call " << __PRETTY_FUNCTION__ << endl;
			if (p.v != nullptr)
				p.v->incr_ref();
			v = p.v;
			cout << "exit " << __PRETTY_FUNCTION__ << endl;
		}

		ptr_t & operator= (ptr_t const & p) {
			cout << "call " << __PRETTY_FUNCTION__ << endl;
			if(&p == this) {
				cout << "exit " << __PRETTY_FUNCTION__ << endl;
				return *this;
			}

			if(v != nullptr) {
				v->decr_ref();
			}

			if(p.v != nullptr) {
				p.v->incr_ref();
			}

			v = p.v;

			cout << "exit " << __PRETTY_FUNCTION__ << endl;
			return *this;

		}

		~ptr_t() {
			cout << "call " << __PRETTY_FUNCTION__ << endl;
			if(v != nullptr)
				v->decr_ref();
			cout << "exit " << __PRETTY_FUNCTION__ << endl;
		}

		_composite_surface_t & operator*() {
			return *(v);
		}

		_composite_surface_t * operator->() {
			return v;
		}

		operator _composite_surface_t *() {
			cout << "call " << __PRETTY_FUNCTION__ << endl;
			cout << "exit " << __PRETTY_FUNCTION__ << endl;
			return v;
		}

		bool operator== (_composite_surface_t * p) {
			return p == v;
		}

		friend composite_surface_manager_t;

	};

private:
	ptr_t _get_composite_surface(Display * dpy, Window w) {
		_key_t key(dpy, w);
		_map_iter_t x = _data.find(key);
		if(x != _data.end()) {
			return ptr_t(x->second);
		} else {
			_composite_surface_t * h = new _composite_surface_t(this, dpy, w);
			_data[key] = h;
			return ptr_t(_data[key]);
		}
	}

	bool _has_composite_surface(Display * dpy, Window w) {
		_key_t key(dpy, w);
		_map_iter_t x = _data.find(key);
		return x != _data.end();
	}

	void print_content() {
		for(auto &i: _data) {
			cout << "ITEMS : (" << i.first.first << "," << i.first.second << ") nref = " << i.second->_nref << endl;
		}
	}

	static composite_surface_manager_t mngr;

public:
	static ptr_t get_composite_surface(Display * dpy, Window w) {
		return composite_surface_manager_t::mngr._get_composite_surface(dpy, w);
	}

	static bool has_composite_surface(Display * dpy, Window w) {
		return composite_surface_manager_t::mngr._has_composite_surface(dpy, w);
	}

	~composite_surface_manager_t() {
		/* sanity check */
		if(not _data.empty()) {
			cout << "WARNING: composite_surface_manager is not empty while destroying." << endl;
			print_content();
		}
	}

};


typedef composite_surface_manager_t::ptr_t p_managed_composite_surface_t;

}



#endif /* COMPOSITE_SURFACE_MANAGER_HXX_ */
