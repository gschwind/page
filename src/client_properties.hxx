/*
 * client_properties.hxx
 *
 * copyright (2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#ifndef CLIENT_PROPERTIES_HXX_
#define CLIENT_PROPERTIES_HXX_

#include <X11/X.h>
#include <X11/extensions/shape.h>

#include "utils.hxx"
#include "motif_hints.hxx"
#include "display.hxx"
#include "tree.hxx"

namespace page {

typedef long card32;

using namespace std;

class client_properties_t {
private:
	display_t *                  _cnx;
	Window                       _id;

	bool                         _has_valid_window_attributes;
	XWindowAttributes            _wa;

	/* ICCCM */

	string *                     _wm_name;
	string *                     _wm_icon_name;
	XSizeHints *                 _wm_normal_hints;
	XWMHints *                   _wm_hints;
	vector<string> *             _wm_class;
	Window *                     _wm_transient_for;
	list<Atom> *                 _wm_protocols;
	vector<Window> *             _wm_colormap_windows;
	string *                     _wm_client_machine;

	/* wm_state is writen by WM */
	card32 *                     _wm_state;

	/* EWMH */

	string *                     __net_wm_name;
	string *                     __net_wm_visible_name;
	string *                     __net_wm_icon_name;
	string *                     __net_wm_visible_icon_name;
	unsigned long *              __net_wm_desktop;
	list<Atom> *                 __net_wm_window_type;
	list<Atom> *                 __net_wm_state;
	list<Atom> *                 __net_wm_allowed_actions;
	vector<card32> *             __net_wm_struct;
	vector<card32> *             __net_wm_struct_partial;
	vector<card32> *             __net_wm_icon_geometry;
	vector<card32> *             __net_wm_icon;
	unsigned long *              __net_wm_pid;
	bool                         __net_wm_handled_icons;
	Time *                       __net_wm_user_time;
	Window *                     __net_wm_user_time_window;
	vector<card32> *             __net_frame_extents;
	vector<card32> *             __net_wm_opaque_region;
	unsigned long *              __net_wm_bypass_compositor;

	/* OTHERs */
	motif_wm_hints_t *           _motif_hints;

	region *                     _shape;

	/** short cut **/
	Atom A(atom_e atom) {
		return _cnx->A(atom);
	}

public:

	client_properties_t(client_properties_t const & c) {

		_has_valid_window_attributes = c._has_valid_window_attributes;
		memcpy(&_wa, &c._wa, sizeof(XWindowAttributes));

		_id = c._id;
		_cnx = c._cnx;

		/* ICCCM */
		_wm_name = safe_copy(c._wm_name);
		_wm_icon_name = safe_copy(c._wm_icon_name);
		_wm_normal_hints = safe_copy(c._wm_normal_hints);
		_wm_hints = safe_copy(c._wm_hints);
		_wm_class = safe_copy(c._wm_class);
		_wm_transient_for = safe_copy(c._wm_transient_for);
		_wm_protocols = safe_copy(c._wm_protocols);
		_wm_colormap_windows = safe_copy(c._wm_colormap_windows);
		_wm_client_machine = safe_copy(c._wm_client_machine);
		_wm_state = safe_copy(c._wm_state);

		/* EWMH */
		__net_wm_name = safe_copy(c.__net_wm_name);
		__net_wm_visible_name = safe_copy(c.__net_wm_visible_name);
		__net_wm_icon_name = safe_copy(c.__net_wm_icon_name);
		__net_wm_visible_icon_name = safe_copy(c.__net_wm_visible_icon_name);
		__net_wm_desktop = safe_copy(c.__net_wm_desktop);
		__net_wm_window_type = safe_copy(c.__net_wm_window_type);
		__net_wm_state = safe_copy(c.__net_wm_state);
		__net_wm_allowed_actions = safe_copy(c.__net_wm_allowed_actions);
		__net_wm_struct = safe_copy(c.__net_wm_struct);
		__net_wm_struct_partial = safe_copy(c.__net_wm_struct_partial);
		__net_wm_icon_geometry = safe_copy(c.__net_wm_icon_geometry);
		__net_wm_icon = safe_copy(c.__net_wm_icon);
		__net_wm_pid = safe_copy(c.__net_wm_pid);
		__net_wm_handled_icons = c.__net_wm_handled_icons;
		__net_wm_user_time = safe_copy(c.__net_wm_user_time);
		__net_wm_user_time_window = safe_copy(c.__net_wm_user_time_window);
		__net_frame_extents = safe_copy(c.__net_frame_extents);
		__net_wm_opaque_region = safe_copy(c.__net_wm_opaque_region);
		__net_wm_bypass_compositor = safe_copy(c.__net_wm_bypass_compositor);

		_motif_hints = safe_copy(c._motif_hints);

		_shape = safe_copy(c._shape);

	}

	client_properties_t(display_t * cnx, Window id) :
			_cnx(cnx), _id(id) {

		_has_valid_window_attributes = false;
		bzero(&_wa, sizeof(XWindowAttributes));

		/* ICCCM */
		_wm_name = nullptr;
		_wm_icon_name = nullptr;
		_wm_normal_hints = nullptr;
		_wm_hints = nullptr;
		_wm_class = nullptr;
		_wm_transient_for = nullptr;
		_wm_protocols = nullptr;
		_wm_colormap_windows = nullptr;
		_wm_client_machine = nullptr;
		_wm_state = nullptr;

		/* EWMH */
		__net_wm_name = nullptr;
		__net_wm_visible_name = nullptr;
		__net_wm_icon_name = nullptr;
		__net_wm_visible_icon_name = nullptr;
		__net_wm_desktop = nullptr;
		__net_wm_window_type = nullptr;
		__net_wm_state = nullptr;
		__net_wm_allowed_actions = nullptr;
		__net_wm_struct = nullptr;
		__net_wm_struct_partial = nullptr;
		__net_wm_icon_geometry = nullptr;
		__net_wm_icon = nullptr;
		__net_wm_pid = nullptr;
		__net_wm_handled_icons = false;
		__net_wm_user_time = nullptr;
		__net_wm_user_time_window = nullptr;
		__net_frame_extents = nullptr;
		__net_wm_opaque_region = nullptr;
		__net_wm_bypass_compositor = nullptr;

		_motif_hints = nullptr;

		_shape = nullptr;

	}

	~client_properties_t() {
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

		update_shape();

	}

	void delete_all_properties() {

		/* ICCCM */
		safe_delete(_wm_name);
		safe_delete(_wm_icon_name);
		safe_delete(_wm_normal_hints);
		safe_delete(_wm_hints);
		safe_delete(_wm_class);
		safe_delete(_wm_transient_for);
		safe_delete(_wm_protocols);
		safe_delete(_wm_colormap_windows);
		safe_delete(_wm_client_machine);
		safe_delete(_wm_state);

		/* EWMH */
		safe_delete(__net_wm_name);
		safe_delete(__net_wm_visible_name);
		safe_delete(__net_wm_icon_name);
		safe_delete(__net_wm_visible_icon_name);
		safe_delete(__net_wm_desktop);
		safe_delete(__net_wm_window_type);
		safe_delete(__net_wm_state);
		safe_delete(__net_wm_allowed_actions);
		safe_delete(__net_wm_struct);
		safe_delete(__net_wm_struct_partial);
		safe_delete(__net_wm_icon_geometry);
		safe_delete(__net_wm_icon);
		safe_delete(__net_wm_pid);
		__net_wm_handled_icons = false;
		safe_delete(__net_wm_user_time);
		safe_delete(__net_wm_user_time_window);
		safe_delete(__net_frame_extents);
		safe_delete(__net_wm_opaque_region);
		safe_delete(__net_wm_bypass_compositor);

		safe_delete(_motif_hints);

		safe_delete(_shape);

	}

	bool read_window_attributes() {
		_has_valid_window_attributes = (_cnx->get_window_attributes(_id, &_wa) != 0);
		return _has_valid_window_attributes;
	}

	void update_wm_name() {
		safe_delete(_wm_name);
		_wm_name = _cnx->read_wm_name(_id);
	}

	void update_wm_icon_name() {
		safe_delete(_wm_icon_name);
		_wm_icon_name = _cnx->read_wm_icon_name(_id);
	}

	void update_wm_normal_hints() {
		safe_delete(_wm_normal_hints);
		_wm_normal_hints = _cnx->read_wm_normal_hints(_id);
	}

	void update_wm_hints() {
		safe_delete(_wm_hints);
		_wm_hints = _cnx->read_wm_hints(_id);
	}

	void update_wm_class() {
		safe_delete(_wm_class);
		_wm_class = _cnx->read_wm_class(_id);
	}

	void update_wm_transient_for() {
		safe_delete(_wm_transient_for);
		_wm_transient_for = _cnx->read_wm_transient_for(_id);
	}

	void update_wm_protocols() {
		safe_delete(_wm_protocols);
		_wm_protocols = _cnx->read_net_wm_protocols(_id);
	}

	void update_wm_colormap_windows() {
		safe_delete(_wm_colormap_windows);
		_wm_colormap_windows = _cnx->read_wm_colormap_windows(_id);
	}

	void update_wm_client_machine() {
		safe_delete(_wm_client_machine);
		_wm_client_machine = _cnx->read_wm_client_machine(_id);
	}

	void update_wm_state() {
		safe_delete(_wm_state);
		_wm_state = _cnx->read_wm_state(_id);
	}

	/* EWMH */

	void update_net_wm_name() {
		safe_delete(__net_wm_name);
		__net_wm_name = _cnx->read_net_wm_name(_id);
	}

	void update_net_wm_visible_name() {
		safe_delete(__net_wm_visible_name);
		__net_wm_visible_name = _cnx->read_net_wm_visible_name(_id);
	}

	void update_net_wm_icon_name() {
		safe_delete(__net_wm_icon_name);
		__net_wm_icon_name = _cnx->read_net_wm_icon_name(_id);
	}

	void update_net_wm_visible_icon_name() {
		safe_delete(__net_wm_visible_icon_name);
		__net_wm_visible_icon_name = _cnx->read_net_wm_visible_icon_name(_id);
	}

	void update_net_wm_desktop() {
		safe_delete(__net_wm_desktop);
		__net_wm_desktop = _cnx->read_net_wm_desktop(_id);
	}

	void update_net_wm_window_type() {
		safe_delete(__net_wm_window_type);
		__net_wm_window_type = _cnx->read_net_wm_window_type(_id);
	}

	void update_net_wm_state() {
		safe_delete(__net_wm_state);
		__net_wm_state = _cnx->read_net_wm_state(_id);
	}

	void update_net_wm_allowed_actions() {
		safe_delete(__net_wm_allowed_actions);
		__net_wm_allowed_actions = _cnx->read_net_wm_allowed_actions(_id);
	}

	void update_net_wm_struct() {
		safe_delete(__net_wm_struct);
		__net_wm_struct = _cnx->read_net_wm_struct(_id);
	}

	void update_net_wm_struct_partial() {
		safe_delete(__net_wm_struct_partial);
		__net_wm_struct_partial = _cnx->read_net_wm_struct_partial(_id);
	}

	void update_net_wm_icon_geometry() {
		safe_delete(__net_wm_icon_geometry);
		__net_wm_icon_geometry = _cnx->read_net_wm_icon_geometry(_id);
	}

	void update_net_wm_icon() {
		safe_delete(__net_wm_icon);
		__net_wm_icon = _cnx->read_net_wm_icon(_id);
	}

	void update_net_wm_pid() {
		safe_delete(__net_wm_pid);
		__net_wm_pid = _cnx->read_net_wm_pid(_id);
	}

	void update_net_wm_handled_icons();

	void update_net_wm_user_time() {
		safe_delete(__net_wm_user_time);
		__net_wm_user_time = _cnx->read_net_wm_user_time(_id);
	}

	void update_net_wm_user_time_window() {
		safe_delete(__net_wm_user_time_window);
		__net_wm_user_time_window = _cnx->read_net_wm_user_time_window(_id);
	}

	void update_net_frame_extents() {
		safe_delete(__net_frame_extents);
		__net_frame_extents = _cnx->read_net_frame_extents(_id);
	}

	void update_net_wm_opaque_region() {
		safe_delete(__net_wm_opaque_region);
		__net_wm_opaque_region = _cnx->read_net_wm_opaque_region(_id);
	}

	void update_net_wm_bypass_compositor() {
		safe_delete(__net_wm_bypass_compositor);
		__net_wm_bypass_compositor = _cnx->read_net_wm_bypass_compositor(_id);
	}

	void update_motif_hints() {
		safe_delete(_motif_hints);
		_motif_hints = _cnx->read_motif_wm_hints(_id);
	}

	void update_shape() {
		safe_delete(_shape);

		int count;
		int ordering;
		XRectangle * rect = XShapeGetRectangles(_cnx->dpy(), _id, ShapeBounding, &count, &ordering);

		if (rect != NULL) {
			_shape = new region{};
			for (unsigned i = 0; i < count; ++i) {
				*_shape += rectangle(rect[i]);
			}
			XFree(rect);
		}

	}


	bool has_motif_border() {
		if (_motif_hints != nullptr) {
			if (_motif_hints->flags & MWM_HINTS_DECORATIONS) {
				if (not (_motif_hints->decorations & MWM_DECOR_BORDER)
						and not ((_motif_hints->decorations & MWM_DECOR_ALL))) {
					return false;
				}
			}
		}
		return true;
	}

	void set_net_wm_desktop(unsigned long n) {
		_cnx->change_property(_id, _NET_WM_DESKTOP, CARDINAL, 32, &n, 1);
		safe_delete(__net_wm_desktop);
		__net_wm_desktop = new unsigned long(n);
	}

public:

	void print_window_attributes() {
		printf(">>> Window: #%lu\n", _id);
		printf("> size: %dx%d+%d+%d\n", _wa.width, _wa.height, _wa.x, _wa.y);
		printf("> border_width: %d\n", _wa.border_width);
		printf("> depth: %d\n", _wa.depth);
		printf("> visual #%p\n", _wa.visual);
		printf("> root: #%lu\n", _wa.root);
		if (_wa.c_class == CopyFromParent) {
			printf("> class: CopyFromParent\n");
		} else if (_wa.c_class == InputOutput) {
			printf("> class: InputOutput\n");
		} else if (_wa.c_class == InputOnly) {
			printf("> class: InputOnly\n");
		} else {
			printf("> class: Unknown\n");
		}

		if (_wa.map_state == IsViewable) {
			printf("> map_state: IsViewable\n");
		} else if (_wa.map_state == IsUnviewable) {
			printf("> map_state: IsUnviewable\n");
		} else if (_wa.map_state == IsUnmapped) {
			printf("> map_state: IsUnmapped\n");
		} else {
			printf("> map_state: Unknown\n");
		}

		printf("> bit_gravity: %d\n", _wa.bit_gravity);
		printf("> win_gravity: %d\n", _wa.win_gravity);
		printf("> backing_store: %dlx\n", _wa.backing_store);
		printf("> backing_planes: %lx\n", _wa.backing_planes);
		printf("> backing_pixel: %lx\n", _wa.backing_pixel);
		printf("> save_under: %d\n", _wa.save_under);
		printf("> colormap: <Not Implemented>\n");
		printf("> all_event_masks: %08lx\n", _wa.all_event_masks);
		printf("> your_event_mask: %08lx\n", _wa.your_event_mask);
		printf("> do_not_propagate_mask: %08lx\n", _wa.do_not_propagate_mask);
		printf("> override_redirect: %d\n", _wa.override_redirect);
		printf("> screen: %p\n", _wa.screen);
	}


	void print_properties() {
		/* ICCCM */
		if(_wm_name != nullptr)
			cout << "* WM_NAME = " << *_wm_name << endl;

		if(_wm_icon_name != nullptr)
			cout << "* WM_ICON_NAME = " << *_wm_icon_name << endl;

		//if(wm_normal_hints != nullptr)
		//	cout << "WM_NORMAL_HINTS = " << *wm_normal_hints << endl;

		//if(wm_hints != nullptr)
		//	cout << "WM_HINTS = " << *wm_hints << endl;

		if(_wm_class != nullptr)
			cout << "* WM_CLASS = " << (*_wm_class)[0] << "," << (*_wm_class)[1] << endl;

		if(_wm_transient_for != nullptr)
			cout << "* WM_TRANSIENT_FOR = " << *_wm_transient_for << endl;

		if(_wm_protocols != nullptr) {
			cout << "* WM_PROTOCOLS = ";
			for(auto x: *_wm_protocols) {
				cout << _cnx->get_atom_name(x) << " ";
			}
			cout << endl;
		}

		if(_wm_colormap_windows != nullptr)
			cout << "WM_COLORMAP_WINDOWS = " << (*_wm_colormap_windows)[0] << endl;

		if(_wm_client_machine != nullptr)
			cout << "* WM_CLIENT_MACHINE = " << *_wm_client_machine << endl;

		if(_wm_state != nullptr) {
			cout << "* WM_STATE = " << *_wm_state << endl;
		}


		/* EWMH */
		if(__net_wm_name != nullptr)
			cout << "* _NET_WM_NAME = " << *__net_wm_name << endl;

		if(__net_wm_visible_name != nullptr)
			cout << "* _NET_WM_VISIBLE_NAME = " << *__net_wm_visible_name << endl;

		if(__net_wm_icon_name != nullptr)
			cout << "* _NET_WM_ICON_NAME = " << *__net_wm_icon_name << endl;

		if(__net_wm_visible_icon_name != nullptr)
			cout << "* _NET_WM_VISIBLE_ICON_NAME = " << *__net_wm_visible_icon_name << endl;

		if(__net_wm_desktop != nullptr)
			cout << "* _NET_WM_DESKTOP = " << *__net_wm_desktop << endl;

		if(__net_wm_window_type != nullptr) {
			cout << "* _NET_WM_WINDOW_TYPE = ";
			for(auto x: *__net_wm_window_type) {
				cout << _cnx->get_atom_name(x) << " ";
			}
			cout << endl;
		}

		if(__net_wm_state != nullptr) {
			cout << "* _NET_WM_STATE = ";
			for(auto x: *__net_wm_state) {
				cout << _cnx->get_atom_name(x) << " ";
			}
			cout << endl;
		}

		if(__net_wm_allowed_actions != nullptr) {
			cout << "* _NET_WM_ALLOWED_ACTIONS = ";
			for(auto x: *__net_wm_allowed_actions) {
				cout << _cnx->get_atom_name(x) << " ";
			}
			cout << endl;
		}

		if(__net_wm_struct != nullptr) {
			cout << "* _NET_WM_STRUCT = ";
			for(auto x: *__net_wm_struct) {
				cout << x << " ";
			}
			cout << endl;
		}

		if(__net_wm_struct_partial != nullptr) {
			cout << "* _NET_WM_PARTIAL_STRUCT = ";
			for(auto x: *__net_wm_struct_partial) {
				cout << x << " ";
			}
			cout << endl;
		}

		if(__net_wm_icon_geometry != nullptr) {
			cout << "* _NET_WM_ICON_GEOMETRY = ";
			for(auto x: *__net_wm_icon_geometry) {
				cout << x << " ";
			}
			cout << endl;
		}

		if(__net_wm_icon != nullptr)
			cout << "* _NET_WM_ICON = " << "TODO" << endl;

		if(__net_wm_pid != nullptr)
			cout << "* _NET_WM_PID = " << *__net_wm_pid << endl;

		//if(_net_wm_handled_icons != false)
		//	;

		if(__net_wm_user_time != nullptr)
			cout << "* _NET_WM_USER_TIME = " << *__net_wm_user_time << endl;

		if(__net_wm_user_time_window != nullptr)
			cout << "* _NET_WM_USER_TIME_WINDOW = " << *__net_wm_user_time_window << endl;

		if(__net_frame_extents != nullptr) {
			cout << "* _NET_FRAME_EXTENTS = ";
			for(auto x: *__net_frame_extents) {
				cout << x << " ";
			}
			cout << endl;
		}

		//_net_wm_opaque_region = nullptr;
		//_net_wm_bypass_compositor = nullptr;
		//motif_hints = nullptr;
	}

	Atom type() {
		Atom type = None;

		list<Atom> net_wm_window_type;
		bool override_redirect = (_wa.override_redirect == True)?true:false;

		if(__net_wm_window_type == nullptr) {
			/**
			 * Fallback from ICCCM.
			 **/

			if(!override_redirect) {
				/* Managed windows */
				if(_wm_transient_for == nullptr) {
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
			net_wm_window_type = *(__net_wm_window_type);
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
			if (_wm_class != nullptr
					and __net_wm_state != nullptr
					and type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
				if ((*(_wm_class))[0] == "Eclipse") {
					if(has_key(*__net_wm_state, A(_NET_WM_STATE_SKIP_TASKBAR))) {
						type = A(_NET_WM_WINDOW_TYPE_DND);
					}
				}
			}
		}

		return type;

	}

	display_t *          cnx() const { return _cnx; }
	Window               id() const { return _id; }

	bool                 has_valid_window_attributes() const { return _has_valid_window_attributes; }

	XWindowAttributes const & wa() const { return _wa; }

	/* ICCCM */

	string const *                     wm_name() const { return _wm_name; }
	string const *                     wm_icon_name() const { return _wm_icon_name; };
	XSizeHints const *                 wm_normal_hints() const { return _wm_normal_hints; }
	XWMHints const *                   wm_hints() const { return _wm_hints; }
	vector<string> const *             wm_class() const {return _wm_class; }
	Window const *                     wm_transient_for() const { return _wm_transient_for; }
	list<Atom> const *                 wm_protocols() const { return _wm_protocols; }
	vector<Window> const *             wm_colormap_windows() const { return _wm_colormap_windows; }
	string const *                     wm_client_machine() const { return _wm_client_machine; }

	/* wm_state is writen by WM */
	card32 const *                     wm_state() const {return _wm_state; }

	/* EWMH */

	string const *                     net_wm_name() const { return __net_wm_name; }
	string const *                     net_wm_visible_name() const { return __net_wm_visible_name; }
	string const *                     net_wm_icon_name() const { return __net_wm_icon_name; }
	string const *                     net_wm_visible_icon_name() const { return __net_wm_visible_icon_name; }
	unsigned long const *              net_wm_desktop() const { return __net_wm_desktop; }
	list<Atom> const *                 net_wm_window_type() const { return __net_wm_window_type; }
	list<Atom> const *                 net_wm_state() const { return __net_wm_state; }
	list<Atom> const *                 net_wm_allowed_actions() const { return __net_wm_allowed_actions; }
	vector<card32> const *             net_wm_struct() const { return __net_wm_struct; }
	vector<card32> const *             net_wm_struct_partial() const { return __net_wm_struct_partial; }
	vector<card32> const *             net_wm_icon_geometry() const { return __net_wm_icon_geometry; }
	vector<card32> const *             net_wm_icon() const { return __net_wm_icon; }
	unsigned long const *              net_wm_pid() const { return __net_wm_pid; }
	bool                               net_wm_handled_icons() const { return __net_wm_handled_icons; }
	Time const *                       net_wm_user_time() const { return __net_wm_user_time; }
	Window const *                     net_wm_user_time_window() const { return __net_wm_user_time_window; }
	vector<card32> const *             net_frame_extents() const { return __net_frame_extents; }
	vector<card32> const *             net_wm_opaque_region() const { return __net_wm_opaque_region; }
	unsigned long const *              net_wm_bypass_compositor() const { return __net_wm_bypass_compositor; }

	/* OTHERs */
	motif_wm_hints_t const *           motif_hints() const { return _motif_hints; }

	region const *                     shape() const { return _shape; }

	void net_wm_state_add(atom_e atom) {
		if(__net_wm_state == nullptr) {
			__net_wm_state = new list<Atom>;
		}
		/** remove it if already focused **/
		__net_wm_state->remove(A(atom));
		/** add it **/
		__net_wm_state->push_back(A(atom));
		_cnx->write_net_wm_state(_id, *(__net_wm_state));
	}

	void net_wm_state_remove(atom_e atom) {
		if(__net_wm_state == nullptr) {
			__net_wm_state = new list<Atom>;
		}

		__net_wm_state->remove(A(atom));
		_cnx->write_net_wm_state(_id, *(__net_wm_state));
	}

	void net_wm_allowed_actions_add(atom_e atom) {
		if(__net_wm_allowed_actions == nullptr) {
			__net_wm_allowed_actions = new list<Atom>;
		}

		__net_wm_allowed_actions->remove(A(atom));
		__net_wm_allowed_actions->push_back(A(atom));
		_cnx->write_net_wm_allowed_actions(_id, *(__net_wm_allowed_actions));
	}

	void process_event(XConfigureEvent const & e) {
		_wa.override_redirect = e.override_redirect;
		_wa.width = e.width;
		_wa.height = e.height;
		_wa.x = e.x;
		_wa.y = e.y;
		_wa.border_width = e.border_width;
	}

};

}



#endif /* CLIENT_PROPERTIES_HXX_ */
