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

#include <typeinfo>

#include <X11/X.h>

#include "utils.hxx"
#include "motif_hints.hxx"
#include "xconnection.hxx"

namespace page {

typedef long card32;

using namespace std;

class client_base_t {
public:
	xconnection_t *              _cnx;
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

public:

	client_base_t(client_base_t const & c) {

		has_valid_window_attributes = c.has_valid_window_attributes;
		memcpy(&wa, &c.wa, sizeof(XWindowAttributes));

		_id = c._id;
		_cnx = c._cnx;

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

	client_base_t(xconnection_t * cnx, Window id) :
			_cnx(cnx), _id(id) {

		has_valid_window_attributes = false;
		bzero(&wa, sizeof(XWindowAttributes));

		/* ICCCM */
		wm_name = 0;
		wm_icon_name = 0;
		wm_normal_hints = 0;
		wm_hints = 0;
		wm_class = 0;
		wm_transient_for = 0;
		wm_protocols = 0;
		wm_colormap_windows = 0;
		wm_client_machine = 0;
		wm_state = 0;

		/* EWMH */
		_net_wm_name = 0;
		_net_wm_visible_name = 0;
		_net_wm_icon_name = 0;
		_net_wm_visible_icon_name = 0;
		_net_wm_desktop = 0;
		_net_wm_window_type = 0;
		_net_wm_state = 0;
		_net_wm_allowed_actions = 0;
		_net_wm_struct = 0;
		_net_wm_struct_partial = 0;
		_net_wm_icon_geometry = 0;
		_net_wm_icon = 0;
		_net_wm_pid = 0;
		_net_wm_handled_icons = false;
		_net_wm_user_time = 0;
		_net_wm_user_time_window = 0;
		_net_frame_extents = 0;
		_net_wm_opaque_region = 0;
		_net_wm_bypass_compositor = 0;

		motif_hints = 0;

	}

	virtual ~client_base_t() {
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
		if (motif_hints != 0) {
			if (motif_hints->flags & MWM_HINTS_DECORATIONS) {
				if (not (motif_hints->decorations & MWM_DECOR_BORDER)
						and not ((motif_hints->decorations & MWM_DECOR_ALL))) {
					return false;
				}
			}
		}
		return true;
	}

public:

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

};

}

#endif /* CLIENT_HXX_ */
