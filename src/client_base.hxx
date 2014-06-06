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

#include "utils.hxx"
#include "motif_hints.hxx"
#include "display.hxx"
#include "tree.hxx"

namespace page {

typedef long card32;

using namespace std;

class client_base_t : public tree_t {
public:
	display_t *              _cnx;
	Window                       _id;

	bool                         has_valid_window_attributes;
	XWindowAttributes            wa;

	/* ICCCM */

	string *                     wm_name;
	string *                     wm_icon_name;
	XSizeHints *                 wm_normal_hints;
	XWMHints *                   wm_hints;
	vector<string> *             wm_class;
	Window *                     wm_transient_for;
	list<Atom> *                 wm_protocols;
	vector<Window> *             wm_colormap_windows;
	string *                     wm_client_machine;

	/* wm_state is writen by WM */
	card32 *                     wm_state;

	/* EWMH */

	string *                     _net_wm_name;
	string *                     _net_wm_visible_name;
	string *                     _net_wm_icon_name;
	string *                     _net_wm_visible_icon_name;
	unsigned long *              _net_wm_desktop;
	list<Atom> *                 _net_wm_window_type;
	list<Atom> *                 _net_wm_state;
	list<Atom> *                 _net_wm_allowed_actions;
	vector<card32> *             _net_wm_struct;
	vector<card32> *             _net_wm_struct_partial;
	vector<card32> *             _net_wm_icon_geometry;
	vector<card32> *             _net_wm_icon;
	unsigned long *              _net_wm_pid;
	bool                         _net_wm_handled_icons;
	Time *                       _net_wm_user_time;
	Window *                     _net_wm_user_time_window;
	vector<card32> *             _net_frame_extents;
	vector<card32> *             _net_wm_opaque_region;
	unsigned long *              _net_wm_bypass_compositor;

	/* OTHERs */

	motif_wm_hints_t *           motif_hints;


	/** derived properties **/

	list<tree_t *>               _childen;

	// window title cache
	string                       _title;

	/** short cut **/
	Atom A(atom_e atom) {
		return _cnx->A(atom);
	}

public:

	client_base_t(client_base_t const & c) {

		has_valid_window_attributes = c.has_valid_window_attributes;
		memcpy(&wa, &c.wa, sizeof(XWindowAttributes));

		_id = c._id;
		_cnx = c._cnx;
		_title = c._title;
		_childen = c._childen;

		/* ICCCM */
		wm_name = safe_copy(c.wm_name);
		wm_icon_name = safe_copy(c.wm_icon_name);
		wm_normal_hints = safe_copy(c.wm_normal_hints);
		wm_hints = safe_copy(c.wm_hints);
		wm_class = safe_copy(c.wm_class);
		wm_transient_for = safe_copy(c.wm_transient_for);
		wm_protocols = safe_copy(c.wm_protocols);
		wm_colormap_windows = safe_copy(c.wm_colormap_windows);
		wm_client_machine = safe_copy(c.wm_client_machine);
		wm_state = safe_copy(c.wm_state);

		/* EWMH */
		_net_wm_name = safe_copy(c._net_wm_name);
		_net_wm_visible_name = safe_copy(c._net_wm_visible_name);
		_net_wm_icon_name = safe_copy(c._net_wm_icon_name);
		_net_wm_visible_icon_name = safe_copy(c._net_wm_visible_icon_name);
		_net_wm_desktop = safe_copy(c._net_wm_desktop);
		_net_wm_window_type = safe_copy(c._net_wm_window_type);
		_net_wm_state = safe_copy(c._net_wm_state);
		_net_wm_allowed_actions = safe_copy(c._net_wm_allowed_actions);
		_net_wm_struct = safe_copy(c._net_wm_struct);
		_net_wm_struct_partial = safe_copy(c._net_wm_struct_partial);
		_net_wm_icon_geometry = safe_copy(c._net_wm_icon_geometry);
		_net_wm_icon = safe_copy(c._net_wm_icon);
		_net_wm_pid = safe_copy(c._net_wm_pid);
		_net_wm_handled_icons = c._net_wm_handled_icons;
		_net_wm_user_time = safe_copy(c._net_wm_user_time);
		_net_wm_user_time_window = safe_copy(c._net_wm_user_time_window);
		_net_frame_extents = safe_copy(c._net_frame_extents);
		_net_wm_opaque_region = safe_copy(c._net_wm_opaque_region);
		_net_wm_bypass_compositor = safe_copy(c._net_wm_bypass_compositor);

		motif_hints = safe_copy(c.motif_hints);

	}

	client_base_t(display_t * cnx, Window id) :
			_cnx(cnx), _id(id) {

		has_valid_window_attributes = false;
		bzero(&wa, sizeof(XWindowAttributes));

		/* ICCCM */
		wm_name = nullptr;
		wm_icon_name = nullptr;
		wm_normal_hints = nullptr;
		wm_hints = nullptr;
		wm_class = nullptr;
		wm_transient_for = nullptr;
		wm_protocols = nullptr;
		wm_colormap_windows = nullptr;
		wm_client_machine = nullptr;
		wm_state = nullptr;

		/* EWMH */
		_net_wm_name = nullptr;
		_net_wm_visible_name = nullptr;
		_net_wm_icon_name = nullptr;
		_net_wm_visible_icon_name = nullptr;
		_net_wm_desktop = nullptr;
		_net_wm_window_type = nullptr;
		_net_wm_state = nullptr;
		_net_wm_allowed_actions = nullptr;
		_net_wm_struct = nullptr;
		_net_wm_struct_partial = nullptr;
		_net_wm_icon_geometry = nullptr;
		_net_wm_icon = nullptr;
		_net_wm_pid = nullptr;
		_net_wm_handled_icons = false;
		_net_wm_user_time = nullptr;
		_net_wm_user_time_window = nullptr;
		_net_frame_extents = nullptr;
		_net_wm_opaque_region = nullptr;
		_net_wm_bypass_compositor = nullptr;

		motif_hints = nullptr;

	}

	virtual ~client_base_t() {
		cout << "call " << __FUNCTION__ << endl;
		delete_all_properties();
	}

	void read_all_properties() {

		/* ICCCM */
		update_wm_name();
		update_wm_icon_name();
		update_wm_normal_hints();
		update_wm_hints();
		update_wm_class();
		update_wm_transient_for();
		update_wm_protocols();
		update_wm_colormap_windows();
		update_wm_client_machine();
		update_wm_state();

		/* EWMH */
		update_net_wm_name();
		update_net_wm_visible_name();
		update_net_wm_icon_name();
		update_net_wm_visible_icon_name();
		update_net_wm_desktop();
		update_net_wm_window_type();
		update_net_wm_state();
		update_net_wm_allowed_actions();
		update_net_wm_struct();
		update_net_wm_struct_partial();
		update_net_wm_icon_geometry();
		update_net_wm_icon();
		update_net_wm_pid ();
		//update_net_wm_handled_icons();
		update_net_wm_user_time();
		update_net_wm_user_time_window ();
		update_net_frame_extents ();
		update_net_wm_opaque_region ();
		update_net_wm_bypass_compositor();

		update_motif_hints();

		update_title();

	}

	void delete_all_properties() {

		/* ICCCM */
		safe_delete(wm_name);
		safe_delete(wm_icon_name);
		safe_delete(wm_normal_hints);
		safe_delete(wm_hints);
		safe_delete(wm_class);
		safe_delete(wm_transient_for);
		safe_delete(wm_protocols);
		safe_delete(wm_colormap_windows);
		safe_delete(wm_client_machine);
		safe_delete(wm_state);

		/* EWMH */
		safe_delete(_net_wm_name);
		safe_delete(_net_wm_visible_name);
		safe_delete(_net_wm_icon_name);
		safe_delete(_net_wm_visible_icon_name);
		safe_delete(_net_wm_desktop);
		safe_delete(_net_wm_window_type);
		safe_delete(_net_wm_state);
		safe_delete(_net_wm_allowed_actions);
		safe_delete(_net_wm_struct);
		safe_delete(_net_wm_struct_partial);
		safe_delete(_net_wm_icon_geometry);
		safe_delete(_net_wm_icon);
		safe_delete(_net_wm_pid);
		_net_wm_handled_icons = false;
		safe_delete(_net_wm_user_time);
		safe_delete(_net_wm_user_time_window);
		safe_delete(_net_frame_extents);
		safe_delete(_net_wm_opaque_region);
		safe_delete(_net_wm_bypass_compositor);

		safe_delete(motif_hints);

	}

	bool read_window_attributes() {
		has_valid_window_attributes = (_cnx->get_window_attributes(_id, &wa) != 0);
		return has_valid_window_attributes;
	}

	void update_wm_name() {
		safe_delete(wm_name);
		wm_name = _cnx->read_wm_name(_id);
	}

	void update_wm_icon_name() {
		safe_delete(wm_icon_name);
		wm_icon_name = _cnx->read_wm_icon_name(_id);
	}

	void update_wm_normal_hints() {
		safe_delete(wm_normal_hints);
		wm_normal_hints = _cnx->read_wm_normal_hints(_id);
	}

	void update_wm_hints() {
		safe_delete(wm_hints);
		wm_hints = _cnx->read_wm_hints(_id);
	}

	void update_wm_class() {
		safe_delete(wm_class);
		wm_class = _cnx->read_wm_class(_id);
	}

	void update_wm_transient_for() {
		safe_delete(wm_transient_for);
		wm_transient_for = _cnx->read_wm_transient_for(_id);
	}

	void update_wm_protocols() {
		safe_delete(wm_protocols);
		wm_protocols = _cnx->read_net_wm_protocols(_id);
	}

	void update_wm_colormap_windows() {
		safe_delete(wm_colormap_windows);
		wm_colormap_windows = _cnx->read_wm_colormap_windows(_id);
	}

	void update_wm_client_machine() {
		safe_delete(wm_client_machine);
		wm_client_machine = _cnx->read_wm_client_machine(_id);
	}

	void update_wm_state() {
		safe_delete(wm_state);
		wm_state = _cnx->read_wm_state(_id);
	}

	/* EWMH */

	void update_net_wm_name() {
		safe_delete(_net_wm_name);
		_net_wm_name = _cnx->read_net_wm_name(_id);
	}

	void update_net_wm_visible_name() {
		safe_delete(_net_wm_visible_name);
		_net_wm_visible_name = _cnx->read_net_wm_visible_name(_id);
	}

	void update_net_wm_icon_name() {
		safe_delete(_net_wm_icon_name);
		_net_wm_icon_name = _cnx->read_net_wm_icon_name(_id);
	}

	void update_net_wm_visible_icon_name() {
		safe_delete(_net_wm_visible_icon_name);
		_net_wm_visible_icon_name = _cnx->read_net_wm_visible_icon_name(_id);
	}

	void update_net_wm_desktop() {
		safe_delete(_net_wm_desktop);
		_net_wm_desktop = _cnx->read_net_wm_desktop(_id);
	}

	void update_net_wm_window_type() {
		safe_delete(_net_wm_window_type);
		_net_wm_window_type = _cnx->read_net_wm_window_type(_id);
	}

	void update_net_wm_state() {
		safe_delete(_net_wm_state);
		_net_wm_state = _cnx->read_net_wm_state(_id);
	}

	void update_net_wm_allowed_actions() {
		safe_delete(_net_wm_allowed_actions);
		_net_wm_allowed_actions = _cnx->read_net_wm_allowed_actions(_id);
	}

	void update_net_wm_struct() {
		safe_delete(_net_wm_struct);
		_net_wm_struct = _cnx->read_net_wm_struct(_id);
	}

	void update_net_wm_struct_partial() {
		safe_delete(_net_wm_struct_partial);
		_net_wm_struct_partial = _cnx->read_net_wm_struct_partial(_id);
	}

	void update_net_wm_icon_geometry() {
		safe_delete(_net_wm_icon_geometry);
		_net_wm_icon_geometry = _cnx->read_net_wm_icon_geometry(_id);
	}

	void update_net_wm_icon() {
		safe_delete(_net_wm_icon);
		_net_wm_icon = _cnx->read_net_wm_icon(_id);
	}

	void update_net_wm_pid() {
		safe_delete(_net_wm_pid);
		_net_wm_pid = _cnx->read_net_wm_pid(_id);
	}

	void update_net_wm_handled_icons();

	void update_net_wm_user_time() {
		safe_delete(_net_wm_user_time);
		_net_wm_user_time = _cnx->read_net_wm_user_time(_id);
	}

	void update_net_wm_user_time_window() {
		safe_delete(_net_wm_user_time_window);
		_net_wm_user_time_window = _cnx->read_net_wm_user_time_window(_id);
	}

	void update_net_frame_extents() {
		safe_delete(_net_frame_extents);
		_net_frame_extents = _cnx->read_net_frame_extents(_id);
	}

	void update_net_wm_opaque_region() {
		safe_delete(_net_wm_opaque_region);
		_net_wm_opaque_region = _cnx->read_net_wm_opaque_region(_id);
	}

	void update_net_wm_bypass_compositor() {
		safe_delete(_net_wm_bypass_compositor);
		_net_wm_bypass_compositor = _cnx->read_net_wm_bypass_compositor(_id);
	}

	void update_motif_hints() {
		safe_delete(motif_hints);
		motif_hints = _cnx->read_motif_wm_hints(_id);
	}


	bool has_motif_border() {
		if (motif_hints != nullptr) {
			if (motif_hints->flags & MWM_HINTS_DECORATIONS) {
				if (not (motif_hints->decorations & MWM_DECOR_BORDER)
						and not ((motif_hints->decorations & MWM_DECOR_ALL))) {
					return false;
				}
			}
		}
		return true;
	}

	void set_net_wm_desktop(unsigned long n) {
		_cnx->change_property(_id, _NET_WM_DESKTOP, CARDINAL, 32, &n, 1);
		safe_delete(_net_wm_desktop);
		_net_wm_desktop = new unsigned long(n);
	}

public:


	void update_title() {
			string name;
			if (_net_wm_name != 0) {
				_title = *(_net_wm_name);
			} else if (wm_name != 0) {
				_title = *(wm_name);
			} else {
				std::stringstream s(std::stringstream::in | std::stringstream::out);
				s << "#" << (_id) << " (noname)";
				_title = s.str();
			}
	}

	void add_subclient(client_base_t * s) {
		_childen.push_back(s);
		s->set_parent(this);
	}

	void remove_subclient(client_base_t * s) {
		_childen.remove(s);
		s->set_parent(nullptr);
	}

	string const & title() const {
		return _title;
	}

	bool is_window(Window w) {
		return w == _id;
	}

	virtual bool has_window(Window w) {
		return w == _id;
	}

	virtual Window base() const {
		return _id;
	}

	virtual Window orig() const {
		return _id;
	}

	virtual string get_node_name() const {
		return _get_node_name<'c'>();
	}

	virtual void replace(tree_t * src, tree_t * by) {
		printf("Unexpected use of managed_window_base_t::replace\n");
	}

	virtual list<tree_t *> childs() const {
		list<tree_t *> ret(_childen.begin(), _childen.end());
		return ret;
	}


	void raise_child(tree_t * t) {

		if(has_key(_childen, t)) {
			_childen.remove(t);
			_childen.push_back(t);
		}

		if(_parent != nullptr) {
			_parent->raise_child(this);
		}

	}

	Atom type() {
		Atom type = None;

		list<Atom> net_wm_window_type;
		bool override_redirect = (wa.override_redirect == True)?true:false;

		if(_net_wm_window_type == 0) {
			/**
			 * Fallback from ICCCM.
			 **/

			if(!override_redirect) {
				/* Managed windows */
				if(wm_transient_for == 0) {
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
			net_wm_window_type = *(_net_wm_window_type);
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
			if (this->wm_class != 0 and this->wm_state != 0
					and type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
				if ((*(this->wm_class))[0] == "Eclipse") {
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
		_childen.remove(t);
	}


	void print_window_attributes() {
		printf(">>> Window: #%lu\n", _id);
		printf("> size: %dx%d+%d+%d\n", wa.width, wa.height, wa.x, wa.y);
		printf("> border_width: %d\n", wa.border_width);
		printf("> depth: %d\n", wa.depth);
		printf("> visual #%p\n", wa.visual);
		printf("> root: #%lu\n", wa.root);
		if (wa.c_class == CopyFromParent) {
			printf("> class: CopyFromParent\n");
		} else if (wa.c_class == InputOutput) {
			printf("> class: InputOutput\n");
		} else if (wa.c_class == InputOnly) {
			printf("> class: InputOnly\n");
		} else {
			printf("> class: Unknown\n");
		}

		if (wa.map_state == IsViewable) {
			printf("> map_state: IsViewable\n");
		} else if (wa.map_state == IsUnviewable) {
			printf("> map_state: IsUnviewable\n");
		} else if (wa.map_state == IsUnmapped) {
			printf("> map_state: IsUnmapped\n");
		} else {
			printf("> map_state: Unknown\n");
		}

		printf("> bit_gravity: %d\n", wa.bit_gravity);
		printf("> win_gravity: %d\n", wa.win_gravity);
		printf("> backing_store: %dlx\n", wa.backing_store);
		printf("> backing_planes: %lx\n", wa.backing_planes);
		printf("> backing_pixel: %lx\n", wa.backing_pixel);
		printf("> save_under: %d\n", wa.save_under);
		printf("> colormap: ?\n");
		printf("> all_event_masks: %08lx\n", wa.all_event_masks);
		printf("> your_event_mask: %08lx\n", wa.your_event_mask);
		printf("> do_not_propagate_mask: %08lx\n", wa.do_not_propagate_mask);
		printf("> override_redirect: %d\n", wa.override_redirect);
		printf("> screen: %p\n", wa.screen);
	}


	void print_properties() {
		/* ICCCM */
		if(wm_name != nullptr)
			cout << "* WM_NAME = " << *wm_name << endl;

		if(wm_icon_name != nullptr)
			cout << "* WM_ICON_NAME = " << *wm_icon_name << endl;

		//if(wm_normal_hints != nullptr)
		//	cout << "WM_NORMAL_HINTS = " << *wm_normal_hints << endl;

		//if(wm_hints != nullptr)
		//	cout << "WM_HINTS = " << *wm_hints << endl;

		if(wm_class != nullptr)
			cout << "* WM_CLASS = " << (*wm_class)[0] << "," << (*wm_class)[1] << endl;

		if(wm_transient_for != nullptr)
			cout << "* WM_TRANSIENT_FOR = " << *wm_transient_for << endl;

		if(wm_protocols != nullptr) {
			cout << "* WM_PROTOCOLS = ";
			for(auto x: *wm_protocols) {
				cout << _cnx->get_atom_name(x) << " ";
			}
			cout << endl;
		}

		if(wm_colormap_windows != nullptr)
			cout << "WM_COLORMAP_WINDOWS = " << (*wm_colormap_windows)[0] << endl;

		if(wm_client_machine != nullptr)
			cout << "* WM_CLIENT_MACHINE = " << *wm_client_machine << endl;

		if(wm_state != nullptr) {
			cout << "* WM_STATE = " << *wm_state << endl;
		}


		/* EWMH */
		if(_net_wm_name != nullptr)
			cout << "* _NET_WM_NAME = " << *_net_wm_name << endl;

		if(_net_wm_visible_name != nullptr)
			cout << "* _NET_WM_VISIBLE_NAME = " << *_net_wm_visible_name << endl;

		if(_net_wm_icon_name != nullptr)
			cout << "* _NET_WM_ICON_NAME = " << *_net_wm_icon_name << endl;

		if(_net_wm_visible_icon_name != nullptr)
			cout << "* _NET_WM_VISIBLE_ICON_NAME = " << *_net_wm_visible_icon_name << endl;

		if(_net_wm_desktop != nullptr)
			cout << "* _NET_WM_DESKTOP = " << *_net_wm_desktop << endl;

		if(_net_wm_window_type != nullptr) {
			cout << "* _NET_WM_WINDOW_TYPE = ";
			for(auto x: *_net_wm_window_type) {
				cout << _cnx->get_atom_name(x) << " ";
			}
			cout << endl;
		}

		if(_net_wm_state != nullptr) {
			cout << "* _NET_WM_STATE = ";
			for(auto x: *_net_wm_state) {
				cout << _cnx->get_atom_name(x) << " ";
			}
			cout << endl;
		}

		if(_net_wm_allowed_actions != nullptr) {
			cout << "* _NET_WM_ALLOWED_ACTIONS = ";
			for(auto x: *_net_wm_allowed_actions) {
				cout << _cnx->get_atom_name(x) << " ";
			}
			cout << endl;
		}

		if(_net_wm_struct != nullptr) {
			cout << "* _NET_WM_STRUCT = ";
			for(auto x: *_net_wm_struct) {
				cout << x << " ";
			}
			cout << endl;
		}

		if(_net_wm_struct_partial != nullptr) {
			cout << "* _NET_WM_PARTIAL_STRUCT = ";
			for(auto x: *_net_wm_struct_partial) {
				cout << x << " ";
			}
			cout << endl;
		}

		if(_net_wm_icon_geometry != nullptr) {
			cout << "* _NET_WM_ICON_GEOMETRY = ";
			for(auto x: *_net_wm_struct_partial) {
				cout << x << " ";
			}
			cout << endl;
		}

		if(_net_wm_icon != nullptr)
			cout << "* _NET_WM_ICON = " << "TODO" << endl;

		if(_net_wm_pid != nullptr)
			cout << "* _NET_WM_PID = " << *_net_wm_pid << endl;

		//if(_net_wm_handled_icons != false)
		//	;

		if(_net_wm_user_time != nullptr)
			cout << "* _NET_WM_USER_TIME = " << *_net_wm_user_time << endl;

		if(_net_wm_user_time_window != nullptr)
			cout << "* _NET_WM_USER_TIME_WINDOW = " << *_net_wm_user_time_window << endl;

		if(_net_frame_extents != nullptr) {
			cout << "* _NET_FRAME_EXTENTS = ";
			for(auto x: *_net_frame_extents) {
				cout << x << " ";
			}
			cout << endl;
		}

		//_net_wm_opaque_region = nullptr;
		//_net_wm_bypass_compositor = nullptr;
		//motif_hints = nullptr;
	}


};

}

#endif /* CLIENT_HXX_ */
