/*
 * render_context.cxx
 *
 * copyright (2010-2014) Benoit Gschwind
 *
 * This code is licensed under the GPLv3. see COPYING file for more details.
 *
 */

#include <unistd.h>
#include <algorithm>
#include <list>
#include <memory>

#include <cstdlib>

#include "composite_surface_manager.hxx"

#include "utils.hxx"
#include "compositor.hxx"
#include "atoms.hxx"

#include "display.hxx"

namespace page {

using namespace std;

#define CHECK_CAIRO(x) do { \
	x;\
	/*print_cairo_status(cr, __FILE__, __LINE__);*/ \
} while(false)


static void _draw_crossed_box(cairo_t * cr, rect const & box, double r, double g,
		double b) {

	cairo_save(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
	cairo_identity_matrix(cr);

	cairo_set_source_rgb(cr, r, g, b);
	cairo_set_line_width(cr, 1.0);
	cairo_rectangle(cr, box.x + 0.5, box.y + 0.5, box.w - 1.0, box.h - 1.0);
	cairo_reset_clip(cr);
	cairo_stroke(cr);

	cairo_new_path(cr);
	cairo_move_to(cr, box.x + 0.5, box.y + 0.5);
	cairo_line_to(cr, box.x + box.w - 1.0, box.y + box.h - 1.0);

	cairo_move_to(cr, box.x + box.w - 1.0, box.y + 0.5);
	cairo_line_to(cr, box.x + 0.5, box.y + box.h - 1.0);
	cairo_stroke(cr);

	cairo_restore(cr);
}

void compositor_t::init_composite_overlay() {
	/* create and map the composite overlay window */
	xcb_composite_get_overlay_window_cookie_t ck = xcb_composite_get_overlay_window(_cnx->xcb(), _cnx->root());
	xcb_composite_get_overlay_window_reply_t * r = xcb_composite_get_overlay_window_reply(_cnx->xcb(), ck, 0);
	if(r == nullptr) {
		throw exception_t("cannot create compositor window overlay");
	}

	composite_overlay = r->overlay_win;

	/* user input pass through composite overlay (mouse click for example)) */
	_cnx->allow_input_passthrough(composite_overlay);

	// DISABLE auto redirection.
	/** Automatically redirect windows, but paint sub-windows manually */
	// xcb_composite_redirect_subwindows(_cnx->xcb(), _cnx->root(), XCB_COMPOSITE_REDIRECT_MANUAL);
}

void compositor_t::release_composite_overlay() {
	// DISABLE auto redirection.
	//xcb_composite_unredirect_subwindows(_cnx->xcb(), _cnx->root(), XCB_COMPOSITE_REDIRECT_MANUAL);
	_cnx->disable_input_passthrough(composite_overlay);
	xcb_composite_release_overlay_window(_cnx->xcb(), composite_overlay);
	composite_overlay = XCB_NONE;
}

compositor_t::compositor_t(display_t * cnx, composite_surface_manager_t * cmgr) :
		_cnx(cnx),
		_cmgr(cmgr)
		{
	composite_back_buffer = XCB_NONE;

	_A = std::shared_ptr<atom_handler_t>(new atom_handler_t(_cnx->xcb()));

	/* initialize composite */
	init_composite_overlay();

	_show_damaged = false;
	_show_opac = false;

#ifdef WITH_PANGO
	_fps_font_desc = pango_font_description_from_string("Ubuntu Mono Bold 11");
	_fps_font_map = pango_cairo_font_map_new();
	_fps_context = pango_font_map_create_context(_fps_font_map);
#endif

	/* this will scan for all windows */
	update_layout();

}

compositor_t::~compositor_t() {

#ifdef WITH_PANGO
	pango_font_description_free(_fps_font_desc);
	g_object_unref(_fps_context);
	g_object_unref(_fps_font_map);
#endif

	if(composite_back_buffer != XCB_NONE) {
		xcb_free_pixmap(_cnx->xcb(), composite_back_buffer);
	}

	release_composite_overlay();
};

void compositor_t::render(tree_t * t) {

	auto _graph_scene = t->get_all_children_root_first();

	/** remove invisible elements **/
	{
		auto end = std::remove_if(_graph_scene.begin(), _graph_scene.end(), [](shared_ptr<tree_t> const & x) { return not x->is_visible(); });
		_graph_scene.resize(std::distance(_graph_scene.begin(), end));
	}

	/** collect damaged area **/
	for (auto &i : _graph_scene) {
		_damaged += i->get_damaged();
	}

	/** clip damage area to visible screen **/
	_damaged &= _desktop_region;

	/** no damage at all => no repair to do, return **/
	if(_damaged.empty())
		return;

	time64_t cur = time64_t::now();
	_fps_history.push_front(cur);
	if(_fps_history.size() > _FPS_WINDOWS) {
		_fps_history.pop_back();
	}

	_damaged_area.push_front(_damaged.area()/_desktop_region_area);
	if(_damaged_area.size() > _FPS_WINDOWS) {
		_damaged_area.pop_back();
	}

	cairo_surface_t * _back_buffer = cairo_xcb_surface_create(_cnx->xcb(), composite_back_buffer, _cnx->root_visual(), width, height);

	cairo_t * cr = cairo_create(_back_buffer);

	/** compute area where we have only direct rendering **/
	region _direct_render;
	for(auto & i: _graph_scene) {
		_direct_render -= i->get_visible_region();
		_direct_render += i->get_opaque_region();
	}

	_direct_render &= _damaged;

	_direct_render_area.push_front(_direct_render.area() / _desktop_region_area);
	if(_direct_render_area.size() > _FPS_WINDOWS) {
		_direct_render_area.pop_back();
	}

	region _composited_area = _damaged - _direct_render;

	/** pass 1 render all composited area **/
	for (auto & dmg : _composited_area) {
		for (auto &i : _graph_scene) {
			i->render(cr, dmg);
		}
	}

	/** pass 2 from to to bottom, render opaque area **/
	for (auto i = _graph_scene.rbegin(); i != _graph_scene.rend(); ++i) {
		region x = (*i)->get_opaque_region() & _direct_render;
		for (auto & dmg : x) {
			(*i)->render(cr, dmg);
			if (_show_opac)
				_draw_crossed_box(cr, dmg, 0.0, 1.0, 0.0);
		}
		_direct_render -= x;
	}

	if (_show_damaged) {
		for (auto &i : _damaged) {
			_draw_crossed_box(cr, i, 1.0, 0.0, 0.0);
		}
	}

	_damaged.clear();

	CHECK_CAIRO(cairo_surface_flush(_back_buffer));

	warn(cairo_get_reference_count(cr) == 1);
	cairo_destroy(cr);

	cairo_surface_t * front_buffer = get_front_surface();
	cr = cairo_create(front_buffer);
	cairo_set_source_surface(cr, _back_buffer, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
	cairo_surface_flush(front_buffer);
	cairo_surface_destroy(front_buffer);
	cairo_surface_destroy(_back_buffer);

}

void compositor_t::update_layout() {

	if(composite_back_buffer != XCB_NONE) {
		xcb_free_pixmap(_cnx->xcb(), composite_back_buffer);
		composite_back_buffer = XCB_NONE;
	}

	/** update root size infos **/

	xcb_get_geometry_cookie_t ck0 = xcb_get_geometry(_cnx->xcb(), _cnx->root());
	xcb_randr_get_screen_resources_cookie_t ck1 = xcb_randr_get_screen_resources(_cnx->xcb(), _cnx->root());

	xcb_get_geometry_reply_t * geometry = xcb_get_geometry_reply(_cnx->xcb(), ck0, nullptr);
	xcb_randr_get_screen_resources_reply_t * randr_resources = xcb_randr_get_screen_resources_reply(_cnx->xcb(), ck1, 0);

	if(geometry == nullptr or randr_resources == nullptr) {
		throw exception_t("FATAL: cannot read root window attributes");
	}

	map<xcb_randr_crtc_t, xcb_randr_get_crtc_info_reply_t *> crtc_info;

	vector<xcb_randr_get_crtc_info_cookie_t> ckx(xcb_randr_get_screen_resources_crtcs_length(randr_resources));
	xcb_randr_crtc_t * crtc_list = xcb_randr_get_screen_resources_crtcs(randr_resources);
	for (unsigned k = 0; k < xcb_randr_get_screen_resources_crtcs_length(randr_resources); ++k) {
		ckx[k] = xcb_randr_get_crtc_info(_cnx->xcb(), crtc_list[k], XCB_CURRENT_TIME);
	}

	for (unsigned k = 0; k < xcb_randr_get_screen_resources_crtcs_length(randr_resources); ++k) {
		xcb_randr_get_crtc_info_reply_t * r = xcb_randr_get_crtc_info_reply(_cnx->xcb(), ckx[k], 0);
		if(r != nullptr) {
			crtc_info[crtc_list[k]] = r;
		}
	}

	_desktop_region.clear();

	for(auto i: crtc_info) {
		rect area{i.second->x, i.second->y, i.second->width, i.second->height};
		_desktop_region += area;
	}

	_desktop_region_area = _desktop_region.area();

	printf("layout = %s\n", _desktop_region.to_string().c_str());

	_damaged += rect{geometry->x, geometry->y, geometry->width, geometry->height};

	composite_back_buffer = xcb_generate_id(_cnx->xcb());
	xcb_create_pixmap(_cnx->xcb(), _cnx->root_depth(), composite_back_buffer,
			composite_overlay, geometry->width, geometry->height);

	width = geometry->width;
	height = geometry->height;

	for(auto i: crtc_info) {
		if(i.second != nullptr)
			free(i.second);
	}

	if(geometry != nullptr) {
		free(geometry);
	}

	if(randr_resources != nullptr) {
		free(randr_resources);
	}

}

xcb_window_t compositor_t::get_composite_overlay() {
	return composite_overlay;
}

void compositor_t::pango_printf(cairo_t * cr, double x, double y,
		char const * fmt, ...) {

#ifdef WITH_PANGO

	va_list l;
	va_start(l, fmt);

	cairo_save(cr);
	cairo_move_to(cr, x, y);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

	static char buffer[4096];
	vsnprintf(buffer, 4096, fmt, l);

	PangoLayout * pango_layout = pango_layout_new(_fps_context);
	pango_layout_set_font_description(pango_layout, _fps_font_desc);
	pango_cairo_update_layout(cr, pango_layout);
	pango_layout_set_text(pango_layout, buffer, -1);
	pango_cairo_layout_path(cr, pango_layout);
	g_object_unref(pango_layout);

	cairo_set_line_width(cr, 3.0);
	cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

	cairo_stroke_preserve(cr);

	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_fill(cr);

	cairo_restore(cr);
#endif

}

cairo_surface_t * compositor_t::get_front_surface() const {
	return cairo_xcb_surface_create(_cnx->xcb(), composite_overlay,
			_cnx->root_visual(), width, height);
}

shared_ptr<pixmap_t> compositor_t::create_composite_pixmap(unsigned width, unsigned height) {
	xcb_pixmap_t pix = xcb_generate_id(_cnx->xcb());
	xcb_create_pixmap(_cnx->xcb(), _cnx->root_depth(), pix, _cnx->root(), width, height);
	return make_shared<pixmap_t>(_cnx, _cnx->root_visual(), pix, width, height);
}

shared_ptr<pixmap_t> compositor_t::create_screenshot() {
	xcb_pixmap_t pix = xcb_generate_id(_cnx->xcb());
	xcb_create_pixmap(_cnx->xcb(), _cnx->root_depth(), pix, _cnx->root(), width, height);
	auto screenshot = make_shared<pixmap_t>(_cnx, _cnx->root_visual(), pix, width, height);

	cairo_t * cr = cairo_create(screenshot->get_cairo_surface());
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_surface_t * _back_buffer = cairo_xcb_surface_create(_cnx->xcb(), composite_back_buffer, _cnx->root_visual(), width, height);
	cairo_set_source_surface(cr, _back_buffer, 0.0, 0.0);
	cairo_paint(cr);
	cairo_surface_destroy(_back_buffer);
	cairo_destroy(cr);
	return screenshot;
}

double compositor_t::get_fps() {
	int64_t diff = _fps_history.front() - _fps_history.back();
	if (diff > 0L) {
		return (_fps_history.size() * 1000000000.0) / diff;
	} else {
		return NAN;
	}
}

deque<double> const & compositor_t::get_direct_area_history() {
	return _direct_render_area;
}

deque<double> const & compositor_t::get_damaged_area_history() {
	return _damaged_area;
}



}

