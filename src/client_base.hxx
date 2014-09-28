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
#include <typeinfo>

#include <X11/X.h>

#include "client_properties.hxx"
#include "utils.hxx"
#include "motif_hints.hxx"
#include "display.hxx"
#include "tree.hxx"

namespace page {

typedef long card32;

using namespace std;

class client_base_t : public tree_t {
protected:

	/** handle properties of client */
	shared_ptr<client_properties_t> _properties;

	/** sub-clients **/
	list<tree_t *>               _children;

	// window title cache
	string                       _title;

	/** short cut **/
	Atom A(atom_e atom) {
		return _properties->cnx()->A(atom);
	}

public:

	client_base_t(client_base_t const & c) :
		_properties(c._properties),
		_title(c._title),
		_children(c._children)
	{

	}

	client_base_t(shared_ptr<client_properties_t> props) :
		_properties(props),
		_children(),
		_title()
	{
		update_title();
	}

	virtual ~client_base_t() {

	}

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
			string name;
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
		_children.push_back(s);
		s->set_parent(this);
	}

	void remove_subclient(client_base_t * s) {
		_children.remove(s);
		s->set_parent(nullptr);
	}

	string const & title() const {
		return _title;
	}

	bool is_window(Window w) {
		return w == _properties->id();
	}

	virtual bool has_window(Window w) const = 0;
	virtual Window base() const = 0;
	virtual Window orig() const = 0;
	virtual i_rect const & base_position() const = 0;
	virtual i_rect const & orig_position() const = 0;

	virtual string get_node_name() const {
		return _get_node_name<'c'>();
	}

	virtual void replace(tree_t * src, tree_t * by) {
		printf("Unexpected use of client_base_t::replace\n");
	}

	virtual list<tree_t *> childs() const {
		list<tree_t *> ret(_children.begin(), _children.end());
		return ret;
	}


	void raise_child(tree_t * t) {

		if(has_key(_children, t)) {
			_children.remove(t);
			_children.push_back(t);
		}

		if(_parent != nullptr) {
			_parent->raise_child(this);
		}

	}

	Atom type() {
		Atom type = None;

		list<Atom> net_wm_window_type;
		bool override_redirect = (_properties->wa().override_redirect == True)?true:false;

		if(_properties->net_wm_window_type() == nullptr) {
			/**
			 * Fallback from ICCCM.
			 **/

			if(!override_redirect) {
				/* Managed windows */
				if(_properties->wm_transient_for() == nullptr) {
					/**
					 * Extended ICCCM:
					 * _NET_WM_WINDOW_TYPE_NORMAL [...] Managed windows with neither
					 * _NET_WM_WINDOW_TYPE nor WM_TRANSIENT_FOR set MUST be taken
					 * as this type.
					 **/
					net_wm_window_type.push_back(A(_NET_WM_WINDOW_TYPE_NORMAL));
				} else {
					/**
					 * Extended ICCCM:
					 * _NET_WM_WINDOW_TYPE_DIALOG [...] If _NET_WM_WINDOW_TYPE is
					 * not set, then managed windows with WM_TRANSIENT_FOR set MUST
					 * be taken as this type.
					 **/
					net_wm_window_type.push_back(A(_NET_WM_WINDOW_TYPE_DIALOG));
				}

			} else {
				/**
				 * Override-redirected windows.
				 *
				 * Extended ICCCM:
				 * _NET_WM_WINDOW_TYPE_NORMAL [...] Override-redirect windows
				 * without _NET_WM_WINDOW_TYPE, must be taken as this type, whether
				 * or not they have WM_TRANSIENT_FOR set.
				 **/
				net_wm_window_type.push_back(A(_NET_WM_WINDOW_TYPE_NORMAL));
			}
		} else {
			net_wm_window_type = *(_properties->net_wm_window_type());
		}

		/* always fall back to normal */
		net_wm_window_type.push_back(A(_NET_WM_WINDOW_TYPE_NORMAL));

		/* TODO: make this ones */
		static set<Atom> known_type;
		if (known_type.size() == 0) {
			known_type.insert(A(_NET_CURRENT_DESKTOP));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_DESKTOP));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_DOCK));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_TOOLBAR));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_MENU));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_UTILITY));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_SPLASH));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_DIALOG));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_POPUP_MENU));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_TOOLTIP));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_NOTIFICATION));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_COMBO));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_DND));
			known_type.insert(A(_NET_WM_WINDOW_TYPE_NORMAL));
		}

		/** find the first known window type **/
		for (auto i : net_wm_window_type) {
			//printf("Check for %s\n", cnx->get_atom_name(*i).c_str());
			if (has_key(known_type, i)) {
				type = i;
				break;
			}
		}

		/** HACK FOR ECLIPSE **/
		{
			list<Atom> wm_state;
			if (_properties->wm_class() != nullptr
					and _properties->wm_state() != nullptr
					and type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
				if ((*(_properties->wm_class()))[0] == "Eclipse") {
					auto x = find(wm_state.begin(), wm_state.end(),
							A(_NET_WM_STATE_SKIP_TASKBAR));
					if (x != wm_state.end()) {
						type = A(_NET_WM_WINDOW_TYPE_DND);
					}
				}
			}
		}

		return type;

	}

	void remove(tree_t * t) {
		_children.remove(t);
	}

	void print_window_attributes() {
		_properties->print_window_attributes();
	}

	void print_properties() {
		_properties->print_properties();
	}

//	virtual void render(cairo_t * cr, time_t time) {
//		/* draw nothing */
//		cout << "call " << __FUNCTION__ << endl;
//	}

	void process_event(XConfigureEvent const & e) {
		_properties->process_event(e);
	}


	display_t *          cnx() const { return _properties->cnx(); }
	bool                 has_valid_window_attributes() const { return _properties->has_valid_window_attributes(); }

	XWindowAttributes const & wa() const { return _properties->wa(); }

	/* ICCCM */

	string const *                     wm_name() const { return _properties->wm_name(); }
	string const *                     wm_icon_name() const { return _properties->wm_icon_name(); };
	XSizeHints const *                 wm_normal_hints() const { return _properties->wm_normal_hints(); }
	XWMHints const *                   wm_hints() const { return _properties->wm_hints(); }
	vector<string> const *             wm_class() const { return _properties->wm_class(); }
	Window const *                     wm_transient_for() const { return _properties->wm_transient_for(); }
	list<Atom> const *                 wm_protocols() const { return _properties->wm_protocols(); }
	vector<Window> const *             wm_colormap_windows() const { return _properties->wm_colormap_windows(); }
	string const *                     wm_client_machine() const { return _properties->wm_client_machine(); }

	/* wm_state is writen by WM */
	card32 const *                     wm_state() const {return _properties->wm_state(); }

	/* EWMH */

	string const *                     net_wm_name() const { return _properties->net_wm_name(); }
	string const *                     net_wm_visible_name() const { return _properties->net_wm_visible_name(); }
	string const *                     net_wm_icon_name() const { return _properties->net_wm_icon_name(); }
	string const *                     net_wm_visible_icon_name() const { return _properties->net_wm_visible_icon_name(); }
	unsigned long const *              net_wm_desktop() const { return _properties->net_wm_desktop(); }
	list<Atom> const *                 net_wm_window_type() const { return _properties->net_wm_window_type(); }
	list<Atom> const *                 net_wm_state() const { return _properties->net_wm_state(); }
	list<Atom> const *                 net_wm_allowed_actions() const { return _properties->net_wm_allowed_actions(); }
	vector<card32> const *             net_wm_struct() const { return _properties->net_wm_struct(); }
	vector<card32> const *             net_wm_struct_partial() const { return _properties->net_wm_struct_partial(); }
	vector<card32> const *             net_wm_icon_geometry() const { return _properties->net_wm_icon_geometry(); }
	vector<card32> const *             net_wm_icon() const { return _properties->net_wm_icon(); }
	unsigned long const *              net_wm_pid() const { return _properties->net_wm_pid(); }
	bool                               net_wm_handled_icons() const { return _properties->net_wm_handled_icons(); }
	Time const *                       net_wm_user_time() const { return _properties->net_wm_user_time(); }
	Window const *                     net_wm_user_time_window() const { return _properties->net_wm_user_time_window(); }
	vector<card32> const *             net_frame_extents() const { return _properties->net_frame_extents(); }
	vector<card32> const *             net_wm_opaque_region() const { return _properties->net_wm_opaque_region(); }
	unsigned long const *              net_wm_bypass_compositor() const { return _properties->net_wm_bypass_compositor(); }

	/* OTHERs */
	motif_wm_hints_t const *           motif_hints() const { return _properties->motif_hints(); }

	region const *                     shape() const { return _properties->shape(); }

	i_rect                             position() { return _properties->position(); }

};

}

#endif /* CLIENT_HXX_ */
