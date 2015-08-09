/*
 * unmanaged_window.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "display.hxx"
#include "client_not_managed.hxx"

namespace page {

using namespace std;

client_not_managed_t::client_not_managed_t(page_context_t * ctx, xcb_atom_t type, std::shared_ptr<client_properties_t> c) :
		client_base_t{ctx, c},
		_net_wm_type{type},
		_shadow{nullptr},
		_base_renderable{nullptr}
{
	_is_visible = true;
	if (cnx()->lock(orig())) {
		cnx()->select_input(orig(), UNMANAGED_ORIG_WINDOW_EVENT_MASK);
		xcb_shape_select_input(cnx()->xcb(), orig(), 1);
		_properties->update_shape();
		cnx()->unlock();
	}

	_ctx->csm()->register_window(orig());

}

client_not_managed_t::~client_not_managed_t() {
	cnx()->select_input(_properties->id(), XCB_EVENT_MASK_NO_EVENT);
	_ctx->csm()->unregister_window(orig());
	_ctx->add_global_damage(_visible_region_cache);

}

xcb_atom_t client_not_managed_t::net_wm_type() {
	return _net_wm_type;
}

bool client_not_managed_t::has_window(xcb_window_t w) const {
	return w == _properties->id();
}

string client_not_managed_t::get_node_name() const {
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

void client_not_managed_t::update_layout(time64_t const time) {
	if(not _is_visible)
		return;

	_base_position = _properties->position();

	_update_visible_region();
	_update_opaque_region();

	region dmg { _ctx->csm()->get_damaged(_properties->id()) };
	dmg.translate(_base_position.x, _base_position.y);
	_damage_cache += dmg;
	_ctx->csm()->clear_damaged(_properties->id());

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
}

region client_not_managed_t::get_visible_region() {
	return _visible_region_cache;
}

void client_not_managed_t::_update_visible_region() {
	/** update visible cache **/
	_visible_region_cache = region{_base_position};
}

void client_not_managed_t::_update_opaque_region() {
	/** update opaque region cache **/
	_opaque_region_cache.clear();

	if (net_wm_opaque_region() != nullptr) {
		_opaque_region_cache = region { *(net_wm_opaque_region()) };
	} else {
		if (geometry()->depth == 24) {
			_opaque_region_cache = rect{0, 0, _base_position.w, _base_position.h};
		}
	}
	_opaque_region_cache.translate(_base_position.x, _base_position.y);
}

region client_not_managed_t::get_opaque_region() {
	return _opaque_region_cache;
}

region client_not_managed_t::get_damaged() {
	return _damage_cache;
}

xcb_window_t client_not_managed_t::base() const {
	return _properties->id();
}

xcb_window_t client_not_managed_t::orig() const {
	return _properties->id();
}

rect const & client_not_managed_t::base_position() const {
	_base_position = _properties->position();
	return _base_position;
}

rect const & client_not_managed_t::orig_position() const {
	_base_position = _properties->position();
	return _base_position;
}

void client_not_managed_t::render(cairo_t * cr, region const & area) {
	auto pix = _ctx->csm()->get_last_pixmap(_properties->id());

	if (pix != nullptr) {
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(cr, pix->get_cairo_surface(),
				_base_position.x, _base_position.y);
		region r = region{_base_position} & area;
		for (auto &i : r) {
			cairo_clip(cr, i);
			cairo_mask_surface(cr, pix->get_cairo_surface(),
					_base_position.x, _base_position.y);
		}
		cairo_restore(cr);
	}

}

void client_not_managed_t::hide() {
	for(auto x: _children) {
		x->hide();
	}

	_is_visible = false;
	//unmap();
}

void client_not_managed_t::show() {
	_is_visible = true;
	//map();
	for(auto x: _children) {
		x->show();
	}
}

void client_not_managed_t::render_finished() {
	_damage_cache.clear();
}

}
