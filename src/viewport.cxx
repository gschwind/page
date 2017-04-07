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

#include "page.hxx"
#include "notebook.hxx"
#include "viewport.hxx"
#include "workspace.hxx"

namespace page {

using namespace std;

viewport_t::viewport_t(tree_t * ref, rect const & area) :
		page_component_t{ref},
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
	auto n = make_shared<notebook_t>(this);
	_subtree = static_pointer_cast<page_component_t>(n);
	push_back(_subtree);
	_subtree->set_allocation(_page_area);
}

viewport_t::~viewport_t() {
	destroy_renderable();
	xcb_destroy_window(_root->_ctx->dpy()->xcb(), _win);
	_root->_ctx->_page_windows.erase(_win);

	_win = XCB_NONE;
}

void viewport_t::replace(shared_ptr<page_component_t> src, shared_ptr<page_component_t> by) {
	//printf("replace %p by %p\n", src, by);

	assert(has_key(_children, src));

	if (_subtree == src) {
		remove(_subtree);
		_subtree = by;
		push_back(_subtree);
		_subtree->set_allocation(_page_area);

		if(_is_visible)
			_subtree->show();
		else
			_subtree->hide();

	} else {
		throw std::runtime_error("viewport: bad child replacement!");
	}
}

void viewport_t::remove(shared_ptr<tree_t> src) {
	assert(has_key(_children, src));
	tree_t::remove(_subtree);
	_subtree.reset();
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

string viewport_t::get_node_name() const {
	return _get_node_name<'V'>();
}

void viewport_t::update_layout(time64_t const time) {

}

void viewport_t::render_finished() {
	_damaged.clear();
}

void viewport_t::reconfigure()
{
	if(not _root->is_enable())
		return;

	auto _ctx = _root->_ctx;
	auto _dpy = _root->_ctx->dpy();

	if (_is_visible) {
		_dpy->map(_win);
		update_renderable();
	} else {
		_dpy->unmap(_win);
		destroy_renderable();
	}

}

void viewport_t::on_workspace_enable()
{
	auto _ctx = _root->_ctx;
	auto _dpy = _root->_ctx->dpy();
	reconfigure();
}

void viewport_t::on_workspace_disable()
{
	auto _ctx = _root->_ctx;
	auto _dpy = _root->_ctx->dpy();

	_dpy->unmap(_win);
	destroy_renderable();

}

rect viewport_t::allocation() const {
	return _effective_area;
}

rect const & viewport_t::page_area() const {
	return _page_area;
}

void viewport_t::hide() {
	tree_t::hide();
	reconfigure();
}

void viewport_t::show() {
	tree_t::show();
	reconfigure();
}

void viewport_t::destroy_renderable() {
	_back_surf.reset();
}

void viewport_t::update_renderable() {
	if(_root->_ctx->cmp() != nullptr and _back_surf == nullptr) {
		_back_surf = make_shared<pixmap_t>(_root->_ctx->dpy(), PIXMAP_RGB, _page_area.w, _page_area.h);
		_is_durty = true;
	}
	_root->_ctx->dpy()->move_resize(_win, _effective_area);
}

void viewport_t::create_window() {
	_win = xcb_generate_id(_root->_ctx->dpy()->xcb());

	xcb_visualid_t visual = _root->_ctx->dpy()->root_visual()->visual_id;
	int depth = _root->_ctx->dpy()->root_depth();

	/** if visual is 32 bits, this values are mandatory **/
	xcb_colormap_t cmap = xcb_generate_id(_root->_ctx->dpy()->xcb());
	xcb_create_colormap(_root->_ctx->dpy()->xcb(), XCB_COLORMAP_ALLOC_NONE, cmap, _root->_ctx->dpy()->root(), visual);

	uint32_t value_mask = 0;
	uint32_t value[5];

	value_mask |= XCB_CW_BACK_PIXEL;
	value[0] = _root->_ctx->dpy()->xcb_screen()->black_pixel;

	value_mask |= XCB_CW_BORDER_PIXEL;
	value[1] = _root->_ctx->dpy()->xcb_screen()->black_pixel;

	value_mask |= XCB_CW_OVERRIDE_REDIRECT;
	value[2] = True;

	value_mask |= XCB_CW_EVENT_MASK;
	value[3] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_LEAVE_WINDOW;

	value_mask |= XCB_CW_COLORMAP;
	value[4] = cmap;

	_win = xcb_generate_id(_root->_ctx->dpy()->xcb());
	xcb_create_window(_root->_ctx->dpy()->xcb(), depth, _win, _root->_ctx->dpy()->root(), _effective_area.x, _effective_area.y, _effective_area.w, _effective_area.h, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, visual, value_mask, value);
	_root->_ctx->_page_windows.insert(_win);

	_root->_ctx->dpy()->set_window_cursor(_win, _root->_ctx->dpy()->xc_left_ptr);

	/**
	 * This grab will freeze input for all client, all mouse button, until
	 * we choose what to do with them with XAllowEvents. we can choose to keep
	 * grabbing events or release event and allow further processing by other clients.
	 **/
	xcb_grab_button(_root->_ctx->dpy()->xcb(), false, _win,
			DEFAULT_BUTTON_EVENT_MASK,
			XCB_GRAB_MODE_SYNC, // synchronous pointer grab will freeze the cursor until action is made.
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

	auto splits = gather_children_root_first<split_t>();
	for (auto x : splits) {
		x->render_legacy(cr);
	}

	auto notebooks = gather_children_root_first<notebook_t>();
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

	if(_exposed and _root->_ctx->cmp() == nullptr) {
		_exposed = false;
		paint_expose();
	}

}

/* mark renderable_page for redraw */
void viewport_t::queue_redraw() {
	_root->_ctx->schedule_repaint();
	_is_durty = true;
}

region viewport_t::get_damaged() {
	return _damaged;
}

xcb_window_t viewport_t::get_toplevel_xid() const {
	return _win;
}

void viewport_t::paint_expose() {
	if(not _is_visible)
		return;

	cairo_surface_t * surf = cairo_xcb_surface_create(_root->_ctx->dpy()->xcb(), _win, _root->_ctx->dpy()->root_visual(), _effective_area.w, _effective_area.h);
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
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_surface(cr, _back_surf->get_cairo_surface(),
			_effective_area.x, _effective_area.y);
	region r = region{_effective_area} & area;
	for (auto &i : r.rects()) {
		cairo_clip(cr, i);
		cairo_paint(cr);
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
