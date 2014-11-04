/*
 * properties.hxx
 *
 *  Created on: 30 oct. 2014
 *      Author: gschwind
 */

#ifndef PROPERTIES_HXX_
#define PROPERTIES_HXX_

#include <X11/Xlib.h>
#include <xcb/xcb.h>

#include <algorithm>

#include "atoms.hxx"


namespace page {

struct wm_state_data_t {
	int state;
	xcb_window_t icon;
};

template<typename T>
struct property_helper_t {
	static const int format = 0;
	static T * marshal(void * _tmp, int length);
	static void serialize(T * in, char * &data, int& length);
};

template<>
struct property_helper_t<std::string> {
	static const int format = 8;

	static std::string * marshal(void * _tmp, int length) {
		char * tmp = reinterpret_cast<char*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		std::string * ret = new std::string{&tmp[0], &tmp[length]};
		return ret;
	}

	static void serialize(std::string * in, char * &data, int& length) {
		data = new char[in->size()+1];
		length = in->size()+1;
		char * tmp = reinterpret_cast<char*>(data);
		copy(in->begin(), in->end(), tmp);
		tmp[in->size()] = 0;
	}

};

template<>
struct property_helper_t<std::vector<std::string>> {
	static const int format = 8;
	static std::vector<std::string> * marshal(void * _tmp, int length) {
		char * tmp = reinterpret_cast<char*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		auto x = std::find(&tmp[0], &tmp[length], 0);
		if(x != &tmp[length]) {
			std::vector<std::string> * ret = new std::vector<std::string>;
			ret->push_back(std::string{tmp, x});
			auto x1 = std::find(++x, &tmp[length], 0);
			ret->push_back(std::string{x, x1});
			return ret;
		}
		return nullptr;
	}

	static void serialize(std::vector<std::string> * in, char * &data, int& length) {
		int size = 0;
		for(auto &i: *in) {
			size += i.size() + 1;
		}

		size += 1;

		data = new char[size];
		length = size;

		char * tmp = reinterpret_cast<char*>(data);
		char * offset = tmp;

		for(auto &i: *in) {
			offset = copy(i.begin(), i.end(), offset);
			*(offset++) = 0;
		}
		*offset = 0;
	}

};


template<>
struct property_helper_t<std::list<int32_t>> {
	static const int format = 32;

	static std::list<int32_t> * marshal(void * _tmp, int length) {
		int32_t * tmp = reinterpret_cast<int32_t*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		std::list<int32_t> * ret = new std::list<int32_t>{&tmp[0], &tmp[length]};
		return ret;
	}

	static void serialize(std::list<int32_t> * in, char * &data, int& length) {
		data = new char[sizeof(int32_t)*in->size()];
		int32_t * tmp = reinterpret_cast<int32_t*>(data);
		copy(in->begin(), in->end(), tmp);
		length = in->size();
	}

};

template<>
struct property_helper_t<std::list<uint32_t>> {
	static const int format = 32;

	static std::list<uint32_t> * marshal(void * _tmp, int length) {
		uint32_t * tmp = reinterpret_cast<uint32_t*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		std::list<uint32_t> * ret = new std::list<uint32_t>{&tmp[0], &tmp[length]};
		return ret;
	}

	static void serialize(std::list<uint32_t> * in, char * &data, int& length) {
		data = new char[sizeof(uint32_t)*in->size()];
		uint32_t * tmp = reinterpret_cast<uint32_t*>(data);
		copy(in->begin(), in->end(), tmp);
		length = in->size();
	}

};

template<>
struct property_helper_t<std::vector<int32_t>> {
	static const int format = 32;

	static std::vector<int32_t> * marshal(void * _tmp, int length) {
		int32_t * tmp = reinterpret_cast<int32_t*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		std::vector<int32_t> * ret = new std::vector<int32_t>{&tmp[0], &tmp[length]};
		return ret;
	}

	static void serialize(std::vector<int32_t> * in, char * &data, int& length) {
		data = new char[sizeof(int32_t)*in->size()];
		int32_t * tmp = reinterpret_cast<int32_t*>(data);
		copy(in->begin(), in->end(), tmp);
		length = in->size();
	}

};

template<>
struct property_helper_t<std::vector<uint32_t>> {
	static const int format = 32;

	static std::vector<uint32_t> * marshal(void * _tmp, int length) {
		uint32_t * tmp = reinterpret_cast<uint32_t*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		std::vector<uint32_t> * ret = new std::vector<uint32_t>{&tmp[0], &tmp[length]};
		return ret;
	}

	static void serialize(std::vector<uint32_t> * in, char * &data, int& length) {
		data = new char[sizeof(uint32_t)*in->size()];
		uint32_t * tmp = reinterpret_cast<uint32_t*>(data);
		copy(in->begin(), in->end(), tmp);
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

	static void serialize(int32_t * in, char * &data, int& length) {
		data = new char[sizeof(int32_t)];
		int32_t * tmp = reinterpret_cast<int32_t*>(data);
		*tmp = *in;
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

	static void serialize(uint32_t * in, char * &data, int& length) {
		data = new char[sizeof(uint32_t)];
		uint32_t * tmp = reinterpret_cast<uint32_t*>(data);
		*tmp = *in;
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

	static void serialize(XSizeHints * in, char * &data, int& length) {
		data = new char[sizeof(int32_t)*18];
		int32_t * tmp = reinterpret_cast<int32_t*>(data);
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

	static void serialize(XWMHints * in, char * &data, int& length) {
		data = new char[sizeof(int32_t)*9];
		int32_t * tmp = reinterpret_cast<int32_t*>(data);
		tmp[0] = in->flags;
		tmp[1] = in->input;
		tmp[2] = in->initial_state;
		tmp[3] = in->icon_pixmap;
		tmp[4] = in->icon_window;
		tmp[5] = in->icon_x;
		tmp[6] = in->icon_y;
		tmp[7] = in->icon_mask ;
		tmp[8] = in->window_group;

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

	static void serialize(wm_state_data_t * in, char * &data, int& length) {
		data = new char[sizeof(int32_t)*2];
		int32_t * tmp = reinterpret_cast<int32_t*>(data);
		tmp[0] = in->state;
		tmp[1] = in->icon;
		length = 2;
	}
};

template<>
struct property_helper_t<motif_wm_hints_t> {
	static const int format = 32;
	static motif_wm_hints_t * marshal(void * _tmp, int length) {
		uint32_t * tmp = reinterpret_cast<uint32_t*>(_tmp);
		if (tmp != nullptr) {
			motif_wm_hints_t * hints = new motif_wm_hints_t;
			if (length == 5) {
				if (hints != nullptr) {
					hints->flags = tmp[0];
					hints->functions = tmp[1];
					hints->decorations = tmp[2];
					hints->input_mode = tmp[3];
					hints->status = tmp[4];
				}
				return hints;
			}
		}
		return nullptr;
	}

	static void serialize(motif_wm_hints_t * in, char * &data, int& length) {
		data = new char[sizeof(int32_t)*5];
		int32_t * tmp = reinterpret_cast<int32_t*>(data);
		tmp[0] = in->flags;
		tmp[1] = in->functions;
		tmp[2] = in->decorations;
		tmp[3] = in->input_mode;
		tmp[4] = in->status;
		length = 5;
	}
};

template<atom_e name, atom_e type, typename T>
struct property_t {
	T * data;

	property_t(T * data) : data(data) {

	}

	property_t() : data(nullptr) {

	}

	~property_t() {
		delete data;
	}

	property_t & operator=(T * x) {
		delete data;
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

/** client properties **/
using wm_name_t =                   property_t<WM_NAME,                    STRING,             std::string>; // 8
using wm_icon_name_t =              property_t<WM_ICON_NAME,               STRING,             std::string>; // 8
using wm_normal_hints_t =           property_t<WM_NORMAL_HINTS,            WM_SIZE_HINTS,      XSizeHints>; // 32
using wm_hints_t =                  property_t<WM_HINTS,                   WM_HINTS,           XWMHints>; // 32
using wm_class_t =                  property_t<WM_CLASS,                   STRING,             std::vector<std::string>>; // 8
using wm_transient_for_t =          property_t<WM_TRANSIENT_FOR,           WINDOW,             xcb_window_t>; // 32
using wm_protocols_t =              property_t<WM_PROTOCOLS,               ATOM,               std::list<xcb_atom_t>>; // 32
using wm_colormap_windows_t =       property_t<WM_COLORMAP_WINDOWS,        WINDOW,             std::vector<xcb_window_t>>; // 32
using wm_client_machine_t =         property_t<WM_CLIENT_MACHINE,          STRING,             std::string>; // 8
using wm_state_t =                  property_t<WM_STATE,                   WM_STATE,           wm_state_data_t>; // 32

using net_wm_name_t =               property_t<_NET_WM_NAME,               UTF8_STRING,        std::string>; // 8
using net_wm_visible_name_t =       property_t<_NET_WM_VISIBLE_NAME,       UTF8_STRING,        std::string>; // 8
using net_wm_icon_name_t =          property_t<_NET_WM_ICON_NAME,          UTF8_STRING,        std::string>; // 8
using net_wm_visible_icon_name_t =  property_t<_NET_WM_VISIBLE_ICON_NAME,  UTF8_STRING,        std::string>; // 8
using net_wm_desktop_t =            property_t<_NET_WM_DESKTOP,            CARDINAL,           unsigned int>; // 32
using net_wm_window_type_t =        property_t<_NET_WM_WINDOW_TYPE,        ATOM,               std::list<xcb_atom_t>>; // 32
using net_wm_state_t =              property_t<_NET_WM_STATE,              ATOM,               std::list<xcb_atom_t>>; // 32
using net_wm_allowed_actions_t =    property_t<_NET_WM_ALLOWED_ACTIONS,    ATOM,               std::list<xcb_atom_t>>; // 32
using net_wm_strut_t =              property_t<_NET_WM_STRUT,              CARDINAL,           std::vector<int>>; // 32
using net_wm_strut_partial_t =      property_t<_NET_WM_STRUT_PARTIAL,      CARDINAL,           std::vector<int>>; // 32
using net_wm_icon_geometry_t =      property_t<_NET_WM_ICON_GEOMETRY,      CARDINAL,           std::vector<int>>; // 32
using net_wm_icon_t =               property_t<_NET_WM_ICON,               CARDINAL,           std::vector<uint32_t>>; // 32
using net_wm_pid_t =                property_t<_NET_WM_PID,                CARDINAL,           unsigned int>; // 32
//using net_wm_handled_icons_t =    properties_t<_NET_WM_HANDLED_ICONS,    STRING,             std::string>; // 8
using net_wm_user_time_t =          property_t<_NET_WM_USER_TIME,          CARDINAL,           unsigned int>; // 32
using net_wm_user_time_window_t =   property_t<_NET_WM_USER_TIME_WINDOW,   WINDOW,             xcb_window_t>; // 32
using net_frame_extents_t =         property_t<_NET_FRAME_EXTENTS,         CARDINAL,           std::vector<int>>; // 32
using net_wm_opaque_region_t =      property_t<_NET_WM_OPAQUE_REGION,      CARDINAL,           std::vector<int>>; // 32
using net_wm_bypass_compositor_t =  property_t<_NET_WM_BYPASS_COMPOSITOR,  CARDINAL,           unsigned int>; // 32
using motif_hints_t =               property_t<_MOTIF_WM_HINTS,            _MOTIF_WM_HINTS,    motif_wm_hints_t>; // 8

/** root properties **/
using net_active_window_t =         property_t<_NET_ACTIVE_WINDOW,         WINDOW,             xcb_window_t>; // 32

}



#endif /* PROPERTIES_HXX_ */
