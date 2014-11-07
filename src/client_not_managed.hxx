/*
 * unmanaged_window.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef CLIENT_NOT_MANAGED_HXX_
#define CLIENT_NOT_MANAGED_HXX_

#include <X11/Xlib.h>

#include <memory>


#include "client_properties.hxx"
#include "client_base.hxx"
#include "display.hxx"

#include "renderable.hxx"
#include "composite_surface_manager.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "renderable_unmanaged_outer_gradien.hxx"
#include "renderable_pixmap.hxx"

namespace page {

class client_not_managed_t : public client_base_t {
private:

	static unsigned long const UNMANAGED_ORIG_WINDOW_EVENT_MASK =
	XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE;

	xcb_atom_t _net_wm_type;

	std::shared_ptr<composite_surface_t> surf;

	mutable i_rect _base_position;

	/* avoid copy */
	client_not_managed_t(client_not_managed_t const &);
	client_not_managed_t & operator=(client_not_managed_t const &);

public:

	client_not_managed_t(xcb_atom_t type, std::shared_ptr<client_properties_t> c, std::shared_ptr<composite_surface_t> surf) :
			client_base_t(c),
			_net_wm_type(type),
			surf(surf)
	{
		_is_hidden = false;
		if (cnx()->lock(orig())) {
			cnx()->select_input(orig(), UNMANAGED_ORIG_WINDOW_EVENT_MASK);
			xcb_shape_select_input(cnx()->xcb(), orig(), true);
			cnx()->unlock();
		}
	}

	~client_not_managed_t() {
		cnx()->select_input(_properties->id(), XCB_EVENT_MASK_NO_EVENT);
	}

	xcb_atom_t net_wm_type() {
		return _net_wm_type;
	}

	bool has_window(xcb_window_t w) const {
		return w == _properties->id();
	}

	void map() {
		_properties->cnx()->map(_properties->id());
	}

	void unmap() {
		_properties->cnx()->unmap(_properties->id());
	}

	virtual std::string get_node_name() const {
		std::string s = _get_node_name<'U'>();
		std::ostringstream oss;
		oss << s << " " << orig();
		return oss.str();
	}

	virtual void prepare_render(std::vector<std::shared_ptr<renderable_t>> & out, page::time_t const & time) {

		i_rect pos(_properties->geometry()->x, _properties->geometry()->y,
				_properties->geometry()->width, _properties->geometry()->height);

		xcb_atom_t t = _properties->wm_type();
		if (t == A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)
				or t == A(_NET_WM_WINDOW_TYPE_MENU)
				or t == A(_NET_WM_WINDOW_TYPE_POPUP_MENU)) {
			auto x = new renderable_unmanaged_outer_gradien_t{pos, 4};
			out += std::shared_ptr<renderable_t>{x};
		}

		if (surf != nullptr) {
			out += get_base_renderable();
		}

		for(auto i: _children) {
			i->prepare_render(out, time);
		}

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

	std::shared_ptr<renderable_t> get_base_renderable() {
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
			return std::shared_ptr<renderable_t>{x};

		} else {
			/* return null */
			return std::shared_ptr<renderable_t>{};
		}
	}

	xcb_window_t base() const {
		return _properties->id();
	}

	xcb_window_t orig() const {
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

	void get_visible_children(std::vector<tree_t *> & out) {
		out.push_back(this);
		for (auto i : tree_t::children()) {
			i->get_visible_children(out);
		}
	}

};

}

#endif /* CLIENT_NOT_MANAGED_HXX_ */
