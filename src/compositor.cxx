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

#define CHECK_CAIRO(x) do { \
	x;\
	/*print_cairo_status(cr, __FILE__, __LINE__);*/ \
} while(false)


static void _draw_crossed_box(cairo_t * cr, i_rect const & box, double r, double g,
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
	composite_back_buffer = None;

	_A = std::shared_ptr<atom_handler_t>(new atom_handler_t(_cnx->dpy()));

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

	_forecast_count = 0;
	_missed_forecast = 0;
	back_buffer_state = XdbeCopied;

}

compositor_t::~compositor_t() {

	pango_font_description_free(_fps_font_desc);

	g_object_unref(_fps_context);
	g_object_unref(_fps_font_map);

	if(composite_back_buffer != None)
		XdbeDeallocateBackBufferName(_cnx->dpy(), composite_back_buffer);

	release_composite_overlay();
};

void compositor_t::render() {
	page::time_t cur;
	cur.get_time();


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

	if(_damaged.empty())
		return;

	/**
	 * forecast next best buffer swapping
	 * If the buffer was damaged over 75% of the desktop, then
	 * We prefer no copy buffer swapping else copy buffer.
	 * maybe a lower threthold is better.
	 **/
	_forecast_count += 1;
	if((double)_damaged.area()/(double)_desktop_region.area() > 0.50) {
		if(back_buffer_state == XdbeCopied) {
			_missed_forecast += 1;
		}
		back_buffer_state = XdbeUndefined;
	} else {
		if(back_buffer_state == XdbeUndefined) {
			_missed_forecast += 1;
		}
		back_buffer_state = XdbeCopied;
	}

	/** if we did not copied the back buffer, repair all screen **/
	if(back_buffer_state == XdbeUndefined) {
		_damaged = _desktop_region;
	}

	if (_show_fps) {
		_damaged += region{_debug_x,_debug_y,_FPS_WINDOWS*2+200,100};
	}

	_need_render = false;

	_fps_top = (_fps_top + 1) % _FPS_WINDOWS;
	_fps_history[_fps_top] = cur;
	_damaged_area[_fps_top] = _damaged.area();

	cairo_surface_t * _back_buffer = cairo_xcb_surface_create(_cnx->xcb(), composite_back_buffer, _cnx->root_visual(), root_attributes.width,
			root_attributes.height);

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
			pango_printf(cr, _FPS_WINDOWS*2+20,15, "miss rate: %6.1f %%", ((double)_missed_forecast/(double)_forecast_count)*100.0);
			pango_printf(cr, _FPS_WINDOWS*2+20,30, "surface count: %d", surf_count);
			pango_printf(cr, _FPS_WINDOWS*2+20,45, "surface size: %d KB", surf_size/1024);

			cairo_restore(cr);

		}
	}

	_damaged.clear();

	CHECK_CAIRO(cairo_surface_flush(_back_buffer));

	warn(cairo_get_reference_count(cr) == 1);
	cairo_destroy(cr);
	warn(cairo_surface_get_reference_count(_back_buffer) == 1);
	cairo_surface_destroy(_back_buffer);
	_back_buffer = nullptr;

	/** swap back buffer and front buffer **/
	XdbeSwapInfo si;
	si.swap_window = composite_overlay;
	si.swap_action = back_buffer_state;
	XdbeSwapBuffers(_cnx->dpy(), &si, 1);

}

//region compositor_t::read_damaged_region(xcb_damage_damage_t d) {
//
//	region result;
//
//	/* create an empty region */
//	xcb_xfixes_region_t region = xcb_generate_id(_cnx->xcb());
//	xcb_xfixes_create_region(_cnx->xcb(), region, 0, 0);
//
//	/* get damaged region and remove them from damaged status */
//	xcb_damage_subtract(_cnx->xcb(), d, XCB_XFIXES_REGION_NONE, region);
//
//
//	/* get all i_rects for the damaged region */
//
//	xcb_xfixes_fetch_region_cookie_t ck = xcb_xfixes_fetch_region(_cnx->xcb(), region);
//
//	xcb_generic_error_t * err;
//	xcb_xfixes_fetch_region_reply_t * r = xcb_xfixes_fetch_region_reply(_cnx->xcb(), ck, &err);
//
//	if (err == nullptr and r != nullptr) {
//		xcb_rectangle_t * rect = xcb_xfixes_fetch_region_rectangles(r);
//		for (unsigned k = 0; k < r->length; ++k) {
//			//printf("rect %dx%d+%d+%d\n", rects[i].width, rects[i].height, rects[i].x, rects[i].y);
//			result += i_rect(rect[k]);
//		}
//		//free(rect);
//		free(r);
//	}
//
//	xcb_xfixes_destroy_region(_cnx->xcb(), region);
//
//	return result;
//}

//void compositor_t::process_event(xcb_generic_event_t const * e) {
//
////	if (e.xany.type > 0 && e.xany.type < LASTEvent) {
////		printf("#%lu type: %s, send_event: %u, window: %lu\n", e.xany.serial,
////				x_event_name[e.xany.type], e.xany.send_event, e.xany.window);
////	} else {
////		printf("#%lu type: %u, send_event: %u, window: %lu\n", e.xany.serial,
////				e.xany.type, e.xany.send_event, e.xany.window);
////	}
//
//
//	if (e->response_type == XCB_CIRCULATE_NOTIFY) {
//		//process_event(e.xcirculate);
//	} else if (e->response_type == XCB_CONFIGURE_NOTIFY) {
//		//process_event(e.xconfigure);
//	} else if (e->response_type == XCB_CREATE_NOTIFY) {
//		//process_event(e.xcreatewindow);
//	} else if (e->response_type == XCB_DESTROY_NOTIFY) {
//		//process_event(reinterpret_cast<xcb_destroy_notify_event_t const *>(e));
//	} else if (e->response_type == XCB_MAP_NOTIFY) {
//		//process_event(reinterpret_cast<xcb_map_notify_event_t const *>(e));
//	} else if (e->response_type == XCB_REPARENT_NOTIFY) {
//		//process_event(e.xreparent);
//	} else if (e->response_type == XCB_UNMAP_NOTIFY) {
//		//process_event(e.xunmap);
//	}
//
//}

void compositor_t::update_layout() {

	XGetWindowAttributes(_cnx->dpy(), DefaultRootWindow(_cnx->dpy()), &root_attributes);

	if(composite_back_buffer != None)
		XdbeDeallocateBackBufferName(_cnx->dpy(), composite_back_buffer);

	_desktop_region.clear();

	XRRScreenResources * resources = XRRGetScreenResourcesCurrent(_cnx->dpy(),
			DefaultRootWindow(_cnx->dpy()));

	for (int i = 0; i < resources->ncrtc; ++i) {
		XRRCrtcInfo * info = XRRGetCrtcInfo(_cnx->dpy(), resources,
				resources->crtcs[i]);

		/** if the CRTC has at less one output bound **/
		if(info->noutput > 0) {
			i_rect area(info->x, info->y, info->width, info->height);
			_desktop_region = _desktop_region + area;

			if(i == 0) {
				_debug_x = info->x + 40;
				_debug_y = info->y + info->height - 120;
			}

		}
		XRRFreeCrtcInfo(info);
	}
	XRRFreeScreenResources(resources);

	printf("layout = %s\n", _desktop_region.to_string().c_str());

	composite_back_buffer = XdbeAllocateBackBufferName(_cnx->dpy(), composite_overlay, XdbeCopied);

}


//void compositor_t::process_events() {
//	XEvent ev;
//	while(XPending(_cnx->dpy())) {
//		XNextEvent(_cnx->dpy(), &ev);
//		process_event(ev);
//	}
//}

/**
 * Check event without blocking.
 **/
//bool compositor_t::process_check_event() {
//	XEvent e;
//
//	int x;
//	/** Passive check of events in queue, never flush any thing **/
//	if ((x = XEventsQueued(_cnx->dpy(), QueuedAlready)) > 0) {
//		/** should not block or flush **/
//		XNextEvent(_cnx->dpy(), &e);
//		process_event(e);
//		return true;
//	} else {
//		return false;
//	}
//}

xcb_window_t compositor_t::get_composite_overlay() {
	return composite_overlay;
}

void compositor_t::renderable_clear() {
	_graph_scene.clear();
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

}

