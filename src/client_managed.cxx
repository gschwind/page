/*
 * managed_window.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "leak_checker.hxx"

#include <cairo.h>
#include <cairo-xlib.h>
#include <cairo-xcb.h>

#include "page.hxx"
#include "renderable_floating_outer_gradien.hxx"
#include "client_managed.hxx"
#include "notebook.hxx"
#include "utils.hxx"
#include "grab_handlers.hxx"

namespace page {

using namespace std;

client_managed_t::client_managed_t(page_t * ctx, client_proxy_p proxy) :
	_ctx{ctx},
	_client_proxy{proxy},
	_managed_type{MANAGED_FLOATING},
	_net_wm_type{proxy->wm_type()},
	_floating_wished_position{},
	_absolute_position{},
	_icon(nullptr),
	_has_focus{false},
	_demands_attention{false},
	_current_owner_view{nullptr}
{

	_update_title();
	rect pos{_client_proxy->position()};

	//printf("window default position = %s\n", pos.to_string().c_str());

	if(_net_wm_type == A(_NET_WM_WINDOW_TYPE_DOCK))
		_managed_type = MANAGED_DOCK;

	_floating_wished_position = pos;
	_absolute_position = pos;

	_apply_floating_hints_constraint();

	if(not _client_proxy->wa().override_redirect)
		_client_proxy->add_to_save_set();
	//_client_proxy->set_border_width(0);
	_client_proxy->select_input(MANAGED_ORIG_WINDOW_EVENT_MASK);
	//_client_proxy->select_input_shape(true);

	update_icon();

}

client_managed_t::~client_managed_t() {

	on_destroy.signal(this);

	_client_proxy->delete_net_wm_state();
	_client_proxy->delete_wm_state();

}

void client_managed_t::fake_configure_unsafe(rect const & location) {
	//printf("fake_reconfigure = %dx%d+%d+%d\n", _wished_position.w,
	//		_wished_position.h, _wished_position.x, _wished_position.y);
	_client_proxy->fake_configure(location, 0);
}

void client_managed_t::delete_window(xcb_timestamp_t t) {
	printf("request close for '%s'\n", title().c_str());
	_client_proxy->delete_window(t);
}

void client_managed_t::set_managed_type(managed_window_type_e type) {
	if(_managed_type == MANAGED_DOCK) {
		std::list<atom_e> net_wm_allowed_actions;
		_client_proxy->net_wm_allowed_actions_set(net_wm_allowed_actions);
		//reconfigure();
	} else {

		std::list<atom_e> net_wm_allowed_actions;
		net_wm_allowed_actions.push_back(_NET_WM_ACTION_CLOSE);
		net_wm_allowed_actions.push_back(_NET_WM_ACTION_FULLSCREEN);
		_client_proxy->net_wm_allowed_actions_set(net_wm_allowed_actions);

		_managed_type = type;

		//reconfigure();
	}
}

void client_managed_t::focus(xcb_timestamp_t t) {
	icccm_focus_unsafe(t);
}

managed_window_type_e client_managed_t::get_type() {
	return _managed_type;
}

bool client_managed_t::is(managed_window_type_e type) {
	return _managed_type == type;
}

void client_managed_t::icccm_focus_unsafe(xcb_timestamp_t t) {

	if(_demands_attention) {
		_demands_attention = false;
		_client_proxy->net_wm_state_remove(_NET_WM_STATE_DEMANDS_ATTENTION);
	}

	if (_client_proxy->get<p_wm_protocols>() != nullptr) {
		if (has_key(*(_client_proxy->get<p_wm_protocols>()), A(WM_TAKE_FOCUS))) {

			xcb_client_message_event_t ev;
			ev.response_type = XCB_CLIENT_MESSAGE;
			ev.format = 32;
			ev.type = A(WM_PROTOCOLS);
			ev.window = _client_proxy->id();
			ev.data.data32[0] = A(WM_TAKE_FOCUS);
			ev.data.data32[1] = t;

			_client_proxy->send_event(0, XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<char*>(&ev));

			return;

		}
	}

	if (_client_proxy->get<p_wm_hints>() != nullptr) {
		if (_client_proxy->get<p_wm_hints>()->input != False) {
			_client_proxy->set_input_focus(XCB_INPUT_FOCUS_NONE, t);
		}
	} else {
		/** no WM_HINTS, guess hints.input == True **/
		_client_proxy->set_input_focus(XCB_INPUT_FOCUS_NONE, t);
	}

}


/**
 * find the workspace for the window, if undefined or invalid, atribute a new
 * one.
 **/
unsigned client_managed_t::ensure_workspace() {

	/* check current status */
	auto workspace = get<p_net_wm_desktop>();
	auto transient_for = get<p_wm_transient_for>();
	unsigned final_workspace;
	if (workspace != nullptr) {
		final_workspace = *workspace;
	} else if (has_wm_state_stiky()) {
		final_workspace = ALL_DESKTOP;
	} else if (transient_for != nullptr) {
		// TODO
//		auto parent = dynamic_pointer_cast<client_managed_t>(
//				_ctx->find_client_with(*transient_for));
//		if (parent != nullptr) {
//			final_workspace = _ctx->find_current_workspace(parent);
//		}
		final_workspace = _ctx->get_current_workspace()->id();
	} else {
		final_workspace = _ctx->get_current_workspace()->id();
	}

	if(final_workspace != ALL_DESKTOP
			and final_workspace >= _ctx->get_workspace_count()) {
		final_workspace = _ctx->get_current_workspace()->id();
	}

	if (workspace == nullptr) {
		set_net_wm_desktop(final_workspace);
	} else if (*workspace != final_workspace) {
		set_net_wm_desktop(final_workspace);
	}

	return final_workspace;

}

bool client_managed_t::skip_task_bar() {
	if (_client_proxy->get<p_net_wm_state>() != nullptr) {
		return has_key(*(_client_proxy->get<p_net_wm_state>()),
				static_cast<xcb_atom_t>(A(_NET_WM_STATE_SKIP_TASKBAR)));
	}
	return false;
}

xcb_atom_t client_managed_t::net_wm_type() {
	return _net_wm_type;
}

bool client_managed_t::get_wm_normal_hints(XSizeHints * size_hints) {
	if(_client_proxy->get<p_wm_normal_hints>() != nullptr) {
		*size_hints = *(_client_proxy->get<p_wm_normal_hints>());
		return true;
	} else {
		return false;
	}
}

void client_managed_t::net_wm_state_add(atom_e atom)
{
	_client_proxy->net_wm_state_add(atom);
}

void client_managed_t::net_wm_state_remove(atom_e atom)
{
	_client_proxy->net_wm_state_remove(atom);
}

void client_managed_t::net_wm_state_delete()
{
	/**
	 * This one is for removing the window manager tag, thus only check if the window
	 * still exist. (don't need lock);
	 **/
	_client_proxy->delete_net_wm_state();
}

void client_managed_t::wm_state_delete()
{
	/**
	 * This one is for removing the window manager tag, thus only check if the window
	 * still exist. (don't need lock);
	 **/

	_client_proxy->delete_wm_state();
}

bool client_managed_t::has_wm_state_fullscreen()
{
	auto const * wm_state = _client_proxy->get<p_net_wm_state>();
	if (wm_state != nullptr) {
		return has_key(*wm_state, A(_NET_WM_STATE_FULLSCREEN));
	}
	return false;
}

bool client_managed_t::has_wm_state_stiky()
{
	auto const * wm_state = _client_proxy->get<p_net_wm_state>();
	if (wm_state != nullptr) {
		return has_key(*wm_state, A(_NET_WM_STATE_STICKY));
	}
	return false;
}

bool client_managed_t::has_wm_state_modal()
{
	auto const * wm_state = _client_proxy->get<p_net_wm_state>();
	if (wm_state != nullptr) {
		return has_key(*wm_state, A(_NET_WM_STATE_MODAL));
	}
	return false;
}

void client_managed_t::set_floating_wished_position(rect const & pos) {
	_floating_wished_position = pos;
}

rect const & client_managed_t::get_wished_position() {
	return _absolute_position;
}

rect const & client_managed_t::get_floating_wished_position() {
	return _floating_wished_position;
}

display_t * client_managed_t::cnx() {
	return _client_proxy->cnx();
}

void client_managed_t::set_focus_state(bool is_focused) {
	_has_focus = is_focused;
	if (_has_focus) {
		net_wm_state_add(_NET_WM_STATE_FOCUSED);
	} else {
		net_wm_state_remove(_NET_WM_STATE_FOCUSED);
	}
	on_focus_change.signal(this);
}

void client_managed_t::net_wm_allowed_actions_add(atom_e atom) {
	_client_proxy->net_wm_allowed_actions_add(atom);
}

void client_managed_t::map_unsafe() {
	_client_proxy->xmap();

}

void client_managed_t::unmap_unsafe() {


	//_client_proxy->unmap();

}

void client_managed_t::_update_title() {
	string name;
	if (_client_proxy->get<p_net_wm_name>() != nullptr) {
		_title = *(_client_proxy->get<p_net_wm_name>());
	} else if (_client_proxy->get<p_wm_name>() != nullptr) {
		_title = *(_client_proxy->get<p_wm_name>());
	} else {
		stringstream s(stringstream::in | stringstream::out);
		s << "#" << (_client_proxy->id()) << " (noname)";
		_title = s.str();
	}

	if(false) {
		stringstream s(stringstream::in | stringstream::out);
		s << "(" << (_client_proxy->id()) << ") " << _title;
		_title = s.str();
	}
}

void client_managed_t::update_title() {
	_update_title();
	on_title_change.signal(this);
}

bool client_managed_t::prefer_window_border() const {
	if (_client_proxy->get<p_motif_hints>() != nullptr) {
		if(not (_client_proxy->get<p_motif_hints>()->flags & MWM_HINTS_DECORATIONS))
			return true;
		if(_client_proxy->get<p_motif_hints>()->decorations != 0x00)
			return true;
		return false;
	}
	return true;
}

auto client_managed_t::current_owner_view() const -> view_t *
{
	return _current_owner_view;
}

void client_managed_t::acquire(view_t * v)
{
	_current_owner_view = v;
}
void client_managed_t::release(view_t * v)
{
	assert(_current_owner_view == v);
	_current_owner_view = nullptr;
}

shared_ptr<icon16> client_managed_t::icon() const {
	return _icon;
}

void client_managed_t::update_icon() {
	_icon = make_shared<icon16>(this);
}

xcb_atom_t client_managed_t::A(atom_e atom) {
	return cnx()->A(atom);
}

void client_managed_t::set_current_workspace(unsigned int n) {
	_client_proxy->set_net_wm_desktop(n);
}

void client_managed_t::set_demands_attention(bool x) {
	if (x) {
		_client_proxy->net_wm_state_add(_NET_WM_STATE_DEMANDS_ATTENTION);
	} else {
		_client_proxy->net_wm_state_remove(_NET_WM_STATE_DEMANDS_ATTENTION);
	}
	_demands_attention = x;
}

bool client_managed_t::demands_attention() {
	return _demands_attention;
}

string const & client_managed_t::title() const {
	return _title;
}

void client_managed_t::on_property_notify(xcb_property_notify_event_t const * e) {
	if (e->atom == A(_NET_WM_NAME) or e->atom == A(WM_NAME)) {
		update_title();
	} else if (e->atom == A(_NET_WM_ICON)) {
		update_icon();
	} else if (e->atom == A(_NET_WM_OPAQUE_REGION)) {
		on_opaque_region_change.signal(this);
	} else if (e->atom == A(_NET_WM_STRUT_PARTIAL)) {
		on_strut_change.signal(this);
	} else if (e->atom == A(_NET_WM_STRUT)) {
		on_strut_change.signal(this);
	}
}

void client_managed_t::_apply_floating_hints_constraint() {

	if(_client_proxy->get<p_wm_normal_hints>()!= nullptr) {
		XSizeHints const * s = _client_proxy->get<p_wm_normal_hints>();

		if (s->flags & PBaseSize) {
			if (_floating_wished_position.w < s->base_width)
				_floating_wished_position.w = s->base_width;
			if (_floating_wished_position.h < s->base_height)
				_floating_wished_position.h = s->base_height;
		} else if (s->flags & PMinSize) {
			if (_floating_wished_position.w < s->min_width)
				_floating_wished_position.w = s->min_width;
			if (_floating_wished_position.h < s->min_height)
				_floating_wished_position.h = s->min_height;
		}

		if (s->flags & PMaxSize) {
			if (_floating_wished_position.w > s->max_width)
				_floating_wished_position.w = s->max_width;
			if (_floating_wished_position.h > s->max_height)
				_floating_wished_position.h = s->max_height;
		}

	}
}

void client_managed_t::read_all_properties() {
	_client_proxy->read_all_properties();
}

void client_managed_t::update_shape() {
	_client_proxy->update_shape();
}


bool client_managed_t::has_motif_border() {
	if (_client_proxy->get<p_motif_hints>() != nullptr) {
		if (_client_proxy->get<p_motif_hints>()->flags & MWM_HINTS_DECORATIONS) {
			if (not (_client_proxy->get<p_motif_hints>()->decorations & MWM_DECOR_BORDER)
					and not ((_client_proxy->get<p_motif_hints>()->decorations & MWM_DECOR_ALL))) {
				return false;
			}
		}
	}
	return true;
}

void client_managed_t::set_net_wm_desktop(unsigned long n) {
	_client_proxy->set_net_wm_desktop(n);
}

bool client_managed_t::is_window(xcb_window_t w) {
	return w == _client_proxy->id();
}

xcb_atom_t client_managed_t::wm_type() {
	return _client_proxy->wm_type();
}

void client_managed_t::print_window_attributes() {
	_client_proxy->print_window_attributes();
}

void client_managed_t::print_properties() {
	_client_proxy->print_properties();
}

void client_managed_t::process_event(xcb_configure_notify_event_t const * e) {
	_client_proxy->process_event(e);
	on_configure_notify.signal(this);
}

auto client_managed_t::cnx() const -> display_t * { return _client_proxy->cnx(); }

auto client_managed_t::transient_for() -> client_managed_p
{
	client_managed_p transient_for = nullptr;
	if (_client_proxy->get<p_wm_transient_for>() != nullptr) {
		transient_for = _ctx->find_client_with(*(_client_proxy->get<p_wm_transient_for>()));
		if (transient_for == nullptr)
			printf("Warning transient for an unknown client\n");
	}
	return transient_for;
}

auto client_managed_t::shape() const -> region const * { return _client_proxy->shape(); }
auto client_managed_t::position() -> rect { return _client_proxy->position(); }

/* find the bigger window that is smaller than w and h */
dimention_t<unsigned> client_managed_t::compute_size_with_constrain(unsigned w, unsigned h) {

	/* has no constrain */
	if (get<p_wm_normal_hints>() == nullptr)
		return dimention_t<unsigned> { w, h };

	XSizeHints const * sh = get<p_wm_normal_hints>();

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

}

