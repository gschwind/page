/*
 * notebook.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include "notebook.hxx"

namespace page {

notebook_t::notebook_t(page_context_t * ctx, bool keep_selected) :
		_ctx{ctx},
		_parent{nullptr},
		_is_default{false},
		_selected{nullptr},
		_is_hidden{false},
		_keep_selected{keep_selected}
{

}

notebook_t::~notebook_t() {

}

bool notebook_t::add_client(client_managed_t * x, bool prefer_activate) {
	assert(not has_key(_clients, x));
	assert(x != nullptr);
	x->set_parent(this);
	_children.push_back(x);
	_clients.push_front(x);
	_client_to_tab[x] = std::make_shared<theme_tab_t>();

	if(prefer_activate) {
		start_fading();

		if(_selected != nullptr) {
			_selected->iconify();
		}

		update_client_position(x);
		x->normalize();
		x->reconfigure();
		_selected = x;

	} else {
		x->iconify();
		if(_selected != nullptr) {
			/* do nothing */
		} else {
			/** no prev surf is used **/
			_selected = x;
		}
	}

	return true;
}

i_rect notebook_t::get_new_client_size() {
	return i_rect{
		allocation().x + _ctx->theme()->notebook.margin.left,
		allocation().y + _ctx->theme()->notebook.margin.top + _ctx->theme()->notebook.tab_height,
		allocation().w - _ctx->theme()->notebook.margin.right - _ctx->theme()->notebook.margin.left,
		allocation().h - _ctx->theme()->notebook.margin.top - _ctx->theme()->notebook.margin.bottom - _ctx->theme()->notebook.tab_height
	};
}

void notebook_t::replace(page_component_t * src, page_component_t * by) {
	throw std::runtime_error("cannot replace in notebook");
}

void notebook_t::remove(tree_t * src) {
	client_managed_t * mw = dynamic_cast<client_managed_t*>(src);
	if (has_key(_clients, mw) and mw != nullptr) {
		remove_client(mw);
	}
}

void notebook_t::activate_client(client_managed_t * x) {
	if (has_key(_clients, x)) {
		set_selected(x);
	}
}

std::list<client_managed_t *> const & notebook_t::get_clients() {
	return _clients;
}

void notebook_t::remove_client(client_managed_t * x) {
	assert(has_key(_clients, x));

	/** update selection **/
	if (_selected == x) {
		start_fading();
		_selected = nullptr;
	}

	// cleanup
	_children.remove(x);
	x->set_parent(nullptr);
	_clients.remove(x);
	_client_to_tab.erase(x);

	if(_keep_selected and not _children.empty() and _selected == nullptr) {
		_selected = dynamic_cast<client_managed_t*>(_children.back());

		if (_selected != nullptr) {
			update_client_position(_selected);
			_selected->normalize();
			_selected->reconfigure();
		}

		fading_notebook.reset();
	}

}

void notebook_t::set_selected(client_managed_t * c) {
	/** already selected **/
	if(_selected == c and not c->is_iconic())
		return;

	start_fading();

	update_client_position(c);
	c->normalize();
	c->reconfigure();

	if(_selected != nullptr and c != _selected) {
		_selected->iconify();
	}
	/** set selected **/
	_selected = c;
}

void notebook_t::update_client_position(client_managed_t * c) {
	/* compute the window placement within notebook */
	i_rect client_size = compute_client_size(c);
	c->set_notebook_wished_position(client_size);
	c->reconfigure();
}

void notebook_t::iconify_client(client_managed_t * x) {
	assert(has_key(_clients, x));

	/** already iconified **/
	if(_selected != x)
		return;

	start_fading();

	if (_selected != nullptr) {
		_selected->iconify();
	}

}

notebook_t * notebook_t::get_nearest_notebook() {
	return this;
}

void notebook_t::delete_all() {

}

void notebook_t::unmap_all() {
	if (_selected != nullptr) {
		iconify_client(_selected);
	}
}

void notebook_t::map_all() {
	if (_selected != nullptr) {
		_selected->normalize();
	}
}

i_rect notebook_t::get_absolute_extend() {
	return allocation();
}

region notebook_t::get_area() {
	if (_selected != nullptr) {
		region area{allocation()};
		area -= _selected->get_base_position();
		return area;
	} else {
		return region(allocation());
	}
}

void notebook_t::set_allocation(i_rect const & area) {
	if (area == _allocation)
		return;

	_allocation = area;

	tab_area.x = _allocation.x;
	tab_area.y = _allocation.y;
	tab_area.w = _allocation.w;
	tab_area.h = _ctx->theme()->notebook.tab_height;

	client_area.x = _allocation.x + _ctx->theme()->notebook.margin.left;
	client_area.y = _allocation.y + _ctx->theme()->notebook.margin.top + _ctx->theme()->notebook.tab_height;
	client_area.w = _allocation.w - _ctx->theme()->notebook.margin.left - _ctx->theme()->notebook.margin.right;
	client_area.h = _allocation.h - _ctx->theme()->notebook.margin.top - _ctx->theme()->notebook.margin.bottom - _ctx->theme()->notebook.tab_height;

	top_area.x = _allocation.x;
	top_area.y = _allocation.y + _ctx->theme()->notebook.tab_height;
	top_area.w = _allocation.w;
	top_area.h = (_allocation.h - _ctx->theme()->notebook.tab_height) * 0.2;

	bottom_area.x = _allocation.x;
	bottom_area.y = _allocation.y + (0.8 * (allocation().h - _ctx->theme()->notebook.tab_height));
	bottom_area.w = _allocation.w;
	bottom_area.h = (_allocation.h - _ctx->theme()->notebook.tab_height) * 0.2;

	left_area.x = _allocation.x;
	left_area.y = _allocation.y + _ctx->theme()->notebook.tab_height;
	left_area.w = _allocation.w * 0.2;
	left_area.h = (_allocation.h - _ctx->theme()->notebook.tab_height);

	right_area.x = _allocation.x + _allocation.w * 0.8;
	right_area.y = _allocation.y + _ctx->theme()->notebook.tab_height;
	right_area.w = _allocation.w * 0.2;
	right_area.h = (_allocation.h - _ctx->theme()->notebook.tab_height);

	popup_top_area.x = _allocation.x;
	popup_top_area.y = _allocation.y + _ctx->theme()->notebook.tab_height;
	popup_top_area.w = _allocation.w;
	popup_top_area.h = (_allocation.h - _ctx->theme()->notebook.tab_height) * 0.5;

	popup_bottom_area.x = _allocation.x;
	popup_bottom_area.y = _allocation.y + _ctx->theme()->notebook.tab_height
			+ (0.5 * (_allocation.h - _ctx->theme()->notebook.tab_height));
	popup_bottom_area.w = _allocation.w;
	popup_bottom_area.h = (_allocation.h - _ctx->theme()->notebook.tab_height) * 0.5;

	popup_left_area.x = _allocation.x;
	popup_left_area.y = _allocation.y + _ctx->theme()->notebook.tab_height;
	popup_left_area.w = _allocation.w * 0.5;
	popup_left_area.h = (_allocation.h - _ctx->theme()->notebook.tab_height);

	popup_right_area.x = _allocation.x + allocation().w * 0.5;
	popup_right_area.y = _allocation.y + _ctx->theme()->notebook.tab_height;
	popup_right_area.w = _allocation.w * 0.5;
	popup_right_area.h = (_allocation.h - _ctx->theme()->notebook.tab_height);

	popup_center_area.x = _allocation.x + allocation().w * 0.2;
	popup_center_area.y = _allocation.y + _ctx->theme()->notebook.tab_height + (allocation().h - _ctx->theme()->notebook.tab_height) * 0.2;
	popup_center_area.w = _allocation.w * 0.6;
	popup_center_area.h = (_allocation.h - _ctx->theme()->notebook.tab_height) * 0.6;


	if(client_area.w <= 0) {
		client_area.w = 1;
	}

	if(client_area.h <= 0) {
		client_area.h = 1;
	}

	if(_selected != nullptr) {
		update_client_position(_selected);
		_selected->reconfigure();
	}

}

i_rect notebook_t::compute_client_size(client_managed_t * c) {
	dimention_t<unsigned> size =
			c->compute_size_with_constrain(client_area.w, client_area.h);

	/** if the client cannot fit into client_area, clip it **/
	if(size.width > client_area.w) {
		size.width = client_area.w;
	}

	if(size.height > client_area.h) {
		size.height = client_area.h;
	}

	/* compute the window placement within notebook */
	i_rect client_size;
	client_size.x = floor((client_area.w - size.width) / 2.0);
	client_size.y = floor((client_area.h - size.height) / 2.0);
	client_size.w = size.width;
	client_size.h = size.height;

	client_size.x += client_area.x;
	client_size.y += client_area.y;

	return client_size;

}

client_managed_t const * notebook_t::selected() const {
	return _selected;
}

bool notebook_t::is_default() const {
	return _is_default;
}

void notebook_t::set_default(bool x) {
	_is_default = x;
}

std::list<tree_t *> notebook_t::childs() const {
	return std::list<tree_t *>{_children.begin(), _children.end()};
}

void notebook_t::raise_child(tree_t * t) {

	if (has_key(_children, t)) {
		_children.remove(t);
		_children.push_back(t);

		if(_parent != nullptr) {
			_parent->raise_child(this);
		}

	} else if (t != nullptr) {
		throw exception_t("notebook_t::raise_child trying to raise a non child tree");
	}

}

std::string notebook_t::get_node_name() const {
	return _get_node_name<'N'>();
}

void notebook_t::render_legacy(cairo_t * cr, int x_offset, int y_offset) const {
	update_theme_notebook(x_offset, y_offset);
	_ctx->theme()->render_notebook(cr, &theme_notebook);
}

client_managed_t * notebook_t::get_selected() {
	return _selected;
}

void notebook_t::prepare_render(std::vector<std::shared_ptr<renderable_t>> & out, page::time_t const & time) {

	if(_is_hidden) {
		return;
	}

	if (time < (swap_start + animation_duration)) {

		if (fading_notebook == nullptr) {
			return;
		}

		double ratio = (static_cast<double>(time - swap_start) / static_cast<double const>(animation_duration));
		ratio = ratio*1.05 - 0.025;
		//ratio = (cos(ratio*M_PI-M_PI)*1.05+1.0)/2.0;

		if (_selected != nullptr) {
			if (_selected->get_last_pixmap() != nullptr) {
				fading_notebook->update_client(_selected->get_last_pixmap(), _selected->get_base_position());
				fading_notebook->update_client_area(client_area);
			}
		}

		fading_notebook->set_ratio(ratio);
		//printf("ratio = %f\n", ratio);
		out += dynamic_pointer_cast<renderable_t>(fading_notebook);

		if (_selected != nullptr) {
			for(auto i: _selected->childs()) {
				i->prepare_render(out, time);
			}
		}

	} else {
		/** animation is terminated **/
		if(fading_notebook != nullptr) {
			fading_notebook.reset();
			/** force redraw of notebook **/
			std::shared_ptr<renderable_t> x{new renderable_empty_t{_allocation}};
			out += x;
		}

		if (_selected != nullptr) {

			std::shared_ptr<renderable_t> x{_selected->get_base_renderable()};

			if (x != nullptr) {
				out += x;
			}

			/** bypass prepare_render of notebook childs **/
			for (auto & i : _selected->childs()) {
				i->prepare_render(out, time);
			}
		}

	}

}

i_rect notebook_t::compute_notebook_bookmark_position() const {
	return i_rect{
		_allocation.x + _allocation.w
		- _ctx->theme()->notebook.close_width
		- _ctx->theme()->notebook.hsplit_width
		- _ctx->theme()->notebook.vsplit_width
		- _ctx->theme()->notebook.mark_width,
		_allocation.y,
		_ctx->theme()->notebook.mark_width,
		_ctx->theme()->notebook.tab_height
	};
}

i_rect notebook_t::compute_notebook_vsplit_position() const {
	return i_rect{
		_allocation.x + _allocation.w
			- _ctx->theme()->notebook.close_width
			- _ctx->theme()->notebook.hsplit_width
			- _ctx->theme()->notebook.vsplit_width,
		_allocation.y,
		_ctx->theme()->notebook.vsplit_width,
		_ctx->theme()->notebook.tab_height
	};
}

i_rect notebook_t::compute_notebook_hsplit_position() const {

	return i_rect{
		_allocation.x + _allocation.w - _ctx->theme()->notebook.close_width - _ctx->theme()->notebook.hsplit_width,
		_allocation.y,
		_ctx->theme()->notebook.hsplit_width,
		_ctx->theme()->notebook.tab_height
	};

}

i_rect notebook_t::compute_notebook_close_position() const {
	return i_rect{
		_allocation.x + _allocation.w - _ctx->theme()->notebook.close_width,
		_allocation.y,
		_ctx->theme()->notebook.close_width,
		_ctx->theme()->notebook.tab_height
	};
}

i_rect notebook_t::compute_notebook_menu_position() const {
	return i_rect{
		_allocation.x,
		_allocation.y,
		_ctx->theme()->notebook.menu_button_width,
		_ctx->theme()->notebook.tab_height
	};

}


void notebook_t::compute_areas_for_notebook(std::vector<page_event_t> * l, int x_offset, int y_offset) const {

	{
		page_event_t nc{PAGE_EVENT_NOTEBOOK_CLOSE};
		nc.position = compute_notebook_close_position();
		nc.position.x -= x_offset;
		nc.position.y -= y_offset;
		nc.nbk = this;
		l->push_back(nc);

		page_event_t nhs{PAGE_EVENT_NOTEBOOK_HSPLIT};
		nhs.position = compute_notebook_hsplit_position();
		nhs.position.x -= x_offset;
		nhs.position.y -= y_offset;
		nhs.nbk = this;
		l->push_back(nhs);

		page_event_t nvs{PAGE_EVENT_NOTEBOOK_VSPLIT};
		nvs.position = compute_notebook_vsplit_position();
		nvs.position.x -= x_offset;
		nvs.position.y -= y_offset;
		nvs.nbk = this;
		l->push_back(nvs);

		page_event_t nm{PAGE_EVENT_NOTEBOOK_MARK};
		nm.position = compute_notebook_bookmark_position();
		nm.position.x -= x_offset;
		nm.position.y -= y_offset;
		nm.nbk = this;
		l->push_back(nm);

		page_event_t nmn{PAGE_EVENT_NOTEBOOK_MENU};
		nmn.position = compute_notebook_menu_position();
		nmn.position.x -= x_offset;
		nmn.position.y -= y_offset;
		nmn.nbk = this;
		l->push_back(nmn);

	}

	if(_clients.size() > 0) {

		if(_selected != nullptr) {
			i_rect & b = theme_notebook.selected_client->position;

			page_event_t ncclose{PAGE_EVENT_NOTEBOOK_CLIENT_CLOSE};

			ncclose.position.x = b.x + b.w - _ctx->theme()->notebook.selected_close_width;
			ncclose.position.y = b.y;
			ncclose.position.w = _ctx->theme()->notebook.selected_close_width;
			ncclose.position.h = _ctx->theme()->notebook.tab_height;
			ncclose.nbk = this;
			ncclose.clt = _selected;
			l->push_back(ncclose);

			page_event_t ncub{PAGE_EVENT_NOTEBOOK_CLIENT_UNBIND};

			ncub.position.x = b.x + b.w
					- _ctx->theme()->notebook.selected_close_width
					- _ctx->theme()->notebook.selected_unbind_width;
			ncub.position.y = b.y;
			ncub.position.w = _ctx->theme()->notebook.selected_unbind_width;
			ncub.position.h = _ctx->theme()->notebook.tab_height;
			ncub.nbk = this;
			ncub.clt = _selected;
			l->push_back(ncub);

			page_event_t nc{PAGE_EVENT_NOTEBOOK_CLIENT};
			nc.position = b;
			nc.nbk = this;
			nc.clt = _selected;
			l->push_back(nc);

		}

		auto c = _clients.begin();
		for (auto const & tab: theme_notebook.clients_tab) {
			page_event_t nc{PAGE_EVENT_NOTEBOOK_CLIENT};
			nc.position = tab->position;
			nc.nbk = this;
			nc.clt = *c;
			l->push_back(nc);
			++c;
		}

	}
}

void notebook_t::get_all_children(std::vector<tree_t *> & out) const {
	for(auto x: _children) {
		out.push_back(x);
		x->get_all_children(out);
	}
}

void notebook_t::update_theme_notebook(int x_offset, int y_offset) const {
	theme_notebook.clients_tab.clear();
	i_rect allocation{_allocation};
	allocation.x -= x_offset;
	allocation.y -= y_offset;

	theme_notebook.root_x = x_offset;
	theme_notebook.root_y = y_offset;

	if (_clients.size() != 0) {
		double selected_box_width = (allocation.w
				- _ctx->theme()->notebook.close_width
				- _ctx->theme()->notebook.hsplit_width
				- _ctx->theme()->notebook.vsplit_width
				- _ctx->theme()->notebook.mark_width
				- _ctx->theme()->notebook.menu_button_width)
				- _clients.size() * _ctx->theme()->notebook.iconic_tab_width;
		double offset = allocation.x + _ctx->theme()->notebook.menu_button_width;

		if (_selected != nullptr){
			/** copy the tab context **/
			auto tab = make_shared<theme_tab_t>(*_client_to_tab[_selected]);
			theme_notebook.selected_client = tab;
			tab->position = i_rect{
					(int)floor(offset),
					allocation.y, (int)floor(
					(int)(offset + selected_box_width) - floor(offset)),
					(int)_ctx->theme()->notebook.tab_height };

			if(_selected->is_focused()) {
				tab->tab_color = _ctx->theme()->get_focused_color();
			} else {
				tab->tab_color = _ctx->theme()->get_selected_color();
			}

			tab->title = _selected->title();
			tab->icon = _selected->icon();
			tab->is_iconic = _selected->is_iconic();
			theme_notebook.has_selected_client = true;
		} else {
			theme_notebook.has_selected_client = false;
		}

		offset += selected_box_width;
		unsigned k = 0;
		for (auto i : _clients) {
			auto const & tab = _client_to_tab[i];
			theme_notebook.clients_tab.push_back(tab);
			tab->position = i_rect{
				(int)floor(offset),
				allocation.y,
				(int)(floor(offset + _ctx->theme()->notebook.iconic_tab_width) - floor(offset)),
				(int)_ctx->theme()->notebook.tab_height};

			if(i->is_focused()) {
				tab->tab_color = _ctx->theme()->get_focused_color();
			} else if(_selected == i) {
				tab->tab_color = _ctx->theme()->get_selected_color();
			} else {
				tab->tab_color = _ctx->theme()->get_normal_color();
			}
			tab->title = i->title();
			tab->icon = i->icon();
			tab->is_iconic = i->is_iconic();
			offset += _ctx->theme()->notebook.iconic_tab_width;
			++k;
		}
	} else {
		theme_notebook.has_selected_client = false;
	}

	theme_notebook.allocation = allocation;
	if(_selected != nullptr) {
		theme_notebook.client_position = _selected->base_position();
	}
	theme_notebook.is_default = is_default();
}

void notebook_t::start_fading() {
	if(_ctx->cmp() == nullptr)
		return;

	swap_start.update_to_current_time();

	update_theme_notebook(client_area.x, client_area.y);

	auto pix = _ctx->cmp()->create_composite_pixmap(client_area.w, client_area.h);
	cairo_surface_t * surf = pix->get_cairo_surface();
	cairo_t * cr = cairo_create(surf);
	_ctx->theme()->render_notebook(cr, &theme_notebook);

	/* paste the current window */
	if(_selected != nullptr) {
		std::shared_ptr<pixmap_t> pix = _selected->get_last_pixmap();
		if(pix != nullptr) {
			i_rect pos = _selected->get_base_position();
			i_rect cl{pos.x - client_area.x, pos.y - client_area.y, pos.w, pos.h};
			cairo_clip(cr, cl);
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
			cairo_set_source_surface(cr, pix->get_cairo_surface(), cl.x, cl.y);
			cairo_mask_surface(cr, pix->get_cairo_surface(), cl.x, cl.y);
		}
	}

	cairo_destroy(cr);

	fading_notebook = std::make_shared<renderable_notebook_fading_t>(pix, client_area);

}



}
