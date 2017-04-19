/*
 * grab_handlers.cxx
 *
 *  Created on: 24 juin 2015
 *      Author: gschwind
 */

#include <page-types.hxx>
#include <iostream>

#include "page.hxx"
#include "grab_handlers.hxx"
#include "view_notebook.hxx"
#include "view_floating.hxx"
#include "view_fullscreen.hxx"

namespace page {

using namespace std;

grab_default_t::grab_default_t(page_t * c) :
	_ctx{c}
{

}

grab_default_t::~grab_default_t()
{

}

void grab_default_t::button_press(xcb_button_press_event_t const * e)
{

}

void grab_default_t::button_motion(xcb_motion_notify_event_t const * e)
{

}

void grab_default_t::button_release(xcb_button_release_event_t const * e)
{

}

void grab_default_t::key_press(xcb_key_press_event_t const * ev)
{

}

void grab_default_t::key_release(xcb_key_release_event_t const * e)
{
	/* get KeyCode for Unmodified Key */
	xcb_keysym_t k = _ctx->keymap()->get(e->detail);

	if (k == 0)
		return;

	if (XK_Escape == k) {
		_ctx->grab_stop(e->time);
	}

}

grab_split_t::grab_split_t(page_t * ctx, shared_ptr<split_t> s) : grab_default_t{ctx}, _split{s} {
	_slider_area = s->to_root_position(s->get_split_bar_area());
	_split_ratio = s->ratio();
	_split_root_allocation = s->root_location();
	_ps = make_shared<popup_split_t>(s.get(), s);
	_ctx->overlay_add(_ps);
	_ps->show();
}

grab_split_t::~grab_split_t() {
	if(_ps != nullptr) {
		_ctx->add_global_damage(_ps->get_visible_region());
		_ps->detach_myself();
	}
}

void grab_split_t::button_press(xcb_button_press_event_t const *) {
	/* ignore */
}

void grab_split_t::button_motion(xcb_motion_notify_event_t const * e) {
	if(_split.expired()) {
		_ctx->grab_stop(e->time);
		return;
	}

	if (_split.lock()->type() == VERTICAL_SPLIT) {
		_split_ratio = (e->root_x
				- _split_root_allocation.x)
				/ (double) (_split_root_allocation.w);
	} else {
		_split_ratio = (e->root_y
				- _split_root_allocation.y)
				/ (double) (_split_root_allocation.h);
	}

	_split_ratio = _split.lock()->compute_split_constaint(_split_ratio);

	_ps->set_position(_split_ratio);
	_ctx->schedule_repaint();

}

void grab_split_t::button_release(xcb_button_release_event_t const * e) {
	if(_split.expired()) {
		_ctx->grab_stop(e->time);
		return;
	}

	if (e->detail == XCB_BUTTON_INDEX_1) {

		if (_split.lock()->type() == VERTICAL_SPLIT) {
			_split_ratio = (e->root_x
					- _split_root_allocation.x)
					/ (double) (_split_root_allocation.w);
		} else {
			_split_ratio = (e->root_y
					- _split_root_allocation.y)
					/ (double) (_split_root_allocation.h);
		}

		if (_split_ratio > 0.95)
			_split_ratio = 0.95;
		if (_split_ratio < 0.05)
			_split_ratio = 0.05;

		_split_ratio = _split.lock()->compute_split_constaint(_split_ratio);

		_split.lock()->queue_redraw();
		_split.lock()->set_split(_split_ratio);
		_ctx->grab_stop(e->time);
	}
}

grab_bind_view_notebook_t::grab_bind_view_notebook_t(page_t * ctx,
		view_notebook_p x, xcb_button_t button, rect const & pos) :
		grab_default_t{ctx},
		workspace{x->workspace()},
		c{x},
		start_position{pos},
		target_notebook{},
		zone{NOTEBOOK_AREA_NONE},
		pn0{},
		_button{button}
{
	pn0 = make_shared<popup_notebook0_t>(x.get());
}

grab_bind_view_notebook_t::~grab_bind_view_notebook_t() {
	if(pn0 != nullptr) {
		_ctx->add_global_damage(pn0->get_visible_region());
		pn0->detach_myself();
	}
}

void grab_bind_view_notebook_t::_find_target_notebook(int x, int y,
		notebook_p & target, notebook_area_e & zone) {

	target = nullptr;
	zone = NOTEBOOK_AREA_NONE;

	/* place the popup */
	auto ln = workspace->gather_children_root_first<notebook_t>();
	for (auto i : ln) {
		if (i->_area.tab.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_TAB;
			target = i;
			break;
		} else if (i->_area.right.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_RIGHT;
			target = i;
			break;
		} else if (i->_area.top.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_TOP;
			target = i;
			break;
		} else if (i->_area.bottom.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_BOTTOM;
			target = i;
			break;
		} else if (i->_area.left.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_LEFT;
			target = i;
			break;
		} else if (i->_area.popup_center.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_CENTER;
			target = i;
			break;
		}
	}
}

void grab_bind_view_notebook_t::button_press(xcb_button_press_event_t const * e) {

}

void grab_bind_view_notebook_t::button_motion(xcb_motion_notify_event_t const * e) {
	if(c.expired()) {
		_ctx->grab_stop(e->time);
		return;
	}

	/* do not start drag&drop for small move */
	if (not start_position.is_inside(e->root_x, e->root_y) and pn0->parent() == nullptr) {
		_ctx->overlay_add(pn0);
		pn0->show();
	}

	if (pn0 == nullptr)
		return;

	notebook_p new_target;
	notebook_area_e new_zone;
	_find_target_notebook(e->root_x, e->root_y, new_target, new_zone);

	if((new_target != target_notebook.lock() or new_zone != zone) and new_zone != NOTEBOOK_AREA_NONE) {
		target_notebook = new_target;
		zone = new_zone;
		switch(zone) {
		case NOTEBOOK_AREA_TAB:
			pn0->move_resize(new_target->_area.tab);
			break;
		case NOTEBOOK_AREA_RIGHT:
			pn0->move_resize(new_target->_area.popup_right);
			break;
		case NOTEBOOK_AREA_TOP:
			pn0->move_resize(new_target->_area.popup_top);
			break;
		case NOTEBOOK_AREA_BOTTOM:
			pn0->move_resize(new_target->_area.popup_bottom);
			break;
		case NOTEBOOK_AREA_LEFT:
			pn0->move_resize(new_target->_area.popup_left);
			break;
		case NOTEBOOK_AREA_CENTER:
			pn0->move_resize(new_target->_area.popup_center);
			break;
		}
	}
}

void grab_bind_view_notebook_t::button_release(xcb_button_release_event_t const * e) {
	if(c.expired()) {
		_ctx->grab_stop(e->time);
		return;
	}

	auto c = this->c.lock();

	if (e->detail == _button) {
		notebook_p new_target;
		notebook_area_e new_zone;
		_find_target_notebook(e->root_x, e->root_y, new_target, new_zone);

		/* if the mouse is no where, keep old location */
		if ((new_target == nullptr or new_zone == NOTEBOOK_AREA_NONE)
				and not target_notebook.expired()) {
			new_zone = zone;
			new_target = target_notebook.lock();
		}

		if (start_position.is_inside(e->root_x, e->root_y)) {
			c->xxactivate(e->time);
			_ctx->grab_stop(e->time);
			return;
		}

		switch(zone) {
		case NOTEBOOK_AREA_TAB:
		case NOTEBOOK_AREA_CENTER:
			if(new_target != c->parent_notebook()) {
				_ctx->move_notebook_to_notebook(c, new_target, e->time);
			}
			break;
		case NOTEBOOK_AREA_TOP:
			_ctx->split_top(new_target, c, e->time);
			break;
		case NOTEBOOK_AREA_LEFT:
			_ctx->split_left(new_target, c, e->time);
			break;
		case NOTEBOOK_AREA_BOTTOM:
			_ctx->split_bottom(new_target, c, e->time);
			break;
		case NOTEBOOK_AREA_RIGHT:
			_ctx->split_right(new_target, c, e->time);
			break;
		default:
			c->xxactivate(e->time);
		}

		_ctx->grab_stop(e->time);

	}
}


grab_bind_view_floating_t::grab_bind_view_floating_t(page_t * ctx, view_floating_p x, xcb_button_t button, rect const & pos) :
		grab_default_t{ctx},
		c{x},
		start_position{pos},
		target_notebook{},
		zone{NOTEBOOK_AREA_NONE},
		pn0{},
		_button{button}
{
	pn0 = make_shared<popup_notebook0_t>(x.get());
}

grab_bind_view_floating_t::~grab_bind_view_floating_t() {
	if(pn0 != nullptr) {
		_ctx->add_global_damage(pn0->get_visible_region());
		pn0->detach_myself();
	}
}

void grab_bind_view_floating_t::_find_target_notebook(int x, int y,
		notebook_p & target, notebook_area_e & zone) {

	target = nullptr;
	zone = NOTEBOOK_AREA_NONE;

	/* place the popup */
	auto ln = _ctx->get_current_workspace()->gather_children_root_first<notebook_t>();
	for (auto i : ln) {
		if (i->_area.tab.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_TAB;
			target = i;
			break;
		} else if (i->_area.right.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_RIGHT;
			target = i;
			break;
		} else if (i->_area.top.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_TOP;
			target = i;
			break;
		} else if (i->_area.bottom.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_BOTTOM;
			target = i;
			break;
		} else if (i->_area.left.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_LEFT;
			target = i;
			break;
		} else if (i->_area.popup_center.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_CENTER;
			target = i;
			break;
		}
	}
}

void grab_bind_view_floating_t::button_press(xcb_button_press_event_t const * e) {

}

void grab_bind_view_floating_t::button_motion(xcb_motion_notify_event_t const * e) {
	if(c.expired()) {
		_ctx->grab_stop(e->time);
		return;
	}

	/* do not start drag&drop for small move */
	if (not start_position.is_inside(e->root_x, e->root_y) and pn0->parent() == nullptr) {
		_ctx->overlay_add(pn0);
		pn0->show();
	}

	if (pn0 == nullptr)
		return;

	shared_ptr<notebook_t> new_target;
	notebook_area_e new_zone;
	_find_target_notebook(e->root_x, e->root_y, new_target, new_zone);

	if((new_target != target_notebook.lock() or new_zone != zone) and new_zone != NOTEBOOK_AREA_NONE) {
		target_notebook = new_target;
		zone = new_zone;
		switch(zone) {
		case NOTEBOOK_AREA_TAB:
			pn0->move_resize(new_target->_area.tab);
			break;
		case NOTEBOOK_AREA_RIGHT:
			pn0->move_resize(new_target->_area.popup_right);
			break;
		case NOTEBOOK_AREA_TOP:
			pn0->move_resize(new_target->_area.popup_top);
			break;
		case NOTEBOOK_AREA_BOTTOM:
			pn0->move_resize(new_target->_area.popup_bottom);
			break;
		case NOTEBOOK_AREA_LEFT:
			pn0->move_resize(new_target->_area.popup_left);
			break;
		case NOTEBOOK_AREA_CENTER:
			pn0->move_resize(new_target->_area.popup_center);
			break;
		}
	}
}

void grab_bind_view_floating_t::button_release(xcb_button_release_event_t const * e) {
	if(c.expired()) {
		_ctx->grab_stop(e->time);
		return;
	}

	auto c = this->c.lock();

	if (e->detail == _button) {
		notebook_p new_target;
		notebook_area_e new_zone;
		_find_target_notebook(e->root_x, e->root_y, new_target, new_zone);

		/* if the mouse is no where, keep old location */
		if((new_target == nullptr or new_zone == NOTEBOOK_AREA_NONE) and not target_notebook.expired()) {
			new_zone = zone;
			new_target = target_notebook.lock();
		}

		if(start_position.is_inside(e->root_x, e->root_y)) {
			c->workspace()->switch_floating_to_notebook(c, e->time);
			_ctx->grab_stop(e->time);
			return;
		}

		switch(zone) {
		case NOTEBOOK_AREA_TAB:
		case NOTEBOOK_AREA_CENTER:
			_ctx->move_floating_to_notebook(c, new_target, e->time);
			break;
		case NOTEBOOK_AREA_TOP:
			_ctx->split_top(new_target, c, e->time);
			break;
		case NOTEBOOK_AREA_LEFT:
			_ctx->split_left(new_target, c, e->time);
			break;
		case NOTEBOOK_AREA_BOTTOM:
			_ctx->split_bottom(new_target, c, e->time);
			break;
		case NOTEBOOK_AREA_RIGHT:
			_ctx->split_right(new_target, c, e->time);
			break;
		default:
			c->raise();
			c->workspace()->set_focus(c, e->time);
		}

		_ctx->grab_stop(e->time);

	}
}


grab_floating_move_t::grab_floating_move_t(page_t * ctx, view_floating_p f, unsigned int button, int x, int y) :
		grab_default_t{ctx},
		f{f},
		original_position{f->_client->get_wished_position()},
		final_position{f->_client->get_wished_position()},
		x_root{x},
		y_root{y},
		button{button},
		popup_original_position{f->_base_position},
		pfm{}
{

	f->raise();
	pfm = make_shared<popup_notebook0_t>(f.get());
	pfm->move_resize(popup_original_position);
	_ctx->overlay_add(pfm);
	pfm->show();
	_ctx->dpy()->set_window_cursor(f->_base->id(), _ctx->dpy()->xc_fleur);
	_ctx->dpy()->set_window_cursor(f->_client->_client_proxy->id(), _ctx->dpy()->xc_fleur);
}

grab_floating_move_t::~grab_floating_move_t() {
	if(pfm != nullptr) {
		_ctx->add_global_damage(pfm->get_visible_region());
		pfm->detach_myself();
	}
}

void grab_floating_move_t::button_press(xcb_button_press_event_t const * e) {
	/* ignore */
}

void grab_floating_move_t::button_motion(xcb_motion_notify_event_t const * e) {
	if(f.expired()) {
		_ctx->grab_stop(e->time);
		return;
	}

	/* compute new window position */
	rect new_position = original_position;
	new_position.x += e->root_x - x_root;
	new_position.y += e->root_y - y_root;
	final_position = new_position;

	rect new_popup_position = popup_original_position;
	new_popup_position.x += e->root_x - x_root;
	new_popup_position.y += e->root_y - y_root;
	pfm->move_resize(new_popup_position);
	_ctx->schedule_repaint();

}

void grab_floating_move_t::button_release(xcb_button_release_event_t const * e) {
	if(f.expired()) {
		_ctx->grab_stop(e->time);
		return;
	}

	auto f = this->f.lock();

	if (e->detail == XCB_BUTTON_INDEX_1 or e->detail == XCB_BUTTON_INDEX_3 or e->detail == button) {

		_ctx->dpy()->set_window_cursor(f->_base->id(), XCB_NONE);

		_ctx->dpy()->set_window_cursor(f->_client->_client_proxy->id(), XCB_NONE);

		f->_client->set_floating_wished_position(final_position);
		f->reconfigure();

		f->_root->set_focus(f, e->time);
		_ctx->grab_stop(e->time);
	}
}

xcb_cursor_t grab_floating_resize_t::_get_cursor() {
	switch(mode) {
	case RESIZE_TOP:
		return _ctx->dpy()->xc_top_side;
		break;
	case RESIZE_BOTTOM:
		return _ctx->dpy()->xc_bottom_side;
		break;
	case RESIZE_LEFT:
		return _ctx->dpy()->xc_left_side;
		break;
	case RESIZE_RIGHT:
		return _ctx->dpy()->xc_right_side;
		break;
	case RESIZE_TOP_LEFT:
		return _ctx->dpy()->xc_top_left_corner;
		break;
	case RESIZE_TOP_RIGHT:
		return _ctx->dpy()->xc_top_right_corner;
		break;
	case RESIZE_BOTTOM_LEFT:
		return _ctx->dpy()->xc_bottom_left_corner;
		break;
	case RESIZE_BOTTOM_RIGHT:
		return _ctx->dpy()->xc_bottom_righ_corner;
		break;
	}

	return XCB_WINDOW_NONE;
}

grab_floating_resize_t::grab_floating_resize_t(page_t * ctx, view_floating_p f, xcb_button_t button, int x, int y, resize_mode_e mode) :
		grab_default_t{ctx},
		f{f},
		mode{mode},
		x_root{x},
		y_root{y},
		original_position{f->_client->_absolute_position},
		final_position{f->_client->get_wished_position()},
		button{button}

{

	f->raise();
	pfm = make_shared<popup_notebook0_t>(f.get());

	rect popup_new_position = original_position;
	if (f->_client->has_motif_border()) {
		popup_new_position.x -= _ctx->theme()->floating.margin.left;
		popup_new_position.y -= _ctx->theme()->floating.margin.top;
		popup_new_position.y -= _ctx->theme()->floating.title_height;
		popup_new_position.w += _ctx->theme()->floating.margin.left
				+ _ctx->theme()->floating.margin.right;
		popup_new_position.h += _ctx->theme()->floating.margin.top
				+ _ctx->theme()->floating.margin.bottom;
		popup_new_position.h += _ctx->theme()->floating.title_height;
	}

	pfm->move_resize(popup_new_position);
	_ctx->overlay_add(pfm);
	pfm->show();


	_ctx->dpy()->set_window_cursor(f->_base->id(), _get_cursor());

}

grab_floating_resize_t::~grab_floating_resize_t() {
	if(pfm != nullptr) {
		_ctx->add_global_damage(pfm->get_visible_region());
		pfm->detach_myself();
	}
}

void grab_floating_resize_t::button_press(xcb_button_press_event_t const * e) {

}

void grab_floating_resize_t::button_motion(xcb_motion_notify_event_t const * e) {
	if(f.expired()) {
		_ctx->grab_stop(e->time);
		return;
	}

	rect size = original_position;

	switch(mode) {
	case RESIZE_TOP_LEFT:
		size.w -= e->root_x - x_root;
		size.h -= e->root_y - y_root;
		break;
	case RESIZE_TOP:
		size.h -= e->root_y - y_root;
		break;
	case RESIZE_TOP_RIGHT:
		size.w += e->root_x - x_root;
		size.h -= e->root_y - y_root;
		break;
	case RESIZE_LEFT:
		size.w -= e->root_x - x_root;
		break;
	case RESIZE_RIGHT:
		size.w += e->root_x - x_root;
		break;
	case RESIZE_BOTTOM_LEFT:
		size.w -= e->root_x - x_root;
		size.h += e->root_y - y_root;
		break;
	case RESIZE_BOTTOM:
		size.h += e->root_y - y_root;
		break;
	case RESIZE_BOTTOM_RIGHT:
		size.w += e->root_x - x_root;
		size.h += e->root_y - y_root;
		break;
	}

	/* apply normal hints */
	dimention_t<unsigned> final_size =
			f.lock()->_client->compute_size_with_constrain(size.w, size.h);
	size.w = final_size.width;
	size.h = final_size.height;

	if (size.h < 1)
		size.h = 1;
	if (size.w < 1)
		size.w = 1;

	/* do not allow to large windows */
//	if (size.w > _root_position.w - 100)
//		size.w = _root_position.w - 100;
//	if (size.h > _root_position.h - 100)
//		size.h = _root_position.h - 100;

	int x_diff = 0;
	int y_diff = 0;

	switch(mode) {
	case RESIZE_TOP_LEFT:
		x_diff = original_position.w - size.w;
		y_diff = original_position.h - size.h;
		break;
	case RESIZE_TOP:
		y_diff = original_position.h - size.h;
		break;
	case RESIZE_TOP_RIGHT:
		y_diff = original_position.h - size.h;
		break;
	case RESIZE_LEFT:
		x_diff = original_position.w - size.w;
		break;
	case RESIZE_RIGHT:
		break;
	case RESIZE_BOTTOM_LEFT:
		x_diff = original_position.w - size.w;
		break;
	case RESIZE_BOTTOM:
		break;
	case RESIZE_BOTTOM_RIGHT:
		break;
	}

	size.x += x_diff;
	size.y += y_diff;
	final_position = size;

	rect popup_new_position = size;
	if (f.lock()->_client->has_motif_border()) {
		popup_new_position.x -= _ctx->theme()->floating.margin.left;
		popup_new_position.y -= _ctx->theme()->floating.margin.top;
		popup_new_position.y -= _ctx->theme()->floating.title_height;
		popup_new_position.w += _ctx->theme()->floating.margin.left
				+ _ctx->theme()->floating.margin.right;
		popup_new_position.h += _ctx->theme()->floating.margin.top
				+ _ctx->theme()->floating.margin.bottom;
		popup_new_position.h += _ctx->theme()->floating.title_height;
	}

	pfm->move_resize(popup_new_position);

}

void grab_floating_resize_t::button_release(xcb_button_release_event_t const * e) {
	if(f.expired()) {
		_ctx->grab_stop(e->time);
		return;
	}

	auto f = this->f.lock();

	if (e->detail == button) {
		_ctx->dpy()->set_window_cursor(f->_base->id(), XCB_NONE);
		_ctx->dpy()->set_window_cursor(f->_client->_client_proxy->id(), XCB_NONE);
		f->_client->set_floating_wished_position(final_position);
		f->reconfigure();
		f->_root->set_focus(f, e->time);
		_ctx->grab_stop(e->time);
	}
}

grab_fullscreen_client_t::grab_fullscreen_client_t(page_t * ctx, view_fullscreen_p mw, xcb_button_t button, int x, int y) :
 grab_default_t{ctx},
 mw{mw},
 pn0{nullptr},
 button{button}
{
	v = _ctx->find_mouse_viewport(x, y);
	pn0 = make_shared<popup_notebook0_t>(mw.get());
	pn0->move_resize(mw->_client->_absolute_position);
	_ctx->overlay_add(pn0);
	pn0->show();

}

grab_fullscreen_client_t::~grab_fullscreen_client_t() {
	if(pn0 != nullptr) {
		_ctx->add_global_damage(pn0->get_visible_region());
		pn0->detach_myself();
	}
}

void grab_fullscreen_client_t::button_press(xcb_button_press_event_t const * e) {

}

void grab_fullscreen_client_t::button_motion(xcb_motion_notify_event_t const * e) {
	if(mw.expired()) {
		_ctx->grab_stop(e->time);
		return;
	}

	shared_ptr<viewport_t> new_viewport = _ctx->find_mouse_viewport(e->root_x, e->root_y);

	if(new_viewport != v.lock()) {
		if(new_viewport != nullptr) {
			pn0->move_resize(new_viewport->raw_area());
			_ctx->schedule_repaint();
		}
		v = new_viewport;
	}

}

void grab_fullscreen_client_t::button_release(xcb_button_release_event_t const * e) {
	if(mw.expired()) {
		_ctx->grab_stop(e->time);
		return;
	}

	if (button == e->detail) {
		/** drop the fullscreen window to the new viewport **/

		auto new_viewport = _ctx->find_mouse_viewport(e->root_x, e->root_y);

		if(new_viewport != nullptr) {
			_ctx->move_fullscreen_to_viewport(mw.lock(), new_viewport);
		}

		_ctx->grab_stop(e->time);

	}
}

void grab_alt_tab_t::_destroy_client(client_managed_t * c) {
	_destroy_func_map.erase(c);

	_client_list.remove_if([](view_w const & x) -> bool { return x.expired(); });

	for(auto & x: _popup_list) {
		x->destroy_client(c);
	}
}

grab_alt_tab_t::grab_alt_tab_t(page_t * ctx, list<view_p> managed_window, xcb_timestamp_t time) : grab_default_t{ctx} {
	_client_list = weak(managed_window);

	auto viewport_list = _ctx->get_current_workspace()->get_viewports();

	for(auto v: viewport_list) {
		auto pat = popup_alt_tab_t::create(v.get(), managed_window, v);
		pat->show();
		if(_client_list.size() > 0)
			pat->selected(_client_list.front());
		_ctx->overlay_add(pat);
		_popup_list.push_back(pat);
	}

	if(_client_list.size() > 0) {
		_selected = _client_list.front();
	}

}

grab_alt_tab_t::~grab_alt_tab_t() {
	for(auto x: _popup_list) {
		_ctx->add_global_damage(x->get_visible_region());
		x->detach_myself();
	}
}

void grab_alt_tab_t::button_press(xcb_button_press_event_t const * e) {

	if (e->detail == XCB_BUTTON_INDEX_1) {
		for(auto & pat: _popup_list) {
			pat->grab_button_press(e);
			auto _mw = pat->selected();
			if(not _mw.expired()) {
				auto mw = _mw.lock();
				_ctx->activate(mw, e->time);
				break;
			}
		}

		_ctx->grab_stop(e->time);

	}
}

void grab_alt_tab_t::button_motion(xcb_motion_notify_event_t const * e) {
	for(auto & pat: _popup_list) {
		pat->grab_button_motion(e);
		auto _mw = pat->selected();
		if(not _mw.expired()) {
			_selected = _mw;
		}
	}
}

void grab_alt_tab_t::key_press(xcb_key_press_event_t const * e) {
	/* get KeyCode for Unmodified Key */
	xcb_keysym_t k = _ctx->keymap()->get(e->detail);

	if (k == 0)
		return;

	/** XCB_MOD_MASK_2 is num_lock, thus ignore his state **/
	unsigned int state = e->state;
	if(_ctx->keymap()->numlock_mod_mask() != 0) {
		state &= ~(_ctx->keymap()->numlock_mod_mask());
	}

	if (k == XK_Tab and (state == XCB_MOD_MASK_1)) {
		if(_selected.expired()) {
			if(_client_list.size() > 0) {
				_selected = _client_list.front();
				for(auto & pat: _popup_list) {
					pat->selected(_selected);
				}
			}
		} else {
			auto c = _selected.lock();
			_selected.reset();
			auto xc = std::find_if(_client_list.begin(), _client_list.end(),
				[c](view_w & x) -> bool { return c == x.lock(); });

			if(xc != _client_list.end())
				++xc;
			if(xc != _client_list.end()) {
				_selected = (*xc);
			}

			for(auto & pat: _popup_list) {
				pat->selected(_selected);
			}

			_ctx->schedule_repaint();

		}
	}
}

void grab_alt_tab_t::key_release(xcb_key_release_event_t const * e) {
	/* get KeyCode for Unmodified Key */
	xcb_keysym_t k = _ctx->keymap()->get(e->detail);

	if (k == 0)
		return;

	/** XCB_MOD_MASK_2 is num_lock, thus ignore his state **/
	unsigned int state = e->state;
	if(_ctx->keymap()->numlock_mod_mask() != 0) {
		state &= ~(_ctx->keymap()->numlock_mod_mask());
	}

	if (XK_Escape == k) {
		_ctx->grab_stop(e->time);
		return;
	}

	/** here we guess Mod1 is bound to Alt **/
	if (XK_Alt_L == k or XK_Alt_R == k) {
		if(not _selected.expired()) {
			auto mw = _selected.lock();
			_ctx->activate(mw, e->time);
		}
		_ctx->grab_stop(e->time);
		return;
	}

}

}
