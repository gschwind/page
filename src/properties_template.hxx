/*
 * Copyright (2017) Benoit Gschwind
 *
 * properties_template.hxx is part of page-compositor.
 *
 * page-compositor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * page-compositor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with page-compositor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef SRC_PROPERTIES_TEMPLATE_HXX_
#define SRC_PROPERTIES_TEMPLATE_HXX_

#include "properties.hxx"

namespace page {

enum {
	p_wm_name = 0,
	p_wm_icon_name,
	p_wm_normal_hints,
	p_wm_hints,
	p_wm_class,
	p_wm_transient_for,
	p_wm_protocols,
	p_wm_colormap_windows,
	p_wm_client_machine,

	p_net_wm_name,
	p_net_wm_visible_name,
	p_net_wm_icon_name,
	p_net_wm_visible_icon_name,
	p_net_wm_window_type,
	p_net_wm_state,
	p_net_wm_allowed_actions,
	p_net_wm_strut,
	p_net_wm_strut_partial,
	p_net_wm_icon_geometry,
	p_net_wm_icon,
	p_net_wm_pid,
	p_net_wm_user_time,
	p_net_wm_user_time_window,
	p_net_frame_extents,
	p_net_wm_opaque_region,
	p_net_wm_bypass_compositor,
	p_motif_hints,

	p_net_wm_desktop,
	p_wm_state,
	properties_count
};

template <int const>
struct ptype { using type = void; };

#define DEF_PROPERTY(cxx_name, x11_name, x11_type, cxx_type) \
	template <> struct ptype<cxx_name> { using type = property_t<x11_name, x11_type, cxx_type>; };

DEF_PROPERTY(p_wm_name,                   WM_NAME,                    STRING,             string) // 8
DEF_PROPERTY(p_wm_icon_name,              WM_ICON_NAME,               STRING,             string) // 8
DEF_PROPERTY(p_wm_normal_hints,           WM_NORMAL_HINTS,            WM_SIZE_HINTS,      XSizeHints) // 32
DEF_PROPERTY(p_wm_hints,                  WM_HINTS,                   WM_HINTS,           XWMHints) // 32
DEF_PROPERTY(p_wm_class,                  WM_CLASS,                   STRING,             vector<string>) // 8
DEF_PROPERTY(p_wm_transient_for,          WM_TRANSIENT_FOR,           WINDOW,             xcb_window_t) // 32
DEF_PROPERTY(p_wm_protocols,              WM_PROTOCOLS,               ATOM,               list<xcb_atom_t>) // 32
DEF_PROPERTY(p_wm_colormap_windows,       WM_COLORMAP_WINDOWS,        WINDOW,             vector<xcb_window_t>) // 32
DEF_PROPERTY(p_wm_client_machine,         WM_CLIENT_MACHINE,          STRING,             string) // 8

DEF_PROPERTY(p_net_wm_name,               _NET_WM_NAME,               UTF8_STRING,        string) // 8
DEF_PROPERTY(p_net_wm_visible_name,       _NET_WM_VISIBLE_NAME,       UTF8_STRING,        string) // 8
DEF_PROPERTY(p_net_wm_icon_name,          _NET_WM_ICON_NAME,          UTF8_STRING,        string) // 8
DEF_PROPERTY(p_net_wm_visible_icon_name,  _NET_WM_VISIBLE_ICON_NAME,  UTF8_STRING,        string) // 8
DEF_PROPERTY(p_net_wm_window_type,        _NET_WM_WINDOW_TYPE,        ATOM,               list<xcb_atom_t>) // 32
DEF_PROPERTY(p_net_wm_state,              _NET_WM_STATE,              ATOM,               list<xcb_atom_t>) // 32
DEF_PROPERTY(p_net_wm_allowed_actions,    _NET_WM_ALLOWED_ACTIONS,    ATOM,               list<xcb_atom_t>) // 32
DEF_PROPERTY(p_net_wm_strut,              _NET_WM_STRUT,              CARDINAL,           vector<int>) // 32
DEF_PROPERTY(p_net_wm_strut_partial,      _NET_WM_STRUT_PARTIAL,      CARDINAL,           vector<int>) // 32
DEF_PROPERTY(p_net_wm_icon_geometry,      _NET_WM_ICON_GEOMETRY,      CARDINAL,           vector<int>) // 32
DEF_PROPERTY(p_net_wm_icon,               _NET_WM_ICON,               CARDINAL,           vector<uint32_t>) // 32
DEF_PROPERTY(p_net_wm_pid,                _NET_WM_PID,                CARDINAL,           unsigned int) // 32
DEF_PROPERTY(p_net_wm_user_time,          _NET_WM_USER_TIME,          CARDINAL,           unsigned int) // 32
DEF_PROPERTY(p_net_wm_user_time_window,   _NET_WM_USER_TIME_WINDOW,   WINDOW,             xcb_window_t) // 32
DEF_PROPERTY(p_net_frame_extents,         _NET_FRAME_EXTENTS,         CARDINAL,           vector<int>) // 32
DEF_PROPERTY(p_net_wm_opaque_region,      _NET_WM_OPAQUE_REGION,      CARDINAL,           vector<int>) // 32
DEF_PROPERTY(p_net_wm_bypass_compositor,  _NET_WM_BYPASS_COMPOSITOR,  CARDINAL,           unsigned int) // 32
DEF_PROPERTY(p_motif_hints,               _MOTIF_WM_HINTS,            _MOTIF_WM_HINTS,    motif_wm_hints_t) // 8

DEF_PROPERTY(p_net_wm_desktop,            _NET_WM_DESKTOP,            CARDINAL,           unsigned int) // 32
DEF_PROPERTY(p_wm_state,                  WM_STATE,                   WM_STATE,           wm_state_data_t) // 32


template <int const ID>
struct property_element_t {
	typename ptype<ID>::type props;
};

template <int const ... LIST>
struct properties_hander_t;

template<typename T, int const ... XLIST>
struct hidden_for_all_id {
	static void fetch(T * ths);
	static void del(T * ths);
	static void update(T * ths, xcb_atom_t atom);
};

template <int const ... LIST>
struct properties_hander_t : public property_element_t<LIST>... {
private:
	xcb_connection_t * xcb;
	shared_ptr<atom_handler_t> A;
	xcb_window_t w;

	template<typename T, int const ... XLIST>
	friend struct hidden_for_all_id;

public:

	properties_hander_t(xcb_connection_t * xcb, shared_ptr<atom_handler_t> A, xcb_window_t w) : xcb{xcb}, A{A}, w{w}
	{

	}

	~properties_hander_t()
	{
		release_all();
	}

	template<int const ID>
	 auto get() -> shared_ptr<typename  ptype<ID>::type::cxx_type> {
		return static_cast<property_element_t<ID> * >(this)->props.read(xcb, A, w);
	}

	template<int const ID>
	void set(shared_ptr<typename ptype<ID>::type::cxx_type> d) {
		static_cast<property_element_t<ID> * >(this)->props.push(xcb, A, w, d);
	}

	template<int const ID>
	void fetch() {
		static_cast<property_element_t<ID> * >(this)->props.fetch(xcb, A, w);
	}

	template<int const ID>
	void del() {
		static_cast<property_element_t<ID> * >(this)->props.push(xcb, A, w, nullptr);
	}

	void fetch_all() {
		hidden_for_all_id<properties_hander_t<LIST...>, LIST...>::fetch(this);
	}

	void del_all() {
		hidden_for_all_id<properties_hander_t<LIST...>, LIST...>::del(this);
	}

	void release_all() {
		hidden_for_all_id<properties_hander_t<LIST...>, LIST...>::release(this);
	}

	void update_all(xcb_atom_t atom) {
		hidden_for_all_id<properties_hander_t<LIST...>, LIST...>::update(this, atom);
	}

};


template<class T, int const ID, int const ... XLIST>
struct hidden_for_all_id<T, ID, XLIST...> {
	static void fetch(T * ths) {
		static_cast<property_element_t<ID> * >(ths)->props.fetch(ths->xcb, ths->A, ths->w);
		hidden_for_all_id<T, XLIST...>::fetch(ths);
	}
	static void del(T * ths) {
		static_cast<property_element_t<ID> * >(ths)->props.push(ths->xcb, ths->A, ths->w, nullptr);
		hidden_for_all_id<T, XLIST...>::del(ths);
	}
	static void update(T * ths, xcb_atom_t atom) {
		if((*(ths->A))(static_cast<atom_e>(ptype<ID>::type::x11_name)) == atom) {
			static_cast<property_element_t<ID> * >(ths)->props.fetch(ths->xcb, ths->A, ths->w);
			return;
		}
		hidden_for_all_id<T, XLIST...>::update(ths, atom);
	}
	static void release(T * ths) {
		static_cast<property_element_t<ID> * >(ths)->props.release(ths->xcb);
		hidden_for_all_id<T, XLIST...>::release(ths);
	}
};

template<class T, int const ID>
struct hidden_for_all_id<T, ID> {
	static void fetch(T * ths) {
		static_cast<property_element_t<ID> * >(ths)->props.fetch(ths->xcb, ths->A, ths->w);
	}
	static void del(T * ths) {
		static_cast<property_element_t<ID> * >(ths)->props.push(ths->xcb, ths->A, ths->w, nullptr);
	}
	static void update(T * ths, xcb_atom_t atom) {
		if((*(ths->A))(static_cast<atom_e>(ptype<ID>::type::x11_name)) == atom) {
			static_cast<property_element_t<ID> * >(ths)->props.fetch(ths->xcb, ths->A, ths->w);
			return;
		}
	}
	static void release(T * ths) {
		static_cast<property_element_t<ID> * >(ths)->props.release(ths->xcb);
	}
};

template<int const, typename>
struct append_to_type_seq { };

template<int const I, template<int const ...> class T, int const ... LIST>
struct append_to_type_seq<I, T<LIST...> >
{
    using type = T<LIST..., I>;
};

template<unsigned int N>
struct properties_list_t
{
    using type = typename append_to_type_seq<N-1, typename properties_list_t<N-1>::type>::type;
};

template<>
struct properties_list_t<0>
{
    using type = properties_hander_t<>;
};


using wm_properties_t = properties_list_t<properties_count>::type;

}

#endif /* SRC_PROPERTIES_TEMPLATE_HXX_ */
