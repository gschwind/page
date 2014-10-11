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

#include <array>
#include <limits>

#include "utils.hxx"
#include "motif_hints.hxx"
#include "display.hxx"
#include "tree.hxx"

namespace page {

struct wm_state_data_t {
	int state;
	xcb_window_t icon;
};


template<typename T>
struct property_helper_t {
	static const int format = 0;
	static T * marshal(void * _tmp, int length);
	static void serialize(T * in, void * &data, int& length);
};

template<>
struct property_helper_t<string> {
	static const int format = 8;

	static string * marshal(void * _tmp, int length) {
		char * tmp = reinterpret_cast<char*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		string * ret = new string{&tmp[0], &tmp[length]};
		return ret;
	}

	static void serialize(string * in, void * &data, int& length) {
		char * tmp = new char[in->size()+1];
		copy(in->begin(), in->end(), tmp);
		tmp[in->size()] = 0;
		data = tmp;
		length = in->size()+1;
	}

};

template<>
struct property_helper_t<vector<string>> {
	static const int format = 8;
	static vector<string> * marshal(void * _tmp, int length) {
		char * tmp = reinterpret_cast<char*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		auto x = find(&tmp[0], &tmp[length], 0);
		if(x != &tmp[length]) {
			vector<string> * ret = new vector<string>;
			ret->push_back(string{tmp, x});
			auto x1 = find(++x, &tmp[length], 0);
			ret->push_back(string{x, x1});
			return ret;
		}
		return nullptr;
	}

	static void serialize(vector<string> * in, void * &data, int& length) {
		int size = 0;
		for(auto &i: *in) {
			size += i.size() + 1;
		}

		size += 1;

		char * tmp = new char[size];
		char * offset = tmp;

		for(auto &i: *in) {
			offset = copy(i.begin(), i.end(), offset);
			*(offset++) = 0;
		}
		*offset = 0;

		data = tmp;
		length = size;

	}

};


template<>
struct property_helper_t<list<int32_t>> {
	static const int format = 32;

	static list<int32_t> * marshal(void * _tmp, int length) {
		int32_t * tmp = reinterpret_cast<int32_t*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		list<int32_t> * ret = new list<int32_t>{&tmp[0], &tmp[length]};
		return ret;
	}

	static void serialize(list<int32_t> * in, void * &data, int& length) {
		int32_t * tmp = new int32_t[in->size()];
		copy(in->begin(), in->end(), tmp);
		data = tmp;
		length = in->size();
	}

};

template<>
struct property_helper_t<list<uint32_t>> {
	static const int format = 32;

	static list<uint32_t> * marshal(void * _tmp, int length) {
		uint32_t * tmp = reinterpret_cast<uint32_t*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		list<uint32_t> * ret = new list<uint32_t>{&tmp[0], &tmp[length]};
		return ret;
	}

	static void serialize(list<uint32_t> * in, void * &data, int& length) {
		uint32_t * tmp = new uint32_t[in->size()];
		copy(in->begin(), in->end(), tmp);
		data = tmp;
		length = in->size();
	}

};

template<>
struct property_helper_t<vector<int32_t>> {
	static const int format = 32;

	static vector<int32_t> * marshal(void * _tmp, int length) {
		int32_t * tmp = reinterpret_cast<int32_t*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		vector<int32_t> * ret = new vector<int32_t>{&tmp[0], &tmp[length]};
		return ret;
	}

	static void serialize(vector<int32_t> * in, void * &data, int& length) {
		int32_t * tmp = new int32_t[in->size()];
		copy(in->begin(), in->end(), tmp);
		data = tmp;
		length = in->size();
	}

};

template<>
struct property_helper_t<vector<uint32_t>> {
	static const int format = 32;

	static vector<uint32_t> * marshal(void * _tmp, int length) {
		uint32_t * tmp = reinterpret_cast<uint32_t*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		vector<uint32_t> * ret = new vector<uint32_t>{&tmp[0], &tmp[length]};
		return ret;
	}

	static void serialize(vector<uint32_t> * in, void * &data, int& length) {
		uint32_t * tmp = new uint32_t[in->size()];
		copy(in->begin(), in->end(), tmp);
		data = tmp;
		length = in->size();
	}

};

template<>
struct property_helper_t<int32_t> {
	static const int format = 32;
	static int32_t * marshal(void * _tmp, int length) {
		int32_t * tmp = reinterpret_cast<int32_t*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		if(length == 0) {
			return nullptr;
		}

		if(length != 1) {
			printf("WARNING: property length (%d) unexpected\n", length);
		}

		int32_t * ret = new int32_t;
		*ret = tmp[0];
		return ret;
	}

	static void serialize(int32_t * in, void * &data, int& length) {
		int32_t * tmp = new int32_t;
		*tmp = *in;
		data = tmp;
		length = 1;
	}

};

template<>
struct property_helper_t<uint32_t> {
	static const int format = 32;
	static uint32_t * marshal(void * _tmp, int length) {
		uint32_t * tmp = reinterpret_cast<uint32_t*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		if(length == 0) {
			return nullptr;
		}

		if(length != 1) {
			printf("WARNING: property length (%d) unexpected\n", length);
		}

		uint32_t * ret = new uint32_t;
		*ret = tmp[0];
		return ret;
	}

	static void serialize(uint32_t * in, void * &data, int& length) {
		uint32_t * tmp = new uint32_t;
		*tmp = *in;
		data = tmp;
		length = 1;
	}

};


template<>
struct property_helper_t<XSizeHints> {
	static const int format = 32;
	static XSizeHints * marshal(void * _tmp, int length) {
		int32_t * tmp = reinterpret_cast<int *>(_tmp);
		if (tmp != nullptr) {
			if (length == 18) {
				XSizeHints * size_hints = new XSizeHints;
				if (size_hints) {
					size_hints->flags = tmp[0];
					size_hints->x = tmp[1];
					size_hints->y = tmp[2];
					size_hints->width = tmp[3];
					size_hints->height = tmp[4];
					size_hints->min_width = tmp[5];
					size_hints->min_height = tmp[6];
					size_hints->max_width = tmp[7];
					size_hints->max_height = tmp[8];
					size_hints->width_inc = tmp[9];
					size_hints->height_inc = tmp[10];
					size_hints->min_aspect.x = tmp[11];
					size_hints->min_aspect.y = tmp[12];
					size_hints->max_aspect.x = tmp[13];
					size_hints->max_aspect.y = tmp[14];
					size_hints->base_width = tmp[15];
					size_hints->base_height = tmp[16];
					size_hints->win_gravity = tmp[17];
					return size_hints;
				}
			}
		}
		return nullptr;
	}

	static void serialize(XSizeHints * in, void * &data, int& length) {
		int32_t * tmp = new int32_t[18];
		tmp[0] = in->flags;
		tmp[1] = in->x;
		tmp[2] = in->y;
		tmp[3] = in->width;
		tmp[4] = in->height;
		tmp[5] = in->min_width;
		tmp[6] = in->min_height;
		tmp[7] = in->max_width;
		tmp[8] = in->max_height;
		tmp[9] = in->width_inc;
		tmp[10] = in->height_inc;
		tmp[11] = in->min_aspect.x;
		tmp[12] = in->min_aspect.y;
		tmp[13] = in->max_aspect.x;
		tmp[14] = in->max_aspect.y;
		tmp[15] = in->base_width;
		tmp[16] = in->base_height;
		tmp[17] = in->win_gravity;

		data = tmp;
		length = 18;
	}

};

template<>
struct property_helper_t<XWMHints> {
	static const int format = 32;
	static XWMHints * marshal(void * _tmp, int length) {
		int32_t * tmp = reinterpret_cast<int32_t *>(_tmp);
		if (tmp != nullptr) {
			if (length == 9) {
				XWMHints * hints = new XWMHints;
				if (hints != nullptr) {
					hints->flags = tmp[0];
					hints->input = tmp[1];
					hints->initial_state = tmp[2];
					hints->icon_pixmap = tmp[3];
					hints->icon_window = tmp[4];
					hints->icon_x = tmp[5];
					hints->icon_y = tmp[6];
					hints->icon_mask = tmp[7];
					hints->window_group = tmp[8];
					return hints;
				}
			}
		}
		return nullptr;
	}

	static void serialize(XWMHints * in, void * &data, int& length) {
		int32_t * tmp = new int32_t[9];
		tmp[0] = in->flags;
		tmp[1] = in->input;
		tmp[2] = in->initial_state;
		tmp[3] = in->icon_pixmap;
		tmp[4] = in->icon_window;
		tmp[5] = in->icon_x;
		tmp[6] = in->icon_y;
		tmp[7] = in->icon_mask ;
		tmp[8] = in->window_group;

		data = tmp;
		length = 9;
	}
};

template<>
struct property_helper_t<wm_state_data_t> {
	static const int format = 32;
	static wm_state_data_t * marshal(void * _tmp, int length) {
		int32_t * tmp = reinterpret_cast<int32_t *>(_tmp);
		if (tmp != nullptr) {
			if (length == 2) {
				wm_state_data_t * data = new wm_state_data_t;
				if (data != nullptr) {
					data->state = tmp[0];
					data->icon = tmp[1];
					return data;
				}
			}
		}
		return nullptr;
	}

	static void serialize(wm_state_data_t * in, void * &data, int& length) {
		int32_t * tmp = new int32_t[9];
		tmp[0] = in->state;
		tmp[1] = in->icon;
		data = tmp;
		length = 2;
	}
};

template<atom_e name, atom_e type, typename T>
struct property_t {
	T * data;

	property_t() : data(nullptr) {

	}

	~property_t() {
		if(data != nullptr) {
			delete data;
		}
	}

	property_t & operator=(T * x) {
		if(data != nullptr) {
			delete data;
		}
		data = x;
		return *this;
	}

	operator T*() {
		return data;
	}

	operator T const *() const {
		return data;
	}

	T * operator->() {
		return data;
	}

	T const * operator->() const {
		return data;
	}

	T & operator*() {
		return *data;
	}

	T const & operator*() const {
		return *data;
	}

};

template<atom_e name, atom_e type, typename xT >
struct properties_fetcher_t {

	property_t<name, type, xT> & p;
	xcb_get_property_cookie_t ck;
	properties_fetcher_t(property_t<name, type, xT> & p, display_t * cnx, xcb_window_t w) : p(p) {
		ck = xcb_get_property(cnx->xcb(), 0, static_cast<xcb_window_t>(w), cnx->A(name), cnx->A(type), 0, numeric_limits<uint32_t>::max());
	}

	void update(display_t * cnx) {
		xcb_generic_error_t * err;
		xcb_get_property_reply_t * r = xcb_get_property_reply(cnx->xcb(), ck, &err);

		if(err != nullptr or r == nullptr) {
			if(r != nullptr)
				free(r);
			p = nullptr;
		} else if(r->length == 0 or r->format != property_helper_t<xT>::format) {
			if(r != nullptr)
				free(r);
			p = nullptr;
		} else {
			int length = xcb_get_property_value_length(r) /  (property_helper_t<xT>::format / 8);
			void * tmp = (xcb_get_property_value(r));
			xT * ret = property_helper_t<xT>::marshal(tmp, length);
			free(r);
			p = ret;
		}
	}

};

template<atom_e name, atom_e type, typename xT >
void write_property(display_t * cnx, xcb_window_t w, property_t<name, type, xT> & p) {
	void * data;
	int length;
	property_helper_t<xT>::serialize(p.data, data, length);
	xcb_change_property(cnx->xcb(), XCB_PROP_MODE_REPLACE, w, name, type, property_helper_t<xT>::format, length, data);

}

template<atom_e name, atom_e type, typename xT>
inline properties_fetcher_t<name, type, xT> make_property_fetcher_t(property_t<name, type, xT> & p, display_t * cnx, xcb_window_t w) {
	return properties_fetcher_t<name, type, xT>{p, cnx, w};
}

using wm_name_t =                   property_t<WM_NAME,                    STRING,             string>; // 8
using wm_icon_name_t =              property_t<WM_ICON_NAME,               STRING,             string>; // 8
using wm_normal_hints_t =           property_t<WM_NORMAL_HINTS,            WM_SIZE_HINTS,      XSizeHints>; // 32
using wm_hints_t =                  property_t<WM_HINTS,                   WM_HINTS,           XWMHints>; // 32
using wm_class_t =                  property_t<WM_CLASS,                   STRING,             vector<string>>; // 8
using wm_transient_for_t =          property_t<WM_TRANSIENT_FOR,           WINDOW,             xcb_window_t>; // 32
using wm_protocols_t =              property_t<WM_PROTOCOLS,               ATOM,               list<xcb_atom_t>>; // 32
using wm_colormap_windows_t =       property_t<WM_COLORMAP_WINDOWS,        WINDOW,             vector<xcb_window_t>>; // 32
using wm_client_machine_t =         property_t<WM_CLIENT_MACHINE,          STRING,             string>; // 8
using wm_state_t =                  property_t<WM_STATE,                   WM_STATE,           wm_state_data_t>; // 32

using net_wm_name_t =               property_t<_NET_WM_NAME,               UTF8_STRING,        string>; // 8
using net_wm_visible_name_t =       property_t<_NET_WM_VISIBLE_NAME,       UTF8_STRING,        string>; // 8
using net_wm_icon_name_t =          property_t<_NET_WM_ICON_NAME,          UTF8_STRING,        string>; // 8
using net_wm_visible_icon_name_t =  property_t<_NET_WM_VISIBLE_ICON_NAME,  UTF8_STRING,        string>; // 8
using net_wm_desktop_t =            property_t<_NET_WM_DESKTOP,            CARDINAL,           unsigned int>; // 32
using net_wm_window_type_t =        property_t<_NET_WM_WINDOW_TYPE,        ATOM,               list<xcb_atom_t>>; // 32
using net_wm_state_t =              property_t<_NET_WM_STATE,              ATOM,               list<xcb_atom_t>>; // 32
using net_wm_allowed_actions_t =    property_t<_NET_WM_ALLOWED_ACTIONS,    ATOM,               list<xcb_atom_t>>; // 32
using net_wm_strut_t =              property_t<_NET_WM_STRUT,              CARDINAL,           vector<int>>; // 32
using net_wm_strut_partial_t =      property_t<_NET_WM_STRUT_PARTIAL,      CARDINAL,           vector<int>>; // 32
using net_wm_icon_geometry_t =      property_t<_NET_WM_ICON_GEOMETRY,      CARDINAL,           vector<int>>; // 32
using net_wm_icon_t =               property_t<_NET_WM_ICON,               CARDINAL,           vector<int>>; // 32
using net_wm_pid_t =                property_t<_NET_WM_PID,                CARDINAL,           unsigned int>; // 32
//using net_wm_handled_icons_t =    properties_t<_NET_WM_HANDLED_ICONS,    STRING,             string>; // 8
using net_wm_user_time_t =          property_t<_NET_WM_USER_TIME,          CARDINAL,           unsigned int>; // 32
using net_wm_user_time_window_t =   property_t<_NET_WM_USER_TIME_WINDOW,   WINDOW,             xcb_window_t>; // 32
using net_frame_extents_t =         property_t<_NET_FRAME_EXTENTS,         CARDINAL,           vector<int>>; // 32
using net_wm_opaque_region_t =      property_t<_NET_WM_OPAQUE_REGION,      CARDINAL,           vector<int>>; // 32
using net_wm_bypass_compositor_t =  property_t<_NET_WM_BYPASS_COMPOSITOR,  CARDINAL,           unsigned int>; // 32
//using motif_hints_t =             properties_t<WM_MOTIF_HINTS,           STRING,             string>; // 8

using namespace std;

class client_properties_t {
private:
	display_t *                  _cnx;
	xcb_window_t                 _id;

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
	motif_wm_hints_t *           _motif_hints;

	region *                     _shape;

	/** short cut **/
	Atom A(atom_e atom) {
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
		auto xcb_wm_name = make_property_fetcher_t(_wm_name, _cnx, xid());
		auto xcb_wm_icon_name = make_property_fetcher_t(_wm_icon_name, _cnx, xid());
		auto xcb_wm_normal_hints = make_property_fetcher_t(_wm_normal_hints, _cnx, xid());
		auto xcb_wm_hints = make_property_fetcher_t(_wm_hints, _cnx, xid());
		auto xcb_wm_class = make_property_fetcher_t(_wm_class, _cnx, xid());
		auto xcb_wm_transient_for = make_property_fetcher_t(_wm_transient_for, _cnx, xid());
		auto xcb_wm_protocols = make_property_fetcher_t(_wm_protocols, _cnx, xid());
		auto xcb_wm_colormap_windows = make_property_fetcher_t(_wm_colormap_windows, _cnx, xid());
		auto xcb_wm_client_machine = make_property_fetcher_t(_wm_client_machine, _cnx, xid());
		auto xcb_wm_state = make_property_fetcher_t(_wm_state, _cnx, xid());

		auto xcb_net_wm_name = make_property_fetcher_t(__net_wm_name, _cnx, xid());
		auto xcb_net_wm_visible_name = make_property_fetcher_t(__net_wm_visible_name, _cnx, xid());
		auto xcb_net_wm_icon_name = make_property_fetcher_t(__net_wm_icon_name, _cnx, xid());
		auto xcb_net_wm_visible_icon_name = make_property_fetcher_t(__net_wm_visible_icon_name, _cnx, xid());
		auto xcb_net_wm_desktop = make_property_fetcher_t(__net_wm_desktop, _cnx, xid());
		auto xcb_net_wm_window_type = make_property_fetcher_t(__net_wm_window_type, _cnx, xid());
		auto xcb_net_wm_state = make_property_fetcher_t(__net_wm_state, _cnx, xid());
		auto xcb_net_wm_allowed_actions = make_property_fetcher_t(__net_wm_allowed_actions, _cnx, xid());
		auto xcb_net_wm_strut = make_property_fetcher_t(__net_wm_strut, _cnx, xid());
		auto xcb_net_wm_strut_partial = make_property_fetcher_t(__net_wm_strut_partial, _cnx, xid());
		auto xcb_net_wm_icon_geometry = make_property_fetcher_t(__net_wm_icon_geometry, _cnx, xid());
		auto xcb_net_wm_icon = make_property_fetcher_t(__net_wm_icon, _cnx, xid());
		auto xcb_net_wm_pid = make_property_fetcher_t(__net_wm_pid, _cnx, xid());
		//auto xcb_net_wm_handled_icons = make_property_fetcher_t(_wm_state, _cnx, xid());
		auto xcb_net_wm_user_time = make_property_fetcher_t(__net_wm_user_time, _cnx, xid());
		auto xcb_net_wm_user_time_window = make_property_fetcher_t(__net_wm_user_time_window, _cnx, xid());
		auto xcb_net_frame_extents = make_property_fetcher_t(__net_frame_extents, _cnx, xid());
		auto xcb_net_wm_opaque_region = make_property_fetcher_t(__net_wm_opaque_region, _cnx, xid());
		auto xcb_net_wm_bypass_compositor = make_property_fetcher_t(__net_wm_bypass_compositor, _cnx, xid());

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

	}

	void delete_all_properties() {

		safe_delete(_motif_hints);

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
		//safe_delete(_wm_name);
		_wm_name = _cnx->read_wm_name(_id);
	}

	void update_wm_icon_name() {
		//safe_delete(_wm_icon_name);
		_wm_icon_name = _cnx->read_wm_icon_name(_id);
	}

	void update_wm_normal_hints() {
		//safe_delete(_wm_normal_hints);
		_wm_normal_hints = _cnx->read_wm_normal_hints(_id);
	}

	void update_wm_hints() {
		//safe_delete(_wm_hints);
		_wm_hints = _cnx->read_wm_hints(_id);
	}

	void update_wm_class() {
		//safe_delete(_wm_class);
		_wm_class = _cnx->read_wm_class(_id);
	}

	void update_wm_transient_for() {
		auto x = make_property_fetcher_t(_wm_transient_for, _cnx, xid());
		x.update(_cnx);
	}

	void update_wm_protocols() {
		auto x = make_property_fetcher_t(_wm_protocols, _cnx, xid());
		x.update(_cnx);
	}

	void update_wm_colormap_windows() {
		auto x = make_property_fetcher_t(_wm_colormap_windows, _cnx, xid());
		x.update(_cnx);
	}

	void update_wm_client_machine() {
		//safe_delete(_wm_client_machine);
		_wm_client_machine = _cnx->read_wm_client_machine(_id);
	}

	void update_wm_state() {
		auto x = make_property_fetcher_t(_wm_state, _cnx, xid());
		x.update(_cnx);
	}

	/* EWMH */

	void update_net_wm_name() {
		//safe_delete(__net_wm_name);
		__net_wm_name = _cnx->read_net_wm_name(_id);
	}

	void update_net_wm_visible_name() {
		//safe_delete(__net_wm_visible_name);
		__net_wm_visible_name = _cnx->read_net_wm_visible_name(_id);
	}

	void update_net_wm_icon_name() {
		//safe_delete(__net_wm_icon_name);
		__net_wm_icon_name = _cnx->read_net_wm_icon_name(_id);
	}

	void update_net_wm_visible_icon_name() {
		//safe_delete(__net_wm_visible_icon_name);
		__net_wm_visible_icon_name = _cnx->read_net_wm_visible_icon_name(_id);
	}

	void update_net_wm_desktop() {
		auto x = make_property_fetcher_t(__net_wm_desktop, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_window_type() {
		auto x = make_property_fetcher_t(__net_wm_window_type, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_state() {
		auto x = make_property_fetcher_t(__net_wm_state, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_allowed_actions() {
		auto x = make_property_fetcher_t(__net_wm_allowed_actions, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_struct() {
		auto x = make_property_fetcher_t(__net_wm_strut, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_struct_partial() {
		auto x = make_property_fetcher_t(__net_wm_strut_partial, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_icon_geometry() {
		auto x = make_property_fetcher_t(__net_wm_icon_geometry, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_icon() {
		auto x = make_property_fetcher_t(__net_wm_icon, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_pid() {
		auto x = make_property_fetcher_t(__net_wm_pid, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_handled_icons();

	void update_net_wm_user_time() {
		auto x = make_property_fetcher_t(__net_wm_user_time, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_user_time_window() {
		auto x = make_property_fetcher_t(__net_wm_user_time_window, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_frame_extents() {
		auto x = make_property_fetcher_t(__net_frame_extents, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_opaque_region() {
		auto x = make_property_fetcher_t(__net_wm_opaque_region, _cnx, xid());
		x.update(_cnx);
	}

	void update_net_wm_bypass_compositor() {
		auto x = make_property_fetcher_t(__net_wm_bypass_compositor, _cnx, xid());
		x.update(_cnx);
	}

	void update_motif_hints() {
		//safe_delete(_motif_hints);
		_motif_hints = _cnx->read_motif_wm_hints(_id);
	}

	void update_shape() {
		//safe_delete(_shape);

		int count;
		int ordering;
		XRectangle * rect = XShapeGetRectangles(_cnx->dpy(), _id, ShapeBounding, &count, &ordering);

		if (rect != NULL) {
			_shape = new region{};
			for (unsigned i = 0; i < count; ++i) {
				*_shape += i_rect(rect[i]);
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

	void set_net_wm_desktop(unsigned int n) {
		_cnx->change_property(_id, _NET_WM_DESKTOP, CARDINAL, 32, &n, 1);
		//safe_delete(__net_wm_desktop);
		__net_wm_desktop = new unsigned int{n};
	}

public:

	void print_window_attributes() {
		printf(">>> Window: #%u\n", _id);
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
				cout << _cnx->get_atom_name(x) << " ";
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

	Atom type() {
		Atom type = None;

		list<xcb_atom_t> net_wm_window_type;
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
		static set<xcb_atom_t> known_type;
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
					if(has_key(*__net_wm_state, static_cast<xcb_atom_t>(A(_NET_WM_STATE_SKIP_TASKBAR)))) {
						type = A(_NET_WM_WINDOW_TYPE_DND);
					}
				}
			}
		}

		return type;

	}

	display_t *          cnx() const { return _cnx; }
	xcb_window_t         id() const { return _id; }

	auto wa() const -> xcb_get_window_attributes_reply_t const * { return _wa; }
	auto geometry() const -> xcb_get_geometry_reply_t const * { return _geometry; }

	/* ICCCM */

	string const *                     wm_name() const { return _wm_name; }
	string const *                     wm_icon_name() const { return _wm_icon_name; };
	XSizeHints const *                 wm_normal_hints() const { return _wm_normal_hints; }
	XWMHints const *                   wm_hints() const { return _wm_hints; }
	vector<string> const *             wm_class() const {return _wm_class; }
	xcb_window_t const *               wm_transient_for() const { return _wm_transient_for; }
	list<xcb_atom_t> const *           wm_protocols() const { return _wm_protocols; }
	vector<xcb_window_t> const *       wm_colormap_windows() const { return _wm_colormap_windows; }
	string const *                     wm_client_machine() const { return _wm_client_machine; }

	/* wm_state is writen by WM */
	wm_state_data_t const *            wm_state() const {return _wm_state; }

	/* EWMH */

	string const *                     net_wm_name() const { return __net_wm_name; }
	string const *                     net_wm_visible_name() const { return __net_wm_visible_name; }
	string const *                     net_wm_icon_name() const { return __net_wm_icon_name; }
	string const *                     net_wm_visible_icon_name() const { return __net_wm_visible_icon_name; }
	unsigned int const *               net_wm_desktop() const { return __net_wm_desktop; }
	list<xcb_atom_t> const *           net_wm_window_type() const { return __net_wm_window_type; }
	list<xcb_atom_t> const *           net_wm_state() const { return __net_wm_state; }
	list<xcb_atom_t> const *           net_wm_allowed_actions() const { return __net_wm_allowed_actions; }
	vector<int> const *                net_wm_struct() const { return __net_wm_strut; }
	vector<int> const *                net_wm_struct_partial() const { return __net_wm_strut_partial; }
	vector<int> const *                net_wm_icon_geometry() const { return __net_wm_icon_geometry; }
	vector<int> const *                net_wm_icon() const { return __net_wm_icon; }
	unsigned int const *               net_wm_pid() const { return __net_wm_pid; }
	//bool                               net_wm_handled_icons() const { return __net_wm_handled_icons; }
	uint32_t const *                   net_wm_user_time() const { return __net_wm_user_time; }
	xcb_window_t const *               net_wm_user_time_window() const { return __net_wm_user_time_window; }
	vector<int> const *                net_frame_extents() const { return __net_frame_extents; }
	vector<int> const *                net_wm_opaque_region() const { return __net_wm_opaque_region; }
	unsigned int const *               net_wm_bypass_compositor() const { return __net_wm_bypass_compositor; }

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
		write_property(_cnx, xid(), __net_wm_state);
	}

	void net_wm_state_remove(atom_e atom) {
		if(__net_wm_state == nullptr) {
			__net_wm_state = new list<xcb_atom_t>;
		}

		__net_wm_state->remove(A(atom));
		write_property(_cnx, xid(), __net_wm_state);
	}

	void net_wm_allowed_actions_add(atom_e atom) {
		if(__net_wm_allowed_actions == nullptr) {
			__net_wm_allowed_actions = new list<xcb_atom_t>;
		}

		__net_wm_allowed_actions->remove(A(atom));
		__net_wm_allowed_actions->push_back(A(atom));
		write_property(_cnx, xid(), __net_wm_allowed_actions);
	}

	void process_event(XConfigureEvent const & e) {
		_wa->override_redirect = e.override_redirect;
		_geometry->width = e.width;
		_geometry->height = e.height;
		_geometry->x = e.x;
		_geometry->y = e.y;
		_geometry->border_width = e.border_width;
	}

	i_rect position() const { return i_rect{_geometry->x, _geometry->y, _geometry->width, _geometry->height}; }

};

}



#endif /* CLIENT_PROPERTIES_HXX_ */
