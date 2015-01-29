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

notebook_t::notebook_t(theme_t const * theme) :
		_theme{theme},
		_parent{nullptr},
		_is_default{false},
		_selected{nullptr},
		_is_hidden{false}
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
	_client_map.insert(x);

	if(prefer_activate) {
		swap_start.update_to_current_time();

		if(_selected != nullptr) {
			/** get current surface then iconify **/
			if (_selected->get_last_pixmap() != nullptr) {
				prev_loc = _selected->base_position();
				prev_surf = _selected->get_last_pixmap();
			}
			_selected->iconify();
		} else {
			/** no prev surf is used **/
			prev_surf.reset();
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
			prev_surf.reset();
		}
	}

	update_theme_notebook();
	return true;
}

i_rect notebook_t::get_new_client_size() {
	return i_rect(allocation().x + _theme->notebook.margin.left,
			allocation().y + _theme->notebook.margin.top,
			allocation().w - _theme->notebook.margin.right - _theme->notebook.margin.left,
			allocation().h - _theme->notebook.margin.top - _theme->notebook.margin.bottom);
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
	if ((_client_map.find(x)) != _client_map.end()) {
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
		swap_start.update_to_current_time();
		if (x->get_last_pixmap() != nullptr) {
			prev_surf = x->get_last_pixmap();
			prev_loc = x->base_position();
		}
		_selected = nullptr;
	}

	// cleanup
	_children.remove(x);
	x->set_parent(nullptr);
	_clients.remove(x);
	_client_map.erase(x);
	update_theme_notebook();

}

void notebook_t::set_selected(client_managed_t * c) {
	/** already selected **/
	if(_selected == c and not c->is_iconic())
		return;

	swap_start.update_to_current_time();
	update_client_position(c);
	c->normalize();
	c->reconfigure();
//	_clients.remove(c);
//	_clients.push_front(c);

	/** iconify current selected **/
	if (_selected != nullptr and c != _selected) {
		/** store current surface then iconify **/
		if (_selected->get_last_pixmap() != nullptr) {
			prev_surf = _selected->get_last_pixmap();
			prev_loc = _selected->base_position();
		}
		_selected->iconify();
		fading_notebook.reset();
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

	swap_start.update_to_current_time();

	if (_selected != nullptr) {
		if (_selected->get_last_pixmap() != nullptr) {
			prev_surf = _selected->get_last_pixmap();
			prev_loc = _selected->base_position();
		}
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
	if (area == allocation())
		return;

	_allocation = area;

	tab_area.x = allocation().x;
	tab_area.y = allocation().y;
	tab_area.w = allocation().w;
	tab_area.h = _theme->notebook.margin.top;

	top_area.x = allocation().x;
	top_area.y = allocation().y + _theme->notebook.margin.top;
	top_area.w = allocation().w;
	top_area.h = (allocation().h - _theme->notebook.margin.top) * 0.2;

	bottom_area.x = allocation().x;
	bottom_area.y = allocation().y + _theme->notebook.margin.top + (0.8 * (allocation().h - _theme->notebook.margin.top));
	bottom_area.w = allocation().w;
	bottom_area.h = (allocation().h - _theme->notebook.margin.top) * 0.2;

	left_area.x = allocation().x;
	left_area.y = allocation().y + _theme->notebook.margin.top;
	left_area.w = allocation().w * 0.2;
	left_area.h = (allocation().h - _theme->notebook.margin.top);

	right_area.x = allocation().x + allocation().w * 0.8;
	right_area.y = allocation().y + _theme->notebook.margin.top;
	right_area.w = allocation().w * 0.2;
	right_area.h = (allocation().h - _theme->notebook.margin.top);

	popup_top_area.x = allocation().x;
	popup_top_area.y = allocation().y + _theme->notebook.margin.top;
	popup_top_area.w = allocation().w;
	popup_top_area.h = (allocation().h - _theme->notebook.margin.top) * 0.5;

	popup_bottom_area.x = allocation().x;
	popup_bottom_area.y = allocation().y + _theme->notebook.margin.top
			+ (0.5 * (allocation().h - _theme->notebook.margin.top));
	popup_bottom_area.w = allocation().w;
	popup_bottom_area.h = (allocation().h - _theme->notebook.margin.top) * 0.5;

	popup_left_area.x = allocation().x;
	popup_left_area.y = allocation().y + _theme->notebook.margin.top;
	popup_left_area.w = allocation().w * 0.5;
	popup_left_area.h = (allocation().h - _theme->notebook.margin.top);

	popup_right_area.x = allocation().x + allocation().w * 0.5;
	popup_right_area.y = allocation().y + _theme->notebook.margin.top;
	popup_right_area.w = allocation().w * 0.5;
	popup_right_area.h = (allocation().h - _theme->notebook.margin.top);

	popup_center_area.x = allocation().x + allocation().w * 0.2;
	popup_center_area.y = allocation().y + _theme->notebook.margin.top + (allocation().h - _theme->notebook.margin.top) * 0.2;
	popup_center_area.w = allocation().w * 0.6;
	popup_center_area.h = (allocation().h - _theme->notebook.margin.top) * 0.6;

	client_area.x = allocation().x + _theme->notebook.margin.left;
	client_area.y = allocation().y + _theme->notebook.margin.top;
	client_area.w = allocation().w - _theme->notebook.margin.left - _theme->notebook.margin.right;
	client_area.h = allocation().h - _theme->notebook.margin.top - _theme->notebook.margin.bottom;

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

	update_theme_notebook();

}


void notebook_t::compute_client_size_with_constraint(client_managed_t * c,
		unsigned int max_width, unsigned int max_height, unsigned int & width,
		unsigned int & height) {

	//printf("XXX max : %d %d\n", max_width, max_height);

	/* default size if no size_hints is provided */
	width = max_width;
	height = max_height;

	XSizeHints size_hints = { 0 };
	if(!c->get_wm_normal_hints(&size_hints)) {
		return;
	}

	::page::compute_client_size_with_constraint(size_hints, max_width,
			max_height, width, height);

	//printf("XXX result : %d %d\n", width, height);
}

i_rect notebook_t::compute_client_size(client_managed_t * c) {
	unsigned int height, width;
	compute_client_size_with_constraint(c, allocation().w - _theme->notebook.margin.left - _theme->notebook.margin.right,
			allocation().h - _theme->notebook.margin.top - _theme->notebook.margin.bottom, width, height);

	/** if the client cannot fit into client_area, clip it **/
	if(width > client_area.w) {
		width = client_area.w;
	}

	if(height > client_area.h) {
		height = client_area.h;
	}

	/* compute the window placement within notebook */
	i_rect client_size;
	client_size.x = floor((client_area.w - width) / 2.0);
	client_size.y = floor((client_area.h - height) / 2.0);
	client_size.w = width;
	client_size.h = height;

	client_size.x += client_area.x;
	client_size.y += client_area.y;

	//printf("Computed client size %s\n", client_size.to_std::string().c_str());
	return client_size;

}

void notebook_t::set_theme(theme_t const * theme) {
	_theme = theme;
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
	std::list<tree_t *> ret(_children.begin(), _children.end());
	return ret;
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

void notebook_t::render_legacy(cairo_t * cr, i_rect const & area) const {
	update_theme_notebook();
	_theme->render_notebook(cr, &theme_notebook, area);
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
			std::shared_ptr<pixmap_t> prev = prev_surf;
			i_rect prev_pos = prev_loc;

			std::shared_ptr<pixmap_t> next { nullptr };
			i_rect next_pos;

			if (_selected != nullptr) {
				if (_selected->get_last_pixmap() != nullptr) {
					next = _selected->get_last_pixmap();
					next_pos = _selected->get_base_position();
				}
			}

			fading_notebook = std::shared_ptr<renderable_notebook_fading_t> {
					new renderable_notebook_fading_t { prev, next, prev_pos,
							next_pos } };

		}

		double ratio = (static_cast<double>(time - swap_start) / static_cast<double const>(animation_duration));
		ratio = ratio*1.05 - 0.025;
		//ratio = (cos(ratio*M_PI-M_PI)*1.05+1.0)/2.0;

		if (_selected != nullptr) {
			if (_selected->get_last_pixmap() != nullptr) {
				fading_notebook->update_next(_selected->get_last_pixmap());
				fading_notebook->update_next_pos(_selected->get_base_position());
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
		prev_surf.reset();
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

i_rect notebook_t::compute_notebook_bookmark_position(
		i_rect const & allocation) const {
	return i_rect{allocation.x + allocation.w - 4 * 17 - 2, allocation.y + 2,
			16, 16};
}

i_rect notebook_t::compute_notebook_vsplit_position(
		i_rect const & allocation) const {

	return i_rect{allocation.x + allocation.w - 3 * 17 - 2, allocation.y + 2,
			16, 16};
}

i_rect notebook_t::compute_notebook_hsplit_position(
		i_rect const & allocation) const {

	return i_rect{allocation.x + allocation.w - 2 * 17 - 2, allocation.y + 2,
			16, 16};

}

i_rect notebook_t::compute_notebook_close_position(
		i_rect const & allocation) const {

	return i_rect{allocation.x + allocation.w - 1 * 17 - 2, allocation.y + 2,
			16, 16};
}

i_rect notebook_t::compute_notebook_menu_position(
		i_rect const & allocation) const {

	return i_rect{allocation.x, allocation.y,
			40, 20};

}


void notebook_t::compute_areas_for_notebook(std::vector<page_event_t> * l) const {

	{
		page_event_t nc(PAGE_EVENT_NOTEBOOK_CLOSE);
		nc.position = compute_notebook_close_position(_allocation);
		nc.nbk = this;
		l->push_back(nc);

		page_event_t nhs(PAGE_EVENT_NOTEBOOK_HSPLIT);
		nhs.position = compute_notebook_hsplit_position(_allocation);
		nhs.nbk = this;
		l->push_back(nhs);

		page_event_t nvs(PAGE_EVENT_NOTEBOOK_VSPLIT);
		nvs.position = compute_notebook_vsplit_position(_allocation);
		nvs.nbk = this;
		l->push_back(nvs);

		page_event_t nm(PAGE_EVENT_NOTEBOOK_MARK);
		nm.position = compute_notebook_bookmark_position(_allocation);
		nm.nbk = this;
		l->push_back(nm);

		page_event_t nmn(PAGE_EVENT_NOTEBOOK_MENU);
		nmn.position = compute_notebook_menu_position(_allocation);
		nmn.nbk = this;
		l->push_back(nmn);

	}

	update_theme_notebook();

	if(_clients.size() > 0) {

		if(_selected != nullptr) {
			i_rect & b = theme_notebook.selected_client.position;

			page_event_t ncclose(PAGE_EVENT_NOTEBOOK_CLIENT_CLOSE);

			ncclose.position.x = b.x + b.w - 35;
			ncclose.position.y = b.y;
			ncclose.position.w = 35;
			ncclose.position.h = b.h-3;
			ncclose.nbk = this;
			ncclose.clt = _selected;
			l->push_back(ncclose);

			page_event_t ncub(PAGE_EVENT_NOTEBOOK_CLIENT_UNBIND);

			ncub.position.x = b.x + b.w - 25 - 30;
			ncub.position.y = b.y;
			ncub.position.w = 16;
			ncub.position.h = 16;
			ncub.nbk = this;
			ncub.clt = _selected;
			l->push_back(ncub);

			page_event_t nc(PAGE_EVENT_NOTEBOOK_CLIENT);
			nc.position = b;
			nc.nbk = this;
			nc.clt = _selected;
			l->push_back(nc);

		}

		auto c = _clients.begin();
		for (unsigned k = 0; k < theme_notebook.clients_tab.size(); ++k) {
			i_rect & b = theme_notebook.clients_tab[k].position;
			page_event_t nc { PAGE_EVENT_NOTEBOOK_CLIENT };
			nc.position = b;
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

void notebook_t::update_theme_notebook() const {
	theme_notebook.clients_tab.clear();

	if (_clients.size() != 0) {
		theme_notebook.clients_tab.resize(_clients.size());

		double box_width = 33.0;
		double selected_box_width = (_allocation.w - 17.0 * 5.0 - 40.0) - (_clients.size()) * box_width;
		double offset = _allocation.x + 40.0;

		if (_selected != nullptr){
			i_rect b { (int)floor(offset), _allocation.y, (int)floor(
					(int)(offset + selected_box_width) - floor(offset)),
					(int)_theme->notebook.margin.top - 1 };
			theme_notebook.selected_client.position = b;
			theme_notebook.selected_client.selected = true;
			theme_notebook.selected_client.focuced = _selected->is_focused();
			theme_notebook.selected_client.title = _selected->title();
			theme_notebook.selected_client.demand_attention = _selected->demands_attention();
			theme_notebook.selected_client.icon = _selected->icon();
			theme_notebook.selected_client.is_iconic = _selected->is_iconic();
			theme_notebook.has_selected_client = true;
		} else {
			theme_notebook.has_selected_client = false;
		}

		offset += selected_box_width;
		unsigned k = 0;
		for (auto i : _clients) {
				i_rect b { (int)floor(offset), _allocation.y, (int)(floor(
						offset + box_width) - floor(offset)),
						(int)_theme->notebook.margin.top - 1 };
			theme_notebook.clients_tab[k].position = b;
			theme_notebook.clients_tab[k].selected = (i == _selected);
			theme_notebook.clients_tab[k].focuced = i->is_focused();
			theme_notebook.clients_tab[k].title = i->title();
			theme_notebook.clients_tab[k].demand_attention = i->demands_attention();
			theme_notebook.clients_tab[k].icon = i->icon();
			theme_notebook.clients_tab[k].is_iconic = i->is_iconic();
			offset += box_width;
			++k;
		}
	} else {
		theme_notebook.has_selected_client = false;
	}

	theme_notebook.allocation = _allocation;
	if(_selected != nullptr) {
		theme_notebook.client_position = _selected->base_position();
	}
	theme_notebook.is_default = is_default();
}



}
