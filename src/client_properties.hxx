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

class client_properties_t {
private:
	display_t *                  _cnx;
	xcb_window_t                 _id;

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
		return _cnx->A(atom);
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
			_cnx(cnx), _id(id) {

		_wa = nullptr;
		_geometry = nullptr;

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
		__net_wm_strut = nullptr;
		__net_wm_strut_partial = nullptr;
		__net_wm_icon_geometry = nullptr;
		__net_wm_icon = nullptr;
		__net_wm_pid = nullptr;
		//__net_wm_handled_icons = false;
		__net_wm_user_time = nullptr;
		__net_wm_user_time_window = nullptr;
		__net_frame_extents = nullptr;
		__net_wm_opaque_region = nullptr;
		__net_wm_bypass_compositor = nullptr;

		_motif_hints = nullptr;

		_shape = nullptr;

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
		auto xcb_wm_name = display_t::make_property_fetcher_t(_wm_name, _cnx, xid());
		auto xcb_wm_icon_name = display_t::make_property_fetcher_t(_wm_icon_name, _cnx, xid());
		auto xcb_wm_normal_hints = display_t::make_property_fetcher_t(_wm_normal_hints, _cnx, xid());
		auto xcb_wm_hints = display_t::make_property_fetcher_t(_wm_hints, _cnx, xid());
		auto xcb_wm_class = display_t::make_property_fetcher_t(_wm_class, _cnx, xid());
		auto xcb_wm_transient_for = display_t::make_property_fetcher_t(_wm_transient_for, _cnx, xid());
		auto xcb_wm_protocols = display_t::make_property_fetcher_t(_wm_protocols, _cnx, xid());
		auto xcb_wm_colormap_windows = display_t::make_property_fetcher_t(_wm_colormap_windows, _cnx, xid());
		auto xcb_wm_client_machine = display_t::make_property_fetcher_t(_wm_client_machine, _cnx, xid());
		auto xcb_wm_state = display_t::make_property_fetcher_t(_wm_state, _cnx, xid());

		auto xcb_net_wm_name = display_t::make_property_fetcher_t(__net_wm_name, _cnx, xid());
		auto xcb_net_wm_visible_name = display_t::make_property_fetcher_t(__net_wm_visible_name, _cnx, xid());
		auto xcb_net_wm_icon_name = display_t::make_property_fetcher_t(__net_wm_icon_name, _cnx, xid());
		auto xcb_net_wm_visible_icon_name = display_t::make_property_fetcher_t(__net_wm_visible_icon_name, _cnx, xid());
		auto xcb_net_wm_desktop = display_t::make_property_fetcher_t(__net_wm_desktop, _cnx, xid());
		auto xcb_net_wm_window_type = display_t::make_property_fetcher_t(__net_wm_window_type, _cnx, xid());
		auto xcb_net_wm_state = display_t::make_property_fetcher_t(__net_wm_state, _cnx, xid());
		auto xcb_net_wm_allowed_actions = display_t::make_property_fetcher_t(__net_wm_allowed_actions, _cnx, xid());
		auto xcb_net_wm_strut = display_t::make_property_fetcher_t(__net_wm_strut, _cnx, xid());
		auto xcb_net_wm_strut_partial = display_t::make_property_fetcher_t(__net_wm_strut_partial, _cnx, xid());
		auto xcb_net_wm_icon_geometry = display_t::make_property_fetcher_t(__net_wm_icon_geometry, _cnx, xid());
		auto xcb_net_wm_icon = display_t::make_property_fetcher_t(__net_wm_icon, _cnx, xid());
		auto xcb_net_wm_pid = display_t::make_property_fetcher_t(__net_wm_pid, _cnx, xid());
		//auto xcb_net_wm_handled_icons = make_property_fetcher_t(_wm_state, _cnx, xid());
		auto xcb_net_wm_user_time = display_t::make_property_fetcher_t(__net_wm_user_time, _cnx, xid());
		auto xcb_net_wm_user_time_window = display_t::make_property_fetcher_t(__net_wm_user_time_window, _cnx, xid());
		auto xcb_net_frame_extents = display_t::make_property_fetcher_t(__net_frame_extents, _cnx, xid());
		auto xcb_net_wm_opaque_region = display_t::make_property_fetcher_t(__net_wm_opaque_region, _cnx, xid());
		auto xcb_net_wm_bypass_compositor = display_t::make_property_fetcher_t(__net_wm_bypass_compositor, _cnx, xid());

		xcb_wm_name.update(_cnx);
		xcb_wm_icon_name.update(_cnx);
		xcb_wm_normal_hints.update(_cnx);
		xcb_wm_hints.update(_cnx);
		xcb_wm_class.update(_cnx);
		xcb_wm_transient_for.update(_cnx);
		xcb_wm_protocols.update(_cnx);
		xcb_wm_colormap_windows.update(_cnx);
		xcb_wm_client_machine.update(_cnx);
		xcb_wm_state.update(_cnx);

		xcb_net_wm_name.update(_cnx);
		xcb_net_wm_visible_name.update(_cnx);
		xcb_net_wm_icon_name.update(_cnx);
		xcb_net_wm_visible_icon_name.update(_cnx);
		xcb_net_wm_desktop.update(_cnx);
		xcb_net_wm_window_type.update(_cnx);
		xcb_net_wm_state.update(_cnx);
		xcb_net_wm_allowed_actions.update(_cnx);
		xcb_net_wm_strut.update(_cnx);
		xcb_net_wm_strut_partial.update(_cnx);
		xcb_net_wm_icon_geometry.update(_cnx);
		xcb_net_wm_icon.update(_cnx);
		xcb_net_wm_pid.update(_cnx);
		//auto xcb_net_wm_handled_icons = make_property_fetcher_t(_wm_state, _cnx, xid());
		xcb_net_wm_user_time.update(_cnx);
		xcb_net_wm_user_time_window.update(_cnx);
		xcb_net_frame_extents.update(_cnx);
		xcb_net_wm_opaque_region.update(_cnx);
		xcb_net_wm_bypass_compositor.update(_cnx);

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

		auto ck0 = xcb_get_window_attributes(_cnx->xcb(), xid());
		auto ck1 = xcb_get_geometry(_cnx->xcb(), xid());
		_wa = xcb_get_window_attributes_reply(_cnx->xcb(), ck0, nullptr);
		_geometry = xcb_get_geometry_reply(_cnx->xcb(), ck1, nullptr);
		return _wa != nullptr and _geometry != nullptr;
	}

	void update_wm_name() {
		auto x = display_t::make_property_fetcher_t(_wm_name, _cnx, xid());
		x.update(_cnx);
	}

	void update_wm_icon_name() {
		auto x = display_t::make_property_fetcher_t(_wm_icon_name, _cnx, xid());
		x.update(_cnx);
	}

	void update_wm_normal_hints() {
		auto x = display_t::make_property_fetcher_t(_wm_normal_hints, _cnx, xid());
		x.update(_cnx);
	}

	void update_wm_hints() {
		auto x = display_t::make_property_fetcher_t(_wm_hints, _cnx, xid());
		x.update(_cnx);
	}

	void update_wm_class() {
		auto x = display_t::make_property_fetcher_t(_wm_class, _cnx, xid());
		x.update(_cnx);
		update_type();
	}

	void update_wm_transient_for() {
		auto x = display_t::make_property_fetcher_t(_wm_transient_for, _cnx, xid());
		x.update(_cnx);
		update_type();
	}

	void update_wm_protocols() {
		auto x = display_t::make_property_fetcher_t(_wm_protocols, _cnx, xid());
		x.update(_cnx);
	}

	void update_wm_colormap_windows() {
		auto x = display_t::make_property_fetcher_t(_wm_colormap_windows, _cnx, xid());
		x.update(_cnx);
	}

	void update_wm_client_machine() {
		auto x = display_t::make_property_fetcher_t(_wm_client_machine, _cnx, xid());
		x.update(_cnx);
	}

	void update_wm_state() {
		auto x = display_t::make_property_fetcher_t(_wm_state, _cnx, xid());
		x.update(_cnx);
	}

	/* EWMH */

	void update_net_wm_name() {
		auto x = display_t::make_property_fetcher_t(__net_wm_name, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_visible_name() {
		auto x = display_t::make_property_fetcher_t(__net_wm_visible_name, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_icon_name() {
		auto x = display_t::make_property_fetcher_t(__net_wm_icon_name, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_visible_icon_name() {
		auto x = display_t::make_property_fetcher_t(__net_wm_visible_icon_name, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_desktop() {
		auto x = display_t::make_property_fetcher_t(__net_wm_desktop, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_window_type() {
		auto x = display_t::make_property_fetcher_t(__net_wm_window_type, _cnx, xid());
		x.update(_cnx);
		update_type();
	}

	void update_net_wm_state() {
		auto x = display_t::make_property_fetcher_t(__net_wm_state, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_allowed_actions() {
		auto x = display_t::make_property_fetcher_t(__net_wm_allowed_actions, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_struct() {
		auto x = display_t::make_property_fetcher_t(__net_wm_strut, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_struct_partial() {
		auto x = display_t::make_property_fetcher_t(__net_wm_strut_partial, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_icon_geometry() {
		auto x = display_t::make_property_fetcher_t(__net_wm_icon_geometry, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_icon() {
		auto x = display_t::make_property_fetcher_t(__net_wm_icon, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_pid() {
		auto x = display_t::make_property_fetcher_t(__net_wm_pid, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_handled_icons();

	void update_net_wm_user_time() {
		auto x = display_t::make_property_fetcher_t(__net_wm_user_time, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_user_time_window() {
		auto x = display_t::make_property_fetcher_t(__net_wm_user_time_window, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_frame_extents() {
		auto x = display_t::make_property_fetcher_t(__net_frame_extents, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_opaque_region() {
		auto x = display_t::make_property_fetcher_t(__net_wm_opaque_region, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_bypass_compositor() {
		auto x = display_t::make_property_fetcher_t(__net_wm_bypass_compositor, _cnx, xid());
		x.update(_cnx);
	}

	void update_motif_hints() {
		auto x = display_t::make_property_fetcher_t(_motif_hints, _cnx, xid());
		x.update(_cnx);
	}

	void update_shape() {
		delete _shape;
		_shape = nullptr;

		int count;
		int ordering;

		xcb_shape_get_rectangles_cookie_t ck = xcb_shape_get_rectangles(_cnx->xcb(), _id, XCB_SHAPE_SK_BOUNDING);
		xcb_shape_get_rectangles_reply_t * r = xcb_shape_get_rectangles_reply(_cnx->xcb(), ck, 0);

		if (r != nullptr) {
			_shape = new region{};

			xcb_rectangle_iterator_t i = xcb_shape_get_rectangles_rectangles_iterator(r);
			while(i.rem > 0) {
				*_shape += i_rect{i.data->x, i.data->y, i.data->width, i.data->height};
				xcb_rectangle_next(&i);
			}
			free(r);
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

	void set_net_wm_desktop(unsigned int n) {
		_cnx->change_property(_id, _NET_WM_DESKTOP, CARDINAL, 32, &n, 1);
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
			std::cout << "* WM_NAME = " << *_wm_name << std::endl;

		if(_wm_icon_name != nullptr)
			std::cout << "* WM_ICON_NAME = " << *_wm_icon_name << std::endl;

		//if(wm_normal_hints != nullptr)
		//	std::cout << "WM_NORMAL_HINTS = " << *wm_normal_hints << std::endl;

		//if(wm_hints != nullptr)
		//	std::cout << "WM_HINTS = " << *wm_hints << std::endl;

		if(_wm_class != nullptr)
			std::cout << "* WM_CLASS = " << (*_wm_class)[0] << "," << (*_wm_class)[1] << std::endl;

		if(_wm_transient_for != nullptr)
			std::cout << "* WM_TRANSIENT_FOR = " << *_wm_transient_for << std::endl;

		if(_wm_protocols != nullptr) {
			std::cout << "* WM_PROTOCOLS = ";
			for(auto x: *_wm_protocols) {
				std::cout << _cnx->get_atom_name(x) << " ";
			}
			std::cout << std::endl;
		}

		if(_wm_colormap_windows != nullptr)
			std::cout << "WM_COLORMAP_WINDOWS = " << (*_wm_colormap_windows)[0] << std::endl;

		if(_wm_client_machine != nullptr)
			std::cout << "* WM_CLIENT_MACHINE = " << *_wm_client_machine << std::endl;

		if(_wm_state != nullptr) {
			std::cout << "* WM_STATE = " << _wm_state->state << std::endl;
		}


		/* EWMH */
		if(__net_wm_name != nullptr)
			std::cout << "* _NET_WM_NAME = " << *__net_wm_name << std::endl;

		if(__net_wm_visible_name != nullptr)
			std::cout << "* _NET_WM_VISIBLE_NAME = " << *__net_wm_visible_name << std::endl;

		if(__net_wm_icon_name != nullptr)
			std::cout << "* _NET_WM_ICON_NAME = " << *__net_wm_icon_name << std::endl;

		if(__net_wm_visible_icon_name != nullptr)
			std::cout << "* _NET_WM_VISIBLE_ICON_NAME = " << *__net_wm_visible_icon_name << std::endl;

		if(__net_wm_desktop != nullptr)
			std::cout << "* _NET_WM_DESKTOP = " << *__net_wm_desktop << std::endl;

		if(__net_wm_window_type != nullptr) {
			std::cout << "* _NET_WM_WINDOW_TYPE = ";
			for(auto x: *__net_wm_window_type) {
				std::cout << _cnx->get_atom_name(x) << " ";
			}
			std::cout << std::endl;
		}

		if(__net_wm_state != nullptr) {
			std::cout << "* _NET_WM_STATE = ";
			for(auto x: *__net_wm_state) {
				std::cout << _cnx->get_atom_name(x) << " ";
			}
			std::cout << std::endl;
		}

		if(__net_wm_allowed_actions != nullptr) {
			std::cout << "* _NET_WM_ALLOWED_ACTIONS = ";
			for(auto x: *__net_wm_allowed_actions) {
				std::cout << _cnx->get_atom_name(x) << " ";
			}
			std::cout << std::endl;
		}

		if(__net_wm_strut != nullptr) {
			std::cout << "* _NET_WM_STRUCT = ";
			for(auto x: *__net_wm_strut) {
				std::cout << x << " ";
			}
			std::cout << std::endl;
		}

		if(__net_wm_strut_partial != nullptr) {
			std::cout << "* _NET_WM_PARTIAL_STRUCT = ";
			for(auto x: *__net_wm_strut_partial) {
				std::cout << x << " ";
			}
			std::cout << std::endl;
		}

		if(__net_wm_icon_geometry != nullptr) {
			std::cout << "* _NET_WM_ICON_GEOMETRY = ";
			for(auto x: *__net_wm_icon_geometry) {
				std::cout << x << " ";
			}
			std::cout << std::endl;
		}

		if(__net_wm_icon != nullptr)
			std::cout << "* _NET_WM_ICON = " << "TODO" << std::endl;

		if(__net_wm_pid != nullptr)
			std::cout << "* _NET_WM_PID = " << *__net_wm_pid << std::endl;

		//if(_net_wm_handled_icons != false)
		//	;

		if(__net_wm_user_time != nullptr)
			std::cout << "* _NET_WM_USER_TIME = " << *__net_wm_user_time << std::endl;

		if(__net_wm_user_time_window != nullptr)
			std::cout << "* _NET_WM_USER_TIME_WINDOW = " << *__net_wm_user_time_window << std::endl;

		if(__net_frame_extents != nullptr) {
			std::cout << "* _NET_FRAME_EXTENTS = ";
			for(auto x: *__net_frame_extents) {
				std::cout << x << " ";
			}
			std::cout << std::endl;
		}

		//_net_wm_opaque_region = nullptr;
		//_net_wm_bypass_compositor = nullptr;
		//motif_hints = nullptr;
	}

	void update_type() {
		_wm_type = None;

		std::list<xcb_atom_t> net_wm_window_type;
		bool override_redirect = (_wa->override_redirect == True)?true:false;

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
		static std::set<xcb_atom_t> known_type;
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
				_wm_type = i;
				break;
			}
		}

		/** HACK FOR ECLIPSE **/
		{
			if (_wm_class != nullptr
					and __net_wm_state != nullptr
					and _wm_type == A(_NET_WM_WINDOW_TYPE_NORMAL)) {
				if ((*(_wm_class))[0] == "Eclipse") {
					if(has_key(*__net_wm_state, static_cast<xcb_atom_t>(A(_NET_WM_STATE_SKIP_TASKBAR)))) {
						_wm_type = A(_NET_WM_WINDOW_TYPE_DND);
					}
				}
			}
		}

	}

	xcb_atom_t wm_type() const { return _wm_type; }

	display_t *          cnx() const { return _cnx; }
	xcb_window_t         id() const { return _id; }

	auto wa() const -> xcb_get_window_attributes_reply_t const * { return _wa; }
	auto geometry() const -> xcb_get_geometry_reply_t const * { return _geometry; }

	/* ICCCM */

	std::string const *                     wm_name() const { return _wm_name; }
	std::string const *                     wm_icon_name() const { return _wm_icon_name; };
	XSizeHints const *                 wm_normal_hints() const { return _wm_normal_hints; }
	XWMHints const *                   wm_hints() const { return _wm_hints; }
	std::vector<std::string> const *             wm_class() const {return _wm_class; }
	xcb_window_t const *               wm_transient_for() const { return _wm_transient_for; }
	std::list<xcb_atom_t> const *           wm_protocols() const { return _wm_protocols; }
	std::vector<xcb_window_t> const *       wm_colormap_windows() const { return _wm_colormap_windows; }
	std::string const *                     wm_client_machine() const { return _wm_client_machine; }

	/* wm_state is writen by WM */
	wm_state_data_t const *            wm_state() const {return _wm_state; }

	/* EWMH */

	std::string const *                     net_wm_name() const { return __net_wm_name; }
	std::string const *                     net_wm_visible_name() const { return __net_wm_visible_name; }
	std::string const *                     net_wm_icon_name() const { return __net_wm_icon_name; }
	std::string const *                     net_wm_visible_icon_name() const { return __net_wm_visible_icon_name; }
	unsigned int const *               net_wm_desktop() const { return __net_wm_desktop; }
	std::list<xcb_atom_t> const *           net_wm_window_type() const { return __net_wm_window_type; }
	std::list<xcb_atom_t> const *           net_wm_state() const { return __net_wm_state; }
	std::list<xcb_atom_t> const *           net_wm_allowed_actions() const { return __net_wm_allowed_actions; }
	std::vector<int> const *                net_wm_strut() const { return __net_wm_strut; }
	std::vector<int> const *                net_wm_strut_partial() const { return __net_wm_strut_partial; }
	std::vector<int> const *                net_wm_icon_geometry() const { return __net_wm_icon_geometry; }
	std::vector<uint32_t> const *           net_wm_icon() const { return __net_wm_icon; }
	unsigned int const *                    net_wm_pid() const { return __net_wm_pid; }
	//bool                                  net_wm_handled_icons() const { return __net_wm_handled_icons; }
	uint32_t const *                        net_wm_user_time() const { return __net_wm_user_time; }
	xcb_window_t const *                    net_wm_user_time_window() const { return __net_wm_user_time_window; }
	std::vector<int> const *                net_frame_extents() const { return __net_frame_extents; }
	std::vector<int> const *                net_wm_opaque_region() const { return __net_wm_opaque_region; }
	unsigned int const *                    net_wm_bypass_compositor() const { return __net_wm_bypass_compositor; }

	/* OTHERs */
	motif_wm_hints_t const *           motif_hints() const { return _motif_hints; }

	region const *                     shape() const { return _shape; }

	void net_wm_state_add(atom_e atom) {
		if(__net_wm_state == nullptr) {
			__net_wm_state = new std::list<xcb_atom_t>;
		}
		/** remove it if already focused **/
		__net_wm_state->remove(A(atom));
		/** add it **/
		__net_wm_state->push_back(A(atom));
		_cnx->write_property(xid(), __net_wm_state);
	}

	void net_wm_state_remove(atom_e atom) {
		if(__net_wm_state == nullptr) {
			__net_wm_state = new std::list<xcb_atom_t>;
		}

		__net_wm_state->remove(A(atom));
		_cnx->write_property(xid(), __net_wm_state);
	}

	void net_wm_allowed_actions_add(atom_e atom) {
		if(__net_wm_allowed_actions == nullptr) {
			__net_wm_allowed_actions = new std::list<xcb_atom_t>;
		}

		__net_wm_allowed_actions->remove(A(atom));
		__net_wm_allowed_actions->push_back(A(atom));
		_cnx->write_property(xid(), __net_wm_allowed_actions);
	}

	void net_wm_allowed_actions_set(std::list<atom_e> atom_list) {
		__net_wm_allowed_actions = new std::list<xcb_atom_t>;
		for(auto i: atom_list) {
			__net_wm_allowed_actions->push_back(A(i));
		}
		_cnx->write_property(xid(), __net_wm_allowed_actions);
	}

	void set_wm_state(int state) {
		_wm_state = new wm_state_data_t;
		_wm_state->state = state;
		_wm_state->icon = None;
		_cnx->write_property(xid(), _wm_state);
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

	i_rect position() const { return i_rect{_geometry->x, _geometry->y, _geometry->width, _geometry->height}; }

};

}



#endif /* CLIENT_PROPERTIES_HXX_ */
