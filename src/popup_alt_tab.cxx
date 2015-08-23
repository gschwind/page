/*
 * popup_alt_tab.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "popup_alt_tab.hxx"

#include "client_managed.hxx"
#include "workspace.hxx"

namespace page {

using namespace std;

popup_alt_tab_t::popup_alt_tab_t(page_context_t * ctx, list<shared_ptr<client_managed_t>> client_list) :
	_ctx{ctx},
	_selected{},
	_is_durty{true},
	_exposed{true},
	_damaged{true}
{

	_position = _ctx->get_current_workspace()->get_any_viewport()->allocation();



	_create_composite_window();
	_ctx->dpy()->map(_wid);

	int nx = 1;
	int ny = 1;

	while(true) {
		int width = _position.w/nx;


		/* the square root may produce to much line (or column depend on the point of view
		 * We choose to remove the exide of line, but we could prefer to remove column,
		 * maybe later we will choose to select this on client_area.h/client_area.w ratio
		 *
		 *
		 * /!\ This equation use the properties of integer division.
		 *
		 * we want :
		 *  if client_counts == Q*n => m = Q
		 *  if client_counts == Q*n+R with R != 0 => m = Q + 1
		 *
		 *  within the equation :
		 *   when client_counts == Q*n => (client_counts - 1)/n + 1 == (Q - 1) + 1 == Q
		 *   when client_counts == Q*n + R => (client_counts - 1)/n + 1 == (Q*n+R-1)/n + 1
		 *     => when R == 1: (Q*n+R-1)/n + 1 == Q*n/n+1 = Q + 1
		 *     => when 1 < R <= n-1 => (Q*n+R-1)/n + 1 == Q*n/n + (R-1)/n + 1 with (R-1)/n always == 0
		 *        then (client_counts - 1)/n + 1 == Q + 1
		 *
		 */

		ny = ((client_list.size() - 1) / nx) + 1;


		if(ny * width < _position.h)
			break;
		nx++;
	}

	int width = _position.w / nx;
	int height = _position.h / ny;

	int i = 0;
	for(auto const & c: client_list) {
		int x = i % nx;
		int y = i / nx;

		int x_offset = 0;
		if(y == ny - 1) {
			x_offset = (_position.w - width * (client_list.size() - y*nx)) / 2.0;
		}

		rect pos{x*width+20+_position.x+x_offset, y*height+20+_position.y, width-40, height-40};

		cycle_window_entry_t entry;
		entry.client = c;
		entry.title = c->title();
		entry.icon = make_shared<icon64>(c.get());
		entry._thumbnail = make_shared<renderable_thumbnail_t>(_ctx, c, pos, ANCHOR_CENTER);
		entry.destroy_func = c->on_destroy.connect(this, &popup_alt_tab_t::destroy_client);
		_client_list.push_back(entry);
		++i;

	}

	_selected = _client_list.begin();

}

popup_alt_tab_t::~popup_alt_tab_t() {
	xcb_destroy_window(_ctx->dpy()->xcb(), _wid);
	_client_list.clear();
	_selected = _client_list.end();
}

void popup_alt_tab_t::_create_composite_window() {
		uint32_t value_mask = 0;
		uint32_t value[2];
		value_mask |= XCB_CW_OVERRIDE_REDIRECT;
		value[0] = True;
		value_mask |= XCB_CW_EVENT_MASK;
		value[1] = XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE;
		_wid = xcb_generate_id(_ctx->dpy()->xcb());
		xcb_create_window(_ctx->dpy()->xcb(), 0, _wid, _ctx->dpy()->root(),
				_position.x, _position.y, _position.w, _position.h, 0,
				XCB_WINDOW_CLASS_INPUT_ONLY, _ctx->dpy()->root_visual()->visual_id,
				value_mask, value);
}

void popup_alt_tab_t::move(int x, int y) {
//	_ctx->add_global_damage(_position);
//	_position.x = x;
//	_position.y = y;
//	_ctx->dpy()->move_resize(_wid, _position);
//	_ctx->add_global_damage(_position);
}

void popup_alt_tab_t::show() {
	_is_visible = true;
	_ctx->dpy()->map(_wid);
}

void popup_alt_tab_t::_init() {
	int n = 0;
	for (auto & c : _client_list) {
		c._thumbnail->set_parent(shared_from_this());
		c._thumbnail->show();
	}
}

void popup_alt_tab_t::hide() {
	_is_visible = false;
	_ctx->dpy()->unmap(_wid);
}

rect const & popup_alt_tab_t::position() {
	return _position;
}



void popup_alt_tab_t::select_next() {
	_is_durty = true;
	_exposed = true;

	if(_client_list.size() == 0) {
		_selected = _client_list.end();
		return;
	} else if (_client_list.size() == 1) {
		_selected = _client_list.begin();
		return;
	}

	_selected->_thumbnail->set_mouse_over(false);
	++_selected;
	if(_selected == _client_list.end())
		_selected = _client_list.begin();
	_selected->_thumbnail->set_mouse_over(true);

}

weak_ptr<client_managed_t> popup_alt_tab_t::get_selected() {
	if(_selected == _client_list.end()) {
		return weak_ptr<client_managed_t>{};
	} else {
		return _selected->client;
	}
}

void popup_alt_tab_t::update_backbuffer() {
	if(not _is_durty)
		return;

	_is_durty = false;
	_damaged = true;

}

void popup_alt_tab_t::paint_exposed() {

}

region popup_alt_tab_t::get_damaged() {
	if(_damaged) {
		return region{_position};
	} else {
		return region{};
	}
}

region popup_alt_tab_t::get_opaque_region() {
	return region{};
}

region popup_alt_tab_t::get_visible_region() {
	return region{_position};
}

void popup_alt_tab_t::render(cairo_t * cr, region const & area) {
	cairo_save(cr);
	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
	region r = area & _position;
	for (auto & r : area) {
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_clip(cr, r);
		cairo_paint(cr);
	}
	cairo_restore(cr);
}

void popup_alt_tab_t::destroy_client(client_managed_t * c) {

	auto i = _client_list.begin();
	while(i != _client_list.end()) {
		if(i->client.expired()) {
			if(i == _selected)
				select_next();
			i = _client_list.erase(i);
		} else if (i->client.lock().get() == c) {
			if(i == _selected)
				select_next();
			i = _client_list.erase(i);
		} else {
			++i;
		}
	}
}

xcb_window_t popup_alt_tab_t::get_xid() const {
	return _wid;
	return XCB_WINDOW_NONE;
}

xcb_window_t popup_alt_tab_t::get_parent_xid () const {
	return _wid;
	return XCB_WINDOW_NONE;
}

string popup_alt_tab_t::get_node_name() const {
	return string{"popup_alt_tab_t"};
}

void popup_alt_tab_t::trigger_redraw() {
	if(_exposed) {
		_exposed = false;
		update_backbuffer();
		paint_exposed();
	}
}

void popup_alt_tab_t::render_finished() {
	_damaged = false;
}

void popup_alt_tab_t::update_layout(time64_t const time) {
	update_backbuffer();
}

void popup_alt_tab_t::expose(xcb_expose_event_t const * ev) {
	//if(ev->window == _wid)
	//	_exposed = true;
}

void popup_alt_tab_t::append_children(vector<shared_ptr<tree_t>> & out) const {
	for(auto & e: _client_list) {
		out.push_back(e._thumbnail);
	}
}

void popup_alt_tab_t::_select_from_mouse(int x, int y) {
	auto i = _client_list.begin();
	while(i != _client_list.end()) {
		if(i->_thumbnail->get_visible_region().is_inside(x, y)) {
			_selected->_thumbnail->set_mouse_over(false);
			_selected = i;
			_selected->_thumbnail->set_mouse_over(true);
			break;
		}
		++i;
	}
}

void popup_alt_tab_t::grab_button_press(xcb_button_press_event_t const * ev) {
	_select_from_mouse(ev->root_x, ev->root_y);
}

void popup_alt_tab_t::grab_button_motion(xcb_motion_notify_event_t const * ev) {
	_select_from_mouse(ev->root_x, ev->root_y);
}




}
