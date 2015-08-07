/*
 * grab_handlers.cxx
 *
 *  Created on: 24 juin 2015
 *      Author: gschwind
 */

#include "grab_handlers.hxx"
#include "page_context.hxx"

namespace page {

using namespace std;

grab_split_t::grab_split_t(page_context_t * ctx, shared_ptr<split_t> s) : _ctx{ctx}, _split{s} {
	_slider_area = s->to_root_position(s->get_split_bar_area());
	_split_ratio = s->ratio();
	_split_root_allocation = s->to_root_position(s->allocation());
	_ps = make_shared<popup_split_t>(ctx, s);
	_ctx->overlay_add(_ps);
}

grab_split_t::~grab_split_t() {
	if(_ps != nullptr)
		_ctx->detach(_ps);
}

void grab_split_t::button_press(xcb_button_press_event_t const *) {
	/* ignore */
}

void grab_split_t::button_motion(xcb_motion_notify_event_t const * e) {
	if(_split.expired()) {
		_ctx->grab_stop();
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

	if (_split_ratio > 0.95)
		_split_ratio = 0.95;
	if (_split_ratio < 0.05)
		_split_ratio = 0.05;

	/* Render slider with quite complex render method to avoid flickering */
	rect old_area = _slider_area;
	_split.lock()->compute_split_location(_split_ratio,
			_slider_area.x, _slider_area.y);
	_slider_area = _split.lock()->to_root_position(_slider_area);

	_ps->set_position(_split_ratio);
	_ctx->add_global_damage(_ps->position());
}

void grab_split_t::button_release(xcb_button_release_event_t const * e) {
	if(_split.expired()) {
		_ctx->grab_stop();
		return;
	}

	if (e->detail == XCB_BUTTON_INDEX_1) {
		_split.lock()->queue_redraw();
		if(_ps != nullptr) {
			_ctx->detach(_ps);
			_ps = nullptr;
		}
		_ctx->add_global_damage(_split_root_allocation);
		_split.lock()->set_split(_split_ratio);
		_ctx->grab_stop();
	}
}

grab_bind_client_t::grab_bind_client_t(page_context_t * ctx, shared_ptr<client_managed_t> c, xcb_button_t button, rect const & pos) :
		ctx{ctx},
		c{c},
		start_position{pos},
		target_notebook{},
		zone{NOTEBOOK_AREA_NONE},
		pn0{},
		_button{button}
{


}

grab_bind_client_t::~grab_bind_client_t() {
	if(pn0 != nullptr)
		ctx->detach(pn0);
}

void grab_bind_client_t::_find_target_notebook(int x, int y,
		shared_ptr<notebook_t> & target, notebook_area_e & zone) {

	target = nullptr;
	zone = NOTEBOOK_AREA_NONE;

	/* place the popup */
	auto ln = filter_class<notebook_t>(
			ctx->get_current_workspace()->get_all_children());
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

void grab_bind_client_t::button_press(xcb_button_press_event_t const * e) {

}

void grab_bind_client_t::button_motion(xcb_motion_notify_event_t const * e) {


	/* do not start drag&drop for small move */
	if (not start_position.is_inside(e->root_x, e->root_y) and pn0 == nullptr) {
		pn0 = make_shared<popup_notebook0_t>(ctx);
		ctx->overlay_add(pn0);
	}

	if (pn0 == nullptr)
		return;

	shared_ptr<notebook_t> new_target;
	notebook_area_e new_zone;
	_find_target_notebook(e->root_x, e->root_y, new_target, new_zone);

	if(new_target != target_notebook.lock() or new_zone != zone) {
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

void grab_bind_client_t::button_release(xcb_button_release_event_t const * e) {
	if(c.expired()) {
		ctx->grab_stop();
		return;
	}

	auto c = this->c.lock();

	if (e->detail == _button) {

		shared_ptr<notebook_t> new_target;
		_find_target_notebook(e->root_x, e->root_y, new_target, zone);

		if(new_target == nullptr or zone == NOTEBOOK_AREA_NONE or start_position.is_inside(e->root_x, e->root_y)) {
			if(c->is(MANAGED_FLOATING)) {
				ctx->detach(c);
				ctx->insert_window_in_notebook(c, nullptr, true);
			} else {
				c->activate(nullptr);
				ctx->set_focus(c, e->time);
			}
			ctx->grab_stop();
			return;
		}

		switch(zone) {
		case NOTEBOOK_AREA_TAB:
		case NOTEBOOK_AREA_CENTER:
			if(new_target != c->parent().lock()) {
				new_target->queue_redraw();
				c->queue_redraw();
				ctx->detach(c);
				ctx->insert_window_in_notebook(c, new_target, true);
			}
			break;
		case NOTEBOOK_AREA_TOP:
			ctx->split_top(new_target, c);
			break;
		case NOTEBOOK_AREA_LEFT:
			ctx->split_left(new_target, c);
			break;
		case NOTEBOOK_AREA_BOTTOM:
			ctx->split_bottom(new_target, c);
			break;
		case NOTEBOOK_AREA_RIGHT:
			ctx->split_right(new_target, c);
			break;
		default:
			auto parent = dynamic_pointer_cast<notebook_t>(c->parent().lock());
			if (parent != nullptr) {
				c->queue_redraw();

				/* hide client if option allow shaded client */
				if (parent->selected() == c
						and ((not ctx->get_current_workspace()->client_focus.empty())?ctx->get_current_workspace()->client_focus.front().lock() == c:false)
						and /*_enable_shade_windows*/true) {
					ctx->set_focus(nullptr, e->time);
					parent->iconify_client(c);
				} else {
					std::cout << "activate = " << c << endl;
					c->activate(nullptr);
					ctx->set_focus(c, e->time);
				}
			}
		}

		ctx->grab_stop();

	}
}


grab_floating_move_t::grab_floating_move_t(page_context_t * ctx, shared_ptr<client_managed_t> f, unsigned int button, int x, int y) :
		_ctx{ctx},
		f{f},
		original_position{f->get_wished_position()},
		final_position{f->get_wished_position()},
		x_root{x},
		y_root{y},
		button{button},
		popup_original_position{f->get_base_position()},
		pfm{}
{
	_ctx->safe_raise_window(f);
	pfm = make_shared<popup_notebook0_t>(_ctx);
	pfm->move_resize(popup_original_position);
	_ctx->overlay_add(pfm);
	_ctx->dpy()->set_window_cursor(f->base(), _ctx->dpy()->xc_fleur);
	_ctx->dpy()->set_window_cursor(f->orig(), _ctx->dpy()->xc_fleur);
}

grab_floating_move_t::~grab_floating_move_t() {
	if (pfm != nullptr)
		_ctx->detach(pfm);
}

void grab_floating_move_t::button_press(xcb_button_press_event_t const * e) {
	/* ignore */
}

void grab_floating_move_t::button_motion(xcb_motion_notify_event_t const * e) {
	if(f.expired()) {
		_ctx->grab_stop();
		return;
	}

	_ctx->add_global_damage(f.lock()->get_visible_region());

	/* compute new window position */
	rect new_position = original_position;
	new_position.x += e->root_x - x_root;
	new_position.y += e->root_y - y_root;
	final_position = new_position;

	rect new_popup_position = popup_original_position;
	new_popup_position.x += e->root_x - x_root;
	new_popup_position.y += e->root_y - y_root;
	pfm->move_resize(new_popup_position);

}

void grab_floating_move_t::button_release(xcb_button_release_event_t const * e) {
	if(f.expired()) {
		_ctx->grab_stop();
		return;
	}

	auto f = this->f.lock();

	if (e->detail == XCB_BUTTON_INDEX_1 or e->detail == XCB_BUTTON_INDEX_3 or e->detail == button) {

		_ctx->dpy()->set_window_cursor(f->base(), XCB_NONE);

		if (_ctx->dpy()->lock(f->orig())) {
			_ctx->dpy()->set_window_cursor(f->orig(), XCB_NONE);
			_ctx->dpy()->unlock();
		}

		f->set_floating_wished_position(final_position);
		f->reconfigure();

		_ctx->set_focus(f, e->time);
		_ctx->grab_stop();
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

grab_floating_resize_t::grab_floating_resize_t(page_context_t * ctx, shared_ptr<client_managed_t> f, xcb_button_t button, int x, int y, resize_mode_e mode) :
		_ctx{ctx},
		f{f},
		mode{mode},
		x_root{x},
		y_root{y},
		original_position{f->get_wished_position()},
		final_position{f->get_wished_position()},
		button{button}

{

	_ctx->safe_raise_window(f);
	pfm = make_shared<popup_notebook0_t>(_ctx);
	pfm->move_resize(f->base_position());
	_ctx->overlay_add(pfm);

	_ctx->dpy()->set_window_cursor(f->base(), _get_cursor());

}

grab_floating_resize_t::~grab_floating_resize_t() {
	if(pfm != nullptr)
		_ctx->detach(pfm);
}

void grab_floating_resize_t::button_press(xcb_button_press_event_t const * e) {

}

void grab_floating_resize_t::button_motion(xcb_motion_notify_event_t const * e) {
	if(f.expired()) {
		_ctx->grab_stop();
		return;
	}

	_ctx->add_global_damage(f.lock()->get_visible_region());

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
			f.lock()->compute_size_with_constrain(size.w, size.h);
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
	if (f.lock()->has_motif_border()) {
		popup_new_position.x -= _ctx->theme()->floating.margin.left;
		popup_new_position.y -= _ctx->theme()->floating.margin.top;
		popup_new_position.w += _ctx->theme()->floating.margin.left
				+ _ctx->theme()->floating.margin.right;
		popup_new_position.h += _ctx->theme()->floating.margin.top
				+ _ctx->theme()->floating.margin.bottom;
	}

	pfm->move_resize(popup_new_position);

}

void grab_floating_resize_t::button_release(xcb_button_release_event_t const * e) {
	if(f.expired()) {
		_ctx->grab_stop();
		return;
	}

	auto f = this->f.lock();

	if (e->detail == button) {
		_ctx->dpy()->set_window_cursor(f->base(), XCB_NONE);
		if (_ctx->dpy()->lock(f->orig())) {
			_ctx->dpy()->set_window_cursor(f->orig(), XCB_NONE);
			_ctx->dpy()->unlock();
		}
		f->set_floating_wished_position(final_position);
		f->reconfigure();
		_ctx->set_focus(f, e->time);
		_ctx->grab_stop();
	}
}

grab_fullscreen_client_t::grab_fullscreen_client_t(page_context_t * ctx, shared_ptr<client_managed_t> mw, xcb_button_t button, int x, int y) :
 _ctx{ctx},
 mw{mw},
 pn0{nullptr},
 button{button}
{
	v = _ctx->find_mouse_viewport(x, y);
	pn0 = make_shared<popup_notebook0_t>(ctx);
	pn0->move_resize(mw->base_position());
	_ctx->overlay_add(pn0);
	pn0->show();

}

grab_fullscreen_client_t::~grab_fullscreen_client_t() {
	if (pn0 != nullptr)
		_ctx->detach(pn0);
}

void grab_fullscreen_client_t::button_press(xcb_button_press_event_t const * e) {

}

void grab_fullscreen_client_t::button_motion(xcb_motion_notify_event_t const * e) {
	if(mw.expired()) {
		_ctx->grab_stop();
		return;
	}

	shared_ptr<viewport_t> new_viewport = _ctx->find_mouse_viewport(e->root_x, e->root_y);

	if(new_viewport != v.lock()) {
		if(new_viewport != nullptr) {
			pn0->move_resize(new_viewport->raw_area());
		}
		v = new_viewport;
	}

}

void grab_fullscreen_client_t::button_release(xcb_button_release_event_t const * e) {
	if(mw.expired()) {
		_ctx->grab_stop();
		return;
	}

	if (button == e->detail) {
		/** drop the fullscreen window to the new viewport **/

		shared_ptr<viewport_t> new_viewport = _ctx->find_mouse_viewport(e->root_x, e->root_y);

		if(new_viewport != nullptr) {
			_ctx->fullscreen_client_to_viewport(mw.lock(), new_viewport);
		}

		_ctx->grab_stop();

	}
}

grab_alt_tab_t::grab_alt_tab_t(page_context_t * ctx) : _ctx{ctx} {
	auto _x = _ctx->clients_list();
	list<weak_ptr<client_managed_t>> managed_window{_x.begin(), _x.end()};

	auto focus_history = _ctx->global_client_focus_history();
	/* reorder client to follow focused order */
	for (auto i = focus_history.rbegin();
			i != focus_history.rend();
			++i) {
		if (not i->expired()) {
			managed_window.remove_if([i](weak_ptr<client_managed_t> x) -> bool { return i->lock() == x.lock(); });
			managed_window.push_front(*i);
		}
	}

	/** create all menu entry and find the selected one **/
	list<shared_ptr<cycle_window_entry_t>> v;
	for (auto i : managed_window) {
		if(i.expired())
			continue;

		if(i.lock()->is(MANAGED_DOCK))
			continue;

		if(i.lock()->skip_task_bar())
			continue;

		auto icon = make_shared<icon64>(i.lock().get());
		auto cy = make_shared<cycle_window_entry_t>(i.lock(), i.lock()->title(), icon);
		v.push_back(cy);
	}

	pat = make_shared<popup_alt_tab_t>(_ctx, v, 0);

	/** TODO: show it on all viewport **/
	auto viewport = _ctx->get_current_workspace()->get_any_viewport();
	pat->move(viewport->raw_area().x
							+ (viewport->raw_area().w - pat->position().w) / 2,
					viewport->raw_area().y
							+ (viewport->raw_area().h - pat->position().h) / 2);
	pat->show();
	pat->select_next();
	_ctx->overlay_add(pat);
}

grab_alt_tab_t::~grab_alt_tab_t() {
	cout << "grab_atl_tab terminated" << endl;
	if(pat != nullptr)
		_ctx->detach(pat);
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
		pat->select_next();
		_ctx->add_global_damage(pat->position());
	}

}

void grab_alt_tab_t::key_release(xcb_key_release_event_t const * e) {
	weak_ptr<client_managed_t> _mw = pat->get_selected();
	if(_mw.expired()) {
		xcb_ungrab_keyboard(_ctx->dpy()->xcb(), e->time);
		_ctx->grab_stop();
		return;
	}

	auto mw = _mw.lock();

	/* get KeyCode for Unmodified Key */
	xcb_keysym_t k = _ctx->keymap()->get(e->detail);

	if (k == 0)
		return;

	/** XCB_MOD_MASK_2 is num_lock, thus ignore his state **/
	unsigned int state = e->state;
	if(_ctx->keymap()->numlock_mod_mask() != 0) {
		state &= ~(_ctx->keymap()->numlock_mod_mask());
	}

	/** here we guess Mod1 is bound to Alt **/
	if (XK_Alt_L == k or XK_Alt_R == k) {
		xcb_ungrab_keyboard(_ctx->dpy()->xcb(), e->time);
		mw->activate(nullptr);
		_ctx->set_focus(mw, e->time);
		_ctx->grab_stop();
	}

}

}
