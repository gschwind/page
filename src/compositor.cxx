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
	/** Automatically redirect windows, but paint sub-windows manually */
	xcb_composite_redirect_subwindows(_cnx->xcb(), _cnx->root(), XCB_COMPOSITE_REDIRECT_MANUAL);
}

void compositor_t::release_composite_overlay() {
	xcb_composite_unredirect_subwindows(_cnx->xcb(), _cnx->root(), XCB_COMPOSITE_REDIRECT_MANUAL);
	_cnx->disable_input_passthrough(composite_overlay);
	xcb_composite_release_overlay_window(_cnx->xcb(), composite_overlay);
	composite_overlay = XCB_NONE;
}

compositor_t::compositor_t(display_t * cnx, composite_surface_manager_t * cmgr) : _cnx(cnx), _cmgr(cmgr) {
	composite_back_buffer = XCB_NONE;

	_A = std::shared_ptr<atom_handler_t>(new atom_handler_t(_cnx->xcb()));

	/* initialize composite */
	init_composite_overlay();

	_fps_top = 0;
	_show_fps = false;
	_show_damaged = false;
	_show_opac = false;
	_debug_x = 0;
	_debug_y = 0;

#ifdef WITH_PANGO
	_fps_font_desc = pango_font_description_from_string("Ubuntu Mono Bold 11");
	_fps_font_map = pango_cairo_font_map_new();
	_fps_context = pango_font_map_create_context(_fps_font_map);
#endif

	/* this will scan for all windows */
	update_layout();

}

compositor_t::~compositor_t() {

	pango_font_description_free(_fps_font_desc);

	g_object_unref(_fps_context);
	g_object_unref(_fps_font_map);

	if(composite_back_buffer != XCB_NONE) {
		xcb_free_pixmap(_cnx->xcb(), composite_back_buffer);
	}

	release_composite_overlay();
};

void compositor_t::render(tree_t * t) {

	auto _graph_scene = t->get_all_children();

	/**
	 * remove masked damaged.
	 * i.e. if a damage occur on window partially or fully recovered by
	 * another window, we ignore this damaged region.
	 **/

	/** check if we have at less 2 object, otherwise we cannot have overlap **/
	if (_graph_scene.size() >= 2) {
		std::vector<region> r_damaged;
		std::vector<region> r_opac;
		for (auto &i : _graph_scene) {
			r_damaged.push_back(i->get_damaged());
			r_opac.push_back(i->get_opaque_region());
		}

		/** mask_area accumulate opac area, from top level object, to bottom **/
		region mask_area { };
		for (int k = r_opac.size() - 1; k >= 0; --k) {
			r_damaged[k] -= mask_area;
			mask_area += r_opac[k];
		}

		for (auto &i : r_damaged) {
			_damaged += i;
		}
	} else {
		for (auto &i : _graph_scene) {
			_damaged += i->get_damaged();
		}
	}

	_damaged &= _desktop_region;

	/** no damage at all => no repair to do, return **/
	if(_damaged.empty())
		return;

	if (_show_fps) {
		_damaged += region{_debug_x,_debug_y,_FPS_WINDOWS*2+200,100};
	}

	_need_render = false;

	time64_t cur = time64_t::now();
	_fps_top = (_fps_top + 1) % _FPS_WINDOWS;
	_fps_history[_fps_top] = cur;
	_damaged_area[_fps_top] = _damaged.area();


	cairo_surface_t * _back_buffer = cairo_xcb_surface_create(_cnx->xcb(), composite_back_buffer, _cnx->root_visual(), width, height);

	cairo_t * cr = cairo_create(_back_buffer);

	/** handler 1 pass area **/
	region _direct_render;

	for(auto & i: _graph_scene) {
		_direct_render -= i->get_visible_region();
		_direct_render += i->get_opaque_region();
	}

	_direct_render &= _damaged;

	_direct_render_area[_fps_top] = _direct_render.area();

	region _composited_area = _damaged - _direct_render;

	/** render all composited area **/
	for (auto & dmg : _composited_area) {
		for (auto &i : _graph_scene) {
			i->render(cr, dmg);
		}
	}

	/** render 1 pass area **/
	for(auto i = _graph_scene.rbegin(); i != _graph_scene.rend(); ++i) {
		region x = (*i)->get_opaque_region() & _direct_render;
		for (auto & dmg : x) {
				(*i)->render(cr, dmg);
				if(_show_opac)
					_draw_crossed_box(cr, dmg, 0.0, 1.0, 0.0);
		}

		_direct_render -= x;
	}

	if (_show_damaged) {
		for (auto &i : _damaged) {
			_draw_crossed_box(cr, i, 1.0, 0.0, 0.0);
		}
	}

	if (_show_fps) {
		int _fps_head = (_fps_top + 1) % _FPS_WINDOWS;
		if (static_cast<int64_t>(_fps_history[_fps_head]) != 0L) {


			double fps = (_FPS_WINDOWS * 1000000000.0)
					/ (static_cast<int64_t>(_fps_history[_fps_top])
							- static_cast<int64_t>(_fps_history[_fps_head]));

			cairo_save(cr);
			cairo_identity_matrix(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
			cairo_translate(cr, _debug_x, _debug_y);

			cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 0.5);
			cairo_rectangle(cr, 0.0, 0.0, _FPS_WINDOWS*2.0+200, 100.0);
			cairo_fill(cr);

			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_rectangle(cr, 0.0, 0.0, _FPS_WINDOWS*2.0, 100.0);
			cairo_stroke(cr);

			cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

			cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
			cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
			cairo_new_path(cr);

			double ref = _desktop_region.area();
			double xdmg = _damaged_area[_fps_top];
			cairo_move_to(cr, 0 * 5.0, 100.0 - std::min((xdmg/ref)*100.0, 100.0));

			for(int i = 1; i < _FPS_WINDOWS; ++i) {
				int frm = (_fps_top + i) % _FPS_WINDOWS;
				double xdmg = _damaged_area[frm];
				cairo_line_to(cr, i * 2.0, 100.0 - std::min((xdmg/ref)*100.0, 100.0));
			}
			cairo_stroke(cr);

			cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
			xdmg = _direct_render_area[_fps_top];
			cairo_move_to(cr, 0 * 5.0, 100.0 - std::min((xdmg/ref)*100.0, 100.0));

			for(int i = 1; i < _FPS_WINDOWS; ++i) {
				int frm = (_fps_top + i) % _FPS_WINDOWS;
				double xdmg = _direct_render_area[frm];
				cairo_line_to(cr, i * 2.0, 100.0 - std::min((xdmg/ref)*100.0, 100.0));
			}
			cairo_stroke(cr);

			int surf_count;
			int surf_size;

			_cmgr->make_surface_stats(surf_size, surf_count);

			pango_printf(cr, _FPS_WINDOWS*2+20,0,  "fps:       %6.1f", fps);
			pango_printf(cr, _FPS_WINDOWS*2+20,30, "surface count: %d", surf_count);
			pango_printf(cr, _FPS_WINDOWS*2+20,45, "surface size: %d KB", surf_size/1024);

			cairo_restore(cr);

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

	std::map<xcb_randr_crtc_t, xcb_randr_get_crtc_info_reply_t *> crtc_info;

	std::vector<xcb_randr_get_crtc_info_cookie_t> ckx(xcb_randr_get_screen_resources_crtcs_length(randr_resources));
	xcb_randr_crtc_t * crtc_list = xcb_randr_get_screen_resources_crtcs(randr_resources);
	for (unsigned k = 0; k < xcb_randr_get_screen_resources_crtcs_length(randr_resources); ++k) {
		ckx[k] = xcb_randr_get_crtc_info(_cnx->xcb(), crtc_list[k], XCB_CURRENT_TIME);
	}

	for (unsigned k = 0; k < xcb_randr_get_screen_resources_crtcs_length(randr_resources); ++k) {
		xcb_randr_get_crtc_info_reply_t * r = xcb_randr_get_crtc_info_reply(_cnx->xcb(), ckx[k], 0);
		if(r != nullptr) {
			crtc_info[crtc_list[k]] = r;
		}

		if(k == 0) {
			_debug_x = r->x + 40;
			_debug_y = r->y + r->height - 120;
		}

	}

	_desktop_region.clear();

	for(auto i: crtc_info) {
		rect area{i.second->x, i.second->y, i.second->width, i.second->height};
		_desktop_region += area;
	}

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

}

