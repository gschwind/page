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
#include "renderable_unmanaged_gaussian_shadow.hxx"
#include "renderable_pixmap.hxx"

namespace page {

class client_not_managed_t : public client_base_t {
private:

	static unsigned long const UNMANAGED_ORIG_WINDOW_EVENT_MASK =
	XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE;

	xcb_atom_t _net_wm_type;

	mutable rect _base_position;

	renderable_unmanaged_gaussian_shadow_t<4> * _shadow;
	renderable_pixmap_t * _base_renderable;

	/* avoid copy */
	client_not_managed_t(client_not_managed_t const &);
	client_not_managed_t & operator=(client_not_managed_t const &);

public:

	client_not_managed_t(page_context_t * ctx, xcb_atom_t type, std::shared_ptr<client_properties_t> c) :
			client_base_t{ctx, c},
			_net_wm_type{type}
	{
		_is_hidden = false;
		if (cnx()->lock(orig())) {
			cnx()->select_input(orig(), UNMANAGED_ORIG_WINDOW_EVENT_MASK);
			xcb_shape_select_input(cnx()->xcb(), orig(), 1);
			_properties->update_shape();
			cnx()->unlock();
		}

		_ctx->csm()->register_window(orig());

	}

	~client_not_managed_t() {
		cnx()->select_input(_properties->id(), XCB_EVENT_MASK_NO_EVENT);
		_ctx->csm()->unregister_window(orig());

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
		oss << " " << cnx()->get_atom_name(_properties->wm_type()) << " ";

		if(_properties->net_wm_name() != nullptr) {
			oss << " " << *_properties->net_wm_name();
		}

		if(_properties->geometry() != nullptr) {
			oss << " " << _properties->geometry()->width << "x" << _properties->geometry()->height << "+" << _properties->geometry()->x << "+" << _properties->geometry()->y;
		}

		return oss.str();
	}

	virtual void update_layout(time64_t const time) {

		rect pos(_properties->geometry()->x, _properties->geometry()->y,
				_properties->geometry()->width, _properties->geometry()->height);

		if (_ctx->menu_drop_down_shadow()) {
			xcb_atom_t t = _properties->wm_type();
			if (t == A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU)
					or t == A(_NET_WM_WINDOW_TYPE_MENU)
					or t == A(_NET_WM_WINDOW_TYPE_POPUP_MENU)) {

				delete _shadow;
				_shadow = new renderable_unmanaged_gaussian_shadow_t<4> { pos,
						color_t { 0.0, 0.0, 0.0, 1.0 } };
			}
		}

		if (_ctx->csm()->get_last_pixmap(_properties->id()) != nullptr) {
			update_base_renderable();
		}
	}

	virtual bool need_render(time64_t time) {
		return false;
	}

	region visible_area() const {
		rect rec{base_position()};
		rec.x -= 4;
		rec.y -= 4;
		rec.w += 8;
		rec.h += 8;
		return rec;
	}

	void update_base_renderable() {
		_base_position = _properties->position();

		std::shared_ptr<pixmap_t> surf = _ctx->csm()->get_last_pixmap(_properties->id());
		if (surf != nullptr) {

			region vis;
			region opa;

			vis = rect { 0, 0, _base_position.w, _base_position.h };

			if (shape() != nullptr) {
				region shp;
				shp = *shape();
				vis = shp;
			} else {
				vis = rect { 0, 0, _base_position.w, _base_position.h };
			}

			region xopac;
			if (net_wm_opaque_region() != nullptr) {
				xopac = region { *(net_wm_opaque_region()) };
			} else {
				if (geometry()->depth == 24) {
					xopac = rect { 0, 0, _base_position.w, _base_position.h };
				}
			}

			vis.translate(_base_position.x, _base_position.y);

			xopac.translate(_base_position.x, _base_position.y);
			opa = vis & xopac;

			region dmg { _ctx->csm()->get_damaged(_properties->id()) };
			_ctx->csm()->clear_damaged(_properties->id());
			dmg.translate(_base_position.x, _base_position.y);
			auto x = new renderable_pixmap_t(surf,
					_base_position, dmg);
			x->set_opaque_region(opa);
			x->set_visible_region(vis);
			delete _base_renderable;
			_base_renderable = x;
		}
	}

	xcb_window_t base() const {
		return _properties->id();
	}

	xcb_window_t orig() const {
		return _properties->id();
	}

	virtual rect const & base_position() const {
		_base_position = _properties->position();
		return _base_position;
	}

	virtual rect const & orig_position() const {
		_base_position = _properties->position();
		return _base_position;
	}

	void get_visible_children(std::vector<tree_t *> & out) {
		out.push_back(this);
		for (auto i : tree_t::children()) {
			i->get_visible_children(out);
		}
	}

	void children(std::vector<tree_t *> & out) const {
		if(_shadow != nullptr)
			out.push_back(_shadow);
		if(_base_renderable != nullptr)
			out.push_back(_base_renderable);
		out.insert(out.end(), _children.begin(), _children.end());
	}

};

}

#endif /* CLIENT_NOT_MANAGED_HXX_ */
