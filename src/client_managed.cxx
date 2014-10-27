/*
 * managed_window.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "leak_checker.hxx"

#include <cairo.h>
#include <cairo-xlib.h>
#include "client_managed.hxx"
#include "notebook.hxx"
#include "utils.hxx"

namespace page {

client_managed_t::client_managed_t(Atom net_wm_type,
		ptr<client_properties_t> props, theme_t const * theme) :
				client_base_t{props},
				_theme(theme),
				_managed_type(MANAGED_FLOATING),
				_net_wm_type(net_wm_type),
				_floating_wished_position(),
				_notebook_wished_position(),
				_wished_position(),
				_orig_position(),
				_base_position(),
				_surf(0),
				_top_buffer(0),
				_bottom_buffer(0),
				_left_buffer(0),
				_right_buffer(0),
				_icon(nullptr),
				_orig_visual(0),
				_orig_depth(-1),
				_deco_visual(0),
				_deco_depth(-1),
				_orig(props->id()),
				_base(None),
				_deco(None),
				_floating_area(0),
				_is_focused(false),
				_is_iconic(true),
				_is_hidden(true)
{

	cnx()->add_to_save_set(orig());
	/* set border to zero */
	XSetWindowBorder(cnx()->dpy(), orig(), 0);

	i_rect pos{_properties->position()};

	printf("window default position = %s\n", pos.to_string().c_str());

	_floating_wished_position = pos;
	_notebook_wished_position = pos;

	_orig_visual = _properties->wa()->visual;
	_orig_depth = _properties->geometry()->depth;

	/**
	 * if x == 0 then place window at center of the screen
	 **/
	if (_floating_wished_position.x == 0) {
		_floating_wished_position.x =
				(_properties->geometry()->width - _floating_wished_position.w) / 2;
	}

	if(_floating_wished_position.x - _theme->floating.margin.left < 0) {
		_floating_wished_position.x = _theme->floating.margin.left;
	}

	/**
	 * if y == 0 then place window at center of the screen
	 **/
	if (_floating_wished_position.y == 0) {
		_floating_wished_position.y = (_properties->geometry()->height - _floating_wished_position.h) / 2;
	}

	if(_floating_wished_position.y - _theme->floating.margin.top < 0) {
		_floating_wished_position.y = _theme->floating.margin.top;
	}

	/** check if the window has motif no border **/
	{
		_motif_has_border = cnx()->motif_has_border(_orig);
	}

	/**
	 * Create the base window, window that will content managed window
	 **/

	xcb_window_t wbase;
	xcb_window_t wdeco;
	i_rect b = _floating_wished_position;



	xcb_visualid_t root_visual = cnx()->root_visual()->visual_id;
	int root_depth = cnx()->find_visual_depth(cnx()->root_visual()->visual_id);

	/**
	 * If window visual is 32 bit (have alpha channel, and root do not
	 * have alpha channel, use the window visual, otherwise always prefer
	 * root visual.
	 **/
	if (_orig_depth == 32 && root_depth != 32) {
		_deco_visual = _orig_visual;
		_deco_depth = _orig_depth;

		/** if visual is 32 bits, this values are mandatory **/
		xcb_colormap_t cmap = xcb_generate_id(cnx()->xcb());
		xcb_create_colormap(cnx()->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, cnx()->root(), _deco_visual);

		uint32_t value_mask = 0;
		uint32_t value[4];

		value_mask |= XCB_CW_BACK_PIXEL;
		value[0] = cnx()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_BORDER_PIXEL;
		value[1] = cnx()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_OVERRIDE_REDIRECT;
		value[2] = True;

		value_mask |= XCB_CW_COLORMAP;
		value[3] = cmap;

		wbase = xcb_generate_id(cnx()->xcb());
		xcb_create_window(cnx()->xcb(), _deco_depth, wbase, cnx()->root(), -10, -10, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _deco_visual, value_mask, value);
		wdeco = xcb_generate_id(cnx()->xcb());
		xcb_create_window(cnx()->xcb(), _deco_depth, wdeco, wbase, b.x, b.y, b.w, b.h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _deco_visual, value_mask, value);

	} else {

		/**
		 * Create RGB window for back ground
		 **/

		_deco_visual = cnx()->default_visual()->visual_id;
		_deco_depth = 32;

		/** if visual is 32 bits, this values are mandatory **/
		xcb_colormap_t cmap = xcb_generate_id(cnx()->xcb());
		xcb_create_colormap(cnx()->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, cnx()->root(), _deco_visual);

		/**
		 * To create RGBA window, the following field MUST bet set, for unknown
		 * reason. i.e. border_pixel, background_pixel and colormap.
		 **/
		uint32_t value_mask = 0;
		uint32_t value[4];

		value_mask |= XCB_CW_BACK_PIXEL;
		value[0] = cnx()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_BORDER_PIXEL;
		value[1] = cnx()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_OVERRIDE_REDIRECT;
		value[2] = True;

		value_mask |= XCB_CW_COLORMAP;
		value[3] = cmap;

		wbase = xcb_generate_id(cnx()->xcb());
		xcb_create_window(cnx()->xcb(), _deco_depth, wbase, cnx()->root(), -10, -10, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _deco_visual, value_mask, value);
		wdeco = xcb_generate_id(cnx()->xcb());
		xcb_create_window(cnx()->xcb(), _deco_depth, wdeco, wbase, b.x, b.y, b.w, b.h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _deco_visual, value_mask, value);

	}

	_base = wbase;
	_deco = wdeco;

	cnx()->reparentwindow(_orig, _base, 0, 0);

	select_inputs();
	/* Grab button click */
	grab_button_unfocused();

	_surf = cairo_xcb_surface_create(cnx()->xcb(), _deco, cnx()->find_visual(_deco_visual), b.w, b.h);

	_composite_surf = composite_surface_manager_t::get(cnx()->dpy(), _base);
	composite_surface_manager_t::onmap(cnx()->dpy(), _base);

	update_icon();

}

client_managed_t::~client_managed_t() {

	unselect_inputs();

	if (_surf != nullptr) {
		warn(cairo_surface_get_reference_count(_surf) == 1);
		cairo_surface_destroy(_surf);
		_surf = nullptr;
	}

	if (_floating_area != nullptr) {
		delete _floating_area;
		_floating_area = nullptr;
	}

	destroy_back_buffer();

	xcb_destroy_window(cnx()->xcb(), _deco);
	xcb_destroy_window(cnx()->xcb(), _base);
}

void client_managed_t::reconfigure() {

	if (is(MANAGED_FLOATING)) {
		_wished_position = _floating_wished_position;

		if (_motif_has_border) {
			_base_position.x = _wished_position.x
					- _theme->floating.margin.left;
			_base_position.y = _wished_position.y - _theme->floating.margin.top;
			_base_position.w = _wished_position.w + _theme->floating.margin.left
					+ _theme->floating.margin.right;
			_base_position.h = _wished_position.h + _theme->floating.margin.top
					+ _theme->floating.margin.bottom;

			_orig_position.x = _theme->floating.margin.left;
			_orig_position.y = _theme->floating.margin.top;
			_orig_position.w = _wished_position.w;
			_orig_position.h = _wished_position.h;

		} else {
			_base_position = _wished_position;

			_orig_position.x = 0;
			_orig_position.y = 0;
			_orig_position.w = _wished_position.w;
			_orig_position.h = _wished_position.h;

		}

		cairo_xcb_surface_set_size(_surf, _base_position.w, _base_position.h);

		destroy_back_buffer();
		create_back_buffer();

	} else {
		_wished_position = _notebook_wished_position;
		_base_position = _notebook_wished_position;
		_orig_position = i_rect(0, 0, _base_position.w, _base_position.h);

		destroy_back_buffer();

	}

	if (lock()) {
		cnx()->move_resize(_base, _base_position);
		cnx()->move_resize(_deco,
				i_rect{0, 0, _base_position.w, _base_position.h});
		cnx()->move_resize(_orig, _orig_position);
		fake_configure();
		unlock();
	}
	expose();

}

void client_managed_t::fake_configure() {
	//printf("fake_reconfigure = %dx%d+%d+%d\n", _wished_position.w,
	//		_wished_position.h, _wished_position.x, _wished_position.y);
	cnx()->fake_configure(_orig, _wished_position, 0);
}

void client_managed_t::delete_window(Time t) {
	printf("request close for '%s'\n", title().c_str());

	if (lock()) {
		xcb_client_message_event_t xev;
		xev.response_type = XCB_CLIENT_MESSAGE;
		xev.type = A(WM_PROTOCOLS);
		xev.format = 32;
		xev.window = _orig;
		xev.data.data32[0] = A(WM_DELETE_WINDOW);
		xev.data.data32[1] = t;

		xcb_send_event(cnx()->xcb(), False, _orig, XCB_EVENT_MASK_NO_EVENT,
				reinterpret_cast<char*>(&xev));
		unlock();
	}
}

bool client_managed_t::check_orig_position(i_rect const & position) {
	return position == _orig_position;
}

bool client_managed_t::check_base_position(i_rect const & position) {
	return position == _base_position;
}

void client_managed_t::init_managed_type(managed_window_type_e type) {
	_managed_type = type;
}

void client_managed_t::set_managed_type(managed_window_type_e type) {
	if (lock()) {
		list<Atom> net_wm_allowed_actions;
		net_wm_allowed_actions.push_back(A(_NET_WM_ACTION_CLOSE));
		net_wm_allowed_actions.push_back(A(_NET_WM_ACTION_FULLSCREEN));
		cnx()->write_net_wm_allowed_actions(_orig, net_wm_allowed_actions);

		_managed_type = type;
		reconfigure();
		unlock();
	}
}

void client_managed_t::focus(Time t) {
	set_focus_state(true);
	icccm_focus(t);
}

i_rect client_managed_t::get_base_position() const {
	return _base_position;
}

managed_window_type_e client_managed_t::get_type() {
	return _managed_type;
}

void client_managed_t::set_theme(theme_t const * theme) {
	this->_theme = theme;
}

bool client_managed_t::is(managed_window_type_e type) {
	return _managed_type == type;
}

void client_managed_t::expose() {
	if (is(MANAGED_FLOATING)) {

			theme_managed_window_t fw;

			if(_bottom_buffer != nullptr) {
				fw.cairo_bottom = cairo_create(_bottom_buffer);
			} else {
				fw.cairo_bottom = nullptr;
			}

			if(_top_buffer != nullptr) {
				fw.cairo_top = cairo_create(_top_buffer);
			} else {
				fw.cairo_top = nullptr;
			}

			if(_right_buffer != nullptr) {
				fw.cairo_right = cairo_create(_right_buffer);
			} else {
				fw.cairo_right = nullptr;
			}

			if(_left_buffer != nullptr) {
				fw.cairo_left = cairo_create(_left_buffer);
			} else {
				fw.cairo_left = nullptr;
			}


			fw.demand_attention = false;
			fw.focuced = is_focused();
			fw.position = base_position();
			fw.icon = icon();
			fw.title = title();

			_theme->render_floating(&fw);


		cairo_xcb_surface_set_size(_surf, _base_position.w, _base_position.h);

		cairo_t * _cr = cairo_create(_surf);

		/** top **/
		if (_top_buffer != nullptr) {
			cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(_cr, 0, 0, _base_position.w,
					_theme->floating.margin.top);
			cairo_set_source_surface(_cr, _top_buffer, 0, 0);
			cairo_fill(_cr);
		}

		/** bottom **/
		if (_bottom_buffer != nullptr) {
			cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(_cr, 0,
					_base_position.h - _theme->floating.margin.bottom,
					_base_position.w, _theme->floating.margin.bottom);
			cairo_set_source_surface(_cr, _bottom_buffer, 0,
					_base_position.h - _theme->floating.margin.bottom);
			cairo_fill(_cr);
		}

		/** left **/
		if (_left_buffer != nullptr) {
			cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(_cr, 0.0, _theme->floating.margin.top,
					_theme->floating.margin.left,
					_base_position.h - _theme->floating.margin.top
							- _theme->floating.margin.bottom);
			cairo_set_source_surface(_cr, _left_buffer, 0.0,
					_theme->floating.margin.top);
			cairo_fill(_cr);
		}

		/** right **/
		if (_right_buffer != nullptr) {
			cairo_set_operator(_cr, CAIRO_OPERATOR_SOURCE);
			cairo_rectangle(_cr,
					_base_position.w - _theme->floating.margin.right,
					_theme->floating.margin.top, _theme->floating.margin.right,
					_base_position.h - _theme->floating.margin.top
							- _theme->floating.margin.bottom);
			cairo_set_source_surface(_cr, _right_buffer,
					_base_position.w - _theme->floating.margin.right,
					_theme->floating.margin.top);
			cairo_fill(_cr);
		}

		cairo_surface_flush(_surf);

		warn(cairo_get_reference_count(_cr) == 1);
		cairo_destroy(_cr);
	}
}

void client_managed_t::icccm_focus(Time t) {
	//fprintf(stderr, "Focus time = %lu\n", t);

	if (lock()) {

		if (_properties->wm_hints() != nullptr) {
			if (_properties->wm_hints()->input != False) {
				cnx()->set_input_focus(_orig, RevertToParent, t);
			}
		} else {
			/** no WM_HINTS, guess hints.input == True **/
			cnx()->set_input_focus(_orig, RevertToParent, t);
		}

		if (_properties->wm_protocols() != nullptr) {
			if (has_key(*(_properties->wm_protocols()),
					static_cast<xcb_atom_t>(A(WM_TAKE_FOCUS)))) {
				XEvent ev;
				ev.xclient.display = cnx()->dpy();
				ev.xclient.type = ClientMessage;
				ev.xclient.format = 32;
				ev.xclient.message_type = A(WM_PROTOCOLS);
				ev.xclient.window = _orig;
				ev.xclient.data.l[0] = A(WM_TAKE_FOCUS);
				ev.xclient.data.l[1] = t;
				cnx()->send_event(_orig, False, NoEventMask, &ev);
			}
		}

		unlock();

	}

}

vector<floating_event_t> * client_managed_t::compute_floating_areas(
		theme_managed_window_t * mw) const {

	vector<floating_event_t> * ret = new vector<floating_event_t>();

	floating_event_t fc(FLOATING_EVENT_CLOSE);
	fc.position = compute_floating_close_position(mw->position);
	ret->push_back(fc);

	floating_event_t fb(FLOATING_EVENT_BIND);
	fb.position = compute_floating_bind_position(mw->position);
	ret->push_back(fb);

	int x0 = _theme->floating.margin.left;
	int x1 = mw->position.w - _theme->floating.margin.right;

	int y0 = _theme->floating.margin.bottom;
	int y1 = mw->position.h - _theme->floating.margin.bottom;

	int w0 = mw->position.w - _theme->floating.margin.left
			- _theme->floating.margin.right;
	int h0 = mw->position.h - _theme->floating.margin.bottom
			- _theme->floating.margin.bottom;

	floating_event_t ft(FLOATING_EVENT_TITLE);
	ft.position = i_rect(x0, y0, w0,
			_theme->floating.margin.top - _theme->floating.margin.bottom);
	ret->push_back(ft);

	floating_event_t fgt(FLOATING_EVENT_GRIP_TOP);
	fgt.position = i_rect(x0, 0, w0, _theme->floating.margin.bottom);
	ret->push_back(fgt);

	floating_event_t fgb(FLOATING_EVENT_GRIP_BOTTOM);
	fgb.position = i_rect(x0, y1, w0, _theme->floating.margin.bottom);
	ret->push_back(fgb);

	floating_event_t fgl(FLOATING_EVENT_GRIP_LEFT);
	fgl.position = i_rect(0, y0, _theme->floating.margin.left, h0);
	ret->push_back(fgl);

	floating_event_t fgr(FLOATING_EVENT_GRIP_RIGHT);
	fgr.position = i_rect(x1, y0, _theme->floating.margin.right, h0);
	ret->push_back(fgr);

	floating_event_t fgtl(FLOATING_EVENT_GRIP_TOP_LEFT);
	fgtl.position = i_rect(0, 0, _theme->floating.margin.left,
			_theme->floating.margin.bottom);
	ret->push_back(fgtl);

	floating_event_t fgtr(FLOATING_EVENT_GRIP_TOP_RIGHT);
	fgtr.position = i_rect(x1, 0, _theme->floating.margin.right,
			_theme->floating.margin.bottom);
	ret->push_back(fgtr);

	floating_event_t fgbl(FLOATING_EVENT_GRIP_BOTTOM_LEFT);
	fgbl.position = i_rect(0, y1, _theme->floating.margin.left,
			_theme->floating.margin.bottom);
	ret->push_back(fgbl);

	floating_event_t fgbr(FLOATING_EVENT_GRIP_BOTTOM_RIGHT);
	fgbr.position = i_rect(x1, y1, _theme->floating.margin.right,
			_theme->floating.margin.bottom);
	ret->push_back(fgbr);

	return ret;

}

i_rect client_managed_t::compute_floating_close_position(i_rect const & allocation) const {

	i_rect position;
	position.x = allocation.w - _theme->floating.close_width;
	position.y = 0.0;
	position.w = _theme->floating.close_width;
	position.h = _theme->notebook.margin.top-3;

	return position;
}

i_rect client_managed_t::compute_floating_bind_position(
		i_rect const & allocation) const {

	i_rect position;
	position.x = allocation.w - _theme->floating.bind_width - _theme->floating.close_width;
	position.y = 0.0;
	position.w = 16;
	position.h = 16;

	return position;
}


void client_managed_t::grab_button_focused() {
	if (lock()) {
		/** First ungrab all **/
		ungrab_all_button();

		/** for decoration, grab all **/
		XGrabButton(cnx()->dpy(), (Button1), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx()->dpy(), (Button2), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx()->dpy(), (Button3), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		/** for base, just grab some modified buttons **/
		XGrabButton(cnx()->dpy(), Button1, (Mod1Mask), _base, False,
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		/** for base, just grab some modified buttons **/
		XGrabButton(cnx()->dpy(), Button1, (ControlMask), _base, False,
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);
		unlock();
	}
}

void client_managed_t::grab_button_unfocused() {
	if (lock()) {
		/** First ungrab all **/
		ungrab_all_button();

		XGrabButton(cnx()->dpy(), (Button1), (AnyModifier), _base, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx()->dpy(), (Button2), (AnyModifier), _base, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx()->dpy(), (Button3), (AnyModifier), _base, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		/** for decoration, grab all **/
		XGrabButton(cnx()->dpy(), (Button1), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx()->dpy(), (Button2), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		XGrabButton(cnx()->dpy(), (Button3), (AnyModifier), _deco, (False),
				(ButtonPressMask | ButtonMotionMask | ButtonReleaseMask),
				(GrabModeSync), (GrabModeAsync), None, None);

		unlock();
	}
}


void client_managed_t::ungrab_all_button() {
	if (lock()) {
		XUngrabButton(cnx()->dpy(), AnyButton, AnyModifier, _orig);
		XUngrabButton(cnx()->dpy(), AnyButton, AnyModifier, _base);
		XUngrabButton(cnx()->dpy(), AnyButton, AnyModifier, _deco);
		unlock();
	}
}

void client_managed_t::select_inputs() {
	if (lock()) {
		XSelectInput(cnx()->dpy(), _base, MANAGED_BASE_WINDOW_EVENT_MASK);
		XSelectInput(cnx()->dpy(), _deco, MANAGED_DECO_WINDOW_EVENT_MASK);
		XSelectInput(cnx()->dpy(), _orig, MANAGED_ORIG_WINDOW_EVENT_MASK);
		XShapeSelectInput(cnx()->dpy(), _orig, ShapeNotifyMask);
		unlock();
	}
}

void client_managed_t::unselect_inputs() {
	if (lock()) {
		XSelectInput(cnx()->dpy(), _base, NoEventMask);
		XSelectInput(cnx()->dpy(), _deco, NoEventMask);
		XSelectInput(cnx()->dpy(), _orig, NoEventMask);
		XShapeSelectInput(cnx()->dpy(), _orig, 0);
		unlock();
	}
}

bool client_managed_t::is_fullscreen() {
	if (_properties->net_wm_state() != nullptr) {
		return has_key(*(_properties->net_wm_state()),
				static_cast<xcb_atom_t>(A(_NET_WM_STATE_FULLSCREEN)));
	}
	return false;
}

Atom client_managed_t::net_wm_type() {
	return _net_wm_type;
}

bool client_managed_t::get_wm_normal_hints(XSizeHints * size_hints) {
	if(_properties->wm_normal_hints() != nullptr) {
		*size_hints = *(_properties->wm_normal_hints());
		return true;
	} else {
		return false;
	}
}

void client_managed_t::net_wm_state_add(atom_e atom) {
	if (lock()) {
		_properties->net_wm_state_add(atom);
		unlock();
	}
}

void client_managed_t::net_wm_state_remove(atom_e atom) {
	if (lock()) {
		_properties->net_wm_state_remove(atom);
		unlock();
	}
}

void client_managed_t::net_wm_state_delete() {
	/**
	 * This one is for removing the window manager tag, thus only check if the window
	 * still exist. (don't need lock);
	 **/
	cnx()->grab();
	cnx()->fetch_pending_events();
	if(cnx()->check_for_destroyed_window(_orig)) {
		cnx()->ungrab();
		return;
	}

	cnx()->delete_property(_orig, _NET_WM_STATE);

	cnx()->ungrab();
}

void client_managed_t::normalize() {
	if (lock()) {
		_is_iconic = false;
		cnx()->write_wm_state(_orig, NormalState, None);
		if (not _is_hidden) {
			net_wm_state_remove(_NET_WM_STATE_HIDDEN);
			cnx()->map_window(_orig);
			cnx()->map_window(_deco);
			cnx()->map_window(_base);
		}

		try {
			_composite_surf = composite_surface_manager_t::get(cnx()->dpy(),
					base());
		} catch (...) {
			printf("Error while creating composite surface\n");
		}

		for (auto c : _children) {
			client_managed_t * mw = dynamic_cast<client_managed_t *>(c);
			if (mw != nullptr) {
				mw->normalize();
			}
		}
		unlock();
	}
}

void client_managed_t::iconify() {
	if (lock()) {
		_is_iconic = true;
		net_wm_state_add(_NET_WM_STATE_HIDDEN);
		cnx()->write_wm_state(_orig, IconicState, None);
		cnx()->unmap(_base);
		cnx()->unmap(_deco);
		cnx()->unmap(_orig);

		_composite_surf.reset();

		for (auto c : _children) {
			client_managed_t * mw = dynamic_cast<client_managed_t *>(c);
			if (mw != nullptr) {
				mw->iconify();
			}
		}
		unlock();
	}
}

void client_managed_t::wm_state_delete() {
	/**
	 * This one is for removing the window manager tag, thus only check if the window
	 * still exist. (don't need lock);
	 **/
	cnx()->grab();
	cnx()->fetch_pending_events();
	if(cnx()->check_for_destroyed_window(_orig)) {
		cnx()->ungrab();
		return;
	}

	cnx()->delete_property(_orig, WM_STATE);

	cnx()->ungrab();
}

void client_managed_t::set_floating_wished_position(i_rect const & pos) {
	_floating_wished_position = pos;
}

void client_managed_t::set_notebook_wished_position(i_rect const & pos) {
	_notebook_wished_position = pos;
}

i_rect const & client_managed_t::get_wished_position() {
	return _wished_position;
}

i_rect const & client_managed_t::get_floating_wished_position() {
	return _floating_wished_position;
}

void client_managed_t::destroy_back_buffer() {

	if(_top_buffer != nullptr) {
		warn(cairo_surface_get_reference_count(_top_buffer) == 1);
		cairo_surface_destroy(_top_buffer);
		_top_buffer = nullptr;
	}

	if(_bottom_buffer != nullptr) {
		warn(cairo_surface_get_reference_count(_bottom_buffer) == 1);
		cairo_surface_destroy(_bottom_buffer);
		_bottom_buffer = nullptr;
	}

	if(_left_buffer != nullptr) {
		warn(cairo_surface_get_reference_count(_left_buffer) == 1);
		cairo_surface_destroy(_left_buffer);
		_left_buffer = nullptr;
	}

	if(_right_buffer != nullptr) {
		warn(cairo_surface_get_reference_count(_right_buffer) == 1);
		cairo_surface_destroy(_right_buffer);
		_right_buffer = nullptr;
	}

}

void client_managed_t::create_back_buffer() {

	if (_theme->floating.margin.top > 0) {
		_top_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
				_base_position.w, _theme->floating.margin.top);
	} else {
		_top_buffer = nullptr;
	}

	if (_theme->floating.margin.bottom > 0) {
		_bottom_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
				_base_position.w, _theme->floating.margin.bottom);
	} else {
		_bottom_buffer = nullptr;
	}

	if (_theme->floating.margin.left > 0) {
		_left_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
				_theme->floating.margin.left,
				_base_position.h - _theme->floating.margin.top
						- _theme->floating.margin.bottom);
	} else {
		_left_buffer = nullptr;
	}

	if (_theme->floating.margin.right > 0) {
		_right_buffer = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
				_theme->floating.margin.right,
				_base_position.h - _theme->floating.margin.top
						- _theme->floating.margin.bottom);
	} else {
		_right_buffer = nullptr;
	}

}

vector<floating_event_t> const * client_managed_t::floating_areas() {
	return _floating_area;
}

void client_managed_t::update_floating_areas() {

	if (_floating_area != 0) {
		delete _floating_area;
	}

	theme_managed_window_t tm;
	tm.position = _base_position;
	tm.title = _title;

	_floating_area = compute_floating_areas(&tm);
}

bool client_managed_t::has_window(Window w) {
	return w == _properties->id() or w == _base or w == _deco;
}

string client_managed_t::get_node_name() const {
	string s = _get_node_name<'M'>();
	ostringstream oss;
	oss << s << " " << orig() << " " << title();
	return oss.str();
}

display_t * client_managed_t::cnx() {
	return _properties->cnx();
}

void client_managed_t::prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time) {

	if(_is_hidden) {
		for(auto i: _children) {
			i->prepare_render(out, time);
		}
		return;
	}

	if (_composite_surf != nullptr and not _is_hidden) {

		i_rect loc{base_position()};

		if (_motif_has_border) {
			if(is_focused()) {
				auto x = new renderable_floating_outer_gradien_t(loc, 18.0, 7.0);
				out += ptr<renderable_t> { x };
			} else {
				auto x = new renderable_floating_outer_gradien_t(loc, 8.0, 7.0);
				out += ptr<renderable_t> { x };
			}
		}

		ptr<renderable_t> x { get_base_renderable() };
		if (x != nullptr) {
			out += x;
		}
	}

	for(auto i: _children) {
		i->prepare_render(out, time);
	}

}

ptr<renderable_t> client_managed_t::get_base_renderable() {
	if (_composite_surf != nullptr) {

		region vis;
		region opa;

		if(_managed_type == MANAGED_FULLSCREEN) {
			vis = _base_position;
			opa = _base_position;
		} else {

			vis = _base_position;

			region shp;
			if (shape() != nullptr) {
				shp = *shape();
			} else {
				shp = i_rect{0, 0, _orig_position.w, _orig_position.h};
			}

			region xopac;
			if (net_wm_opaque_region() != nullptr) {
				xopac = region { *(net_wm_opaque_region()) };
			} else {
				if (geometry()->depth == 24) {
					xopac = i_rect{0, 0, _orig_position.w, _orig_position.h};
				}
			}

			opa = shp & xopac;
			opa.translate(_base_position.x + _orig_position.x,
					_base_position.y + _orig_position.y);

		}

		region dmg { _composite_surf->get_damaged() };
		_composite_surf->clear_damaged();
		dmg.translate(_base_position.x, _base_position.y);
		auto x = new renderable_pixmap_t(_composite_surf->get_pixmap(),
				_base_position, dmg);
		x->set_opaque_region(opa);
		x->set_visible_region(vis);
		return ptr<renderable_t>{x};

	} else {
		/* return null */
		return ptr<renderable_t>{};
	}
}

i_rect const & client_managed_t::base_position() const {
	return _base_position;
}

i_rect const & client_managed_t::orig_position() const {
	return _orig_position;
}

bool client_managed_t::has_window(Window w) const {
	return w == _properties->id() or w == _base or w == _deco;
}

region client_managed_t::visible_area() const {

	if(_managed_type == MANAGED_FLOATING) {
		i_rect vis{base_position()};
		vis.x -= 32;
		vis.y -= 32;
		vis.w += 64;
		vis.h += 64;
		return vis;
	}

	return base_position();

}

bool client_managed_t::lock() {
	cnx()->grab();
	cnx()->fetch_pending_events();
	if(cnx()->check_for_destroyed_window(_orig)
			or cnx()->check_for_fake_unmap_window(_orig)) {
		cnx()->ungrab();
		return false;
	}
	return true;
}

void client_managed_t::unlock() {
	cnx()->ungrab();
}


void client_managed_t::set_focus_state(bool is_focused) {
	if (lock()) {
		_is_focused = is_focused;
		if (_is_focused) {
			net_wm_state_add(_NET_WM_STATE_FOCUSED);
			grab_button_focused();
		} else {
			net_wm_state_remove(_NET_WM_STATE_FOCUSED);
			grab_button_unfocused();
		}
		expose();
		unlock();
	}
}

void client_managed_t::net_wm_allowed_actions_add(atom_e atom) {
	if(lock()) {
		_properties->net_wm_allowed_actions_add(atom);
		unlock();
	}
}


}

