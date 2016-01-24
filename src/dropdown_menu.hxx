/*
 * popup_alt_tab.hxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#ifndef DROPDOWN_MENU_HXX_
#define DROPDOWN_MENU_HXX_

#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

#include <string>
#include <memory>
#include <vector>


#include "utils.hxx"
#include "renderable.hxx"
#include "box.hxx"
#include "region.hxx"
#include "icon_handler.hxx"

namespace page {

template<typename TDATA>
class dropdown_menu_entry_t;



template<typename TDATA>
class dropdown_menu_entry_t {

	theme_dropdown_menu_entry_t _theme_data;

	TDATA const _data;

	dropdown_menu_entry_t(dropdown_menu_entry_t const &);
	dropdown_menu_entry_t & operator=(dropdown_menu_entry_t const &);

public:
	dropdown_menu_entry_t(TDATA data, std::shared_ptr<icon16> icon, std::string label) :
		_data(data)
	{
		_theme_data.icon = icon;
		_theme_data.label = label;
	}

	~dropdown_menu_entry_t() {

	}

	TDATA const & data() const {
		return _data;
	}

	std::shared_ptr<icon16> icon() const {
		return _theme_data.icon;
	}

	std::string const & label() const {
		return _theme_data.label;
	}

	theme_dropdown_menu_entry_t const & get_theme_item() {
		return _theme_data;
	}

};

struct dropdown_menu_overlay_t : public tree_t {
	page_context_t * _ctx;
	xcb_pixmap_t _pix;
	cairo_surface_t * _surf;
	rect _position;
	xcb_window_t _wid;
	bool _is_durty;

	dropdown_menu_overlay_t(page_context_t * ctx, rect position) : _ctx{ctx}, _position{position} {
		_is_durty = true;

		xcb_colormap_t cmap = xcb_generate_id(_ctx->dpy()->xcb());
		xcb_create_colormap(_ctx->dpy()->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, _ctx->dpy()->root(), _ctx->dpy()->root_visual()->visual_id);

		uint32_t value_mask = 0;
		uint32_t value[5];

		value_mask |= XCB_CW_BACK_PIXEL;
		value[0] = _ctx->dpy()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_BORDER_PIXEL;
		value[1] = _ctx->dpy()->xcb_screen()->black_pixel;

		value_mask |= XCB_CW_OVERRIDE_REDIRECT;
		value[2] = True;

		value_mask |= XCB_CW_EVENT_MASK;
		value[3] = XCB_EVENT_MASK_EXPOSURE;

		value_mask |= XCB_CW_COLORMAP;
		value[4] = cmap;

		_wid = xcb_generate_id(_ctx->dpy()->xcb());
		xcb_create_window(_ctx->dpy()->xcb(), _ctx->dpy()->root_depth(), _wid, _ctx->dpy()->root(), _position.x, _position.y, _position.w, _position.h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, _ctx->dpy()->root_visual()->visual_id, value_mask, value);


		_pix = xcb_generate_id(_ctx->dpy()->xcb());
		xcb_create_pixmap(_ctx->dpy()->xcb(), _ctx->dpy()->root_depth(), _pix, _wid, _position.w, _position.h);
		_surf = cairo_xcb_surface_create(_ctx->dpy()->xcb(), _pix, _ctx->dpy()->root_visual(), _position.w, _position.h);

	}

	~dropdown_menu_overlay_t() {
		cairo_surface_destroy(_surf);
		xcb_free_pixmap(_ctx->dpy()->xcb(), _pix);
		xcb_destroy_window(_ctx->dpy()->xcb(), _wid);
	}

	void map() {
		_ctx->dpy()->map(_wid);
	}

	rect const & position() {
		return _position;
	}

	xcb_window_t id() const {
		return _wid;
	}

	void expose() {
		cairo_surface_t * surf = cairo_xcb_surface_create(_ctx->dpy()->xcb(),
				_wid, _ctx->dpy()->root_visual(), _position.w, _position.h);
		cairo_t * cr = cairo_create(surf);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_surface(cr, _surf, 0.0, 0.0);
		cairo_rectangle(cr, 0, 0, _position.w, _position.h);
		cairo_fill(cr);
		cairo_destroy(cr);
		cairo_surface_destroy(surf);
	}

	void expose(region const & r) {
		cairo_surface_t * surf = cairo_xcb_surface_create(_ctx->dpy()->xcb(), _wid, _ctx->dpy()->root_visual(), _position.w, _position.h);
		cairo_t * cr = cairo_create(surf);
		for(auto a: r.rects()) {
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_set_source_surface(cr, _surf, 0.0, 0.0);
			cairo_rectangle(cr, a.x, a.y, a.w, a.h);
			cairo_fill(cr);
		}
		cairo_destroy(cr);
		cairo_surface_destroy(surf);
	}

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {

	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return region{_position};
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
			_is_durty = false;
			return region{_position};
		} else {
			return region{};
		}
	}

};

template<typename TDATA>
class dropdown_menu_t : public grab_handler_t {
	static uint32_t const DEFAULT_BUTTON_EVENT_MASK = XCB_EVENT_MASK_BUTTON_PRESS|XCB_EVENT_MASK_BUTTON_RELEASE|XCB_EVENT_MASK_BUTTON_MOTION;


public:
	using item_t = dropdown_menu_entry_t<TDATA>;

private:
	page_context_t * _ctx;
	std::vector<std::shared_ptr<item_t>> _items;
	int _selected;
	shared_ptr<dropdown_menu_overlay_t> pop;
	rect _start_position;
	bool active_grab;
	xcb_button_t _button;
	xcb_timestamp_t _time;

	std::function<void(dropdown_menu_t *, TDATA)> _callback;

public:


	template<typename F>
	dropdown_menu_t(page_context_t * ctx, std::vector<std::shared_ptr<item_t>> items, xcb_button_t button, int x, int y, int width, rect start_position, F f) :
	_ctx{ctx},
	_start_position{start_position},
	_button{button},
	_callback{f},
	_time{XCB_CURRENT_TIME}

	{

		active_grab = false;

		_selected = -1;
		_items = items;

		rect _position;
		_position.x = x;
		_position.y = y;
		_position.w = width;
		_position.h = 24*_items.size();

		pop = make_shared<dropdown_menu_overlay_t>(ctx, _position);
		update_backbuffer();
		pop->map();

		_ctx->overlay_add(pop);

	}

	~dropdown_menu_t() {
		_ctx->detach(pop);
	}

	TDATA const & get_selected() {
		return _items[_selected]->data();
	}

	xcb_timestamp_t time() {
		return _time;
	}

	void update_backbuffer() {

		cairo_t * cr = cairo_create(pop->_surf);

		for (unsigned k = 0; k < _items.size(); ++k) {
			update_items_back_buffer(cr, k);
		}

		cairo_destroy(cr);

		pop->expose(rect(0,0,pop->_position.w,pop->_position.h));

	}

	void update_items_back_buffer(cairo_t * cr, int n) {
		if (n >= 0 and n < _items.size()) {
			rect area(0, 24 * n, pop->_position.w, 24);
			_ctx->theme()->render_menuentry(cr, _items[n]->get_theme_item(), area, n == _selected);
		}
	}

	void set_selected(int s) {
		if(s >= 0 and s < _items.size() and s != _selected) {
			std::swap(_selected, s);
			cairo_t * cr = cairo_create(pop->_surf);
			update_items_back_buffer(cr, _selected);
			update_items_back_buffer(cr, s);
			cairo_destroy(cr);
			pop->_is_durty = true;

			pop->expose(rect(0,0,pop->_position.w,pop->_position.h));

		}
	}

	void update_cursor_position(int x, int y) {
		if (pop->_position.is_inside(x, y)) {
			int s = (int) floor((y - pop->_position.y) / 24.0);
			set_selected(s);
		}
	}

	virtual void button_press(xcb_button_press_event_t const * e) {

	}


	virtual void button_motion(xcb_motion_notify_event_t const * e) {
		update_cursor_position(e->root_x, e->root_y);
	}

	virtual void button_release(xcb_button_release_event_t const * e) {
		if (e->detail == XCB_BUTTON_INDEX_3
				or e->detail == XCB_BUTTON_INDEX_1) {
			if (_start_position.is_inside(e->event_x, e->event_y) and not active_grab) {
				active_grab = true;
				xcb_grab_pointer(_ctx->dpy()->xcb(),
				false, _ctx->dpy()->root(),
						DEFAULT_BUTTON_EVENT_MASK
								| XCB_EVENT_MASK_POINTER_MOTION,
						XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
						XCB_NONE,
						XCB_NONE, e->time);
			} else {
				if (active_grab) {
					xcb_ungrab_pointer(_ctx->dpy()->xcb(), e->time);
					active_grab = false;
				}

				if (pop->_position.is_inside(e->root_x, e->root_y)) {
					update_cursor_position(e->root_x, e->root_y);
					_time = e->time;
					_callback(this, get_selected());
				}

				_ctx->grab_stop();
			}
		}
	}

	virtual void key_press(xcb_key_press_event_t const * ev) { }
	virtual void key_release(xcb_key_release_event_t const * ev) { }


};



}



#endif /* POPUP_ALT_TAB_HXX_ */
