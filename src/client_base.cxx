/*
 * client_base.cxx
 *
 *  Created on: 5 août 2015
 *      Author: gschwind
 */

#include "client_base.hxx"

namespace page {

using namespace std;


/** short cut **/
xcb_atom_t client_base_t::A(atom_e atom) {
	return _properties->cnx()->A(atom);
}

client_base_t::client_base_t(client_base_t const & c) :
	_ctx{c._ctx},
	_properties{c._properties},
	_children{c._children}
{

}

client_base_t::client_base_t(page_context_t * ctx, shared_ptr<client_proxy_t> props) :
	_ctx{ctx},
	_properties{props},
	_children{}
{

}

client_base_t::~client_base_t() { }

void client_base_t::read_all_properties() {
	_properties->read_all_properties();
}

bool client_base_t::read_window_attributes() {
	return _properties->read_window_attributes();
}

void client_base_t::update_wm_name() {
	_properties->update_wm_name();
}

void client_base_t::update_wm_icon_name() {
	_properties->update_wm_icon_name();
}

void client_base_t::update_wm_normal_hints() {
	_properties->update_wm_normal_hints();
}

void client_base_t::update_wm_hints() {
	_properties->update_wm_hints();
}

void client_base_t::update_wm_class() {
	_properties->update_wm_class();
}

void client_base_t::update_wm_transient_for() {
	_properties->update_wm_transient_for();
}

void client_base_t::update_wm_protocols() {
	_properties->update_wm_protocols();
}

void client_base_t::update_wm_colormap_windows() {
	_properties->update_wm_colormap_windows();
}

void client_base_t::update_wm_client_machine() {
	_properties->update_wm_client_machine();
}

void client_base_t::update_wm_state() {
	_properties->update_wm_state();
}

/* EWMH */

void client_base_t::update_net_wm_name() {
	_properties->update_net_wm_name();
}

void client_base_t::update_net_wm_visible_name() {
	_properties->update_net_wm_visible_name();
}

void client_base_t::update_net_wm_icon_name() {
	_properties->update_net_wm_icon_name();
}

void client_base_t::update_net_wm_visible_icon_name() {
	_properties->update_net_wm_visible_icon_name();
}

void client_base_t::update_net_wm_desktop() {
	_properties->update_net_wm_desktop();
}

void client_base_t::update_net_wm_window_type() {
	_properties->update_net_wm_window_type();
}

void client_base_t::update_net_wm_state() {
	_properties->update_net_wm_state();
}

void client_base_t::update_net_wm_allowed_actions() {
	_properties->update_net_wm_allowed_actions();
}

void client_base_t::update_net_wm_struct() {
	_properties->update_net_wm_struct();
}

void client_base_t::update_net_wm_struct_partial() {
	_properties->update_net_wm_struct_partial();
}

void client_base_t::update_net_wm_icon_geometry() {
	_properties->update_net_wm_icon_geometry();
}

void client_base_t::update_net_wm_icon() {
	_properties->update_net_wm_icon();
}

void client_base_t::update_net_wm_pid() {
	_properties->update_net_wm_pid();
}

void client_base_t::update_net_wm_handled_icons() {

}

void client_base_t::update_net_wm_user_time() {
	_properties->update_net_wm_user_time();
}

void client_base_t::update_net_wm_user_time_window() {
	_properties->update_net_wm_user_time_window();
}

void client_base_t::update_net_frame_extents() {
	_properties->update_net_frame_extents();
}

void client_base_t::update_net_wm_opaque_region() {
	_properties->update_net_wm_opaque_region();
}

void client_base_t::update_net_wm_bypass_compositor() {
	_properties->update_net_wm_bypass_compositor();
}

void client_base_t::update_motif_hints() {
	_properties->update_motif_hints();
}

void client_base_t::update_shape() {
	_properties->update_shape();
}


bool client_base_t::has_motif_border() {
	if (_properties->motif_hints() != nullptr) {
		if (_properties->motif_hints()->flags & MWM_HINTS_DECORATIONS) {
			if (not (_properties->motif_hints()->decorations & MWM_DECOR_BORDER)
					and not ((_properties->motif_hints()->decorations & MWM_DECOR_ALL))) {
				return false;
			}
		}
	}
	return true;
}

void client_base_t::set_net_wm_desktop(unsigned long n) {
	_properties->set_net_wm_desktop(n);
}

void client_base_t::add_subclient(shared_ptr<client_base_t> s) {
	assert(s != nullptr);
	_children.push_back(s);
	s->set_parent(shared_from_this());
	if(_is_visible) {
		s->show();
	} else {
		s->hide();
	}
}

bool client_base_t::is_window(xcb_window_t w) {
	return w == _properties->id();
}

string client_base_t::get_node_name() const {
	return _get_node_name<'c'>();
}

xcb_atom_t client_base_t::wm_type() {
	return _properties->wm_type();
}

void client_base_t::print_window_attributes() {
	_properties->print_window_attributes();
}

void client_base_t::print_properties() {
	_properties->print_properties();
}

void client_base_t::process_event(xcb_configure_notify_event_t const * e) {
	_properties->process_event(e);
}

auto client_base_t::cnx() const -> display_t * { return _properties->cnx(); }

/* window attributes */
auto client_base_t::wa() const -> xcb_get_window_attributes_reply_t const * { return _properties->wa(); }

/* window geometry */
auto client_base_t::geometry() const -> xcb_get_geometry_reply_t const * { return _properties->geometry(); }

/* ICCCM (read-only properties for WM) */
auto client_base_t::wm_name() const -> string const * { return _properties->wm_name(); }
auto client_base_t::wm_icon_name() const -> string const * { return _properties->wm_icon_name(); };
auto client_base_t::wm_normal_hints() const -> XSizeHints const * { return _properties->wm_normal_hints(); }
auto client_base_t::wm_hints() const -> XWMHints const * { return _properties->wm_hints(); }
auto client_base_t::wm_class() const -> vector<string> const * { return _properties->wm_class(); }
auto client_base_t::wm_transient_for() const -> xcb_window_t const * { return _properties->wm_transient_for(); }
auto client_base_t::wm_protocols() const -> list<xcb_atom_t> const * { return _properties->wm_protocols(); }
auto client_base_t::wm_colormap_windows() const -> vector<xcb_window_t> const * { return _properties->wm_colormap_windows(); }
auto client_base_t::wm_client_machine() const -> string const * { return _properties->wm_client_machine(); }

/* ICCCM (read-write properties for WM) */
auto client_base_t::wm_state() const -> wm_state_data_t const * {return _properties->wm_state(); }

/* EWMH (read-only properties for WM) */
auto client_base_t::net_wm_name() const -> string const * { return _properties->net_wm_name(); }
auto client_base_t::net_wm_visible_name() const -> string const * { return _properties->net_wm_visible_name(); }
auto client_base_t::net_wm_icon_name() const -> string const * { return _properties->net_wm_icon_name(); }
auto client_base_t::net_wm_visible_icon_name() const -> string const * { return _properties->net_wm_visible_icon_name(); }
auto client_base_t::net_wm_window_type() const -> list<xcb_atom_t> const * { return _properties->net_wm_window_type(); }
auto client_base_t::net_wm_allowed_actions() const -> list<xcb_atom_t> const * { return _properties->net_wm_allowed_actions(); }
auto client_base_t::net_wm_strut() const -> vector<int> const * { return _properties->net_wm_strut(); }
auto client_base_t::net_wm_strut_partial() const -> vector<int> const * { return _properties->net_wm_strut_partial(); }
auto client_base_t::net_wm_icon_geometry() const -> vector<int> const * { return _properties->net_wm_icon_geometry(); }
auto client_base_t::net_wm_icon() const -> vector<uint32_t> const * { return _properties->net_wm_icon(); }
auto client_base_t::net_wm_pid() const -> unsigned int const * { return _properties->net_wm_pid(); }
auto client_base_t::net_wm_handled_icons() const -> bool { throw exception_t("No implemented"); }// { return _properties->net_wm_handled_icons(); }
auto client_base_t::net_wm_user_time() const -> uint32_t const * { return _properties->net_wm_user_time(); }
auto client_base_t::net_wm_user_time_window() const -> xcb_window_t const * { return _properties->net_wm_user_time_window(); }
auto client_base_t::net_wm_opaque_region() const -> vector<int> const * { return _properties->net_wm_opaque_region(); }
auto client_base_t::net_wm_bypass_compositor() const -> unsigned int const * { return _properties->net_wm_bypass_compositor(); }

/* EWMH (read-write properties for WM) */
auto client_base_t::net_wm_desktop() const -> unsigned int const * { return _properties->net_wm_desktop(); }
auto client_base_t::net_wm_state() const -> list<xcb_atom_t> const * { return _properties->net_wm_state(); }
auto client_base_t::net_frame_extents() const -> vector<int> const * { return _properties->net_frame_extents(); }


/* OTHERs */
auto client_base_t::motif_hints() const -> motif_wm_hints_t const * { return _properties->motif_hints(); }
auto client_base_t::shape() const -> region const * { return _properties->shape(); }
auto client_base_t::position() -> rect { return _properties->position(); }


void client_base_t::on_property_notify(xcb_property_notify_event_t const * e) {
	if (e->atom == A(WM_NAME)) {
		update_wm_name();
	} else if (e->atom == A(WM_ICON_NAME)) {
		update_wm_icon_name();
	} else if (e->atom == A(WM_NORMAL_HINTS)) {
		update_wm_normal_hints();
	} else if (e->atom == A(WM_HINTS)) {
		update_wm_hints();
	} else if (e->atom == A(WM_CLASS)) {
		update_wm_class();
	} else if (e->atom == A(WM_TRANSIENT_FOR)) {
		update_wm_transient_for();
	} else if (e->atom == A(WM_PROTOCOLS)) {
		update_wm_protocols();
	} else if (e->atom == A(WM_COLORMAP_WINDOWS)) {
		update_wm_colormap_windows();
	} else if (e->atom == A(WM_CLIENT_MACHINE)) {
		update_wm_client_machine();
	} else if (e->atom == A(WM_STATE)) {
		update_wm_state();
	} else if (e->atom == A(_NET_WM_NAME)) {
		update_net_wm_name();
	} else if (e->atom == A(_NET_WM_VISIBLE_NAME)) {
		update_net_wm_visible_name();
	} else if (e->atom == A(_NET_WM_ICON_NAME)) {
		update_net_wm_icon_name();
	} else if (e->atom == A(_NET_WM_VISIBLE_ICON_NAME)) {
		update_net_wm_visible_icon_name();
	} else if (e->atom == A(_NET_WM_DESKTOP)) {
		update_net_wm_desktop();
	} else if (e->atom == A(_NET_WM_WINDOW_TYPE)) {
		update_net_wm_window_type();
	} else if (e->atom == A(_NET_WM_STATE)) {
		update_net_wm_state();
	} else if (e->atom == A(_NET_WM_ALLOWED_ACTIONS)) {
		update_net_wm_allowed_actions();
	} else if (e->atom == A(_NET_WM_STRUT)) {
		update_net_wm_struct();
	} else if (e->atom == A(_NET_WM_STRUT_PARTIAL)) {
		update_net_wm_struct_partial();
	} else if (e->atom == A(_NET_WM_ICON_GEOMETRY)) {
		update_net_wm_icon_geometry();
	} else if (e->atom == A(_NET_WM_ICON)) {
		update_net_wm_icon();
	} else if (e->atom == A(_NET_WM_PID)) {
		update_net_wm_pid();
	} else if (e->atom == A(_NET_WM_USER_TIME)) {
		update_net_wm_user_time();
	} else if (e->atom == A(_NET_WM_USER_TIME_WINDOW)) {
		update_net_wm_user_time_window();
	} else if (e->atom == A(_NET_FRAME_EXTENTS)) {
		update_net_frame_extents();
	} else if (e->atom == A(_NET_WM_OPAQUE_REGION)) {
		update_net_wm_opaque_region();
	} else if (e->atom == A(_NET_WM_BYPASS_COMPOSITOR)) {
		update_net_wm_bypass_compositor();
	} else if (e->atom == A(_MOTIF_WM_HINTS)) {
		update_motif_hints();
	}
}


void client_base_t::remove(shared_ptr<tree_t> t) {
	if(t == nullptr)
		return;
	auto c = dynamic_pointer_cast<client_base_t>(t);
	if(c == nullptr)
		return;
	_children.remove(c);
}

void client_base_t::append_children(vector<shared_ptr<tree_t>> & out) const {
	out.insert(out.end(), _children.begin(), _children.end());
}

void client_base_t::activate(shared_ptr<tree_t> t) {
	assert(t != nullptr);
	assert(has_key(_children, t));
	tree_t::activate();
	move_back(_children, t);
}

/* find the bigger window that is smaller than w and h */
dimention_t<unsigned> client_base_t::compute_size_with_constrain(unsigned w, unsigned h) {

	/* has no constrain */
	if (wm_normal_hints() == nullptr)
		return dimention_t<unsigned> { w, h };

	XSizeHints const * sh = wm_normal_hints();

	if (sh->flags & PMaxSize) {
		if ((int) w > sh->max_width)
			w = sh->max_width;
		if ((int) h > sh->max_height)
			h = sh->max_height;
	}

	if (sh->flags & PBaseSize) {
		if ((int) w < sh->base_width)
			w = sh->base_width;
		if ((int) h < sh->base_height)
			h = sh->base_height;
	} else if (sh->flags & PMinSize) {
		if ((int) w < sh->min_width)
			w = sh->min_width;
		if ((int) h < sh->min_height)
			h = sh->min_height;
	}

	if (sh->flags & PAspect) {
		if (sh->flags & PBaseSize) {
			/**
			 * ICCCM say if base is set subtract base before aspect checking
			 * reference: ICCCM
			 **/
			if ((w - sh->base_width) * sh->min_aspect.y
					< (h - sh->base_height) * sh->min_aspect.x) {
				/* reduce h */
				h = sh->base_height
						+ ((w - sh->base_width) * sh->min_aspect.y)
								/ sh->min_aspect.x;

			} else if ((w - sh->base_width) * sh->max_aspect.y
					> (h - sh->base_height) * sh->max_aspect.x) {
				/* reduce w */
				w = sh->base_width
						+ ((h - sh->base_height) * sh->max_aspect.x)
								/ sh->max_aspect.y;
			}
		} else {
			if (w * sh->min_aspect.y < h * sh->min_aspect.x) {
				/* reduce h */
				h = (w * sh->min_aspect.y) / sh->min_aspect.x;

			} else if (w * sh->max_aspect.y > h * sh->max_aspect.x) {
				/* reduce w */
				w = (h * sh->max_aspect.x) / sh->max_aspect.y;
			}
		}

	}

	if (sh->flags & PResizeInc) {
		w -= ((w - sh->base_width) % sh->width_inc);
		h -= ((h - sh->base_height) % sh->height_inc);
	}

	return dimention_t<unsigned> { w, h };

}

auto client_base_t::get_xid() const -> xcb_window_t {
	return base();
}

auto client_base_t::get_parent_xid() const -> xcb_window_t {
	return base();
}

}


