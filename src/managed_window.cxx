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
#include "managed_window.hxx"
#include "notebook.hxx"
#include "utils.hxx"

namespace page {

managed_window_t::managed_window_t(Atom net_wm_type,
		shared_ptr<client_properties_t> props, theme_t const * theme) :
				client_base_t(props),
				_theme(theme),
				_type(MANAGED_FLOATING),
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
				_is_durty(true),
				_is_focused(false)
{

	cnx()->add_to_save_set(orig());
	/* set border to zero */
	XSetWindowBorder(cnx()->dpy(), orig(), 0);
	/* assign window to desktop 0 */
	_properties->set_net_wm_desktop(0);

	_floating_wished_position = i_rect(_properties->wa().x, _properties->wa().y, _properties->wa().width, _properties->wa().height);
	_notebook_wished_position = i_rect(_properties->wa().x, _properties->wa().y, _properties->wa().width, _properties->wa().height);

	_orig_visual = _properties->wa().visual;
	_orig_depth = _properties->wa().depth;

	if (_properties->wm_normal_hints() != nullptr) {
		unsigned w = _floating_wished_position.w;
		unsigned h = _floating_wished_position.h;

		::page::compute_client_size_with_constraint(*(_properties->wm_normal_hints()),
				_floating_wished_position.w, _floating_wished_position.h, w, h);

		_floating_wished_position.w = w;
		_floating_wished_position.h = h;
	}

	/**
	 * if x == 0 then place window at center of the screen
	 **/
	if (_floating_wished_position.x == 0) {
		_floating_wished_position.x =
				(_properties->wa().width - _floating_wished_position.w) / 2;
	}

	/**
	 * if y == 0 then place window at center of the screen
	 **/
	if (_floating_wished_position.y == 0) {
		_floating_wished_position.y = (_properties->wa().height - _floating_wished_position.h) / 2;
	}

	/** check if the window has motif no border **/
	{
		_motif_has_border = cnx()->motif_has_border(_orig);
	}

	/**
	 * Create the base window, window that will content managed window
	 **/

	XSetWindowAttributes swa;
	Window wbase;
	Window wdeco;
	i_rect b = _floating_wished_position;

	/** Common window properties **/
	unsigned long value_mask = CWOverrideRedirect;
	swa.override_redirect = True;

	Visual * root_visual = _properties->wa().visual;
	int root_depth = _properties->wa().depth;

	/**
	 * If window visual is 32 bit (have alpha channel, and root do not
	 * have alpha channel, use the window visual, otherwise always prefer
	 * root visual.
	 **/
	if (_orig_depth == 32 && root_depth != 32) {
		_deco_visual = _orig_visual;
		_deco_depth = _orig_depth;

		/** if visual is 32 bits, this values are mandatory **/
		swa.colormap = XCreateColormap(cnx()->dpy(), cnx()->root(), _orig_visual,
		AllocNone);
		swa.background_pixel = BlackPixel(cnx()->dpy(), cnx()->screen());
		swa.border_pixel = BlackPixel(cnx()->dpy(), cnx()->screen());
		value_mask |= CWColormap | CWBackPixel | CWBorderPixel;

		wbase = XCreateWindow(cnx()->dpy(), cnx()->root(), -10, -10, 1, 1, 0, 32,
		InputOutput, _orig_visual, value_mask, &swa);
		wdeco = XCreateWindow(cnx()->dpy(), wbase, b.x, b.y, b.w, b.h, 0, 32,
		InputOutput, _orig_visual, value_mask, &swa);

	} else {

		/**
		 * Create RGB window for back ground
		 **/

		XVisualInfo vinfo;
		if (XMatchVisualInfo(cnx()->dpy(), cnx()->screen(), 32, TrueColor, &vinfo)
				== 0) {
			throw std::runtime_error(
					"Unable to find valid visual for background windows");
		}

		/**
		 * To create RGBA window, the following field MUST bet set, for unknown
		 * reason. i.e. border_pixel, background_pixel and colormap.
		 **/
		swa.border_pixel = 0;
		swa.background_pixel = 0;
		swa.colormap = XCreateColormap(cnx()->dpy(), cnx()->root(), vinfo.visual,
		AllocNone);

		_deco_visual = vinfo.visual;
		_deco_depth = 32;
		value_mask |= CWColormap | CWBackPixel | CWBorderPixel;

		wbase = XCreateWindow(cnx()->dpy(), cnx()->root(), -10, -10, 1, 1, 0, 32,
		InputOutput, vinfo.visual, value_mask, &swa);
		wdeco = XCreateWindow(cnx()->dpy(), wbase, b.x, b.y, b.w, b.h, 0, 32,
		InputOutput, vinfo.visual, value_mask, &swa);
	}

	_base = wbase;
	_deco = wdeco;

	cnx()->reparentwindow(_orig, _base, 0, 0);

	select_inputs();
	/* Grab button click */
	grab_button_unfocused();

	_surf = cairo_xlib_surface_create(cnx()->dpy(), _deco, _deco_visual, b.w,
			b.h);

	_composite_surf = composite_surface_manager_t::get(cnx()->dpy(), _base);
	composite_surface_manager_t::onmap(cnx()->dpy(), _base);

	update_icon();

}

managed_window_t::~managed_window_t() {

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

	XRemoveFromSaveSet(cnx()->dpy(), _orig);
	XDestroyWindow(cnx()->dpy(), _deco);
	XDestroyWindow(cnx()->dpy(), _base);

}

void managed_window_t::reconfigure() {

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

			if (_base_position.x < 0)
				_base_position.x = 0;
			if (_base_position.y < 0)
				_base_position.y = 0;

			_orig_position.x = _theme->floating.margin.left;
			_orig_position.y = _theme->floating.margin.top;
			_orig_position.w = _wished_position.w;
			_orig_position.h = _wished_position.h;

		} else {
			_base_position = _wished_position;

			if (_base_position.x < 0)
				_base_position.x = 0;
			if (_base_position.y < 0)
				_base_position.y = 0;

			_orig_position.x = 0;
			_orig_position.y = 0;
			_orig_position.w = _wished_position.w;
			_orig_position.h = _wished_position.h;
		}

		cairo_xlib_surface_set_size(_surf, _base_position.w, _base_position.h);

		destroy_back_buffer();
		create_back_buffer();

		region r(_orig_position);
		set_opaque_region(_base, r);

	} else {
		_wished_position = _notebook_wished_position;
		_base_position = _notebook_wished_position;
		_orig_position = i_rect(0, 0, _base_position.w, _base_position.h);

		destroy_back_buffer();

		region r(_orig_position);
		set_opaque_region(_base, r);

	}

	cnx()->move_resize(_base, _base_position);
	cnx()->move_resize(_deco,
			i_rect(0, 0, _base_position.w, _base_position.h));
	cnx()->move_resize(_orig, _orig_position);

	fake_configure();

	mark_durty();
	expose();

}

void managed_window_t::fake_configure() {
	//printf("fake_reconfigure = %dx%d+%d+%d\n", _wished_position.w,
	//		_wished_position.h, _wished_position.x, _wished_position.y);
	cnx()->fake_configure(_orig, _wished_position, 0);
}

void managed_window_t::delete_window(Time t) {
	printf("request close for '%s'\n", title().c_str());

	XEvent ev;
	ev.xclient.display = cnx()->dpy();
	ev.xclient.type = ClientMessage;
	ev.xclient.format = 32;
	ev.xclient.message_type = A(WM_PROTOCOLS);
	ev.xclient.window = _orig;
	ev.xclient.data.l[0] = A(WM_DELETE_WINDOW);
	ev.xclient.data.l[1] = t;
	cnx()->send_event(_orig, False, NoEventMask, &ev);
}

bool managed_window_t::check_orig_position(i_rect const & position) {
	return position == _orig_position;
}

bool managed_window_t::check_base_position(i_rect const & position) {
	return position == _base_position;
}

void managed_window_t::init_managed_type(managed_window_type_e type) {
	_type = type;
}

void managed_window_t::set_managed_type(managed_window_type_e type) {

	_is_durty = true;

	list<Atom> net_wm_allowed_actions;
	net_wm_allowed_actions.push_back(A(_NET_WM_ACTION_CLOSE));
	net_wm_allowed_actions.push_back(A(_NET_WM_ACTION_FULLSCREEN));
	cnx()->write_net_wm_allowed_actions(_orig, net_wm_allowed_actions);

	_type = type;
	reconfigure();

}

void managed_window_t::focus(Time t) {
	set_focus_state(true);
	icccm_focus(t);
}

i_rect managed_window_t::get_base_position() const {
	return _base_position;
}

managed_window_type_e managed_window_t::get_type() {
	return _type;
}

void managed_window_t::set_theme(theme_t const * theme) {
	this->_theme = theme;
}

bool managed_window_t::is(managed_window_type_e type) {
	return _type == type;
}

void managed_window_t::expose() {
	if (is(MANAGED_FLOATING)) {

		if (_is_durty) {
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

			_is_durty = false;
			_theme->render_floating(&fw);

		}

		cairo_xlib_surface_set_size(_surf, _base_position.w, _base_position.h);

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

void managed_window_t::icccm_focus(Time t) {
	//fprintf(stderr, "Focus time = %lu\n", t);

	if (_properties->wm_hints() != nullptr) {
		if (_properties->wm_hints()->input == True) {
			cnx()->set_input_focus(_orig, RevertToParent, t);
		}
	} else {
		/** no WM_HINTS, guess hints.input == True **/
		cnx()->set_input_focus(_orig, RevertToParent, t);
	}

	if (_properties->wm_protocols() != nullptr) {
		if (has_key(*(_properties->wm_protocols()), static_cast<xcb_atom_t>(A(WM_TAKE_FOCUS)))) {
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

}

void managed_window_t::set_opaque_region(Window w, region & region) {
	vector<long> data(region.size() * 4);
	region::iterator i = region.begin();
	int k = 0;
	while (i != region.end()) {
		data[k++] = (*i).x;
		data[k++] = (*i).y;
		data[k++] = (*i).w;
		data[k++] = (*i).h;
		++i;
	}

	cnx()->change_property(w, _NET_WM_OPAQUE_REGION, CARDINAL, 32, &data[0],
			data.size());

}

vector<floating_event_t> * managed_window_t::compute_floating_areas(
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

i_rect managed_window_t::compute_floating_close_position(i_rect const & allocation) const {

	i_rect position;
	position.x = allocation.w - _theme->floating.close_width;
	position.y = 0.0;
	position.w = _theme->floating.close_width;
	position.h = _theme->notebook.margin.top-3;

	return position;
}

i_rect managed_window_t::compute_floating_bind_position(
		i_rect const & allocation) const {

	i_rect position;
	position.x = allocation.w - _theme->floating.bind_width - _theme->floating.close_width;
	position.y = 0.0;
	position.w = 16;
	position.h = 16;

	return position;
}


}

