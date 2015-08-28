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
#include "display.hxx"

namespace page {

using namespace std;

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
struct property_helper_t<string> {
	static const int format = 8;

	static string * marshal(void * _tmp, int length) {
		char * tmp = reinterpret_cast<char*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		string * ret = new string{&tmp[0], &tmp[length]};
		return ret;
	}

	static void serialize(string * in, char * &data, int& length) {
		data = new char[in->size()+1];
		length = in->size()+1;
		char * tmp = reinterpret_cast<char*>(data);
		copy(in->begin(), in->end(), tmp);
		tmp[in->size()] = 0;
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

	static void serialize(vector<string> * in, char * &data, int& length) {
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
struct property_helper_t<list<int32_t>> {
	static const int format = 32;

	static list<int32_t> * marshal(void * _tmp, int length) {
		int32_t * tmp = reinterpret_cast<int32_t*>(_tmp);
		if(tmp == nullptr)
			return nullptr;
		list<int32_t> * ret = new list<int32_t>{&tmp[0], &tmp[length]};
		return ret;
	}

	static void serialize(list<int32_t> * in, char * &data, int& length) {
		data = new char[sizeof(int32_t)*in->size()];
		int32_t * tmp = reinterpret_cast<int32_t*>(data);
		copy(in->begin(), in->end(), tmp);
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

	static void serialize(list<uint32_t> * in, char * &data, int& length) {
		data = new char[sizeof(uint32_t)*in->size()];
		uint32_t * tmp = reinterpret_cast<uint32_t*>(data);
		copy(in->begin(), in->end(), tmp);
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

	static void serialize(vector<int32_t> * in, char * &data, int& length) {
		data = new char[sizeof(int32_t)*in->size()];
		int32_t * tmp = reinterpret_cast<int32_t*>(data);
		copy(in->begin(), in->end(), tmp);
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

	static void serialize(vector<uint32_t> * in, char * &data, int& length) {
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
class property_t {

	T * data;
	xcb_get_property_cookie_t ck;

	property_t & operator=(property_t const &);
	property_t(property_t const &);

public:
	using cxx_type = T;
	enum : int { x11_name = name };
	enum : int { x11_type = type };

	property_t() : data{nullptr} {
		ck.sequence = 0;
	}

	~property_t() {
		/* MUST BE RELEASED */
	}

	void fetch(xcb_connection_t * xcb, shared_ptr<atom_handler_t> const & A, xcb_window_t w) {
		release(xcb);
		ck = xcb_get_property(xcb, 0, w, (*A)(name), (*A)(type), 0, numeric_limits<uint32_t>::max());
	}

	T * update(xcb_connection_t * xcb) {
		if(ck.sequence == 0)
			return data;

		xcb_generic_error_t * err;
		xcb_get_property_reply_t * r = xcb_get_property_reply(xcb, ck, &err);

		if(err != nullptr or r == nullptr) {
			if(r != nullptr)
				free(r);
			data = nullptr;
		} else if(r->length == 0 or r->format != property_helper_t<T>::format) {
			if(r != nullptr)
				free(r);
			data = nullptr;
		} else {
			int length = xcb_get_property_value_length(r) /  (property_helper_t<T>::format / 8);
			void * tmp = (xcb_get_property_value(r));
			T * ret = property_helper_t<T>::marshal(tmp, length);
			free(r);
			data = ret;
		}

		ck.sequence = 0;
		return data;
	}

	void release(xcb_connection_t * xcb) {
		delete data;
		data = nullptr;

		if(ck.sequence != 0) {
			xcb_discard_reply(xcb, ck.sequence);
			ck.sequence = 0;
		}
	}

	T * push(xcb_connection_t * xcb, shared_ptr<atom_handler_t> const & A, xcb_window_t w, T * new_data) {
		release(xcb);

		data = new_data;
		if(data != nullptr) {
			char * xdata;
			int length;
			property_helper_t<T>::serialize(data, xdata, length);
			xcb_change_property(xcb, XCB_PROP_MODE_REPLACE, w, (*A)(name), (*A)(type), property_helper_t<T>::format, length, xdata);
			delete[] xdata;
		} else {
			xcb_delete_property(xcb, w, (*A)(name));
		}

		return data;
	}

};

#define RO_PROPERTY(cxx_name, x11_name, x11_type, cxx_type) \
using cxx_name##_t = property_t<x11_name, x11_type, cxx_type>;

#define RW_PROPERTY(cxx_name, x11_name, x11_type, cxx_type) \
using cxx_name##_t = property_t<x11_name, x11_type, cxx_type>;

#include "client_property_list.hxx"

#undef RO_PROPERTY
#undef RW_PROPERTY

/** root properties **/
using net_active_window_t =         property_t<_NET_ACTIVE_WINDOW,         WINDOW,             xcb_window_t>; // 32

}



#endif /* PROPERTIES_HXX_ */
