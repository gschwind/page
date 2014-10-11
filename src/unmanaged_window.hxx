/*
 * unmanaged_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef UNMANAGED_WINDOW_HXX_
#define UNMANAGED_WINDOW_HXX_

#include <X11/X.h>

#include "renderable_pixmap.hxx"
#include "composite_surface_manager.hxx"
#include "client_base.hxx"
#include "display.hxx"
#include "renderable_unmanaged_outer_gradien.hxx"

namespace page {

class client_not_managed_t : public client_base_t {
private:

	static unsigned long const UNMANAGED_ORIG_WINDOW_EVENT_MASK =
	StructureNotifyMask | PropertyChangeMask;

	Atom _net_wm_type;

	shared_ptr<composite_surface_t> surf;

	mutable i_rect _base_position;

	/* avoid copy */
	client_not_managed_t(client_not_managed_t const &);
	client_not_managed_t & operator=(client_not_managed_t const &);

public:

	client_not_managed_t(Atom type, shared_ptr<client_properties_t> c) :
			client_base_t(c),
			_net_wm_type(type)
	{
		_properties->cnx()->grab();
		XSelectInput(_properties->cnx()->dpy(), _properties->id(),
				UNMANAGED_ORIG_WINDOW_EVENT_MASK);
		XShapeSelectInput(_properties->cnx()->dpy(), _properties->id(), ShapeNotifyMask);

		_properties->cnx()->ungrab();
		surf = composite_surface_manager_t::get(_properties->cnx()->dpy(),
				base());
		composite_surface_manager_t::onmap(_properties->cnx()->dpy(), base());
	}

	~client_not_managed_t() {
		XSelectInput(_properties->cnx()->dpy(), _properties->id(), NoEventMask);
	}

	Atom net_wm_type() {
		return _net_wm_type;
	}

	virtual bool has_window(Window w) {
		return w == _properties->id();
	}

	void map() {
		_properties->cnx()->map_window(_properties->id());
	}

	virtual string get_node_name() const {
		string s = _get_node_name<'U'>();
		ostringstream oss;
		oss << s << " " << orig();
		return oss.str();
	}

	virtual void prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time) {

		i_rect pos(_properties->geometry()->x, _properties->geometry()->y,
				_properties->geometry()->width, _properties->geometry()->height);

		Atom t = _properties->type();
		if (t == A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)
				or t == A(_NET_WM_WINDOW_TYPE_MENU)
				or t == A(_NET_WM_WINDOW_TYPE_POPUP_MENU)) {
			auto x = new renderable_unmanaged_outer_gradien_t(pos, 4.0);
			out += ptr<renderable_t>{x};
		}


		if (surf != nullptr) {
			out += get_base_renderable();
		}
		tree_t::_prepare_render(out, time);
	}

	virtual bool need_render(time_t time) {
		return false;
	}

	region visible_area() const {
		i_rect rec{base_position()};
		rec.x -= 4;
		rec.y -= 4;
		rec.w += 8;
		rec.h += 8;
		return rec;
	}

	ptr<renderable_t> get_base_renderable() {
		_base_position = _properties->position();

		if (surf != nullptr) {

			region vis;
			region opa;

			vis = _base_position;

			region shp;
			if (shape() != nullptr) {
				shp = *shape();
			} else {
				shp = i_rect{0, 0, _base_position.w, _base_position.h};
			}

			region xopac;
			if (net_wm_opaque_region() != nullptr) {
				xopac = region { *(net_wm_opaque_region()) };
			} else {
				if (geometry()->depth == 24) {
					xopac = i_rect{0, 0, _base_position.w, _base_position.h};
				}
			}

			opa = shp & xopac;
			opa.translate(_base_position.x,
					_base_position.y);

			region dmg { surf->get_damaged() };
			surf->clear_damaged();
			dmg.translate(_base_position.x, _base_position.y);
			auto x = new renderable_pixmap_t(surf->get_pixmap(),
					_base_position, dmg);
			x->set_opaque_region(opa);
			x->set_visible_region(vis);
			return ptr<renderable_t>{x};

		} else {
			/* return null */
			return ptr<renderable_t>{};
		}
	}

	virtual Window base() const {
		return _properties->id();
	}

	virtual Window orig() const {
		return _properties->id();
	}

	virtual i_rect const & base_position() const {
		_base_position = _properties->position();
		return _base_position;
	}

	virtual i_rect const & orig_position() const {
		_base_position = _properties->position();
		return _base_position;
	}

	virtual bool has_window(Window w) const {
		return w == _properties->id();
	}

};

}

#endif /* UNMANAGED_WINDOW_HXX_ */
