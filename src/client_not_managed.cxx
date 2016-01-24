/*
 * unmanaged_window.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "display.hxx"
#include "client_proxy.hxx"
#include "client_not_managed.hxx"

namespace page {

using namespace std;

client_not_managed_t::client_not_managed_t(page_context_t * ctx, xcb_window_t w, xcb_atom_t type) :
		client_base_t{ctx, w},
		_net_wm_type{type}
{
	_is_visible = true;
	_client_proxy->select_input(UNMANAGED_ORIG_WINDOW_EVENT_MASK);
	_client_proxy->select_input_shape(true);
	_client_proxy->update_shape();
	_client_view = _ctx->create_view(w);
}

client_not_managed_t::~client_not_managed_t() {
	_ctx->add_global_damage(get_visible_region());
	_client_proxy->select_input(XCB_EVENT_MASK_NO_EVENT);
}

xcb_atom_t client_not_managed_t::net_wm_type() {
	return _net_wm_type;
}

bool client_not_managed_t::has_window(xcb_window_t w) const {
	return w == _client_proxy->id();
}

string client_not_managed_t::get_node_name() const {
	std::string s = _get_node_name<'U'>();
	std::ostringstream oss;
	oss << s << " " << orig();
	oss << " " << cnx()->get_atom_name(_client_proxy->wm_type()) << " ";

	if(_client_proxy->net_wm_name() != nullptr) {
		oss << " " << *_client_proxy->net_wm_name();
	}

	oss << " " << _client_proxy->geometry().width << "x" << _client_proxy->geometry().height << "+" << _client_proxy->geometry().x << "+" << _client_proxy->geometry().y;

	return oss.str();
}

void client_not_managed_t::update_layout(time64_t const time) {
	if(not _is_visible)
		return;

	_base_position = _client_proxy->position();

	_update_visible_region();
	_update_opaque_region();

	region dmg { _client_view->get_damaged() };
	dmg.translate(_base_position.x, _base_position.y);
	_damage_cache += dmg;
	_client_view->clear_damaged();

	rect pos(_client_proxy->geometry().x, _client_proxy->geometry().y,
			_client_proxy->geometry().width, _client_proxy->geometry().height);

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
		_opaque_region_cache = region{*(net_wm_opaque_region())};
	} else {
		if (_client_proxy->geometry().depth != 32) {
			_opaque_region_cache = rect{0, 0, _base_position.w, _base_position.h};
		}
	}

	if(shape() != nullptr) {
		_opaque_region_cache &= *shape();
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
	return _client_proxy->id();
}

xcb_window_t client_not_managed_t::orig() const {
	return _client_proxy->id();
}

rect const & client_not_managed_t::base_position() const {
	_base_position = _client_proxy->position();
	return _base_position;
}

rect const & client_not_managed_t::orig_position() const {
	_base_position = _client_proxy->position();
	return _base_position;
}

void client_not_managed_t::render(cairo_t * cr, region const & area) {
	auto pix = _client_view->get_pixmap();

	if (pix != nullptr) {
		cairo_save(cr);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_surface(cr, pix->get_cairo_surface(),
				_base_position.x, _base_position.y);
		region r = region{_base_position} & area;
		for (auto &i : r.rects()) {
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

	_ctx->add_global_damage(get_visible_region());
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

void client_not_managed_t::on_property_notify(xcb_property_notify_event_t const * e) {
	if (e->atom == A(_NET_WM_OPAQUE_REGION)) {
		_update_opaque_region();
	}
}

}
