/*
 * notebook.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "workspace.hxx"
#include "notebook.hxx"
#include "dropdown_menu.hxx"
#include "grab_handlers.hxx"
#include "renderable_unmanaged_gaussian_shadow.hxx"

namespace page {

using namespace std;

notebook_t::notebook_t(page_context_t * ctx) :
		_ctx{ctx},
		_is_default{false},
		_selected{nullptr},
		_exposay{false},
		_mouse_over{-1, -1, nullptr, nullptr},
		_can_hsplit{true},
		_can_vsplit{true},
		_theme_client_tabs_offset{0},
		_has_scroll_arrow{false},
		_layout_is_durty{true},
		_has_mouse_change{true},
		animation_duration{ctx->conf()._fade_in_time}
{

}

notebook_t::~notebook_t() {
	_clients_tab_order.clear();
}

bool notebook_t::add_client(client_managed_p x, bool prefer_activate) {
	assert(not _has_client(x));
	assert(x != nullptr);

	x->set_parent(this);
	x->set_managed_type(MANAGED_NOTEBOOK);
	_children.push_back(x);

	_client_context_t client_context{this, x};
	_clients_tab_order.push_front(client_context);

	if(prefer_activate and not _exposay) {
		_start_fading();

		/* remove current selected */
		if (_selected != nullptr) {
			_selected->iconify();
			_selected->hide();
		}

		/* select the new one */
		_selected = x;
		if(_is_visible) {
			_selected->normalize();
			_selected->show();
		} else {
			_selected->iconify();
			_selected->hide();
		}

	} else {
		x->iconify();
		x->hide();
	}

	_layout_is_durty = true;
	return true;
}

rect notebook_t::_get_new_client_size() {
	return rect(
		allocation().x + _ctx->theme()->notebook.margin.left,
		allocation().y + _ctx->theme()->notebook.margin.top + _ctx->theme()->notebook.tab_height,
		allocation().w - _ctx->theme()->notebook.margin.right - _ctx->theme()->notebook.margin.left,
		allocation().h - _ctx->theme()->notebook.margin.top - _ctx->theme()->notebook.margin.bottom - _ctx->theme()->notebook.tab_height
	);
}

void notebook_t::replace(shared_ptr<page_component_t> src, shared_ptr<page_component_t> by) {
	throw std::runtime_error("cannot replace in notebook");
}

void notebook_t::remove(shared_ptr<tree_t> src) {
	auto mw = dynamic_pointer_cast<client_managed_t>(src);
	if (_has_client(mw)) {
		_remove_client(mw);
	}
}

void notebook_t::_activate_client(shared_ptr<client_managed_t> x) {
	if (_has_client(x)) {
		_set_selected(x);
	}
}

void notebook_t::_remove_client(shared_ptr<client_managed_t> x) {
	auto x_client_context = _find_client_context(x);
	if(x_client_context == _clients_tab_order.end())
		return;

	/** update selection **/
	if (_selected == x) {
		_start_fading();
		_selected->iconify();
		_selected->hide();
		_selected = nullptr;
	}

	// cleanup
	_children.remove(x);
	x->clear_parent();

	_clients_tab_order.erase(x_client_context);

	_mouse_over_reset();

	if(not _ctx->conf()._enable_shade_windows
			and not _children.empty()
			and _selected == nullptr
			and not _exposay) {
		_selected = dynamic_pointer_cast<client_managed_t>(_children.back());

		if (_selected != nullptr and _is_visible) {
			_selected->normalize();
			_selected->show();
		}
	}

	_layout_is_durty = true;

}

void notebook_t::_set_selected(shared_ptr<client_managed_t> c) {
	/** already selected **/
	if(_selected == c and not c->is_iconic())
		return;

	_stop_exposay();
	_start_fading();

	if(_selected != nullptr and c != _selected) {
		_selected->iconify();
		_selected->hide();
	}
	/** set selected **/
	_selected = c;
	if(_is_visible) {
		_selected->normalize();
		_selected->show();
	}

	_layout_is_durty = true;
}

void notebook_t::update_client_position(shared_ptr<client_managed_t> c) {
	/* compute the window placement within notebook */
	_client_position = _compute_client_size(c);
	c->set_notebook_wished_position(to_root_position(_client_position));
	c->reconfigure();
}

void notebook_t::iconify_client(shared_ptr<client_managed_t> x) {
	if(_selected == x) {
		_start_fading();
		_selected->iconify();
		_selected->hide();
		_layout_is_durty = true;
	}
}

void notebook_t::set_allocation(rect const & area) {
	int width, height;
	get_min_allocation(width, height);
	assert(area.w >= width);
	assert(area.h >= height);

	_allocation = area;
	_layout_is_durty = true;
	_has_mouse_change = true;
	_mouse_over.event_x = -1;
	_mouse_over.event_y = -1;
}

void notebook_t::_update_layout() {

	int min_width;
	int min_height;
	get_min_allocation(min_width, min_height);

	if(_allocation.w < min_width * 2 + _ctx->theme()->split.margin.left  + _ctx->theme()->split.margin.right  + _ctx->theme()->split.width) {
		_can_vsplit = false;
	} else {
		_can_vsplit = true;
	}

	if(_allocation.h < min_height * 2 + _ctx->theme()->split.margin.top  + _ctx->theme()->split.margin.bottom  + _ctx->theme()->split.width) {
		_can_hsplit = false;
	} else {
		_can_hsplit = true;
	}

	_client_area.x = _allocation.x + _ctx->theme()->notebook.margin.left;
	_client_area.y = _allocation.y + _ctx->theme()->notebook.margin.top + _ctx->theme()->notebook.tab_height;
	_client_area.w = _allocation.w - _ctx->theme()->notebook.margin.left - _ctx->theme()->notebook.margin.right;
	_client_area.h = _allocation.h - _ctx->theme()->notebook.margin.top - _ctx->theme()->notebook.margin.bottom - _ctx->theme()->notebook.tab_height;

	auto window_position = get_window_position();

	_area.tab.x = _allocation.x + window_position.x;
	_area.tab.y = _allocation.y + window_position.y;
	_area.tab.w = _allocation.w;
	_area.tab.h = _ctx->theme()->notebook.tab_height;

	if(_can_hsplit) {
		_area.top.x = _allocation.x + window_position.x;
		_area.top.y = _allocation.y + window_position.y + _ctx->theme()->notebook.tab_height;
		_area.top.w = _allocation.w;
		_area.top.h = (_allocation.h - _ctx->theme()->notebook.tab_height) * 0.2;

		_area.bottom.x = _allocation.x + window_position.x;
		_area.bottom.y = _allocation.y + window_position.y + (0.8 * (allocation().h - _ctx->theme()->notebook.tab_height));
		_area.bottom.w = _allocation.w;
		_area.bottom.h = (_allocation.h - _ctx->theme()->notebook.tab_height) * 0.2;
	} else {
		_area.top = rect{};
		_area.bottom = rect{};
	}

	if(_can_vsplit) {
		_area.left.x = _allocation.x + window_position.x;
		_area.left.y = _allocation.y + window_position.y + _ctx->theme()->notebook.tab_height;
		_area.left.w = _allocation.w * 0.2;
		_area.left.h = (_allocation.h - _ctx->theme()->notebook.tab_height);

		_area.right.x = _allocation.x + window_position.x + _allocation.w * 0.8;
		_area.right.y = _allocation.y + window_position.y + _ctx->theme()->notebook.tab_height;
		_area.right.w = _allocation.w * 0.2;
		_area.right.h = (_allocation.h - _ctx->theme()->notebook.tab_height);
	} else {
		_area.left = rect{};
		_area.right = rect{};
	}

	_area.popup_top.x = _allocation.x + window_position.x;
	_area.popup_top.y = _allocation.y + window_position.y + _ctx->theme()->notebook.tab_height;
	_area.popup_top.w = _allocation.w;
	_area.popup_top.h = (_allocation.h - _ctx->theme()->notebook.tab_height) * 0.5;

	_area.popup_bottom.x = _allocation.x + window_position.x;
	_area.popup_bottom.y = _allocation.y + window_position.y + _ctx->theme()->notebook.tab_height
			+ (0.5 * (_allocation.h - _ctx->theme()->notebook.tab_height));
	_area.popup_bottom.w = _allocation.w;
	_area.popup_bottom.h = (_allocation.h - _ctx->theme()->notebook.tab_height) * 0.5;

	_area.popup_left.x = _allocation.x + window_position.x;
	_area.popup_left.y = _allocation.y + window_position.y + _ctx->theme()->notebook.tab_height;
	_area.popup_left.w = _allocation.w * 0.5;
	_area.popup_left.h = (_allocation.h - _ctx->theme()->notebook.tab_height);

	_area.popup_right.x = _allocation.x + window_position.x + allocation().w * 0.5;
	_area.popup_right.y = _allocation.y + window_position.y + _ctx->theme()->notebook.tab_height;
	_area.popup_right.w = _allocation.w * 0.5;
	_area.popup_right.h = (_allocation.h - _ctx->theme()->notebook.tab_height);

	_area.popup_center.x = _allocation.x + window_position.x + allocation().w * 0.2;
	_area.popup_center.y = _allocation.y + window_position.y + _ctx->theme()->notebook.tab_height + (allocation().h - _ctx->theme()->notebook.tab_height) * 0.2;
	_area.popup_center.w = _allocation.w * 0.6;
	_area.popup_center.h = (_allocation.h - _ctx->theme()->notebook.tab_height) * 0.6;

	if(_client_area.w <= 0) {
		_client_area.w = 1;
	}

	if(_client_area.h <= 0) {
		_client_area.h = 1;
	}

	for(auto const & c: _clients_tab_order) {
		/* resize all client properly */
		update_client_position(c.client);
	}

	_mouse_over_reset();
	_update_theme_notebook(_theme_notebook);
	_update_notebook_areas();

	queue_redraw();

}

rect notebook_t::_compute_client_size(shared_ptr<client_managed_t> c) {
	dimention_t<unsigned> size =
			c->compute_size_with_constrain(_client_area.w, _client_area.h);

	/** if the client cannot fit into client_area, clip it **/
	if(size.width > _client_area.w) {
		size.width = _client_area.w;
	}

	if(size.height > _client_area.h) {
		size.height = _client_area.h;
	}

	/* compute the window placement within notebook */
	rect client_size;
	client_size.x = floor((_client_area.w - size.width) / 2.0);
	client_size.y = floor((_client_area.h - size.height) / 2.0);
	client_size.w = size.width;
	client_size.h = size.height;

	client_size.x += _client_area.x;
	client_size.y += _client_area.y;

	return client_size;

}

shared_ptr<client_managed_t> notebook_t::selected() const {
	return _selected;
}

bool notebook_t::is_default() const {
	return _is_default;
}

void notebook_t::set_default(bool x) {
	_is_default = x;
	_theme_notebook.is_default = _is_default;
	queue_redraw();
}

void notebook_t::activate() {
	if(_parent != nullptr) {
		_parent->activate(shared_from_this());
	}
}

void notebook_t::activate(shared_ptr<tree_t> t) {
	assert(t != nullptr);
	assert(has_key(_children, t));

	activate();
	move_back(_children, t);

	auto mw = dynamic_pointer_cast<client_managed_t>(t);
	if (mw != nullptr) {
		_set_selected(mw);
	}

}

string notebook_t::get_node_name() const {
	ostringstream oss;
	oss << _get_node_name<'N'>() << " selected = " << _selected;
	return oss.str();
}

void notebook_t::render_legacy(cairo_t * cr) {
	_ctx->theme()->render_notebook(cr, &_theme_notebook);

	if(_theme_client_tabs.size() > 0) {
		cairo_surface_t * pix = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
				_theme_client_tabs.back().position.x + 100,
				_ctx->theme()->notebook.tab_height);
		cairo_t * xcr = cairo_create(pix);

		cairo_set_operator(xcr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba(xcr, 0.0, 0.0, 0.0, 0.0);
		cairo_paint(xcr);

		_ctx->theme()->render_iconic_notebook(xcr, _theme_client_tabs);
		cairo_destroy(xcr);

		cairo_save(cr);
		cairo_set_source_surface(cr, pix,
				_theme_client_tabs_area.x - _theme_client_tabs_offset,
				_theme_client_tabs_area.y);
		cairo_clip(cr, _theme_client_tabs_area);
		cairo_paint(cr);

		cairo_restore(cr);
		cairo_surface_destroy(pix);
	}

}

void notebook_t::update_layout(time64_t const time) {
	tree_t::update_layout(time);
	if(not _transition.empty()) {
		_layout_is_durty = true;
	}

	if(_layout_is_durty) {
		_layout_is_durty = false;
		_has_mouse_change = true;
		_update_layout();
	}

	if(_has_mouse_change) {
		_has_mouse_change = false;
		_update_mouse_over();
	}

	if (fading_notebook != nullptr and time >= (_swap_start + animation_duration)) {
		/** animation is terminated **/
		fading_notebook.reset();
		_ctx->add_global_damage(to_root_position(_allocation));
	}

	if (fading_notebook != nullptr) {
		double ratio = (static_cast<double>(time - _swap_start) / static_cast<double const>(animation_duration));
		ratio = ratio*1.05 - 0.025;
		fading_notebook->set_ratio(ratio);
	}
}

rect notebook_t::_compute_notebook_bookmark_position() const {
	return rect(
		_allocation.x + _allocation.w
		- _ctx->theme()->notebook.close_width
		- _ctx->theme()->notebook.hsplit_width
		- _ctx->theme()->notebook.vsplit_width
		- _ctx->theme()->notebook.mark_width,
		_allocation.y,
		_ctx->theme()->notebook.mark_width,
		_ctx->theme()->notebook.tab_height
	);
}

rect notebook_t::_compute_notebook_vsplit_position() const {
	return rect(
		_allocation.x + _allocation.w
			- _ctx->theme()->notebook.close_width
			- _ctx->theme()->notebook.hsplit_width
			- _ctx->theme()->notebook.vsplit_width,
		_allocation.y,
		_ctx->theme()->notebook.vsplit_width,
		_ctx->theme()->notebook.tab_height
	);
}

rect notebook_t::_compute_notebook_hsplit_position() const {

	return rect(
		_allocation.x + _allocation.w - _ctx->theme()->notebook.close_width - _ctx->theme()->notebook.hsplit_width,
		_allocation.y,
		_ctx->theme()->notebook.hsplit_width,
		_ctx->theme()->notebook.tab_height
	);

}

rect notebook_t::_compute_notebook_close_position() const {
	return rect(
		_allocation.x + _allocation.w - _ctx->theme()->notebook.close_width,
		_allocation.y,
		_ctx->theme()->notebook.close_width,
		_ctx->theme()->notebook.tab_height
	);
}

rect notebook_t::_compute_notebook_menu_position() const {
	return rect(
		_allocation.x,
		_allocation.y,
		_ctx->theme()->notebook.menu_button_width,
		_ctx->theme()->notebook.tab_height
	);

}

void notebook_t::_update_notebook_areas() {

	_client_buttons.clear();

	_area.button_close = _compute_notebook_close_position();
	_area.button_hsplit = _compute_notebook_hsplit_position();
	_area.button_vsplit = _compute_notebook_vsplit_position();
	_area.button_select = _compute_notebook_bookmark_position();
	_area.button_exposay = _compute_notebook_menu_position();

	if(_clients_tab_order.size() > 0) {

		if(_theme_client_tabs_area.w > _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w) {
			_theme_client_tabs_offset = 0;
		}

		if(_theme_client_tabs_area.w > _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w - _theme_client_tabs_offset) {
			_theme_client_tabs_offset = _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w - _theme_client_tabs_area.w;
		}

		if(_theme_client_tabs_offset < 0)
			_theme_client_tabs_offset = 0;

		if(_selected != nullptr) {
			rect & b = _theme_notebook.selected_client.position;

			if (not _selected->is_iconic()) {

				_area.close_client.x = b.x + b.w
						- _ctx->theme()->notebook.selected_close_width;
				_area.close_client.y = b.y;
				_area.close_client.w =
						_ctx->theme()->notebook.selected_close_width;
				_area.close_client.h = _ctx->theme()->notebook.tab_height;

				_area.undck_client.x = b.x + b.w
						- _ctx->theme()->notebook.selected_close_width
						- _ctx->theme()->notebook.selected_unbind_width;
				_area.undck_client.y = b.y;
				_area.undck_client.w = _ctx->theme()->notebook.selected_unbind_width;
				_area.undck_client.h = _ctx->theme()->notebook.tab_height;

			}

			_client_buttons.push_back(std::make_tuple(b, weak_ptr<client_managed_t>{_selected}, &_theme_notebook.selected_client));

		} else {
			_area.close_client = rect{};
			_area.undck_client = rect{};
		}

		auto c = _clients_tab_order.begin();
		for (auto & tab: _theme_client_tabs) {
			rect pos = tab.position;
			pos.x += _theme_client_tabs_area.x - _theme_client_tabs_offset;
			pos.y += _theme_client_tabs_area.y;
			_client_buttons.push_back(make_tuple(pos,
					client_managed_w{c->client}, &tab));
			++c;
		}

	}
}

void notebook_t::_update_theme_notebook(theme_notebook_t & theme_notebook) {
	theme_notebook.root_x = get_window_position().x;
	theme_notebook.root_y = get_window_position().y;
	theme_notebook.can_hsplit = _can_hsplit;
	theme_notebook.can_vsplit = _can_vsplit;
	theme_notebook.client_count = _children.size();

	_theme_client_tabs.clear();

	if (_clients_tab_order.size() != 0) {
		double selected_box_width = ((int)_allocation.w
				- (int)_ctx->theme()->notebook.close_width
				- (int)_ctx->theme()->notebook.hsplit_width
				- (int)_ctx->theme()->notebook.vsplit_width
				- (int)_ctx->theme()->notebook.mark_width
				- (int)_ctx->theme()->notebook.menu_button_width)
				- (int)_clients_tab_order.size() * (int)_ctx->theme()->notebook.iconic_tab_width;

		if(selected_box_width < 200) {
			selected_box_width = 200;
		}

		_theme_client_tabs_area.x = _allocation.x
				+ _ctx->theme()->notebook.menu_button_width
				+ selected_box_width;
		_theme_client_tabs_area.y = _allocation.y;
		_theme_client_tabs_area.w = ((int)_allocation.w
				- (int)_ctx->theme()->notebook.close_width
				- (int)_ctx->theme()->notebook.hsplit_width
				- (int)_ctx->theme()->notebook.vsplit_width
				- (int)_ctx->theme()->notebook.mark_width
				- (int)_ctx->theme()->notebook.menu_button_width
				- (int)selected_box_width);
		_theme_client_tabs_area.h = _ctx->theme()->notebook.tab_height;

		double offset = _allocation.x + _ctx->theme()->notebook.menu_button_width;

		if (_selected != nullptr) {
			/** copy the tab context **/
			theme_notebook.selected_client = theme_tab_t{};
			theme_notebook.selected_client.position = rect{
					(int)floor(offset),
					_allocation.y,
					(int)floor((int)(offset + selected_box_width) - floor(offset)),
					(int)_ctx->theme()->notebook.tab_height };

			if(_selected->has_focus()) {
				theme_notebook.selected_client.tab_color =
						_ctx->theme()->get_focused_color();
			} else {
				theme_notebook.selected_client.tab_color =
						_ctx->theme()->get_selected_color();
			}

			theme_notebook.selected_client.title = _selected->title();
			theme_notebook.selected_client.icon = _selected->icon();
			theme_notebook.selected_client.is_iconic = _selected->is_iconic();
			theme_notebook.has_selected_client = true;
		} else {
			theme_notebook.has_selected_client = false;
		}

		offset = 0;
		for (auto const & i : _clients_tab_order) {
			_theme_client_tabs.push_back(theme_tab_t { });
			auto & tab = _theme_client_tabs.back();
			tab.position = rect {
				(int) floor(offset), 0,
				(int) (floor(offset + _ctx->theme()->notebook.iconic_tab_width)
									- floor(offset)),
				(int) _ctx->theme()->notebook.tab_height };

			if (i.client->has_focus()) {
				tab.tab_color = _ctx->theme()->get_focused_color();
			} else if (_selected == i.client) {
				tab.tab_color = _ctx->theme()->get_selected_color();
			} else {
				tab.tab_color = _ctx->theme()->get_normal_color();
			}
			tab.title = i.client->title();
			tab.icon = i.client->icon();
			tab.is_iconic = i.client->is_iconic();
			offset += _ctx->theme()->notebook.iconic_tab_width;
		}

		_area.left_scroll_arrow = rect{};
		_area.right_scroll_arrow = rect{};
		if(_theme_client_tabs_area.w < _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w) {
			_has_scroll_arrow = true;
			theme_notebook.has_scroll_arrow = true;

			_area.left_scroll_arrow.x = _theme_client_tabs_area.x;
			_area.left_scroll_arrow.y = _allocation.y;
			_area.left_scroll_arrow.w = _ctx->theme()->notebook.left_scroll_arrow_width;
			_area.left_scroll_arrow.h = _ctx->theme()->notebook.tab_height;

			_area.right_scroll_arrow.x = _theme_client_tabs_area.x + _theme_client_tabs_area.w - _ctx->theme()->notebook.right_scroll_arrow_width;
			_area.right_scroll_arrow.y = _allocation.y;
			_area.right_scroll_arrow.w = _ctx->theme()->notebook.left_scroll_arrow_width;
			_area.right_scroll_arrow.h = _ctx->theme()->notebook.tab_height;

			theme_notebook.left_arrow_position = _area.left_scroll_arrow;
			theme_notebook.right_arrow_position = _area.right_scroll_arrow;

			_theme_client_tabs_area.w -= (_ctx->theme()->notebook.left_scroll_arrow_width + _ctx->theme()->notebook.right_scroll_arrow_width);
			_theme_client_tabs_area.x += _ctx->theme()->notebook.left_scroll_arrow_width;

		} else {
			_area.left_scroll_arrow = rect{};
			_area.right_scroll_arrow = rect{};
			_has_scroll_arrow = false;
			theme_notebook.has_scroll_arrow = false;
		}


	} else {
		theme_notebook.has_selected_client = false;
	}

	theme_notebook.allocation = _allocation;
	if(_selected != nullptr) {
		theme_notebook.client_position = _client_position;
	}
	theme_notebook.is_default = is_default();



}

void notebook_t::_start_fading() {

	if(_ctx->cmp() == nullptr)
		return;

	if(fading_notebook != nullptr)
		return;

	_swap_start.update_to_current_time();

	/**
	 * Create image of notebook as it was just before fading start
	 **/
	auto pix = make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGB, _allocation.w, _allocation.h);
	cairo_surface_t * surf = pix->get_cairo_surface();
	cairo_t * cr = cairo_create(surf);
	cairo_save(cr);
	cairo_translate(cr, -_allocation.x, -_allocation.y);
	_ctx->theme()->render_notebook(cr, &_theme_notebook);

	if(_theme_client_tabs.size() > 0) {
		pixmap_t * pix = new pixmap_t(_ctx->dpy(), PIXMAP_RGBA, _theme_client_tabs.back().position.x + 100, _ctx->theme()->notebook.tab_height);
		cairo_t * xcr = cairo_create(pix->get_cairo_surface());

		cairo_set_operator(xcr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba(xcr, 0.0, 0.0, 0.0, 0.0);
		cairo_paint(xcr);

		_ctx->theme()->render_iconic_notebook(xcr, _theme_client_tabs);
		cairo_destroy(xcr);

		cairo_save(cr);
		cairo_set_source_surface(cr, pix->get_cairo_surface(), _theme_client_tabs_area.x - _theme_client_tabs_offset, _theme_client_tabs_area.y);
		cairo_clip(cr, _theme_client_tabs_area);
		cairo_paint(cr);

		cairo_restore(cr);
		delete pix;
	}

	cairo_restore(cr);

	/* paste the current window */
	if (_selected != nullptr) {
		update_client_position(_selected);
		if (not _selected->is_iconic()) {
			auto client_view = _selected->create_view();
			shared_ptr<pixmap_t> pix = client_view->get_pixmap();
			if (pix != nullptr) {
				rect pos = _client_position;
				rect cl { pos.x, pos.y, pos.w, pos.h };

				cairo_reset_clip(cr);
				cairo_clip(cr, cl);
				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
				cairo_set_source_surface(cr, pix->get_cairo_surface(), cl.x, cl.y);
				cairo_mask_surface(cr, pix->get_cairo_surface(), cl.x, cl.y);

			}
		}
	}

	cairo_destroy(cr);
	cairo_surface_flush(surf);
	rect pos = to_root_position(_allocation);
	fading_notebook = make_shared<renderable_notebook_fading_t>(_ctx, pix, pos.x, pos.y);
	fading_notebook->show();
	fading_notebook->set_parent(this);

}

void notebook_t::start_exposay() {
	if(_ctx->cmp() == nullptr)
		return;

	if(_selected != nullptr) {
		iconify_client(_selected);
		_selected = nullptr;
	}

	_exposay = true;
	queue_redraw();
}

void notebook_t::_update_exposay() {
	_exposay_buttons.clear();
	_exposay_thumbnail.clear();
	_exposay_mouse_over = nullptr;

	_theme_notebook.button_mouse_over = NOTEBOOK_BUTTON_NONE;
	_mouse_over.tab = nullptr;
	_mouse_over.exposay = nullptr;

	if(_ctx->cmp() == nullptr)
		return;

	if(not _exposay)
		return;

	if(_clients_tab_order.size() <= 0)
		return;

	unsigned clients_counts = _clients_tab_order.size();

	/*
	 * n is the number of column and m is the number of line of exposay.
	 * Since most window are the size of client_area, we known that n*m will produce n = m
	 * thus use square root to get n
	 */
	int n = static_cast<int>(ceil(sqrt(static_cast<double>(clients_counts))));
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
	int m = ((clients_counts - 1) / n) + 1;

	unsigned width = _client_area.w/n;
	unsigned heigth = _client_area.h/m;
	unsigned xoffset = (_client_area.w-width*n)/2.0 + _client_area.x;
	unsigned yoffset = (_client_area.h-heigth*m)/2.0 + _client_area.y;

	auto it = _clients_tab_order.begin();
	for(int i = 0; i < _clients_tab_order.size(); ++i) {
		unsigned y = i / n;
		unsigned x = i % n;

		if(y == m-1)
			xoffset = (_client_area.w-width*n)/2.0 + _client_area.x
				+ (n*m - _clients_tab_order.size())*width/2.0;

		rect pdst(x*width+1.0+xoffset+8, y*heigth+1.0+yoffset+8, width-2.0-16, heigth-2.0-16);
		_exposay_buttons.push_back(make_tuple(pdst, client_managed_w{it->client}, i));
		pdst = to_root_position(pdst);
		auto thumbnail = make_shared<renderable_thumbnail_t>(_ctx, it->client, pdst, ANCHOR_CENTER);
		_exposay_thumbnail.push_back(thumbnail);
		thumbnail->show();
		++it;
	}

}

void notebook_t::_stop_exposay() {
	_exposay = false;
	_mouse_over.exposay = nullptr;
	_exposay_buttons.clear();
	_exposay_thumbnail.clear();
	queue_redraw();
}

bool notebook_t::button_press(xcb_button_press_event_t const * e) {

	if (e->event != get_parent_xid()) {
		return false;
	}

	/* left click on page window */
	if (e->child == XCB_NONE and e->detail == XCB_BUTTON_INDEX_1) {
		int x = e->event_x;
		int y = e->event_y;

		if (_area.button_close.is_inside(x, y)) {
			_ctx->notebook_close(shared_from_this());
			return true;
		} else if (_area.button_hsplit.is_inside(x, y)) {
			if(_can_hsplit)
				_ctx->split_bottom(shared_from_this(), nullptr);
			return true;
		} else if (_area.button_vsplit.is_inside(x, y)) {
			if(_can_vsplit)
				_ctx->split_right(shared_from_this(), nullptr);
			return true;
		} else if (_area.button_select.is_inside(x, y)) {
			_ctx->get_current_workspace()->set_default_pop(shared_from_this());
			return true;
		} else if (_area.button_exposay.is_inside(x, y)) {
			start_exposay();
			return true;
		} else if (_area.close_client.is_inside(x, y)) {
			if(_selected != nullptr)
				_selected->delete_window(e->time);
			return true;
		} else if (_area.undck_client.is_inside(x, y)) {
			if (_selected != nullptr)
				_ctx->unbind_window(_selected);
			return true;
		} else if (_area.left_scroll_arrow.is_inside(x, y)) {
			_scroll_left(30);
			return true;
		} else if (_area.right_scroll_arrow.is_inside(x, y)) {
			_scroll_right(30);
			return true;
		} else {
			for(auto & i: _client_buttons) {
				if(std::get<0>(i).is_inside(x, y)) {
					auto c = std::get<1>(i).lock();
					_ctx->grab_start(new grab_bind_client_t{_ctx, c, XCB_BUTTON_INDEX_1, to_root_position(std::get<0>(i))});
					_mouse_over_reset();
					return true;
				}
			}

			for(auto & i: _exposay_buttons) {
				if(std::get<0>(i).is_inside(x, y) and not std::get<1>(i).expired()) {
					auto c = std::get<1>(i).lock();
					_ctx->grab_start(new grab_bind_client_t{_ctx, c, XCB_BUTTON_INDEX_1, to_root_position(std::get<0>(i))});
					return true;
				}
			}
		}

	/* rigth click on page */
	} else if (e->child == XCB_NONE and e->detail == XCB_BUTTON_INDEX_3) {
		int x = e->event_x;
		int y = e->event_y;

		if (_area.button_close.is_inside(x, y)) {

		} else if (_area.button_hsplit.is_inside(x, y)) {

		} else if (_area.button_vsplit.is_inside(x, y)) {

		} else if (_area.button_select.is_inside(x, y)) {

		} else if (_area.button_exposay.is_inside(x, y)) {

		} else if (_area.close_client.is_inside(x, y)) {

		} else if (_area.undck_client.is_inside(x, y)) {

		} else {
			for(auto & i: _client_buttons) {
				if(std::get<0>(i).is_inside(x, y)) {
					_start_client_menu(std::get<1>(i).lock(), e->detail, e->root_x, e->root_y);
					return true;
				}
			}

			for(auto & i: _exposay_buttons) {
				if(std::get<0>(i).is_inside(x, y)) {
					_start_client_menu(std::get<1>(i).lock(), e->detail, e->root_x, e->root_y);
					return true;
				}
			}
		}
	} else if (e->child == XCB_NONE and e->detail == XCB_BUTTON_INDEX_4) {
		if(_theme_client_tabs_area.is_inside(e->event_x, e->event_y)) {
			_scroll_left(15);
			return true;
		}
	} else if (e->child == XCB_NONE and e->detail == XCB_BUTTON_INDEX_5) {
		if(_theme_client_tabs_area.is_inside(e->event_x, e->event_y)) {
			_scroll_right(15);
			return true;
		}
	}

	return false;

}

void notebook_t::_start_client_menu(shared_ptr<client_managed_t> c, xcb_button_t button, uint16_t x, uint16_t y) {
	auto callback = [this, c] (dropdown_menu_t<int> * ths, int selected) -> void { this->_process_notebook_client_menu(ths, c, selected); };
	std::vector<std::shared_ptr<dropdown_menu_t<int>::item_t>> v;
	for(unsigned k = 0; k < _ctx->get_workspace_count(); ++k) {
		std::ostringstream os;
		os << "Worspace #" << k;
		v.push_back(std::make_shared<dropdown_menu_t<int>::item_t>(k, nullptr, os.str()));
	}
	v.push_back(std::make_shared<dropdown_menu_t<int>::item_t>(_ctx->get_workspace_count(), nullptr, "To new workspace"));
	_ctx->grab_start(new dropdown_menu_t<int>{_ctx, v, button, x, y, 300, rect{x-10, y-10, 20, 20}, callback});

}

void notebook_t::_process_notebook_client_menu(dropdown_menu_t<int> * ths, shared_ptr<client_managed_t> c, int selected) {
	printf("Change desktop %d for %u\n", selected, c->orig());

	if(selected == _ctx->get_workspace_count()) {
		selected = _ctx->create_workspace();
		_ctx->detach(c);
		_ctx->get_workspace(selected)->default_pop()->add_client(c, false);
		c->set_current_desktop(selected);
		c->activate();
		_ctx->set_focus(c, ths->time());
	} else if (selected != _ctx->get_current_workspace()->id()) {
		_ctx->detach(c);
		_ctx->get_workspace(selected)->default_pop()->add_client(c, false);
		c->set_current_desktop(selected);
		c->activate();
		_ctx->set_focus(c, ths->time());
	}
}

void notebook_t::_update_mouse_over() {

	int x = _mouse_over.event_x;
	int y = _mouse_over.event_y;

	if (_allocation.is_inside(x, y)) {

		notebook_button_e new_button_mouse_over = NOTEBOOK_BUTTON_NONE;
		tuple<rect, weak_ptr<client_managed_t>, theme_tab_t *> * tab = nullptr;
		tuple<rect, weak_ptr<client_managed_t>, int> * exposay = nullptr;

		if (_area.button_close.is_inside(x, y)) {
			new_button_mouse_over = NOTEBOOK_BUTTON_CLOSE;
		} else if (_area.button_hsplit.is_inside(x, y)) {
			new_button_mouse_over = NOTEBOOK_BUTTON_HSPLIT;
		} else if (_area.button_vsplit.is_inside(x, y)) {
			new_button_mouse_over = NOTEBOOK_BUTTON_VSPLIT;
		} else if (_area.button_select.is_inside(x, y)) {
			new_button_mouse_over = NOTEBOOK_BUTTON_MASK;
		} else if (_area.button_exposay.is_inside(x, y)) {
			new_button_mouse_over = NOTEBOOK_BUTTON_EXPOSAY;
		} else if (_area.close_client.is_inside(x, y)) {
			new_button_mouse_over = NOTEBOOK_BUTTON_CLIENT_CLOSE;
		} else if (_area.undck_client.is_inside(x, y)) {
			new_button_mouse_over = NOTEBOOK_BUTTON_CLIENT_UNBIND;
		} else if (_area.left_scroll_arrow.is_inside(x, y)) {
			new_button_mouse_over = NOTEBOOK_BUTTON_LEFT_SCROLL_ARROW;
		} else if (_area.right_scroll_arrow.is_inside(x, y)) {
			new_button_mouse_over = NOTEBOOK_BUTTON_RIGHT_SCROLL_ARROW;
		} else {
			for (auto & i : _client_buttons) {
				if (std::get<0>(i).is_inside(x, y)) {
					tab = &i;
					break;
				}
			}

			for (auto & i : _exposay_buttons) {
				if (std::get<0>(i).is_inside(x, y)) {
					exposay = &i;
					break;
				}
			}
		}

		if(_theme_notebook.button_mouse_over != new_button_mouse_over) {
			_mouse_over_reset();
			_theme_notebook.button_mouse_over = new_button_mouse_over;
			queue_redraw();
		} else if (_mouse_over.tab != tab) {
			_mouse_over_reset();
			_mouse_over.tab = tab;
			_mouse_over_set();
			queue_redraw();
		} else if (_mouse_over.exposay != exposay) {
			_mouse_over_reset();
			_mouse_over.exposay = exposay;
			_mouse_over_set();
			queue_redraw();
		}
	} else {
		if(_theme_notebook.button_mouse_over != NOTEBOOK_BUTTON_NONE or _mouse_over.tab != nullptr or _mouse_over.exposay != nullptr) {
			_mouse_over_reset();
			queue_redraw();
		}
	}

}

bool notebook_t::button_motion(xcb_motion_notify_event_t const * e) {

	if (e->event != get_parent_xid()) {
		_has_mouse_change = true;
		_mouse_over.event_x = -1;
		_mouse_over.event_y = -1;
		return false;
	}

	if (e->child == XCB_NONE) {
		_has_mouse_change = true;
		_mouse_over.event_x = e->event_x;
		_mouse_over.event_y = e->event_y;
	} else {
		_has_mouse_change = true;
		_mouse_over.event_x = -1;
		_mouse_over.event_y = -1;
	}

	return false;

}

bool notebook_t::leave(xcb_leave_notify_event_t const * ev) {
	if(ev->event == get_parent_xid()) {
		_has_mouse_change = true;
		_mouse_over.event_x = -1;
		_mouse_over.event_y = -1;
	}
	return false;
}

void notebook_t::_mouse_over_reset() {
	if (_mouse_over.tab != nullptr) {
		if (std::get<1>(*_mouse_over.tab).lock()->has_focus()) {
			std::get<2>(*_mouse_over.tab)->tab_color =
					_ctx->theme()->get_focused_color();
		} else if (_selected == std::get<1>(*_mouse_over.tab).lock()) {
			std::get<2>(*_mouse_over.tab)->tab_color =
					_ctx->theme()->get_selected_color();
		} else {
			std::get<2>(*_mouse_over.tab)->tab_color =
					_ctx->theme()->get_normal_color();
		}

		tooltips = nullptr;
	}

	if(_mouse_over.exposay != nullptr) {
		_exposay_thumbnail[std::get<2>(*_mouse_over.exposay)]->set_mouse_over(false);
	}

	_theme_notebook.button_mouse_over = NOTEBOOK_BUTTON_NONE;
	_mouse_over.tab = nullptr;
	_mouse_over.exposay = nullptr;
	_exposay_mouse_over = nullptr;

}

void notebook_t::_mouse_over_set() {
	if (_mouse_over.tab != nullptr) {
		std::get<2>(*_mouse_over.tab)->tab_color = _ctx->theme()->get_mouse_over_color();

		rect tab_pos = to_root_position(std::get<0>(*_mouse_over.tab));

		rect pos;
		pos.x = tab_pos.x + tab_pos.w - 256;
		pos.y = tab_pos.y + tab_pos.h;
		pos.w = 256;
		pos.h = 256;

		if(std::get<1>(*_mouse_over.tab).lock() != _selected) {
			tooltips = make_shared<renderable_thumbnail_t>(_ctx, std::get<1>(*_mouse_over.tab).lock(), pos, ANCHOR_TOP_RIGHT);
			tooltips->set_parent(this);
			tooltips->show();
			tooltips->set_mouse_over(true);
		}
	}

	if(_mouse_over.exposay != nullptr) {
		_exposay_mouse_over = make_shared<renderable_unmanaged_gaussian_shadow_t<16>>(_exposay_thumbnail[std::get<2>(*_mouse_over.exposay)]->get_real_position(), color_t{1.0, 0.0, 0.0, 1.0});
		_exposay_thumbnail[std::get<2>(*_mouse_over.exposay)]->set_mouse_over(true);
	} else {
		_exposay_mouse_over = nullptr;
	}
}

void notebook_t::_client_title_change(shared_ptr<client_managed_t> c) {
	for(auto & x: _client_buttons) {
		if(c == std::get<1>(x).lock()) {
			std::get<2>(x)->title = c->title();
		}
	}

	for(auto & x: _exposay_buttons) {
		if(c == std::get<1>(x).lock()) {
			_exposay_thumbnail[std::get<2>(x)]->update_title();
		}
	}

	if(c == _selected) {
		_theme_notebook.selected_client.title = c->title();
	}
	_layout_is_durty = true;
}

void notebook_t::_client_destroy(client_managed_t * c) {
	throw exception_t("not expected call of %d", __PRETTY_FUNCTION__);
}

void notebook_t::_client_focus_change(shared_ptr<client_managed_t> c) {
	_layout_is_durty = true;
}

rect notebook_t::allocation() const {
	return _allocation;
}

void notebook_t::append_children(vector<shared_ptr<tree_t>> & out) const {
	out.insert(out.end(), _children.begin(), _children.end());

	if(_exposay) {
		out.insert(out.end(), _exposay_thumbnail.begin(), _exposay_thumbnail.end());
	}

	if(fading_notebook != nullptr) {
		out.push_back(fading_notebook);
	}

	if(tooltips != nullptr) {
		out.push_back(tooltips);
	}

}

void notebook_t::hide() {
	_is_visible = false;

	if(_selected != nullptr) {
		_selected->iconify();
		_selected->hide();
	}
}

void notebook_t::show() {
	_is_visible = true;

	if(_selected != nullptr) {
		_selected->normalize();
		_selected->show();
	}
}

bool notebook_t::_has_client(shared_ptr<client_managed_t> c) {
	return _find_client_context(c) != _clients_tab_order.end();
}

list<notebook_t::_client_context_t>::iterator
notebook_t::_find_client_context(client_managed_p client) {
	auto itr = _clients_tab_order.begin();
	auto end = _clients_tab_order.end();
	while(itr != end) {
		if(*itr == client)
			return itr;
		++itr;
	}
	return end;
}

void notebook_t::render(cairo_t * cr, region const & area) {

}

region notebook_t::get_opaque_region() {
	return region{};
}

region notebook_t::get_visible_region() {
	return region{};
}

region notebook_t::get_damaged() {
	return region{};
}

shared_ptr<notebook_t> notebook_t::shared_from_this() {
	return dynamic_pointer_cast<notebook_t>(tree_t::shared_from_this());
}

void notebook_t::get_min_allocation(int & width, int & height) const {
	height = _ctx->theme()->notebook.tab_height
			+ _ctx->theme()->notebook.margin.top
			+ _ctx->theme()->notebook.margin.bottom + 20;
	width = _ctx->theme()->notebook.margin.left
			+ _ctx->theme()->notebook.margin.right + 100
			+ _ctx->theme()->notebook.close_width
			+ _ctx->theme()->notebook.selected_close_width
			+ _ctx->theme()->notebook.selected_unbind_width
			+ _ctx->theme()->notebook.vsplit_width
			+ _ctx->theme()->notebook.hsplit_width
			+ _ctx->theme()->notebook.mark_width
			+ _ctx->theme()->notebook.menu_button_width
			+ _ctx->theme()->notebook.iconic_tab_width * 4;
}

void  notebook_t::_scroll_right(int x) {
	if(_theme_client_tabs_area.w == _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w - _theme_client_tabs_offset) {
		return;
	}

	if(_theme_client_tabs.size() < 1)
		return;

	if(_theme_client_tabs_area.w > _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w)
		return;

	int target_offset = _theme_client_tabs_offset + x;

	if(_theme_client_tabs_area.w > _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w - target_offset) {
		target_offset = _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w - _theme_client_tabs_area.w;
	}

	if(_theme_client_tabs_offset < 0)
		target_offset = 0;

	auto transition = std::make_shared<transition_linear_t<notebook_t, int>>(this, &notebook_t::_theme_client_tabs_offset, target_offset, time64_t{0.2});
	add_transition(transition);

	_update_notebook_areas();
	_layout_is_durty = true;
}

void  notebook_t::_scroll_left(int x) {
	if(_theme_client_tabs_offset == 0)
		return;

	if(_theme_client_tabs.size() < 1)
		return;

	if(_theme_client_tabs_area.w > _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w)
		return;

	int target_offset = _theme_client_tabs_offset - x;

	if(_theme_client_tabs_area.w > _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w - target_offset) {
		target_offset = _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w - _theme_client_tabs_area.w;
	}

	if(_theme_client_tabs_offset < 0)
		target_offset = 0;

	auto transition = std::make_shared<transition_linear_t<notebook_t, int>>(this, &notebook_t::_theme_client_tabs_offset, target_offset, time64_t{0.2});
	add_transition(transition);

	_update_notebook_areas();
	_layout_is_durty = true;
}

void notebook_t::_set_theme_tab_offset(int x) {
	if(_theme_client_tabs.size() < 1) {
		_theme_client_tabs_offset = 0;
		return;
	}
	if(x < 0) {
		_theme_client_tabs_offset = 0;
		return;
	}
	if(_theme_client_tabs_area.w > _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w - x) {
		_theme_client_tabs_offset = _theme_client_tabs.back().position.x + _theme_client_tabs.back().position.w - _theme_client_tabs_area.w;
		return;
	}
	_theme_client_tabs_offset = x;
}

notebook_t::_client_context_t::_client_context_t(notebook_t * nbk,
		client_managed_p client) : client{client} {
	title_change_func = client->on_title_change.connect(nbk,
			&notebook_t::_client_title_change);
	destoy_func = client->on_destroy.connect(nbk, &notebook_t::_client_destroy);
	focus_change_func = client->on_focus_change.connect(nbk,
			&notebook_t::_client_focus_change);
}

notebook_t::_client_context_t::~_client_context_t() {
	client = nullptr;
}

bool notebook_t::_client_context_t::operator==(client_managed_p client) const {
	return this->client == client;
}

void notebook_t::queue_redraw() {
	tree_t::queue_redraw();
}

}
