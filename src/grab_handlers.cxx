/*
 * grab_handlers.cxx
 *
 *  Created on: 24 juin 2015
 *      Author: gschwind
 */

#include "grab_handlers.hxx"
#include "page_context.hxx"

namespace page {

grab_split_t::grab_split_t(page_context_t * ctx, split_t * s) : _ctx{ctx}, _split{s} {
	_slider_area = _split->get_split_bar_area();
	_split_ratio = _split->ratio();
	_ps = std::make_shared<popup_split_t>(ctx, s);
	_ctx->overlay_add(_ps);
}

grab_split_t::~grab_split_t() {
	if(_ps != nullptr) {
		_ctx->overlay_remove(_ps);
	}
}

void grab_split_t::button_press(xcb_button_press_event_t const *) {
	/* ignore */
}

void grab_split_t::button_motion(xcb_motion_notify_event_t const * e) {
	if (_split->type() == VERTICAL_SPLIT) {
		_split_ratio = (e->event_x
				- _split->allocation().x)
				/ (double) (_split->allocation().w);
	} else {
		_split_ratio = (e->event_y
				- _split->allocation().y)
				/ (double) (_split->allocation().h);
	}

	if (_split_ratio > 0.95)
		_split_ratio = 0.95;
	if (_split_ratio < 0.05)
		_split_ratio = 0.05;

	/* Render slider with quite complex render method to avoid flickering */
	i_rect old_area = _slider_area;
	_split->compute_split_location(_split_ratio,
			_slider_area.x, _slider_area.y);

	_ps->set_position(_split_ratio);
	_ctx->add_global_damage(_ps->position());
}

void grab_split_t::button_release(xcb_button_release_event_t const * e) {
	if (e->detail == XCB_BUTTON_INDEX_1) {
		_split->queue_redraw();
		if(_ps != nullptr) {
			_ctx->overlay_remove(_ps);
			_ps = nullptr;
		}
		_ctx->add_global_damage(_split->allocation());
		_split->set_split(_split_ratio);
		_ctx->grab_stop();
	}
}

grab_bind_client_t::grab_bind_client_t(page_context_t * ctx, client_managed_t * c, xcb_button_t button, i_rect const & pos) :
		ctx{ctx},
		c{c},
		start_position{pos},
		target_notebook{nullptr},
		zone{NOTEBOOK_AREA_NONE},
		pn0{nullptr},
		_button{button}
{


}

grab_bind_client_t::~grab_bind_client_t() {
	if(pn0 != nullptr) {
		ctx->overlay_remove(pn0);
	}
}

void grab_bind_client_t::_find_target_notebook(int x, int y,
		notebook_t * & target, notebook_area_e & zone) {

	target = nullptr;
	zone = NOTEBOOK_AREA_NONE;

	/* place the popup */
	auto ln = filter_class<notebook_t>(
			ctx->get_current_workspace()->tree_t::get_all_children());
	for (auto i : ln) {
		if (i->tab_area.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_TAB;
			target = i;
			break;
		} else if (i->right_area.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_RIGHT;
			target = i;
			break;
		} else if (i->top_area.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_TOP;
			target = i;
			break;
		} else if (i->bottom_area.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_BOTTOM;
			target = i;
			break;
		} else if (i->left_area.is_inside(x, y)) {
			zone = NOTEBOOK_AREA_LEFT;
			target = i;
			break;
		} else if (i->popup_center_area.is_inside(x, y)) {
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
		pn0 = std::make_shared<popup_notebook0_t>(ctx);
		ctx->overlay_add(pn0);
	}

	if (pn0 == nullptr)
		return;

	notebook_t * new_target;
	notebook_area_e new_zone;
	_find_target_notebook(e->root_x, e->root_y, new_target, new_zone);

	if(new_target != target_notebook or new_zone != zone) {
		target_notebook = new_target;
		zone = new_zone;
		switch(zone) {
		case NOTEBOOK_AREA_TAB:
			pn0->move_resize(target_notebook->tab_area);
			break;
		case NOTEBOOK_AREA_RIGHT:
				pn0->move_resize(target_notebook->popup_right_area);
			break;
		case NOTEBOOK_AREA_TOP:
			pn0->move_resize(target_notebook->popup_top_area);
			break;
		case NOTEBOOK_AREA_BOTTOM:
			pn0->move_resize(target_notebook->popup_bottom_area);
			break;
		case NOTEBOOK_AREA_LEFT:
			pn0->move_resize(target_notebook->popup_left_area);
			break;
		case NOTEBOOK_AREA_CENTER:
				pn0->move_resize(target_notebook->popup_center_area);
			break;
		}
	}
}

void grab_bind_client_t::button_release(xcb_button_release_event_t const * e) {
	if (e->detail == _button) {

		_find_target_notebook(e->root_x, e->root_y, target_notebook, zone);

		if(target_notebook == nullptr or zone == NOTEBOOK_AREA_NONE or start_position.is_inside(e->root_x, e->root_y)) {
			if(c->is(MANAGED_FLOATING)) {
				ctx->insert_window_in_notebook(c, nullptr, true);
			}

			c->activate();
			ctx->set_focus(c, e->time);
			ctx->grab_stop();
			return;
		}

		switch(zone) {
		case NOTEBOOK_AREA_TAB:
		case NOTEBOOK_AREA_CENTER:
			if(target_notebook != c->parent()) {
				target_notebook->queue_redraw();
				c->queue_redraw();
				ctx->detach(c);
				ctx->insert_window_in_notebook(c, target_notebook, true);
			}
			break;
		case NOTEBOOK_AREA_TOP:
			ctx->split_top(target_notebook, c);
			break;
		case NOTEBOOK_AREA_LEFT:
			ctx->split_left(target_notebook, c);
			break;
		case NOTEBOOK_AREA_BOTTOM:
			ctx->split_bottom(target_notebook, c);
			break;
		case NOTEBOOK_AREA_RIGHT:
			ctx->split_right(target_notebook, c);
			break;
		default:
			notebook_t * parent = dynamic_cast<notebook_t *>(c->parent());
			if (parent != nullptr) {
				c->queue_redraw();
				/* hide client if option allow shaded client */
				if (parent->get_selected() == c
						and ctx->get_current_workspace()->client_focus.front() == c
						and /*_enable_shade_windows*/true) {
					ctx->set_focus(nullptr, e->time);
					parent->iconify_client(c);
				} else {
					std::cout << "activate = " << c << endl;
					c->activate();
					ctx->set_focus(c, e->time);
				}
			}
		}

		ctx->grab_stop();

	}
}


grab_floating_move_t::grab_floating_move_t(page_context_t * ctx, client_managed_t * f, unsigned int button, int x, int y) :
		_ctx{ctx},
		f{f},
		original_position{f->get_wished_position()},
		final_position{f->get_wished_position()},
		x_root{x},
		y_root{y},
		button{button},
		popup_original_position{f->get_base_position()},
		pfm{nullptr}
{
	_ctx->safe_raise_window(f);
	pfm = std::make_shared<popup_notebook0_t>(_ctx);
	pfm->move_resize(popup_original_position);
	_ctx->overlay_add(pfm);
	_ctx->dpy()->set_window_cursor(f->base(), _ctx->dpy()->xc_fleur);
	_ctx->dpy()->set_window_cursor(f->orig(), _ctx->dpy()->xc_fleur);
}

grab_floating_move_t::~grab_floating_move_t() {
	if (pfm != nullptr) {
		_ctx->overlay_remove(pfm);
	}
}

void grab_floating_move_t::button_press(xcb_button_press_event_t const * e) {
	/* ignore */
}

void grab_floating_move_t::button_motion(xcb_motion_notify_event_t const * e) {

	_ctx->add_global_damage(f->visible_area());

	/* compute new window position */
	i_rect new_position = original_position;
	new_position.x += e->root_x - x_root;
	new_position.y += e->root_y - y_root;
	final_position = new_position;

	i_rect new_popup_position = popup_original_position;
	new_popup_position.x += e->root_x - x_root;
	new_popup_position.y += e->root_y - y_root;
	pfm->move_resize(new_popup_position);

}

void grab_floating_move_t::button_release(xcb_button_release_event_t const * e) {
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
}

grab_floating_resize_t::grab_floating_resize_t(page_context_t * ctx, client_managed_t * f, xcb_button_t button, int x, int y, resize_mode_e mode) :
		_ctx{ctx},
		f{f},
		mode{mode},
		x_root{x},
		y_root{y},
		original_position{f->get_wished_position()},
		final_position{f->get_wished_position()},
		button{button},
		pfm{nullptr}

{

	_ctx->safe_raise_window(f);
	pfm = std::make_shared<popup_notebook0_t>(_ctx);
	pfm->move_resize(f->base_position());
	_ctx->overlay_add(pfm);

	_ctx->dpy()->set_window_cursor(f->base(), _get_cursor());

}

grab_floating_resize_t::~grab_floating_resize_t() {
	if (pfm != nullptr) {
		_ctx->overlay_remove(pfm);
	}
}

void grab_floating_resize_t::button_press(xcb_button_press_event_t const * e) {

}

void grab_floating_resize_t::button_motion(xcb_motion_notify_event_t const * e) {
	_ctx->add_global_damage(f->visible_area());

	i_rect size = original_position;

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
			f->compute_size_with_constrain(size.w, size.h);
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

	i_rect popup_new_position = size;
	if (f->has_motif_border()) {
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

grab_fullscreen_client_t::grab_fullscreen_client_t(page_context_t * ctx, client_managed_t * mw, xcb_button_t button, int x, int y) :
 _ctx{ctx},
 mw{mw},
 pn0{nullptr},
 button{button}
{
	v = _ctx->find_mouse_viewport(x, y);
	pn0 = std::make_shared<popup_notebook0_t>(ctx);
	pn0->move_resize(mw->base_position());
	_ctx->overlay_add(pn0);
}

grab_fullscreen_client_t::~grab_fullscreen_client_t() {
	if (pn0 != nullptr) {
		_ctx->overlay_remove(pn0);
	}
}

void grab_fullscreen_client_t::button_press(xcb_button_press_event_t const * e) {

}

void grab_fullscreen_client_t::button_motion(xcb_motion_notify_event_t const * e) {
	viewport_t * new_viewport = _ctx->find_mouse_viewport(e->root_x, e->root_y);

	if(new_viewport != nullptr) {
		v = new_viewport;
	}

	if(v != nullptr) {
		pn0->move_resize(v->raw_area());
	}

}

void grab_fullscreen_client_t::button_release(xcb_button_release_event_t const * e) {
	if (button == e->detail) {
		/** drop the fullscreen window to the new viewport **/

		viewport_t * new_viewport = _ctx->find_mouse_viewport(e->root_x, e->root_y);

		if(new_viewport != nullptr) {
			v = new_viewport;
		}

		if(v != nullptr) {
			_ctx->fullscreen_client_to_viewport(mw, v);
		}

		_ctx->grab_stop();

	}
}

}
