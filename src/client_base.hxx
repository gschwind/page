/*
 * client.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 * client_base store/cache all client windows properties define in ICCCM or EWMH.
 *
 * most of them are store with pointer which is null if the properties is not set on client.
 *
 */

#ifndef CLIENT_BASE_HXX_
#define CLIENT_BASE_HXX_

#include <cassert>
#include <memory>

#include "utils.hxx"
#include "region.hxx"

#include "atoms.hxx"
#include "exception.hxx"

#include "client_properties.hxx"
#include "display.hxx"

#include "tree.hxx"
#include "page_context.hxx"

namespace page {

typedef long card32;

/**
 * client_base_t handle all foreign windows, it's the base of
 * client_managed_t and client_not_managed_t.
 **/
class client_base_t : public tree_t {
protected:
	page_context_t * _ctx;
	tree_t * _parent;

	/* handle properties of client */
	std::shared_ptr<client_properties_t> _properties;

	/* sub-clients */
	std::list<client_base_t *> _children;

	// window title cache
	std::string _title;

	bool _is_hidden;

	/** short cut **/
	xcb_atom_t A(atom_e atom) {
		return _properties->cnx()->A(atom);
	}

public:

	client_base_t(client_base_t const & c) :
		_ctx{c._ctx},
		_properties{c._properties},
		_title{c._title},
		_children{c._children},
		_parent{nullptr},
		_is_hidden{true}
	{

	}

	client_base_t(page_context_t * ctx, std::shared_ptr<client_properties_t> props) :
		_ctx{ctx},
		_properties{props},
		_children{},
		_title{}
	{
		update_title();
	}

	virtual ~client_base_t() { }

	void read_all_properties() {
		_properties->read_all_properties();
		update_title();
	}

	bool read_window_attributes() {
		return _properties->read_window_attributes();
	}

	void update_wm_name() {
		_properties->update_wm_name();
	}

	void update_wm_icon_name() {
		_properties->update_wm_icon_name();
	}

	void update_wm_normal_hints() {
		_properties->update_wm_normal_hints();
	}

	void update_wm_hints() {
		_properties->update_wm_hints();
	}

	void update_wm_class() {
		_properties->update_wm_class();
	}

	void update_wm_transient_for() {
		_properties->update_wm_transient_for();
	}

	void update_wm_protocols() {
		_properties->update_wm_protocols();
	}

	void update_wm_colormap_windows() {
		_properties->update_wm_colormap_windows();
	}

	void update_wm_client_machine() {
		_properties->update_wm_client_machine();
	}

	void update_wm_state() {
		_properties->update_wm_state();
	}

	/* EWMH */

	void update_net_wm_name() {
		_properties->update_net_wm_name();
	}

	void update_net_wm_visible_name() {
		_properties->update_net_wm_visible_name();
	}

	void update_net_wm_icon_name() {
		_properties->update_net_wm_icon_name();
	}

	void update_net_wm_visible_icon_name() {
		_properties->update_net_wm_visible_icon_name();
	}

	void update_net_wm_desktop() {
		_properties->update_net_wm_desktop();
	}

	void update_net_wm_window_type() {
		_properties->update_net_wm_window_type();
	}

	void update_net_wm_state() {
		_properties->update_net_wm_state();
	}

	void update_net_wm_allowed_actions() {
		_properties->update_net_wm_allowed_actions();
	}

	void update_net_wm_struct() {
		_properties->update_net_wm_struct();
	}

	void update_net_wm_struct_partial() {
		_properties->update_net_wm_struct_partial();
	}

	void update_net_wm_icon_geometry() {
		_properties->update_net_wm_icon_geometry();
	}

	void update_net_wm_icon() {
		_properties->update_net_wm_icon();
	}

	void update_net_wm_pid() {
		_properties->update_net_wm_pid();
	}

	void update_net_wm_handled_icons();

	void update_net_wm_user_time() {
		_properties->update_net_wm_user_time();
	}

	void update_net_wm_user_time_window() {
		_properties->update_net_wm_user_time_window();
	}

	void update_net_frame_extents() {
		_properties->update_net_frame_extents();
	}

	void update_net_wm_opaque_region() {
		_properties->update_net_wm_opaque_region();
	}

	void update_net_wm_bypass_compositor() {
		_properties->update_net_wm_bypass_compositor();
	}

	void update_motif_hints() {
		_properties->update_motif_hints();
	}

	void update_shape() {
		_properties->update_shape();
	}


	bool has_motif_border() {
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

	void set_net_wm_desktop(unsigned long n) {
		_properties->set_net_wm_desktop(n);
	}

public:

	void update_title() {
			std::string name;
			if (_properties->net_wm_name() != nullptr) {
				_title = *(_properties->net_wm_name());
			} else if (_properties->wm_name() != nullptr) {
				_title = *(_properties->wm_name());
			} else {
				std::stringstream s(std::stringstream::in | std::stringstream::out);
				s << "#" << (_properties->id()) << " (noname)";
				_title = s.str();
			}
	}

	void add_subclient(client_base_t * s) {
		assert(s != nullptr);
		_children.push_back(s);
		s->set_parent(this);
		if(_is_hidden) {
			s->hide();
		} else {
			s->show();
		}
	}

	void remove_subclient(client_base_t * s) {
		_children.remove(s);
		s->set_parent(nullptr);
	}

	std::string const & title() const {
		return _title;
	}

	bool is_window(xcb_window_t w) {
		return w == _properties->id();
	}

	virtual bool has_window(xcb_window_t w) const = 0;
	virtual xcb_window_t base() const = 0;
	virtual xcb_window_t orig() const = 0;
	virtual i_rect const & base_position() const = 0;
	virtual i_rect const & orig_position() const = 0;
	virtual region visible_area() const = 0;

	virtual std::string get_node_name() const {
		return _get_node_name<'c'>();
	}

	virtual void replace(tree_t * src, tree_t * by) {
		printf("Unexpected use of client_base_t::replace\n");
	}

	xcb_atom_t wm_type() {
		return _properties->wm_type();
	}

	void print_window_attributes() {
		_properties->print_window_attributes();
	}

	void print_properties() {
		_properties->print_properties();
	}

	void process_event(xcb_configure_notify_event_t const * e) {
		_properties->process_event(e);
	}

	auto cnx() const -> display_t * { return _properties->cnx(); }

	/* window attributes */
	auto wa() const -> xcb_get_window_attributes_reply_t const * { return _properties->wa(); }

	/* window geometry */
	auto geometry() const -> xcb_get_geometry_reply_t const * { return _properties->geometry(); }

	/* ICCCM (read-only properties for WM) */
	auto wm_name() const -> std::string const * { return _properties->wm_name(); }
	auto wm_icon_name() const -> std::string const * { return _properties->wm_icon_name(); };
	auto wm_normal_hints() const -> XSizeHints const * { return _properties->wm_normal_hints(); }
	auto wm_hints() const -> XWMHints const * { return _properties->wm_hints(); }
	auto wm_class() const -> std::vector<std::string> const * { return _properties->wm_class(); }
	auto wm_transient_for() const -> xcb_window_t const * { return _properties->wm_transient_for(); }
	auto wm_protocols() const -> std::list<xcb_atom_t> const * { return _properties->wm_protocols(); }
	auto wm_colormap_windows() const -> std::vector<xcb_window_t> const * { return _properties->wm_colormap_windows(); }
	auto wm_client_machine() const -> std::string const * { return _properties->wm_client_machine(); }

	/* ICCCM (read-write properties for WM) */
	auto wm_state() const -> wm_state_data_t const * {return _properties->wm_state(); }

	/* EWMH (read-only properties for WM) */
	auto net_wm_name() const -> std::string const * { return _properties->net_wm_name(); }
	auto net_wm_visible_name() const -> std::string const * { return _properties->net_wm_visible_name(); }
	auto net_wm_icon_name() const -> std::string const * { return _properties->net_wm_icon_name(); }
	auto net_wm_visible_icon_name() const -> std::string const * { return _properties->net_wm_visible_icon_name(); }
	auto net_wm_window_type() const -> std::list<xcb_atom_t> const * { return _properties->net_wm_window_type(); }
	auto net_wm_allowed_actions() const -> std::list<xcb_atom_t> const * { return _properties->net_wm_allowed_actions(); }
	auto net_wm_strut() const -> std::vector<int> const * { return _properties->net_wm_strut(); }
	auto net_wm_strut_partial() const -> std::vector<int> const * { return _properties->net_wm_strut_partial(); }
	auto net_wm_icon_geometry() const -> std::vector<int> const * { return _properties->net_wm_icon_geometry(); }
	auto net_wm_icon() const -> std::vector<uint32_t> const * { return _properties->net_wm_icon(); }
	auto net_wm_pid() const -> unsigned int const * { return _properties->net_wm_pid(); }
	auto net_wm_handled_icons() const -> bool;// { return _properties->net_wm_handled_icons(); }
	auto net_wm_user_time() const -> uint32_t const * { return _properties->net_wm_user_time(); }
	auto net_wm_user_time_window() const -> xcb_window_t const * { return _properties->net_wm_user_time_window(); }
	auto net_wm_opaque_region() const -> std::vector<int> const * { return _properties->net_wm_opaque_region(); }
	auto net_wm_bypass_compositor() const -> unsigned int const * { return _properties->net_wm_bypass_compositor(); }

	/* EWMH (read-write properties for WM) */
	auto net_wm_desktop() const -> unsigned int const * { return _properties->net_wm_desktop(); }
	auto net_wm_state() const -> std::list<xcb_atom_t> const * { return _properties->net_wm_state(); }
	auto net_frame_extents() const -> std::vector<int> const * { return _properties->net_frame_extents(); }


	/* OTHERs */
	auto motif_hints() const -> motif_wm_hints_t const * { return _properties->motif_hints(); }
	auto shape() const -> region const * { return _properties->shape(); }
	auto position() -> i_rect { return _properties->position(); }


	void on_property_notify(xcb_property_notify_event_t const * e) {
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

	/**
	 * tree_t interface implementation
	 **/

	void set_parent(tree_t * t) {
		_parent = t;
	}

	tree_t * parent() const {
		return _parent;
	}

	void remove(tree_t * t) {
		client_base_t * c = dynamic_cast<client_base_t *>(t);
		if(c == nullptr)
			return;
		_children.remove(c);
	}

//	void get_all_children(std::vector<tree_t *> & out) const {
//		for(auto x: _children) {
//			out.push_back(x);
//			x->get_all_children(out);
//		}
//	}

	void children(std::vector<tree_t *> & out) const {
		out.insert(out.end(), _children.begin(), _children.end());
	}

	void hide() {
		_is_hidden = true;
		for(auto i: tree_t::children()) {
			i->hide();
		}
	}

	void show() {
		_is_hidden = false;
		for(auto i: tree_t::children()) {
			i->show();
		}
	}

	void get_visible_children(std::vector<tree_t *> & out) {
		if (not _is_hidden) {
			out.push_back(this);
			for (auto i : tree_t::children()) {
				i->get_visible_children(out);
			}
		}
	}

	void activate(tree_t * t = nullptr) {
		/** raise ourself **/
		if(_parent != nullptr) {
			_parent->activate(this);
		}

		/** only client_base_t can be child of client_base_t **/
		client_base_t * c = dynamic_cast<client_base_t *>(t);
		if(has_key(_children, c) and c != nullptr) {
			/** raise the child **/
			_children.remove(c);
			_children.push_back(c);
		}

	}

	/* find the bigger window that is smaller than w and h */
	dimention_t<unsigned> compute_size_with_constrain(unsigned w, unsigned h) {

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

	xcb_window_t get_window() {
		return _properties->id();
	}

};

}

#endif /* CLIENT_HXX_ */
