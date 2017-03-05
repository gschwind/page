/*
 * popup_alt_tab.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "dropdown_menu.hxx"

#include "page.hxx"
#include "tree.hxx"
#include "workspace.hxx"

namespace page {

using namespace std;

dropdown_menu_entry_t::dropdown_menu_entry_t(shared_ptr<icon16> icon,
		string const & label, function<void(xcb_timestamp_t time)> on_click) :
	_theme_data{icon, label},
	_on_click{on_click}
{

}

dropdown_menu_entry_t::~dropdown_menu_entry_t()
{

}

shared_ptr<icon16> dropdown_menu_entry_t::icon() const
{
	return _theme_data.icon;
}

string const & dropdown_menu_entry_t::label() const
{
	return _theme_data.label;
}

theme_dropdown_menu_entry_t const & dropdown_menu_entry_t::get_theme_item()
{
	return _theme_data;
}



dropdown_menu_overlay_t::dropdown_menu_overlay_t(tree_t * ref, rect position) :
	tree_t{ref->_root},
	_ctx{ref->_root->_ctx},
	_position{position}
{
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

dropdown_menu_overlay_t::~dropdown_menu_overlay_t()
{
	cairo_surface_destroy(_surf);
	xcb_free_pixmap(_ctx->dpy()->xcb(), _pix);
	xcb_destroy_window(_ctx->dpy()->xcb(), _wid);
}

void dropdown_menu_overlay_t::map()
{
	_ctx->dpy()->map(_wid);
}

rect const & dropdown_menu_overlay_t::position()
{
	return _position;
}

xcb_window_t dropdown_menu_overlay_t::id() const
{
	return _wid;
}

void dropdown_menu_overlay_t::expose()
{
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

void dropdown_menu_overlay_t::expose(region const & r)
{
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
void dropdown_menu_overlay_t::render(cairo_t * cr, region const & area)
{

}

/**
 * Derived class must return opaque region for this object,
 * If unknown it's safe to leave this empty.
 **/
region dropdown_menu_overlay_t::get_opaque_region()
{
	return region{_position};
}

/**
 * Derived class must return visible region,
 * If unknow the whole screen can be returned, but draw will be called each time.
 **/
region dropdown_menu_overlay_t::get_visible_region()
{
	return region{_position};
}

/**
 * return currently damaged area (absolute)
 **/
region dropdown_menu_overlay_t::get_damaged()
{
	if(_is_durty) {
		_is_durty = false;
		return region{_position};
	} else {
		return region{};
	}
}

void dropdown_menu_overlay_t::expose(xcb_expose_event_t const * ev)
{
	if(ev->window != _wid)
		return;
	expose(region(ev->x, ev->y, ev->width, ev->height));
}




dropdown_menu_t::dropdown_menu_t(tree_t * ref,
		vector<shared_ptr<item_t>> items, xcb_button_t button, int x, int y,
		int width, rect start_position) :
	_ctx{ref->_root->_ctx},
	_start_position{start_position},
	_button{button},
	_time{XCB_CURRENT_TIME}
{

	_selected = -1;
	_items = items;
	has_been_released = false;

	rect _position;
	_position.x = x;
	_position.y = y;
	_position.w = width;
	_position.h = 24*_items.size();

	pop = make_shared<dropdown_menu_overlay_t>(ref, _position);
	update_backbuffer();
	pop->map();

	_ctx->overlay_add(pop);

}

dropdown_menu_t::~dropdown_menu_t()
{
	_ctx->add_global_damage(pop->get_visible_region());
	pop->detach_myself();
}

int dropdown_menu_t::selected()
{
	return _selected;
}

xcb_timestamp_t dropdown_menu_t::time()
{
	return _time;
}

void dropdown_menu_t::update_backbuffer()
{

	cairo_t * cr = cairo_create(pop->_surf);

	for (unsigned k = 0; k < _items.size(); ++k) {
		update_items_back_buffer(cr, k);
	}

	cairo_destroy(cr);

	pop->expose(rect(0,0,pop->_position.w,pop->_position.h));

}

void dropdown_menu_t::update_items_back_buffer(cairo_t * cr, int n)
{
	if (n >= 0 and n < _items.size()) {
		rect area(0, 24 * n, pop->_position.w, 24);
		_ctx->theme()->render_menuentry(cr, _items[n]->get_theme_item(), area, n == _selected);
	}
}

void dropdown_menu_t::set_selected(int s)
{
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

void dropdown_menu_t::update_cursor_position(int x, int y)
{
	if (pop->_position.is_inside(x, y)) {
		int s = (int) floor((y - pop->_position.y) / 24.0);
		set_selected(s);
	}
}

void dropdown_menu_t::button_press(xcb_button_press_event_t const * e)
{

}


void dropdown_menu_t::button_motion(xcb_motion_notify_event_t const * e)
{
	update_cursor_position(e->root_x, e->root_y);
}

void dropdown_menu_t::button_release(xcb_button_release_event_t const * e)
{
	if (e->detail == XCB_BUTTON_INDEX_3 or e->detail == XCB_BUTTON_INDEX_1) {
		if(not pop->_position.is_inside(e->root_x, e->root_y)) {
			if(has_been_released) {
				_ctx->grab_stop(e->time);
			} else {
				has_been_released = true;
			}
		} else {
			update_cursor_position(e->root_x, e->root_y);
			_time = e->time;
			_items[_selected]->_on_click(e->time);
			_ctx->grab_stop(e->time);
		}
	}
}

void dropdown_menu_t::key_press(xcb_key_press_event_t const * ev) { }
void dropdown_menu_t::key_release(xcb_key_release_event_t const * ev) { }



}

