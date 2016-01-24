/*
 * viewport.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <algorithm>
#include <typeinfo>
#include "notebook.hxx"
#include "viewport.hxx"

namespace page {

using namespace std;

viewport_t::viewport_t(page_context_t * ctx, rect const & area) :
		_ctx{ctx},
		_raw_aera{area},
		_effective_area{area},
		_is_durty{true},
		_win{XCB_NONE},
		_back_surf{nullptr},
		_exposed{false},
		_subtree{nullptr}
{
	_page_area = rect{0, 0, _effective_area.w, _effective_area.h};
	create_window();

	_subtree = make_shared<notebook_t>(_ctx);
	_subtree->set_parent(this);
	_subtree->set_allocation(_page_area);

}

viewport_t::~viewport_t() {
	destroy_renderable();
	xcb_destroy_window(_ctx->dpy()->xcb(), _win);
	_win = XCB_NONE;
}

void viewport_t::replace(shared_ptr<page_component_t> src, shared_ptr<page_component_t> by) {
	//printf("replace %p by %p\n", src, by);

	if (_subtree == src) {
		_subtree->clear_parent();
		_subtree = by;
		_subtree->set_parent(this);
		_subtree->set_allocation(_page_area);
	} else {
		throw std::runtime_error("viewport: bad child replacement!");
	}
}

void viewport_t::remove(shared_ptr<tree_t> src) {
	if(src == _subtree) {
		_subtree->clear_parent();
		_subtree.reset();
	}
}

void viewport_t::set_allocation(rect const & area) {
	_effective_area = area;
	_page_area = rect{0, 0, _effective_area.w, _effective_area.h};
	if(_subtree != nullptr)
		_subtree->set_allocation(_page_area);
	update_renderable();
}

void viewport_t::set_raw_area(rect const & area) {
	_raw_aera = area;
}

rect const & viewport_t::raw_area() const {
	return _raw_aera;
}

void viewport_t::activate() {
	if(_parent != nullptr) {
		_parent->activate(shared_from_this());
	}

	queue_redraw();
}


void viewport_t::activate(shared_ptr<tree_t> t) {
	assert(t != nullptr);
	assert(t == _subtree);
	activate();
}

string viewport_t::get_node_name() const {
	return _get_node_name<'V'>();
}

void viewport_t::update_layout(time64_t const time) {

}

void viewport_t::render_finished() {
	_damaged.clear();
}

rect viewport_t::allocation() const {
	return _effective_area;
}

rect const & viewport_t::page_area() const {
	return _page_area;
}

void viewport_t::append_children(vector<shared_ptr<tree_t>> & out) const {
	if(_subtree != nullptr) {
		out.push_back(_subtree);
	}
}

void viewport_t::hide() {
	if(_subtree != nullptr) {
		_subtree->hide();
	}

	_is_visible = false;
	_ctx->dpy()->unmap(_win);
	destroy_renderable();
}

void viewport_t::show() {
	_is_visible = true;
	_ctx->dpy()->map(_win);
	update_renderable();
	if(_subtree != nullptr) {
		_subtree->show();
	}
}

void viewport_t::destroy_renderable() {
	_back_surf.reset();
}

void viewport_t::update_renderable() {
	if(_ctx->cmp() != nullptr) {
		_back_surf = make_shared<pixmap_t>(_ctx->dpy(), PIXMAP_RGB, _page_area.w, _page_area.h);
	}
	_ctx->dpy()->move_resize(_win, _effective_area);
}

void viewport_t::create_window() {
	_win = xcb_generate_id(_ctx->dpy()->xcb());

	xcb_visualid_t visual = _ctx->dpy()->root_visual()->visual_id;
	int depth = _ctx->dpy()->root_depth();

	/** if visual is 32 bits, this values are mandatory **/
	xcb_colormap_t cmap = xcb_generate_id(_ctx->dpy()->xcb());
	xcb_create_colormap(_ctx->dpy()->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, _ctx->dpy()->root(), visual);

	uint32_t value_mask = 0;
	uint32_t value[5];

	value_mask |= XCB_CW_BACK_PIXEL;
	value[0] = _ctx->dpy()->xcb_screen()->black_pixel;

	value_mask |= XCB_CW_BORDER_PIXEL;
	value[1] = _ctx->dpy()->xcb_screen()->black_pixel;

	value_mask |= XCB_CW_OVERRIDE_REDIRECT;
	value[2] = True;

	value_mask |= XCB_CW_EVENT_MASK;
	value[3] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_LEAVE_WINDOW;

	value_mask |= XCB_CW_COLORMAP;
	value[4] = cmap;

	_win = xcb_generate_id(_ctx->dpy()->xcb());
	xcb_create_window(_ctx->dpy()->xcb(), depth, _win, _ctx->dpy()->root(), _effective_area.x, _effective_area.y, _effective_area.w, _effective_area.h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, visual, value_mask, value);

	_ctx->dpy()->set_window_cursor(_win, _ctx->dpy()->xc_left_ptr);

	/**
	 * This grab will freeze input for all client, all mouse button, until
	 * we choose what to do with them with XAllowEvents. we can choose to keep
	 * grabbing events or release event and allow further processing by other clients.
	 **/
	xcb_grab_button(_ctx->dpy()->xcb(), false, _win,
			DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC,
			XCB_GRAB_MODE_ASYNC,
			XCB_NONE,
			XCB_NONE,
			XCB_BUTTON_INDEX_ANY,
			XCB_MOD_MASK_ANY);

}

void viewport_t::_redraw_back_buffer() {
	if(_back_surf == nullptr)
		return;

	if(not _is_durty)
		return;

	cairo_t * cr = cairo_create(_back_surf->get_cairo_surface());
	cairo_identity_matrix(cr);

	auto splits = filter_class<split_t>(get_all_children());
	for (auto x : splits) {
		x->render_legacy(cr);
	}

	auto notebooks = filter_class<notebook_t>(get_all_children());
	for (auto x : notebooks) {
		x->render_legacy(cr);
	}

	cairo_surface_flush(_back_surf->get_cairo_surface());
	warn(cairo_get_reference_count(cr) == 1);
	cairo_destroy(cr);

	_is_durty = false;
	_exposed = true;
	_damaged += _effective_area;

}

void viewport_t::trigger_redraw() {
	/** redraw all child **/
	tree_t::trigger_redraw();
	_redraw_back_buffer();

	if(_exposed and _ctx->cmp() == nullptr) {
		_exposed = false;
		paint_expose();
	}

}

/* mark renderable_page for redraw */
void viewport_t::queue_redraw() {
	_is_durty = true;
}

region viewport_t::get_damaged() {
	return _damaged;
}

xcb_window_t viewport_t::get_parent_xid() const {
	return _win;
}

xcb_window_t viewport_t::get_xid() const {
	return _win;
}

void viewport_t::paint_expose() {
	if(not _is_visible)
		return;

	cairo_surface_t * surf = cairo_xcb_surface_create(_ctx->dpy()->xcb(), _win, _ctx->dpy()->root_visual(), _effective_area.w, _effective_area.h);
	cairo_t * cr = cairo_create(surf);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(cr, _back_surf->get_cairo_surface(), 0.0, 0.0);
	cairo_rectangle(cr, 0.0, 0.0, _effective_area.w, _effective_area.h);
	cairo_fill(cr);

	cairo_destroy(cr);
	cairo_surface_destroy(surf);
}

rect viewport_t::get_window_position() const {
	return _effective_area;
}

void viewport_t::expose(xcb_expose_event_t const * e) {
	if(e->window == _win) {
		_exposed = true;
	}
}

auto viewport_t::get_visible_region() -> region {
	return region{_effective_area};
}

auto viewport_t::get_opaque_region() -> region {
	return region{_effective_area};
}

void viewport_t::render(cairo_t * cr, region const & area) {
	if(not _is_visible)
		return;
	if(_back_surf == nullptr)
		return;
	if(_back_surf->get_cairo_surface() == nullptr)
		return;

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(cr, _back_surf->get_cairo_surface(),
			_effective_area.x, _effective_area.y);
	region r = region{_effective_area} & area;
	for (auto &i : r.rects()) {
		cairo_clip(cr, i);
		cairo_mask_surface(cr, _back_surf->get_cairo_surface(), _effective_area.x, _effective_area.y);
	}
	cairo_restore(cr);
}

void viewport_t::get_min_allocation(int & width, int & height) const {
	width = 0;
	height = 0;

	if(_subtree != nullptr) {
		_subtree->get_min_allocation(width, height);
	}

}

}
