/*
 * popup_frame_move.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef POPUP_FRAME_MOVE_HXX_
#define POPUP_FRAME_MOVE_HXX_

namespace page {

struct popup_frame_move_t: public renderable_t {

	theme_t * _theme;
	icon64 * icon;
	std::string title;

	display_t * _dpy;

protected:
	i_rect _position;

	xcb_window_t _wid;

	bool _has_alpha;
	bool _is_durty;
	bool _is_visible;

public:
	popup_frame_move_t(display_t * dpy, theme_t * theme) : _dpy(dpy),
			_theme(theme), _position { -1, -1, 1, 1 } {

		icon = nullptr;
		_has_alpha = true;
		_is_durty = true;
		_is_visible = false;

		/** if visual is 32 bits, this values are mandatory **/
		xcb_colormap_t cmap = xcb_generate_id(_dpy->xcb());
		xcb_create_colormap(_dpy->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, _dpy->root(), _dpy->root_visual()->visual_id);

		uint32_t value_mask = 0;
		uint32_t value[5];

		value_mask |= XCB_CW_BACK_PIXEL;
		value[0] = _dpy->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_BORDER_PIXEL;
		value[1] = _dpy->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_OVERRIDE_REDIRECT;
		value[2] = True;

		value_mask |= XCB_CW_EVENT_MASK;
		value[3] = XCB_EVENT_MASK_EXPOSURE;

		value_mask |= XCB_CW_COLORMAP;
		value[4] = cmap;

		_wid = xcb_generate_id(_dpy->xcb());
		xcb_create_window(_dpy->xcb(), _dpy->root_depth(), _wid, _dpy->root(), _position.x, _position.y, _position.w, _position.h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _dpy->root_visual()->visual_id, value_mask, value);


	}


	void mark_durty() {
		_is_durty = true;
	}

	void move_resize(i_rect const & area) {
		_position = area;
		_dpy->move_resize(_wid, _position);
	}

	void move(int x, int y) {
		_position.x = x;
		_position.y = y;
	}

	void show() {
		_is_visible = true;
		_dpy->map(_wid);
	}

	void hide() {
		_is_visible = false;
		_dpy->unmap(_wid);
	}

	bool is_visible() {
		return _is_visible;
	}

	i_rect const & position() {
		return _position;
	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return region{};
	}

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() {
		return region{_position};
	}

	/**
	 * return currently damaged area (absolute)
	 **/
	virtual region get_damaged()  {
		if(_is_durty) {
			return region{_position};
		} else {
			return region{};
		}
	}



	~popup_frame_move_t() {
		if (icon != nullptr)
			delete icon;
		xcb_destroy_window(_dpy->xcb(), _wid);
	}

	void update_window(client_base_t * c, std::string title) {
		if (icon != nullptr)
			delete icon;
		icon = new icon64(*c);
		this->title = title;
	}

	virtual void render(cairo_t * cr, region const & area) {

		if(not _is_visible)
			return;

		for (auto & a : area) {
			cairo_save(cr);
			cairo_clip(cr, a);
			cairo_translate(cr, _position.x, _position.y);
			_theme->render_popup_move_frame(cr, icon, _position.w, _position.h,
					title);
			cairo_restore(cr);
		}
	}

};

}


#endif /* POPUP_FRAME_MOVE_HXX_ */
