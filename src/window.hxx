/*
 * window.hxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#ifndef WINDOW_HXX_
#define WINDOW_HXX_

#include <X11/X.h>
#include <X11/Xutil.h>
#include <set>
#include <map>
#include <string>
#include <vector>

#include "atoms.hxx"
#include "region.hxx"

using namespace std;

namespace page {

class window_t {

	/* avoid copy */
	window_t(window_t const &);
	window_t & operator=(window_t const &);

public:

	window_t(Window w) :
			id(w) {

		_wm_hints.value = XAllocWMHints();
		_wm_state.value = WithdrawnState;
		_net_wm_desktop.value = 0;
		_wm_transient_for.value = None;
		_net_wm_user_time.value = 0;

	}
	~window_t();

	struct wm_class_t {
		string res_class;
		string res_name;
	};

	typedef std::set<Atom> atom_set_t;
	typedef std::list<Atom> atom_list_t;
	typedef Window window_key_t;
	typedef std::map<window_key_t, window_t *> window_map_t;

	Window const id;

	template<typename T> struct _cache_data_t {
		bool is_durty;
		bool has_value;
		T value;

		_cache_data_t() {
			is_durty = true;
			has_value = false;
		}

	};

	_cache_data_t<XWindowAttributes> _window_attributes;

	_cache_data_t<wm_class_t> _wm_class;
	_cache_data_t<long> _wm_state;
	_cache_data_t<string> _wm_name;
	_cache_data_t<Window> _wm_transient_for;
	_cache_data_t<XSizeHints> _wm_normal_hints;
	_cache_data_t<XWMHints *> _wm_hints;

	_cache_data_t<string> _net_wm_name;
	_cache_data_t<atom_list_t> _net_wm_window_type;
	_cache_data_t<atom_list_t> _net_wm_state;
	_cache_data_t<atom_list_t> _net_wm_protocols;
	_cache_data_t<atom_list_t> _net_wm_allowed_actions;
	_cache_data_t<vector<long> > _net_wm_partial_struct;
	_cache_data_t<long> _net_wm_desktop;
	_cache_data_t<long> _net_wm_user_time;
	_cache_data_t<vector<long> > _net_wm_icon;

	_cache_data_t<region_t<int> > _shape_region;


	void mark_durty_window_attributes() {
		_window_attributes.is_durty = true;
	}

	void mark_durty_wm_class() {
		_wm_class.is_durty = true;
	}

	void mark_durty_wm_name() {
		_wm_name.is_durty = true;
	}

	void mark_durty_wm_state() {
		_wm_state.is_durty = true;
	}

	void mark_durty_wm_transient_for() {
		_wm_transient_for.is_durty = true;
	}

	void mark_durty_wm_normal_hints() {
		_wm_normal_hints.is_durty = true;
	}

	void mark_durty_wm_hints() {
		_wm_hints.is_durty = true;
	}

	void mark_durty_net_wm_name() {
		_net_wm_name.is_durty = true;
	}

	void mark_durty_net_wm_type() {
		_net_wm_window_type.is_durty = true;
	}

	void mark_durty_net_wm_state() {
		_net_wm_state.is_durty = true;
	}

	void mark_durty_net_wm_protocols() {
		_net_wm_protocols.is_durty = true;
	}

	void mark_durty_net_wm_allowed_actions() {
		_net_wm_allowed_actions.is_durty = true;
	}

	void mark_durty_net_wm_partial_struct() {
		_net_wm_partial_struct.is_durty = true;
	}

	void mark_durty_net_wm_desktop() {
		_net_wm_desktop.is_durty = true;
	}

	void mark_durty_net_wm_user_time() {
		_net_wm_user_time.is_durty = true;
	}

	void mark_durty_net_wm_icon() {
		_net_wm_icon.is_durty = true;
	}

	void mark_durty_shape_region() {
		_shape_region.is_durty = true;
	}

	void mark_durty_all() {
		mark_durty_window_attributes();
		mark_durty_wm_class();
		mark_durty_wm_name();
		mark_durty_wm_state();
		mark_durty_wm_transient_for();
		mark_durty_wm_normal_hints();
		mark_durty_wm_hints();
		mark_durty_net_wm_name();
		mark_durty_net_wm_type();
		mark_durty_net_wm_state();
		mark_durty_net_wm_protocols();
		mark_durty_net_wm_allowed_actions();
		mark_durty_net_wm_partial_struct();
		mark_durty_net_wm_desktop();
		mark_durty_net_wm_user_time();
		mark_durty_net_wm_icon();
		mark_durty_shape_region();
	}

	XWindowAttributes const & get_window_attributes() {
		return _window_attributes.value;
	}

	wm_class_t const & get_wm_class() {
		return _wm_class.value;
	}

	unsigned long const get_wm_state() {
		return _wm_state.value;
	}

	string const & get_wm_name()  {
		return _wm_name.value;
	}

	Window const & get_wm_transient_for()  {
		return _wm_transient_for.value;
	}

	XSizeHints const & get_wm_normal_hints()  {
		return _wm_normal_hints.value;
	}

	XWMHints const * get_wm_hints()  {
		return _wm_hints.value;
	}

	string const & get_net_wm_name()  {
		return _net_wm_name.value;
	}

	atom_list_t const & get_net_wm_window_type()  {
		return _net_wm_window_type.value;
	}

	atom_list_t const & get_net_wm_state()  {
		return _net_wm_state.value;
	}

	atom_list_t const & get_net_wm_protocols()  {
		return _net_wm_protocols.value;
	}

	vector<long> const & get_net_wm_partial_struct()  {
		return _net_wm_partial_struct.value;
	}

	long const & get_net_wm_desktop()  {
		return _net_wm_desktop.value;
	}

	long const & get_net_wm_user_time()  {
		return _net_wm_user_time.value;
	}

	vector<long> const & get_net_wm_icon()  {
		return _net_wm_icon.value;
	}

	region_t<int> const & get_shape_region()  {
		return _shape_region.value;
	}

	bool has_window_attributes() {
		return _window_attributes.has_value;
	}

	bool has_wm_class() {
		return _wm_class.has_value;
	}

	bool has_wm_state() {
		return _wm_state.has_value;
	}

	bool has_wm_name()  {
		return _wm_name.has_value;
	}

	bool has_wm_transient_for()  {
		return _wm_transient_for.has_value;
	}

	bool has_wm_normal_hints()  {
		return _wm_normal_hints.has_value;
	}

	bool has_wm_hints()  {
		return _wm_hints.has_value;
	}

	bool has_net_wm_name()  {
		return _net_wm_name.has_value;
	}

	bool has_net_wm_window_type()  {
		return _net_wm_window_type.has_value;
	}

	bool has_net_wm_state()  {
		return _net_wm_state.has_value;
	}

	bool has_net_wm_protocols()  {
		return _net_wm_protocols.has_value;
	}

	bool has_net_wm_partial_struct()  {
		return _net_wm_partial_struct.has_value;
	}

	bool has_net_wm_desktop()  {
		return _net_wm_desktop.has_value;
	}

	bool has_net_wm_user_time()  {
		return _net_wm_user_time.has_value;
	}

	bool has_net_wm_icon()  {
		return _net_wm_icon.has_value;
	}

	bool has_shape_region()  {
		return _shape_region.has_value;
	}

	/* update windows attributes */
	void process_configure_notify_event(XConfigureEvent const & e);

	box_int_t get_size();

};

typedef std::list<window_t *> window_list_t;
typedef std::set<window_t *> window_set_t;

}

#endif /* WINDOW_HXX_ */
