/*
 * split.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <cstdio>
#include <cairo-xlib.h>
#include <X11/cursorfont.h>
#include <cmath>

#include "split.hxx"
#include "grab_handlers.hxx"

namespace page {

using namespace std;

split_t::split_t(page_context_t * ctx, split_type_e type) :
		_ctx{ctx},
		_type{type},
		_ratio{0.5},
		_has_mouse_over{false},
		_wid{XCB_WINDOW_NONE}
{
	update_allocation();

}

split_t::~split_t() {
	if(_wid != XCB_WINDOW_NONE) {
		xcb_destroy_window(_ctx->dpy()->xcb(), _wid);
	}
}

void split_t::set_allocation(rect const & allocation) {
	_allocation = allocation;
	update_allocation();
}

void split_t::replace(shared_ptr<page_component_t> src, shared_ptr<page_component_t> by) {
	if (_pack0 == src) {
		printf("replace %p by %p\n", src.get(), by.get());
		set_pack0(by);
	} else if (_pack1 == src) {
		printf("replace %p by %p\n", src.get(), by.get());
		set_pack1(by);
	} else {
		throw std::runtime_error("split: bad child replacement!");
	}

	update_allocation();
}

void split_t::set_split(double split) {
	if(split < 0.05)
		split = 0.05;
	if(split > 0.95)
		split = 0.95;
	_ratio = split;
	update_allocation();
}

void split_t::compute_children_allocation(double split, rect & bpack0, rect & bpack1) {

	int pack0_height = 20, pack0_width = 20;
	int pack1_height = 20, pack1_width = 20;

	if(_pack0 != nullptr)
		_pack0->get_min_allocation(pack0_width, pack0_height);
	if(_pack1 != nullptr)
		_pack1->get_min_allocation(pack1_width, pack1_height);

	//cout << "pack0 = " << pack0_width << "," << pack0_height << endl;
	//cout << "pack1 = " << pack1_width << "," << pack1_height << endl;

	if (_type == VERTICAL_SPLIT) {

		int w = allocation().w
				- 2 * _ctx->theme()->split.margin.left
				- 2 * _ctx->theme()->split.margin.right
				- _ctx->theme()->split.width;

		int w0 = floor(w * split + 0.5);
		int w1 = w - w0;


		if(w0 < pack0_width) {
			w1 -= pack0_width - w0;
			w0 = pack0_width;
		}

		if(w1 < pack1_width) {
			w0 -= pack1_width - w1;
			w1 = pack1_width;
		}

		bpack0.x = allocation().x + _ctx->theme()->split.margin.left;
		bpack0.y = allocation().y + _ctx->theme()->split.margin.top;
		bpack0.w = w0;
		bpack0.h = allocation().h - _ctx->theme()->split.margin.top - _ctx->theme()->split.margin.bottom;

		bpack1.x = allocation().x + _ctx->theme()->split.margin.left + w0 + _ctx->theme()->split.margin.right + _ctx->theme()->split.width + _ctx->theme()->split.margin.left;
		bpack1.y = allocation().y + _ctx->theme()->split.margin.top;
		bpack1.w = w1;
		bpack1.h = allocation().h - _ctx->theme()->split.margin.top - _ctx->theme()->split.margin.bottom;

		if(_parent != nullptr) {
			assert(bpack0.w >= pack0_width);
			assert(bpack0.h >= pack0_height);
			assert(bpack1.w >= pack1_width);
			assert(bpack1.h >= pack1_height);
		}

	} else {

		int h = allocation().h - 2 * _ctx->theme()->split.margin.top - 2 * _ctx->theme()->split.margin.bottom - _ctx->theme()->split.width;
		int h0 = floor(h * split + 0.5);
		int h1 = h - h0;

		if(h0 < pack0_height) {
			h1 -= pack0_height - h0;
			h0 = pack0_height;
		}

		if(h1 < pack1_height) {
			h0 -= pack1_height - h1;
			h1 = pack1_height;
		}

		bpack0.x = allocation().x + _ctx->theme()->split.margin.left;
		bpack0.y = allocation().y + _ctx->theme()->split.margin.top;
		bpack0.w = allocation().w - _ctx->theme()->split.margin.left - _ctx->theme()->split.margin.right;
		bpack0.h = h0;

		bpack1.x = allocation().x + _ctx->theme()->split.margin.left;
		bpack1.y = allocation().y + _ctx->theme()->split.margin.top + h0 + _ctx->theme()->split.margin.bottom + _ctx->theme()->split.width + _ctx->theme()->split.margin.top;
		bpack1.w = allocation().w - _ctx->theme()->split.margin.left - _ctx->theme()->split.margin.right;
		bpack1.h = h1;

		if(_parent != nullptr) {
			assert(bpack0.w >= pack0_width);
			assert(bpack0.h >= pack0_height);
			assert(bpack1.w >= pack1_width);
			assert(bpack1.h >= pack1_height);
		}

	}


}

void split_t::update_allocation() {
	//cout << "allocation = " << _allocation.to_string() << endl;
	compute_children_allocation(_ratio, _bpack0, _bpack1);
	//cout << "allocation pack0 = " << _bpack0.to_string() << endl;
	//cout << "allocation pack1 = " << _bpack1.to_string() << endl;
	_split_bar_area = compute_split_bar_location();
	if(_pack0 != nullptr)
		_pack0->set_allocation(_bpack0);
	if(_pack1 != nullptr)
		_pack1->set_allocation(_bpack1);

	if(_wid == XCB_WINDOW_NONE and get_parent_xid() != XCB_WINDOW_NONE) {
		uint32_t cursor;
		if(_type == VERTICAL_SPLIT) {
			cursor = _ctx->dpy()->xc_sb_h_double_arrow;
		} else {
			cursor = _ctx->dpy()->xc_sb_v_double_arrow;
		}
		_wid = _ctx->dpy()->create_input_only_window(get_parent_xid(), _split_bar_area, XCB_CW_CURSOR, &cursor);
		_ctx->dpy()->map(_wid);
	}

	_ctx->dpy()->move_resize(_wid, _split_bar_area);

}

void split_t::set_pack0(shared_ptr<page_component_t> x) {
	if(_pack0 != nullptr) {
		_children.remove(_pack0);
		_pack0->clear_parent();
	}
	_pack0 = x;
	if (_pack0 != nullptr) {
		_pack0->set_parent(this);
		_children.push_back(_pack0);
		update_allocation();
	}
}

void split_t::set_pack1(shared_ptr<page_component_t> x) {
	if(_pack1 != nullptr) {
		_children.remove(_pack1);
		_pack1->clear_parent();
	}
	_pack1 = x;
	if (_pack1 != nullptr) {
		_pack1->set_parent(this);
		_children.push_back(_pack1);
		update_allocation();
	}
}

void split_t::render_legacy(cairo_t * cr) const {
	theme_split_t ts;
	ts.split = _ratio;
	ts.type = _type;
	ts.allocation = compute_split_bar_location();
	ts.root_x = get_window_position().x;
	ts.root_y = get_window_position().y;
	ts.has_mouse_over = _has_mouse_over;
	_ctx->theme()->render_split(cr, &ts);
}

void split_t::activate() {
	if(_parent != nullptr) {
		_parent->activate(shared_from_this());
	}
}

void split_t::activate(shared_ptr<tree_t> t) {
	assert(t != nullptr);
	assert(has_key(_children, t));
	activate();
	move_back(_children, t);
}

void split_t::remove(shared_ptr<tree_t> t) {
	if (_pack0 == t) {
		set_pack0(nullptr);
	} else if (_pack1 == t) {
		set_pack1(nullptr);
	}
}

void split_t::update_layout(time64_t const time) {

}

rect split_t::compute_split_bar_location(rect const & bpack0, rect const & bpack1) const {
	rect ret;
	if (_type == VERTICAL_SPLIT) {
		ret.x = allocation().x + _ctx->theme()->split.margin.left + bpack0.w ;
		ret.y = allocation().y;
		ret.w = _ctx->theme()->split.width + _ctx->theme()->split.margin.left + _ctx->theme()->split.margin.right;
		ret.h = allocation().h;
	} else {
		ret.x = allocation().x;
		ret.y = allocation().y + _ctx->theme()->split.margin.top + bpack0.h ;
		ret.w = allocation().w;
		ret.h = _ctx->theme()->split.width + _ctx->theme()->split.margin.top + _ctx->theme()->split.margin.bottom;
	}
	return ret;
}

rect split_t::compute_split_bar_location() const {
	return compute_split_bar_location(_bpack0, _bpack1);
}


void split_t::append_children(vector<shared_ptr<tree_t>> & out) const {
	out.insert(out.end(), _children.begin(), _children.end());
}

bool split_t::button_press(xcb_button_press_event_t const * e) {
	if (e->event == get_parent_xid()
			and e->child == _wid
			and e->detail == XCB_BUTTON_INDEX_1
			and _split_bar_area.is_inside(e->event_x, e->event_y)) {
		_ctx->grab_start(new grab_split_t { _ctx, shared_from_this() });
		return true;
	} else {
		return false;
	}
}

shared_ptr<split_t> split_t::shared_from_this() {
	return dynamic_pointer_cast<split_t>(tree_t::shared_from_this());
}

auto split_t::get_node_name() const -> string {
	return _get_node_name<'S'>();
}

rect split_t::allocation() const {
	return _allocation;
}

auto split_t::get_visible_region() -> region {
	/** split do not render any thing **/
	return region{};
}


auto split_t::get_opaque_region() -> region {
	/** split do not render any thing **/
	return region{};
}

auto split_t::get_damaged() -> region {
	/** split do not render any thing **/
	return region{};
}

void split_t::render(cairo_t * cr, region const & area) {

}

void split_t::hide() {
	if(_pack0 != nullptr)
		_pack0->hide();
	if(_pack1 != nullptr)
		_pack1->hide();
	_is_visible = false;
}

void split_t::show() {
	_is_visible = true;
	if(_pack1 != nullptr)
		_pack1->show();
	if(_pack0 != nullptr)
		_pack0->show();
}

void split_t::get_min_allocation(int & width, int & height) const {
	int pack0_height = 20, pack0_width = 20;
	int pack1_height = 20, pack1_width = 20;

	if(_pack0 != nullptr)
		_pack0->get_min_allocation(pack0_width, pack0_height);
	if(_pack1 != nullptr)
		_pack1->get_min_allocation(pack1_width, pack1_height);

	if (_type == VERTICAL_SPLIT) {
		width = pack0_width + pack1_width + _ctx->theme()->split.width;
		height = std::max(pack0_height, pack1_height);
	} else {
		width = std::max(pack0_width, pack1_width);
		height = pack0_height + pack1_height + _ctx->theme()->split.width;
	}
}

double split_t::compute_split_constaint(double split) {
	rect bpack0;
	rect bpack1;
	compute_children_allocation(split, bpack0, bpack1);
	rect tmp = compute_split_bar_location(bpack0, bpack1);

	//cout << "constraint allocation pack0 = " << bpack0.to_string() << endl;
	//cout << "constraint allocation pack1 = " << bpack1.to_string() << endl;

	if(_type == VERTICAL_SPLIT) {
		return ((tmp.x + (tmp.w/2)) - allocation().x)/(double)allocation().w;
	} else {
		return ((tmp.y + (tmp.h/2)) - allocation().y)/(double)allocation().h;
	}
}

rect split_t::root_location() {
	return to_root_position(_allocation);
}

void split_t::compute_children_root_allocation(double split, rect & bpack0, rect & bpack1) {
	compute_children_allocation(split, bpack0, bpack1);
	bpack0 = to_root_position(bpack0);
	bpack1 = to_root_position(bpack1);
}

bool split_t::button_motion(xcb_motion_notify_event_t const * ev) {
	if(ev->event != get_parent_xid()) {
		if(_has_mouse_over) {
			_has_mouse_over = false;
			queue_redraw();
		}
		return false;
	}

	if(ev->child != _wid) {
		if(_has_mouse_over) {
			_has_mouse_over = false;
			queue_redraw();
		}
		return false;
	}

	if(_split_bar_area.is_inside(ev->event_x, ev->event_y)) {
		if(not _has_mouse_over) {
			_has_mouse_over = true;
			queue_redraw();
		}
	} else {
		if(_has_mouse_over) {
			_has_mouse_over = false;
			queue_redraw();
		}
	}

	return false;

}


bool split_t::leave(xcb_leave_notify_event_t const * ev) {
	if(ev->event == get_parent_xid()) {
		if(_has_mouse_over) {
			_has_mouse_over = false;
			queue_redraw();
		}
	}
	return false;
}


}
