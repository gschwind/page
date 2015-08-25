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

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <xcb/xcb.h>

#include <limits>
#include <utility>
#include <vector>
#include <list>
#include <iostream>
#include <set>
#include <algorithm>


#include "utils.hxx"
#include "region.hxx"
#include "display.hxx"
#include "motif_hints.hxx"

#include "properties.hxx"

namespace page {

using namespace std;

class client_properties_t {
private:
	display_t *                  _dpy;
	xcb_window_t                 _id;

	bool                         _need_update_type;
	xcb_atom_t                   _wm_type;

	xcb_get_window_attributes_reply_t * _wa;
	xcb_get_geometry_reply_t * _geometry;

	/* ICCCM */

	wm_name_t                    _wm_name;

	wm_icon_name_t               _wm_icon_name;
	wm_normal_hints_t            _wm_normal_hints;
	wm_hints_t                   _wm_hints;
	wm_class_t                   _wm_class;
	wm_transient_for_t           _wm_transient_for;
	wm_protocols_t               _wm_protocols;
	wm_colormap_windows_t        _wm_colormap_windows;
	wm_client_machine_t          _wm_client_machine;

	/* wm_state is writen by WM */
	wm_state_t                   _wm_state;

	/* EWMH */

	net_wm_name_t                __net_wm_name;
	net_wm_visible_name_t        __net_wm_visible_name;
	net_wm_icon_name_t           __net_wm_icon_name;
	net_wm_visible_icon_name_t   __net_wm_visible_icon_name;
	net_wm_desktop_t             __net_wm_desktop;
	net_wm_window_type_t         __net_wm_window_type;
	net_wm_state_t               __net_wm_state;
	net_wm_allowed_actions_t     __net_wm_allowed_actions;
	net_wm_strut_t               __net_wm_strut;
	net_wm_strut_partial_t       __net_wm_strut_partial;
	net_wm_icon_geometry_t       __net_wm_icon_geometry;
	net_wm_icon_t                __net_wm_icon;
	net_wm_pid_t                 __net_wm_pid;
	//net_wm_handled_icons_t       __net_wm_handled_icons;
	net_wm_user_time_t           __net_wm_user_time;
	net_wm_user_time_window_t    __net_wm_user_time_window;
	net_frame_extents_t          __net_frame_extents;
	net_wm_opaque_region_t       __net_wm_opaque_region;
	net_wm_bypass_compositor_t   __net_wm_bypass_compositor;

	/* OTHERs */
	motif_hints_t                _motif_hints;

	region *                     _shape;

	/** short cut **/
	xcb_atom_t A(atom_e atom) {
		return _dpy->A(atom);
	}

	xcb_atom_t B(atom_e atom) {
		return static_cast<xcb_atom_t>(A(atom));
	}

	xcb_window_t xid() {
		return static_cast<xcb_window_t>(_id);
	}

private:
	client_properties_t(client_properties_t const &);
	client_properties_t & operator=(client_properties_t const &);
public:

	client_properties_t(display_t * cnx, xcb_window_t id) :
			_dpy{cnx}, _id{id} {

		_wa = nullptr;
		_geometry = nullptr;

		_motif_hints = nullptr;

		_shape = nullptr;

		_need_update_type = true;
		_wm_type = A(_NET_WM_WINDOW_TYPE_NORMAL);

	}

	~client_properties_t() {
		if(_wa != nullptr)
			free(_wa);
		if(_geometry != nullptr)
			free(_geometry);
		delete_all_properties();
	}

	void read_all_properties() {

		/** Welcome to magic templates ! **/
		_wm_name.fetch(_dpy->xcb(), _dpy->_A, xid());
		_wm_icon_name.fetch(_dpy->xcb(), _dpy->_A, xid());
		_wm_normal_hints.fetch(_dpy->xcb(), _dpy->_A, xid());
		_wm_hints.fetch(_dpy->xcb(), _dpy->_A, xid());
		_wm_class.fetch(_dpy->xcb(), _dpy->_A, xid());
		_wm_transient_for.fetch(_dpy->xcb(), _dpy->_A, xid());
		_wm_protocols.fetch(_dpy->xcb(), _dpy->_A, xid());
		_wm_colormap_windows.fetch(_dpy->xcb(), _dpy->_A, xid());
		_wm_client_machine.fetch(_dpy->xcb(), _dpy->_A, xid());
		_wm_state.fetch(_dpy->xcb(), _dpy->_A, xid());

		__net_wm_name.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_visible_name.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_icon_name.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_visible_icon_name.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_desktop.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_window_type.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_state.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_allowed_actions.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_strut.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_strut_partial.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_icon_geometry.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_icon.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_pid.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_user_time.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_user_time_window.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_frame_extents.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_opaque_region.fetch(_dpy->xcb(), _dpy->_A, xid());
		__net_wm_bypass_compositor.fetch(_dpy->xcb(), _dpy->_A, xid());

		update_motif_hints();
		update_shape();
		update_type();

	}

	void delete_all_properties() {
		safe_delete(_shape);
	}

	bool read_window_attributes() {
		if(_wa != nullptr) {
			free(_wa);
			_wa = nullptr;
		}

		if(_geometry != nullptr) {
			free(_geometry);
			_geometry = nullptr;
		}

		auto ck0 = xcb_get_window_attributes(_dpy->xcb(), xid());
		auto ck1 = xcb_get_geometry(_dpy->xcb(), xid());
		_wa = xcb_get_window_attributes_reply(_dpy->xcb(), ck0, nullptr);
		_geometry = xcb_get_geometry_reply(_dpy->xcb(), ck1, nullptr);
		return _wa != nullptr and _geometry != nullptr;
	}

	void update_wm_name() {
		_wm_name.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_wm_icon_name() {
		_wm_icon_name.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_wm_normal_hints() {
		_wm_normal_hints.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_wm_hints() {
		_wm_hints.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_wm_class() {
		_wm_class.fetch(_dpy->xcb(), _dpy->_A, xid());
		_need_update_type = true;
	}

	void update_wm_transient_for() {
		_wm_transient_for.fetch(_dpy->xcb(), _dpy->_A, xid());
		_need_update_type = true;
	}

	void update_wm_protocols() {
		_wm_protocols.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_wm_colormap_windows() {
		_wm_colormap_windows.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_wm_client_machine() {
		_wm_client_machine.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_wm_state() {
		_wm_state.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	/* EWMH */

	void update_net_wm_name() {
		__net_wm_name.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_visible_name() {
		__net_wm_visible_name.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_icon_name() {
		__net_wm_icon_name.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_visible_icon_name() {
		__net_wm_visible_icon_name.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_desktop() {
		__net_wm_desktop.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_window_type() {
		__net_wm_window_type.fetch(_dpy->xcb(), _dpy->_A, xid());
		_need_update_type = true;
	}

	void update_net_wm_state() {
		__net_wm_state.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_allowed_actions() {
		__net_wm_allowed_actions.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_struct() {
		__net_wm_strut.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_struct_partial() {
		__net_wm_strut_partial.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_icon_geometry() {
		__net_wm_icon_geometry.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_icon() {
		__net_wm_icon.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_pid() {
		__net_wm_pid.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_user_time() {
		__net_wm_user_time.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_user_time_window() {
		__net_wm_user_time_window.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_frame_extents() {
		__net_frame_extents.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_opaque_region() {
		__net_wm_opaque_region.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_net_wm_bypass_compositor() {
		__net_wm_bypass_compositor.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_motif_hints() {
		_motif_hints.fetch(_dpy->xcb(), _dpy->_A, xid());
	}

	void update_shape() {
		delete _shape;
		_shape = nullptr;

		int count;
		int ordering;

		/** request extent to check if shape has been set **/
		xcb_shape_query_extents_cookie_t ck0 = xcb_shape_query_extents(_dpy->xcb(), _id);

		/** in the same time we make the request for rectangle (even if this request isn't needed) **/
		xcb_shape_get_rectangles_cookie_t ck1 = xcb_shape_get_rectangles(_dpy->xcb(), _id, XCB_SHAPE_SK_BOUNDING);

		xcb_shape_query_extents_reply_t * r0 = xcb_shape_query_extents_reply(_dpy->xcb(), ck0, 0);
		xcb_shape_get_rectangles_reply_t * r1 = xcb_shape_get_rectangles_reply(_dpy->xcb(), ck1, 0);

		if (r0 != nullptr) {

			if (r0->bounding_shaped and r1 != nullptr) {

				_shape = new region;

				xcb_rectangle_iterator_t i =
						xcb_shape_get_rectangles_rectangles_iterator(r1);
				while (i.rem > 0) {
					*_shape += rect { i.data->x, i.data->y, i.data->width,
							i.data->height };
					xcb_rectangle_next(&i);
				}

			}
			free(r0);
		}

		if(r1 != nullptr) {
			free(r1);
		}

	}

	void set_net_wm_desktop(unsigned int n) {
		_dpy->change_property(_id, _NET_WM_DESKTOP, CARDINAL, 32, &n, 1);
		//safe_delete(__net_wm_desktop);
		__net_wm_desktop = new unsigned int{n};
	}

public:

	void print_window_attributes() {
		printf(">>> window xid: #%u\n", _id);
		printf("> size: %dx%d+%d+%d\n", _geometry->width, _geometry->height, _geometry->x, _geometry->y);
		printf("> border_width: %d\n", _geometry->border_width);
		printf("> depth: %d\n", _geometry->depth);
		printf("> visual #%u\n", _wa->visual);
		printf("> root: #%u\n", _geometry->root);
		if (_wa->_class == CopyFromParent) {
			printf("> class: CopyFromParent\n");
		} else if (_wa->_class == InputOutput) {
			printf("> class: InputOutput\n");
		} else if (_wa->_class == InputOnly) {
			printf("> class: InputOnly\n");
		} else {
			printf("> class: Unknown\n");
		}

		if (_wa->map_state == IsViewable) {
			printf("> map_state: IsViewable\n");
		} else if (_wa->map_state == IsUnviewable) {
			printf("> map_state: IsUnviewable\n");
		} else if (_wa->map_state == IsUnmapped) {
			printf("> map_state: IsUnmapped\n");
		} else {
			printf("> map_state: Unknown\n");
		}

		printf("> bit_gravity: %d\n", _wa->bit_gravity);
		printf("> win_gravity: %d\n", _wa->win_gravity);
		printf("> backing_store: %dlx\n", _wa->backing_store);
		printf("> backing_planes: %x\n", _wa->backing_planes);
		printf("> backing_pixel: %x\n", _wa->backing_pixel);
		printf("> save_under: %d\n", _wa->save_under);
		printf("> colormap: <Not Implemented>\n");
		printf("> all_event_masks: %08x\n", _wa->all_event_masks);
		printf("> your_event_mask: %08x\n", _wa->your_event_mask);
		printf("> do_not_propagate_mask: %08x\n", _wa->do_not_propagate_mask);
		printf("> override_redirect: %d\n", _wa->override_redirect);
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
				cout << _dpy->get_atom_name(x) << " ";
			}
			cout << endl;
		}

		if(_wm_colormap_windows != nullptr)
			cout << "WM_COLORMAP_WINDOWS = " << (*_wm_colormap_windows)[0] << endl;

		if(_wm_client_machine != nullptr)
			cout << "* WM_CLIENT_MACHINE = " << *_wm_client_machine << endl;

		if(_wm_state != nullptr) {
			cout << "* WM_STATE = " << _wm_state->state << endl;
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
				cout << _dpy->get_atom_name(x) << " ";
			}
			cout << endl;
		}

		if(__net_wm_state != nullptr) {
			cout << "* _NET_WM_STATE = ";
			for(auto x: *__net_wm_state) {
				cout << _dpy->get_atom_name(x) << " ";
			}
			cout << endl;
		}

		if(__net_wm_allowed_actions != nullptr) {
			cout << "* _NET_WM_ALLOWED_ACTIONS = ";
			for(auto x: *__net_wm_allowed_actions) {
				cout << _dpy->get_atom_name(x) << " ";
			}
			cout << endl;
		}

		if(__net_wm_strut != nullptr) {
			cout << "* _NET_WM_STRUCT = ";
			for(auto x: *__net_wm_strut) {
				cout << x << " ";
			}
			cout << endl;
		}

		if(__net_wm_strut_partial != nullptr) {
			cout << "* _NET_WM_PARTIAL_STRUCT = ";
			for(auto x: *__net_wm_strut_partial) {
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

	void update_type() {
		_wm_type = XCB_NONE;

		list<xcb_atom_t> net_wm_window_type;
		bool override_redirect = (_wa->override_redirect == True)?true:false;

		__net_wm_window_type.update(_dpy->xcb());
		_wm_transient_for.update(_dpy->xcb());

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
		static set<xcb_atom_t> known_type;
		if (known_type.size() == 0) {
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
				_wm_type = i;
				break;
			}
		}
	}

	xcb_atom_t wm_type() {
		if(_need_update_type) {
			_need_update_type = false;
			update_type();
		}
		return _wm_type;
	}

	display_t *          cnx() const { return _dpy; }
	xcb_window_t         id() const { return _id; }

	auto wa() const -> xcb_get_window_attributes_reply_t const * { return _wa; }
	auto geometry() const -> xcb_get_geometry_reply_t const * { return _geometry; }

	/* ICCCM */

	string const *                     wm_name() { _wm_name.update(_dpy->xcb()); return _wm_name; }
	string const *                     wm_icon_name() { _wm_icon_name.update(_dpy->xcb()); return _wm_icon_name; };
	XSizeHints const *                 wm_normal_hints() { _wm_normal_hints.update(_dpy->xcb()); return _wm_normal_hints; }
	XWMHints const *                   wm_hints() { _wm_hints.update(_dpy->xcb()); return _wm_hints; }
	vector<string> const *             wm_class() { _wm_class.update(_dpy->xcb()); return _wm_class; }
	xcb_window_t const *               wm_transient_for() { _wm_transient_for.update(_dpy->xcb()); return _wm_transient_for; }
	list<xcb_atom_t> const *           wm_protocols() { _wm_protocols.update(_dpy->xcb()); return _wm_protocols; }
	vector<xcb_window_t> const *       wm_colormap_windows() { _wm_colormap_windows.update(_dpy->xcb()); return _wm_colormap_windows; }
	string const *                     wm_client_machine() { _wm_client_machine.update(_dpy->xcb()); return _wm_client_machine; }

	/* wm_state is writen by WM */
	wm_state_data_t const *            wm_state() {_wm_state.update(_dpy->xcb()); return _wm_state; }

	/* EWMH */

	string const *                     net_wm_name() { __net_wm_name.update(_dpy->xcb()); return __net_wm_name; }
	string const *                     net_wm_visible_name() { __net_wm_visible_name.update(_dpy->xcb()); return __net_wm_visible_name; }
	string const *                     net_wm_icon_name() { __net_wm_icon_name.update(_dpy->xcb()); return __net_wm_icon_name; }
	string const *                     net_wm_visible_icon_name() { __net_wm_visible_icon_name.update(_dpy->xcb()); return __net_wm_visible_icon_name; }
	unsigned int const *               net_wm_desktop() { __net_wm_desktop.update(_dpy->xcb()); return __net_wm_desktop; }
	list<xcb_atom_t> const *           net_wm_window_type() { __net_wm_window_type.update(_dpy->xcb()); return __net_wm_window_type; }
	list<xcb_atom_t> const *           net_wm_state() { __net_wm_state.update(_dpy->xcb()); return __net_wm_state; }
	list<xcb_atom_t> const *           net_wm_allowed_actions() { __net_wm_allowed_actions.update(_dpy->xcb()); return __net_wm_allowed_actions; }
	vector<int> const *                net_wm_strut() { __net_wm_strut.update(_dpy->xcb()); return __net_wm_strut; }
	vector<int> const *                net_wm_strut_partial() { __net_wm_strut_partial.update(_dpy->xcb()); return __net_wm_strut_partial; }
	vector<int> const *                net_wm_icon_geometry() { __net_wm_icon_geometry.update(_dpy->xcb()); return __net_wm_icon_geometry; }
	vector<uint32_t> const *           net_wm_icon() { __net_wm_icon.update(_dpy->xcb()); return __net_wm_icon; }
	unsigned int const *               net_wm_pid() { __net_wm_pid.update(_dpy->xcb()); return __net_wm_pid; }
	uint32_t const *                   net_wm_user_time() { __net_wm_user_time.update(_dpy->xcb()); return __net_wm_user_time; }
	xcb_window_t const *               net_wm_user_time_window() { __net_wm_user_time_window.update(_dpy->xcb()); return __net_wm_user_time_window; }
	vector<int> const *                net_frame_extents() { __net_frame_extents.update(_dpy->xcb()); return __net_frame_extents; }
	vector<int> const *                net_wm_opaque_region() { __net_wm_opaque_region.update(_dpy->xcb()); return __net_wm_opaque_region; }
	unsigned int const *               net_wm_bypass_compositor() { __net_wm_bypass_compositor.update(_dpy->xcb()); return __net_wm_bypass_compositor; }

	/* OTHERs */
	motif_wm_hints_t const *           motif_hints() const { return _motif_hints; }

	region const *                     shape() const { return _shape; }

	void net_wm_state_add(atom_e atom) {
		if(__net_wm_state == nullptr) {
			__net_wm_state = new list<xcb_atom_t>;
		}
		/** remove it if already focused **/
		__net_wm_state->remove(A(atom));
		/** add it **/
		__net_wm_state->push_back(A(atom));
		_dpy->write_property(xid(), __net_wm_state);
	}

	void net_wm_state_remove(atom_e atom) {
		if(__net_wm_state == nullptr) {
			__net_wm_state = new list<xcb_atom_t>;
		}

		__net_wm_state->remove(A(atom));
		_dpy->write_property(xid(), __net_wm_state);
	}

	void net_wm_allowed_actions_add(atom_e atom) {
		if(__net_wm_allowed_actions == nullptr) {
			__net_wm_allowed_actions = new list<xcb_atom_t>;
		}

		__net_wm_allowed_actions->remove(A(atom));
		__net_wm_allowed_actions->push_back(A(atom));
		_dpy->write_property(xid(), __net_wm_allowed_actions);
	}

	void net_wm_allowed_actions_set(list<atom_e> atom_list) {
		__net_wm_allowed_actions = new list<xcb_atom_t>;
		for(auto i: atom_list) {
			__net_wm_allowed_actions->push_back(A(i));
		}
		_dpy->write_property(xid(), __net_wm_allowed_actions);
	}

	void set_wm_state(int state) {
		_wm_state = new wm_state_data_t;
		_wm_state->state = state;
		_wm_state->icon = None;
		_dpy->write_property(xid(), _wm_state);
	}

	void process_event(xcb_configure_notify_event_t const * e) {
		if(_wa->override_redirect != e->override_redirect) {
			_wa->override_redirect = e->override_redirect;
			update_type();
		}
		_geometry->width = e->width;
		_geometry->height = e->height;
		_geometry->x = e->x;
		_geometry->y = e->y;
		_geometry->border_width = e->border_width;
	}

	rect position() const { return rect{_geometry->x, _geometry->y, _geometry->width, _geometry->height}; }

};

}



#endif /* CLIENT_PROPERTIES_HXX_ */
