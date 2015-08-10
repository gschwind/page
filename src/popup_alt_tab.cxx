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



	//_create_composite_window();
	//_ctx->dpy()->map(_wid);
	//_surf = make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGB, _position.w, _position.h);

	int nx = 1;
	int ny = 1;

	while(true) {
		int width = _position.w/nx;
		ny = client_list.size() / nx;
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

		rect pos{x*width+5+_position.x, y*height+5+_position.y, width-10, height-10};

		cycle_window_entry_t entry;
		entry.client = c;
		entry.title = c->title();
		entry.icon = make_shared<icon64>(c.get());
		entry._thumbnail = make_shared<renderable_thumbnail_t>(_ctx, pos, c);
		entry.destroy_func = c->on_destroy.connect(this, &popup_alt_tab_t::destroy_client);
		_client_list.push_back(entry);
		++i;

	}

	_selected = _client_list.begin();

}

popup_alt_tab_t::~popup_alt_tab_t() {
	//xcb_destroy_window(_ctx->dpy()->xcb(), _wid);
	_client_list.clear();
	_selected = _client_list.end();
}

void popup_alt_tab_t::_create_composite_window() {
//	xcb_colormap_t cmap = xcb_generate_id(_ctx->dpy()->xcb());
//	xcb_create_colormap(_ctx->dpy()->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, _ctx->dpy()->root(), _ctx->dpy()->root_visual()->visual_id);
//
//	uint32_t value_mask = 0;
//	uint32_t value[5];
//
//	value_mask |= XCB_CW_BACK_PIXEL;
//	value[0] = _ctx->dpy()->xcb_screen()->black_pixel;
//
//	value_mask |= XCB_CW_BORDER_PIXEL;
//	value[1] = _ctx->dpy()->xcb_screen()->black_pixel;
//
//	value_mask |= XCB_CW_OVERRIDE_REDIRECT;
//	value[2] = True;
//
//	value_mask |= XCB_CW_EVENT_MASK;
//	value[3] = XCB_EVENT_MASK_EXPOSURE;
//
//	value_mask |= XCB_CW_COLORMAP;
//	value[4] = cmap;

	//_wid = xcb_generate_id(_ctx->dpy()->xcb());
	//xcb_create_window(_ctx->dpy()->xcb(), _ctx->dpy()->root_depth(), _wid, _ctx->dpy()->root(),
	//		_position.x, _position.y, _position.w, _position.h, 0,
	//		XCB_WINDOW_CLASS_INPUT_OUTPUT, _ctx->dpy()->root_visual()->visual_id,
	//		value_mask, value);
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
	///_ctx->dpy()->map(_wid);
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
	//_ctx->dpy()->unmap(_wid);
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

//	rect a{0,0,_position.w,_position.h};
//	cairo_t * cr = cairo_create(_surf->get_cairo_surface());
//	//cairo_clip(cr, a);
//	//cairo_translate(cr, _position.x, _position.y);
//	cairo_rectangle(cr, 0, 0, _position.w, _position.h);
//	cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
//	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
//	cairo_fill(cr);
//
//	cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
//	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
//
//	int n = 0;
//	for (auto i : _client_list) {
//		int x = n % 4;
//		int y = n / 4;
//
//		if (i->icon != nullptr) {
//			if (i->icon->get_cairo_surface() != nullptr) {
//				cairo_set_source_surface(cr,
//						i->icon->get_cairo_surface(), x * 80 + 8,
//						y * 80 + 8);
//				cairo_mask_surface(cr, i->icon->get_cairo_surface(),
//						x * 80 + 8, y * 80 + 8);
//
//			}
//		}
//
//		if (i == *_selected) {
//			/** draw a beautiful yellow box **/
//			cairo_set_line_width(cr, 2.0);
//			::cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
//			cairo_rectangle(cr, x * 80 + 8, y * 80 + 8, 64, 64);
//			cairo_stroke(cr);
//		}
//
//		++n;
//
//	}
//	cairo_destroy(cr);
}

void popup_alt_tab_t::paint_exposed() {
//	cairo_surface_t * surf = cairo_xcb_surface_create(_ctx->dpy()->xcb(), _wid,
//			_ctx->dpy()->root_visual(), _position.w, _position.h);
//	cairo_t * cr = cairo_create(surf);
//	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
//	cairo_set_source_surface(cr, _surf->get_cairo_surface(), 0.0, 0.0);
//	cairo_rectangle(cr, 0, 0, _position.w, _position.h);
//	cairo_fill(cr);
//	cairo_destroy(cr);
//	cairo_surface_destroy(surf);
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
	::cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
	region r = area & _position;
	for (auto & r : area) {
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_clip(cr, r);
		//cairo_set_source_surface(cr, _surf->get_cairo_surface(), _position.x, _position.y);
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
	//return _wid;
	return XCB_WINDOW_NONE;
}

xcb_window_t popup_alt_tab_t::get_parent_xid () const {
	//return _wid;
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




}
