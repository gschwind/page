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

namespace page {

typedef long card32;

/**
 * client_base_t handle all foreign windows, it's the base of
 * client_managed_t and client_not_managed_t.
 **/
class client_base_t : public tree_t {
protected:

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
		_properties{c._properties},
		_title{c._title},
		_children{c._children},
		_parent{nullptr},
		_is_hidden{true}
	{

	}

	client_base_t(std::shared_ptr<client_properties_t> props) :
		_properties(props),
		_children(),
		_title()
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

	std::vector<tree_t *> childs() const {
		std::vector<tree_t *> ret(_children.begin(), _children.end());
		return ret;
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

	void get_all_children(std::vector<tree_t *> & out) const {
		for(auto x: _children) {
			out.push_back(x);
			x->get_all_children(out);
		}
	}
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

	void raise_child(tree_t * t = nullptr) {

		/** raise ourself **/
		if(t == nullptr) {
			if(_parent != nullptr) {
				_parent->raise_child(this);
			}

			return;
		}

		/** only client_base_t can be child of client_base_t **/
		client_base_t * c = dynamic_cast<client_base_t *>(t);
		if(has_key(_children, c) and c != nullptr) {

			/** raise the child **/
			_children.remove(c);
			_children.push_back(c);

			/** raise ourself **/
			if(_parent != nullptr) {
				_parent->raise_child(this);
			}

			return;
		}

		/** something is wrong with the code if we reach this line **/
		throw exception_t("client_base::raise_child trying to raise a non child tree");

	}

};

}

#endif /* CLIENT_HXX_ */
