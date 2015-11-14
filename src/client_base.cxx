/*
 * client_base.cxx
 *
 *  Created on: 5 août 2015
 *      Author: gschwind
 */

#include "client_proxy.hxx"

#include "client_base.hxx"

namespace page {

using namespace std;


/** short cut **/
xcb_atom_t client_base_t::A(atom_e atom) {
	return _client_proxy->cnx()->A(atom);
}

//client_base_t::client_base_t(client_base_t const & c) :
//	tree_t{},
//	_ctx{c._ctx},
//	_client_proxy{c._client_proxy}
//{
//
//}

client_base_t::client_base_t(page_context_t * ctx, xcb_window_t w) :
	tree_t{},
	_ctx{ctx},
	_client_proxy{ctx->dpy()->create_client_proxy(w)}
{

}

client_base_t::~client_base_t() {

}

void client_base_t::read_all_properties() {
	_client_proxy->read_all_properties();
}

void client_base_t::update_shape() {
	_client_proxy->update_shape();
}


bool client_base_t::has_motif_border() {
	if (_client_proxy->motif_hints() != nullptr) {
		if (_client_proxy->motif_hints()->flags & MWM_HINTS_DECORATIONS) {
			if (not (_client_proxy->motif_hints()->decorations & MWM_DECOR_BORDER)
					and not ((_client_proxy->motif_hints()->decorations & MWM_DECOR_ALL))) {
				return false;
			}
		}
	}
	return true;
}

void client_base_t::set_net_wm_desktop(unsigned long n) {
	_client_proxy->set_net_wm_desktop(n);
}

void client_base_t::add_subclient(shared_ptr<client_base_t> s) {
	assert(s != nullptr);
	_children.push_back(s);
	s->set_parent(this);
	if(_is_visible) {
		s->show();
	} else {
		s->hide();
	}
}

bool client_base_t::is_window(xcb_window_t w) {
	return w == _client_proxy->id();
}

string client_base_t::get_node_name() const {
	return _get_node_name<'c'>();
}

xcb_atom_t client_base_t::wm_type() {
	return _client_proxy->wm_type();
}

void client_base_t::print_window_attributes() {
	_client_proxy->print_window_attributes();
}

void client_base_t::print_properties() {
	_client_proxy->print_properties();
}

void client_base_t::process_event(xcb_configure_notify_event_t const * e) {
	_client_proxy->process_event(e);
}

auto client_base_t::cnx() const -> display_t * { return _client_proxy->cnx(); }

#define RO_PROPERTY(cxx_name, x11_name, x11_type, cxx_type) \
cxx_type const * client_base_t::cxx_name() { return _client_proxy->cxx_name(); } \
void client_base_t::update_##cxx_name() { _client_proxy->update_##cxx_name(); }

#define RW_PROPERTY(cxx_name, x11_name, x11_type, cxx_type) \
cxx_type const * client_base_t::cxx_name() { return _client_proxy->cxx_name(); } \
void client_base_t::update_##cxx_name() { _client_proxy->update_##cxx_name(); } \
cxx_type const * client_base_t::cxx_name(cxx_type * x) { return _client_proxy->cxx_name(x); }

#include "client_property_list.hxx"

#undef RO_PROPERTY
#undef RW_PROPERTY

auto client_base_t::shape() const -> region const * { return _client_proxy->shape(); }
auto client_base_t::position() -> rect { return _client_proxy->position(); }

void client_base_t::remove(shared_ptr<tree_t> t) {
	if(t == nullptr)
		return;
	auto c = dynamic_pointer_cast<client_base_t>(t);
	if(c == nullptr)
		return;
	_children.remove(c);
	c->clear_parent();
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


