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

notebook_t::notebook_t(page_context_t * ctx, bool keep_selected) :
		_ctx{ctx},
		_is_default{false},
		_selected{nullptr},
		_is_hidden{false},
		_keep_selected{keep_selected},
		_exposay{false},
		_mouse_over{nullptr, nullptr}
{

}

notebook_t::~notebook_t() {

}

bool notebook_t::add_client(shared_ptr<client_managed_t> x, bool prefer_activate) {
	assert(not has_key(_clients, x));
	assert(x != nullptr);

	if(_exposay)
		prefer_activate = false;

	x->set_parent(shared_from_this());
	_children.push_back(x);
	_clients.push_front(x);

	_client_context_t a;

	a.title_change_func = x->on_title_change.connect(this, &notebook_t::_client_title_change);
	a.destoy_func = x->on_destroy.connect(this, &notebook_t::_client_destroy);
	a.activate_func = x->on_activate.connect(this, &notebook_t::_client_activate);
	a.deactivate_func = x->on_deactivate.connect(this, &notebook_t::_client_deactivate);

	_clients_context[x.get()] = a;

	_ctx->csm()->register_window(x->base());

	if(prefer_activate) {
		_stop_exposay();
		_start_fading();
		x->normalize();
		if (_selected != nullptr and _selected != x) {
			_selected->iconify();
		}

		_selected = x;
	} else {
		x->iconify();
		if(_selected != nullptr) {
			/* do nothing */
		} else if (not _exposay) {
			/** no prev surf is used **/
			_selected = x;
		}
	}

	_update_layout();
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
	if (has_key(_clients, mw) and mw != nullptr) {
		_remove_client(mw);
	}
}

void notebook_t::_activate_client(shared_ptr<client_managed_t> x) {
	if (has_key(_clients, x)) {
		_set_selected(x);
	}
}

list<shared_ptr<client_managed_t>> const & notebook_t::get_clients() {
	return _clients;
}

void notebook_t::_remove_client(shared_ptr<client_managed_t> x) {
	assert(has_key(_clients, x));

	if(x == nullptr)
		return;

	/** update selection **/
	if (_selected == x) {
		_start_fading();
		_selected = nullptr;
	}

	// cleanup
	_children.remove(x);
	x->clear_parent();
	_clients.remove(x);

	/* disconnect all signals */
	_clients_context.erase(x.get());

	if(_keep_selected and not _children.empty() and _selected == nullptr) {
		_selected = dynamic_pointer_cast<client_managed_t>(_children.back());

		if (_selected != nullptr) {
			_selected->normalize();
		}
	}

	_update_layout();
	_ctx->csm()->unregister_window(x->base());
}

void notebook_t::_set_selected(shared_ptr<client_managed_t> c) {
	/** already selected **/
	if(_selected == c and not c->is_iconic())
		return;

	_stop_exposay();
	_start_fading();

	if(_selected != nullptr and c != _selected) {
		_selected->iconify();
	}
	/** set selected **/
	_selected = c;
	_selected->normalize();

	_update_layout();
}

void notebook_t::update_client_position(shared_ptr<client_managed_t> c) {
	/* compute the window placement within notebook */
	_client_position = _compute_client_size(c);
	c->set_notebook_wished_position(to_root_position(_client_position));
	c->reconfigure();
}

void notebook_t::iconify_client(shared_ptr<client_managed_t> x) {
	assert(has_key(_clients, x));

	/** already iconified **/
	if(_selected != x)
		return;

	_start_fading();

	if (_selected != nullptr) {
		_selected->iconify();
	}

	_update_layout();

}

void notebook_t::set_allocation(rect const & area) {
	_allocation = area;
	_update_layout();
}

void notebook_t::_update_layout() {
	_client_area.x = _allocation.x + _ctx->theme()->notebook.margin.left;
	_client_area.y = _allocation.y + _ctx->theme()->notebook.margin.top + _ctx->theme()->notebook.tab_height;
	_client_area.w = _allocation.w - _ctx->theme()->notebook.margin.left - _ctx->theme()->notebook.margin.right;
	_client_area.h = _allocation.h - _ctx->theme()->notebook.margin.top - _ctx->theme()->notebook.margin.bottom - _ctx->theme()->notebook.tab_height;

	auto window_position = get_window_position();

	_area.tab.x = _allocation.x + window_position.x;
	_area.tab.y = _allocation.y + window_position.y;
	_area.tab.w = _allocation.w;
	_area.tab.h = _ctx->theme()->notebook.tab_height;

	_area.top.x = _allocation.x + window_position.x;
	_area.top.y = _allocation.y + window_position.y + _ctx->theme()->notebook.tab_height;
	_area.top.w = _allocation.w;
	_area.top.h = (_allocation.h - _ctx->theme()->notebook.tab_height) * 0.2;

	_area.bottom.x = _allocation.x + window_position.x;
	_area.bottom.y = _allocation.y + window_position.y + (0.8 * (allocation().h - _ctx->theme()->notebook.tab_height));
	_area.bottom.w = _allocation.w;
	_area.bottom.h = (_allocation.h - _ctx->theme()->notebook.tab_height) * 0.2;

	_area.left.x = _allocation.x + window_position.x;
	_area.left.y = _allocation.y + window_position.y + _ctx->theme()->notebook.tab_height;
	_area.left.w = _allocation.w * 0.2;
	_area.left.h = (_allocation.h - _ctx->theme()->notebook.tab_height);

	_area.right.x = _allocation.x + window_position.x + _allocation.w * 0.8;
	_area.right.y = _allocation.y + window_position.y + _ctx->theme()->notebook.tab_height;
	_area.right.w = _allocation.w * 0.2;
	_area.right.h = (_allocation.h - _ctx->theme()->notebook.tab_height);

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

	for(auto c: _clients) {
		/* resize all client properly */
		update_client_position(c);
	}


	_mouse_over_reset();
	_update_theme_notebook(_theme_notebook);
	_update_notebook_areas();

	_update_exposay();

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

void notebook_t::activate(shared_ptr<tree_t> t) {

	if(_parent.lock() != nullptr) {
		_parent.lock()->activate(shared_from_this());
	}

	if (has_key(_children, t)) {
		_children.remove(t);
		_children.push_back(t);
		auto mw = dynamic_pointer_cast<client_managed_t>(t);
		if (mw != nullptr) {
			_set_selected(mw);
		}
	} else if (t != nullptr) {
		throw exception_t("notebook_t::raise_child trying to raise a non child tree");
	}

}

string notebook_t::get_node_name() const {
	ostringstream oss;
	oss << _get_node_name<'N'>() << " selected = " << _selected;
	return oss.str();
}

void notebook_t::render_legacy(cairo_t * cr) const {
	_ctx->theme()->render_notebook(cr, &_theme_notebook);
}

void notebook_t::update_layout(time64_t const time) {

	if(_is_hidden) {
		return;
	}

	if (fading_notebook != nullptr and time >= (_swap_start + animation_duration)) {
		/** animation is terminated **/
		fading_notebook.reset();
		_ctx->add_global_damage(to_root_position(_allocation));
		_update_layout();
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

	if(_clients.size() > 0) {

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

		auto c = _clients.begin();
		for (auto & tab: _theme_notebook.clients_tab) {
			_client_buttons.push_back(make_tuple(tab.position, weak_ptr<client_managed_t>{*c}, &tab));
			++c;
		}

	}
}

void notebook_t::_update_theme_notebook(theme_notebook_t & theme_notebook) const {
	theme_notebook.clients_tab.clear();
	theme_notebook.root_x = get_window_position().x;
	theme_notebook.root_y = get_window_position().y;

	if (_clients.size() != 0) {
		double selected_box_width = (_allocation.w
				- _ctx->theme()->notebook.close_width
				- _ctx->theme()->notebook.hsplit_width
				- _ctx->theme()->notebook.vsplit_width
				- _ctx->theme()->notebook.mark_width
				- _ctx->theme()->notebook.menu_button_width)
				- _clients.size() * _ctx->theme()->notebook.iconic_tab_width;
		double offset = _allocation.x + _ctx->theme()->notebook.menu_button_width;

		if (_selected != nullptr) {
			/** copy the tab context **/
			theme_notebook.selected_client = theme_tab_t{};
			theme_notebook.selected_client.position = rect{
					(int)floor(offset),
							_allocation.y, (int)floor(
					(int)(offset + selected_box_width) - floor(offset)),
					(int)_ctx->theme()->notebook.tab_height };

			if(_selected->is_focused()) {
				theme_notebook.selected_client.tab_color = _ctx->theme()->get_focused_color();
			} else {
				theme_notebook.selected_client.tab_color = _ctx->theme()->get_selected_color();
			}

			theme_notebook.selected_client.title = _selected->title();
			theme_notebook.selected_client.icon = _selected->icon();
			theme_notebook.selected_client.is_iconic = _selected->is_iconic();
			theme_notebook.has_selected_client = true;
		} else {
			theme_notebook.has_selected_client = false;
		}

		offset += selected_box_width;
		for (auto i : _clients) {
			theme_notebook.clients_tab.push_back(theme_tab_t{});
			auto & tab = theme_notebook.clients_tab.back();
			tab.position = rect{
				(int)floor(offset),
				_allocation.y,
				(int)(floor(offset + _ctx->theme()->notebook.iconic_tab_width) - floor(offset)),
				(int)_ctx->theme()->notebook.tab_height
			};

			if(i->is_focused()) {
				tab.tab_color = _ctx->theme()->get_focused_color();
			} else if(_selected == i) {
				tab.tab_color = _ctx->theme()->get_selected_color();
			} else {
				tab.tab_color = _ctx->theme()->get_normal_color();
			}
			tab.title = i->title();
			tab.icon = i->icon();
			tab.is_iconic = i->is_iconic();
			offset += _ctx->theme()->notebook.iconic_tab_width;
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
	auto pix = _ctx->cmp()->create_composite_pixmap(_allocation.w, _allocation.h);
	cairo_surface_t * surf = pix->get_cairo_surface();
	cairo_t * cr = cairo_create(surf);
	cairo_save(cr);
	cairo_translate(cr, -_allocation.x, -_allocation.y);
	_ctx->theme()->render_notebook(cr, &_theme_notebook);
	cairo_restore(cr);

	/* paste the current window */
	if (_selected != nullptr) {
		update_client_position(_selected);
		if (not _selected->is_iconic()) {
			std::shared_ptr<pixmap_t> pix = _selected->get_last_pixmap();
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

	fading_notebook = make_shared<renderable_notebook_fading_t>(pix, to_root_position(_allocation));

}

void notebook_t::start_exposay() {
	if(_ctx->cmp() == nullptr)
		return;

	if(_selected != nullptr) {
		iconify_client(_selected);
		_selected = nullptr;
	}

	_exposay = true;
	_update_layout();
}

void notebook_t::_update_exposay() {
	if(_ctx->cmp() == nullptr)
		return;

	_exposay_buttons.clear();
	_exposay_thumbnail.clear();
	_exposay_mouse_over = nullptr;

	if(not _exposay)
		return;

	if(_clients.size() <= 0)
		return;

	unsigned clients_counts = _clients.size();

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

	auto it = _clients.begin();
	for(int i = 0; i < _clients.size(); ++i) {
		unsigned y = i / n;
		unsigned x = i % n;

		if(y == m-1)
			xoffset = (_client_area.w-width*n)/2.0 + _client_area.x + (n*m - _clients.size())*width/2.0;

		rect pdst(x*width+1.0+xoffset+8, y*heigth+1.0+yoffset+8, width-2.0-16, heigth-2.0-16);
		_exposay_buttons.push_back(make_tuple(pdst, weak_ptr<client_managed_t>{*it}, i));
		pdst = to_root_position(pdst);
		_exposay_thumbnail.push_back(make_shared<renderable_thumbnail_t>(_ctx, pdst, *it));
		++it;
	}



}

void notebook_t::_stop_exposay() {
	_exposay = false;
	_exposay_buttons.clear();
	_exposay_thumbnail.clear();
	_ctx->add_global_damage(to_root_position(_allocation));
	_update_layout();
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
			_ctx->split_bottom(shared_from_this(), nullptr);
			return true;
		} else if (_area.button_vsplit.is_inside(x, y)) {
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
		} else {
			for(auto & i: _client_buttons) {
				if(std::get<0>(i).is_inside(x, y)) {
					auto c = std::get<1>(i).lock();
					_ctx->grab_start(new grab_bind_client_t{_ctx, c, XCB_BUTTON_INDEX_1, to_root_position(std::get<0>(i))});
					return true;
				}
			}

			for(auto & i: _exposay_buttons) {
				if(std::get<0>(i).is_inside(x, y)) {
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
	}

	return false;

}

void notebook_t::_start_client_menu(shared_ptr<client_managed_t> c, xcb_button_t button, uint16_t x, uint16_t y) {
	auto callback = [this, c] (int selected) -> void { this->_process_notebook_client_menu(c, selected); };
	std::vector<std::shared_ptr<dropdown_menu_t<int>::item_t>> v;
	for(unsigned k = 0; k < _ctx->get_workspace_count(); ++k) {
		std::ostringstream os;
		os << "Desktop #" << k;
		v.push_back(std::make_shared<dropdown_menu_t<int>::item_t>(k, nullptr, os.str()));
	}
	_ctx->grab_start(new dropdown_menu_t<int>{_ctx, v, button, x, y, 300, rect{x-10, y-10, 20, 20}, callback});

}

void notebook_t::_process_notebook_client_menu(shared_ptr<client_managed_t> c, int selected) {
	printf("Change desktop %d for %u\n", selected, c->orig());
	if (selected != _ctx->get_current_workspace()->id()) {
		_ctx->detach(c);
		c->set_parent(nullptr);
		_ctx->get_workspace(selected)->default_pop()->add_client(c, false);
		c->set_current_desktop(selected);
	}
}

bool notebook_t::button_motion(xcb_motion_notify_event_t const * e) {

	if (e->event != get_parent_xid()) {
		return false;
	}

	int x = e->event_x;
	int y = e->event_y;

	if (e->child == XCB_NONE and _allocation.is_inside(x, y)) {
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
		return true;
	} else {
		if(_theme_notebook.button_mouse_over != NOTEBOOK_BUTTON_NONE or _mouse_over.tab != nullptr or _mouse_over.exposay != nullptr) {
			_mouse_over_reset();
			queue_redraw();
		}
	}

	return false;
}

bool notebook_t::leave(xcb_leave_notify_event_t const * ev) {
	if(ev->event == get_parent_xid()) {
		if(_theme_notebook.button_mouse_over != NOTEBOOK_BUTTON_NONE or _mouse_over.tab != nullptr or _mouse_over.exposay != nullptr) {
			_mouse_over_reset();
			queue_redraw();
		}
	}
	return false;
}

void notebook_t::_mouse_over_reset() {
	if (_mouse_over.tab != nullptr) {
		if (std::get<1>(*_mouse_over.tab).lock()->is_focused()) {
			std::get<2>(*_mouse_over.tab)->tab_color =
					_ctx->theme()->get_focused_color();
		} else if (_selected == std::get<1>(*_mouse_over.tab).lock()) {
			std::get<2>(*_mouse_over.tab)->tab_color =
					_ctx->theme()->get_selected_color();
		} else {
			std::get<2>(*_mouse_over.tab)->tab_color =
					_ctx->theme()->get_normal_color();
		}
	}

	if(_mouse_over.exposay) {
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
	queue_redraw();
}

void notebook_t::_client_destroy(shared_ptr<client_managed_t> c) {
	this->_remove_client(c);
}

void notebook_t::_client_activate(shared_ptr<client_managed_t> c) {
	_update_layout();
	queue_redraw();
}

void notebook_t::_client_deactivate(shared_ptr<client_managed_t> c) {
	_update_layout();
	queue_redraw();
}

rect notebook_t::allocation() const {
	return _allocation;
}

void notebook_t::children(vector<shared_ptr<tree_t>> & out) const {
	out.insert(out.end(), _children.begin(), _children.end());
}

void notebook_t::hide() {
	_is_hidden = true;
	for(auto i: tree_t::children()) {
		i->hide();
	}
}

void notebook_t::show() {
	_is_hidden = false;
	for(auto i: tree_t::children()) {
		i->show();
	}
}

bool notebook_t::_has_client(shared_ptr<client_managed_t> c) {
	return has_key(_clients, c);
}

void notebook_t::_set_keep_selected(bool x) {
	_keep_selected = x;
}


void notebook_t::render(cairo_t * cr, region const & area) {
	if(_is_hidden) {
		return;
	}

	if(_exposay) {
		if(_exposay_mouse_over != nullptr)
			_exposay_mouse_over->render(cr, area);
		for(auto & i: _exposay_thumbnail)
			i->render(cr, area);
	}

}

region notebook_t::get_opaque_region() {
	region ret;
	if(_is_hidden) {
		return ret;
	}
	if(_exposay) {
		if(_exposay_mouse_over != nullptr)
			ret += _exposay_mouse_over->get_opaque_region();
		for(auto & i: _exposay_thumbnail)
			ret += i->get_opaque_region();
	}
	return ret;
}

region notebook_t::get_visible_region() {
	region ret;
	if(_is_hidden) {
		return ret;
	}
	if(_exposay) {
		if(_exposay_mouse_over != nullptr)
			ret += _exposay_mouse_over->get_visible_region();
		for(auto & i: _exposay_thumbnail)
			ret += i->get_visible_region();
	}
	return ret;
}

region notebook_t::get_damaged() {
	region ret;
	if(_is_hidden) {
		return ret;
	}
	if(_exposay) {
		if(_exposay_mouse_over != nullptr)
			ret += _exposay_mouse_over->get_damaged();
		for(auto & i: _exposay_thumbnail)
			ret += i->get_damaged();
	}
	return ret;
}

shared_ptr<notebook_t> notebook_t::shared_from_this() {
	return dynamic_pointer_cast<notebook_t>(tree_t::shared_from_this());
}

}
