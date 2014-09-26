/*
 * notebook.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */


#include <algorithm>
#include <cmath>

#include "renderable_notebook_fading.hxx"
#include "renderable_pixmap.hxx"
#include "notebook.hxx"


namespace page {

notebook_t::notebook_t(theme_t const * theme) : _theme(theme) {
	_is_default = false;
	_selected = nullptr;
}

notebook_t::~notebook_t() {

}

bool notebook_t::add_client(managed_window_t * x, bool prefer_activate) {
	page_assert(not has_key(_clients, x));
	_children.push_back(x);
	x->set_parent(this);
	_clients.push_front(x);
	_client_map.insert(x);

	if(prefer_activate) {
		swap_start.get_time();

		if(_selected != nullptr) {
			/** get current surface then iconify **/
			if (_selected->surf() != nullptr) {
				prev_loc = _selected->base_position();
				prev_surf = _selected->surf()->get_pixmap();
			}
			_selected->iconify();
		} else {
			/** no prev surf is used **/
			prev_surf.reset();
		}

		cur_surf = x->surf();
		update_client_position(x);
		x->normalize();
		_selected = x;

	} else {
		x->iconify();
	}

	return true;
}

i_rect notebook_t::get_new_client_size() {
	return i_rect(allocation().x + _theme->notebook_margin.left,
			allocation().y + _theme->notebook_margin.top,
			allocation().w - _theme->notebook_margin.right - _theme->notebook_margin.left,
			allocation().h - _theme->notebook_margin.top - _theme->notebook_margin.bottom);
}

void notebook_t::replace(tree_t * src, tree_t * by) {
	throw std::runtime_error("cannot replace in notebook");
}

void notebook_t::close(tree_t * src) {

}

void notebook_t::remove(tree_t * src) {
	managed_window_t * mw = dynamic_cast<managed_window_t*>(src);
	if (has_key(_clients, mw) and mw != nullptr) {
		remove_client(mw);
	}
}

void notebook_t::activate_client(managed_window_t * x) {
	if ((_client_map.find(x)) != _client_map.end()) {
		set_selected(x);
	}
}

list<managed_window_t *> const & notebook_t::get_clients() {
	return _clients;
}

void notebook_t::remove_client(managed_window_t * x) {
	page_assert(has_key(_clients, x));

	/** update selection **/
	if (_selected == x) {
		swap_start.get_time();
		if (x->surf() != nullptr) {
			prev_surf = x->surf()->get_pixmap();
			prev_loc = x->base_position();
		}
		_selected = nullptr;
	}

	// cleanup
	_children.remove(x);
	x->set_parent(nullptr);
	_clients.remove(x);
	_client_map.erase(x);
}

void notebook_t::set_selected(managed_window_t * c) {
	/** already selected **/
	if(_selected == c)
		return;

	swap_start.get_time();
	update_client_position(c);
	c->normalize();

	/** iconify current selected **/
	if (_selected != nullptr) {
		/** store current surface then iconify **/
		if (_selected->surf() != nullptr) {
			prev_surf = _selected->surf()->get_pixmap();
			prev_loc = _selected->base_position();
		}
		_selected->iconify();
		fading_notebook.reset();
	}

	/** keep current surface in track **/
	cur_surf = c->surf();
	/** set selected **/
	_selected = c;

}

void notebook_t::update_client_position(managed_window_t * c) {
	/* compute the window placement within notebook */
	i_rect client_size = compute_client_size(c);
	c->set_notebook_wished_position(client_size);
	c->reconfigure();
}

void notebook_t::iconify_client(managed_window_t * x) {
	page_assert(has_key(_clients, x));

	/** already iconified **/
	if(_selected != x)
		return;

	swap_start.get_time();

	if (_selected != nullptr) {
		if (_selected->surf() != nullptr) {
			prev_surf = _selected->surf()->get_pixmap();
			prev_loc = _selected->base_position();
		}
		_selected->iconify();
	}

	cur_surf.reset();

	_selected = nullptr;

}

notebook_t * notebook_t::get_nearest_notebook() {
	return this;
}

void notebook_t::delete_all() {

}

void notebook_t::unmap_all() {
	if (_selected != nullptr) {
		_selected->iconify();
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

	tree_t::set_allocation(area);

	tab_area.x = allocation().x;
	tab_area.y = allocation().y;
	tab_area.w = allocation().w;
	tab_area.h = _theme->notebook_margin.top;

	top_area.x = allocation().x;
	top_area.y = allocation().y + _theme->notebook_margin.top;
	top_area.w = allocation().w;
	top_area.h = (allocation().h - _theme->notebook_margin.top) * 0.2;

	bottom_area.x = allocation().x;
	bottom_area.y = allocation().y + _theme->notebook_margin.top + (0.8 * (allocation().h - _theme->notebook_margin.top));
	bottom_area.w = allocation().w;
	bottom_area.h = (allocation().h - _theme->notebook_margin.top) * 0.2;

	left_area.x = allocation().x;
	left_area.y = allocation().y + _theme->notebook_margin.top;
	left_area.w = allocation().w * 0.2;
	left_area.h = (allocation().h - _theme->notebook_margin.top);

	right_area.x = allocation().x + allocation().w * 0.8;
	right_area.y = allocation().y + _theme->notebook_margin.top;
	right_area.w = allocation().w * 0.2;
	right_area.h = (allocation().h - _theme->notebook_margin.top);

	popup_top_area.x = allocation().x;
	popup_top_area.y = allocation().y + _theme->notebook_margin.top;
	popup_top_area.w = allocation().w;
	popup_top_area.h = (allocation().h - _theme->notebook_margin.top) * 0.5;

	popup_bottom_area.x = allocation().x;
	popup_bottom_area.y = allocation().y + _theme->notebook_margin.top
			+ (0.5 * (allocation().h - _theme->notebook_margin.top));
	popup_bottom_area.w = allocation().w;
	popup_bottom_area.h = (allocation().h - _theme->notebook_margin.top) * 0.5;

	popup_left_area.x = allocation().x;
	popup_left_area.y = allocation().y + _theme->notebook_margin.top;
	popup_left_area.w = allocation().w * 0.5;
	popup_left_area.h = (allocation().h - _theme->notebook_margin.top);

	popup_right_area.x = allocation().x + allocation().w * 0.5;
	popup_right_area.y = allocation().y + _theme->notebook_margin.top;
	popup_right_area.w = allocation().w * 0.5;
	popup_right_area.h = (allocation().h - _theme->notebook_margin.top);

	popup_center_area.x = allocation().x + allocation().w * 0.2;
	popup_center_area.y = allocation().y + _theme->notebook_margin.top + (allocation().h - _theme->notebook_margin.top) * 0.2;
	popup_center_area.w = allocation().w * 0.6;
	popup_center_area.h = (allocation().h - _theme->notebook_margin.top) * 0.6;

	client_area.x = allocation().x + _theme->notebook_margin.left;
	client_area.y = allocation().y + _theme->notebook_margin.top;
	client_area.w = allocation().w - _theme->notebook_margin.left - _theme->notebook_margin.right;
	client_area.h = allocation().h - _theme->notebook_margin.top - _theme->notebook_margin.bottom;

	for(auto i : _client_map) {
		update_client_position(i);
	}

}


void notebook_t::compute_client_size_with_constraint(managed_window_t * c,
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

i_rect notebook_t::compute_client_size(managed_window_t * c) {
	unsigned int height, width;
	compute_client_size_with_constraint(c, allocation().w - _theme->notebook_margin.left - _theme->notebook_margin.right,
			allocation().h - _theme->notebook_margin.top - _theme->notebook_margin.bottom, width, height);

	/* compute the window placement within notebook */
	i_rect client_size;
	client_size.x = floor((client_area.w - width) / 2.0);
	client_size.y = floor((client_area.h - height) / 2.0);
	client_size.w = width;
	client_size.h = height;

	if (client_size.x < 0)
		client_size.x = 0;
	if (client_size.y < 0)
		client_size.y = 0;
	if (client_size.w > client_area.w)
		client_size.w = client_area.w;
	if (client_size.h > client_area.h)
		client_size.h = client_area.h;

	client_size.x += client_area.x;
	client_size.y += client_area.y;

	//printf("Computed client size %s\n", client_size.to_string().c_str());
	return client_size;

}

i_rect const & notebook_t::get_allocation() {
	return allocation();
}

void notebook_t::set_theme(theme_t const * theme) {
	_theme = theme;
}

list<managed_window_base_t const *> notebook_t::clients() const {
	return list<managed_window_base_t const *>(_clients.begin(), _clients.end());
}

managed_window_base_t const * notebook_t::selected() const {
	return _selected;
}

bool notebook_t::is_default() const {
	return _is_default;
}

void notebook_t::set_default(bool x) {
	_is_default = x;
}

list<tree_t *> notebook_t::childs() const {
	list<tree_t *> ret(_children.begin(), _children.end());
	return ret;
}

void notebook_t::raise_child(tree_t * t) {

	if (has_key(_children, t)) {
		_children.remove(t);
		_children.push_back(t);
	}

	if(_parent != nullptr) {
		_parent->raise_child(this);
	}

}

string notebook_t::get_node_name() const {
	return _get_node_name<'N'>();
}

void notebook_t::render_legacy(cairo_t * cr, i_rect const & area) const {
	_theme->render_notebook(cr, this, area);
}

void notebook_t::render(cairo_t * cr, time_t time) {
//	page::time_t d(0, animation_duration);
//	if (time < (swap_start + d)) {
//		double ratio = (sin(
//				static_cast<double>(time - swap_start) / animation_duration
//						* M_PI - M_PI_2) * 1.05 + 1.0) / 2.0;
//
//		ratio = std::min(std::max(0.0, ratio), 1.0);
//
//		i_rect x_prv_loc;
//		i_rect x_new_loc;
//
//		if (prev_surf != nullptr) {
//			x_prv_loc = prev_loc;
//		}
//
//		if (_selected != nullptr) {
//			x_new_loc = _selected->get_base_position();
//		}
//
//		/** render old surface if one is available **/
//		if (prev_surf != nullptr) {
//			cairo_surface_t * s = prev_surf->get_cairo_surface();
//			region r{x_prv_loc};
//			r -= x_new_loc;
//
//			cairo_save(cr);
//			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
//			cairo_pattern_t * p0 = cairo_pattern_create_rgba(1.0, 1.0, 1.0,
//					1.0 - ratio);
//			for (auto &a : r) {
//				cairo_reset_clip(cr);
//				cairo_rectangle(cr, a.x, a.y, a.w, a.h);
//				cairo_clip(cr);
//				cairo_set_source_surface(cr, s, x_prv_loc.x, x_prv_loc.y);
//				cairo_mask(cr, p0);
//			}
//			cairo_pattern_destroy(p0);
//
//			region r1{x_prv_loc};
//			r1 &= x_new_loc;
//			for (auto &a : r1) {
//				cairo_reset_clip(cr);
//				cairo_rectangle(cr, a.x, a.y, a.w, a.h);
//				cairo_clip(cr);
//				cairo_set_source_surface(cr, s, x_prv_loc.x, x_prv_loc.y);
//				cairo_paint(cr);
//			}
//
//			cairo_restore(cr);
//		}
//
//		/** render selected surface if available **/
//		if (_selected != nullptr) {
//			shared_ptr<composite_surface_t> psurf = _selected->surf();
//			if (psurf != nullptr) {
//				shared_ptr<pixmap_t> p = psurf->get_pixmap();
//				if (p != nullptr) {
//					cairo_surface_t * s = p->get_cairo_surface();
//					region r{x_new_loc};
//					r -= x_prv_loc;
//
//					cairo_save(cr);
//					cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
//					cairo_pattern_t * p0 = cairo_pattern_create_rgba(1.0, 1.0,
//							1.0, ratio);
//					cairo_set_source_surface(cr, s, x_new_loc.x, x_new_loc.y);
//					cairo_rectangle(cr, x_new_loc.x, x_new_loc.y, x_new_loc.w,
//							x_new_loc.h);
//					cairo_clip(cr);
//					cairo_mask(cr, p0);
//					cairo_pattern_destroy(p0);
//
//					cairo_restore(cr);
//				}
//			}
//
////			for (auto i : _selected->childs()) {
////				i->render(cr, time);
////			}
//		}
//
//	} else {
//		/** animation is terminated **/
//		prev_surf.reset();
//
//
//		/** draw selected window is one is available **/
//		if (_selected != nullptr) {
//			shared_ptr<composite_surface_t> psurf = _selected->surf();
//			if (psurf != nullptr) {
//				shared_ptr<pixmap_t> p = psurf->get_pixmap();
//				if (p != nullptr) {
//					cairo_surface_t * s = p->get_cairo_surface();
//					i_rect location = _selected->get_base_position();
//
//					cairo_save(cr);
//
//					cairo_set_line_width(cr, 1.0);
//					cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
//					cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
//					cairo_rectangle(cr, location.x - 0.5, location.y - 0.5,
//							location.w + 1.0, location.h + 1.0);
//					cairo_stroke(cr);
//
//					cairo_set_source_surface(cr, s, location.x, location.y);
//					cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
//					cairo_rectangle(cr, location.x, location.y, location.w,
//							location.h);
//					cairo_fill(cr);
//
//					cairo_restore(cr);
//				}
//			}
//
////			for (auto i : _selected->childs()) {
////				i->render(cr, time);
////			}
//
//		}
//
//	}

}

bool notebook_t::need_render(time_t time) {

	page::time_t d(0, animation_duration);
	d += 100000000;
	if (time < (swap_start + d)) {
		return true;
	}

	for(auto i: childs()) {
		if(i->need_render(time)) {
			return true;
		}
	}
	return false;
}

managed_window_t * notebook_t::get_selected() {
	return _selected;
}

void notebook_t::prepare_render(vector<ptr<renderable_t>> & out, page::time_t const & time) {

	if (time < (swap_start + animation_duration)) {

		if (fading_notebook == nullptr) {
			ptr<pixmap_t> prev = prev_surf;
			i_rect prev_pos = prev_loc;

			ptr<pixmap_t> next { nullptr };
			i_rect next_pos;

			if (_selected != nullptr) {
				if (_selected->surf() != nullptr) {
					next = _selected->surf()->get_pixmap();
					next_pos = _selected->get_base_position();
				}
			}

			fading_notebook = ptr<renderable_notebook_fading_t> {
					new renderable_notebook_fading_t { prev, next, prev_pos,
							next_pos } };

		}

		double ratio = (static_cast<double>(time - swap_start) / static_cast<double const>(animation_duration));
		//ratio = (cos(ratio*M_PI-M_PI)*1.05+1.0)/2.0;

		if (_selected != nullptr) {
			if (_selected->surf() != nullptr) {
				fading_notebook->update_next(_selected->surf()->get_pixmap());
				fading_notebook->update_next_pos(_selected->get_base_position());
			}
		}

		fading_notebook->set_ratio(ratio);
		//printf("ratio = %f\n", ratio);
		out += dynamic_pointer_cast<renderable_t>(fading_notebook);

	} else {
		/** animation is terminated **/
		prev_surf.reset();
		fading_notebook.reset();

		if (_selected != nullptr) {

			ptr<renderable_t> x{_selected->get_base_renderable()};

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




}
