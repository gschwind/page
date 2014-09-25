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

#include <cstdlib>

#include "compositor.hxx"
#include "atoms.hxx"

#include "display.hxx"

namespace page {

#define CHECK_CAIRO(x) do { \
	x;\
	print_cairo_status(cr, __FILE__, __LINE__); \
} while(false)


static void _draw_crossed_box(cairo_t * cr, rectangle const & box, double r, double g,
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
	composite_overlay = XCompositeGetOverlayWindow(_cnx->dpy(), _cnx->root());
	/* user input pass through composite overlay (mouse click for example)) */
	allow_input_passthrough(_cnx->dpy(), composite_overlay);
	/** Automatically redirect windows, but paint sub-windows manually */
	XCompositeRedirectSubwindows(_cnx->dpy(), _cnx->root(), CompositeRedirectManual);
}

void compositor_t::release_composite_overlay() {
	XCompositeUnredirectSubwindows(_cnx->dpy(), _cnx->root(), CompositeRedirectManual);
	disable_input_passthrough(_cnx->dpy(), composite_overlay);
	XCompositeReleaseOverlayWindow(_cnx->dpy(), composite_overlay);
	composite_overlay = None;
}

compositor_t::compositor_t(display_t * cnx, int damage_event, int xshape_event, int xrandr_event) : _cnx(cnx), damage_event(damage_event), xshape_event(xshape_event), xrandr_event(xrandr_event) {
	composite_back_buffer = None;

	_A = shared_ptr<atom_handler_t>(new atom_handler_t(_cnx->dpy()));

	/* initialize composite */
	init_composite_overlay();

	_fps_top = 0;
	_show_fps = false;

#ifdef WITH_PANGO
	_fps_font_desc = pango_font_description_from_string("Ubuntu 16");
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

	if(composite_back_buffer != None)
		XdbeDeallocateBackBufferName(_cnx->dpy(), composite_back_buffer);

	release_composite_overlay();
};

void compositor_t::render() {
	page::time_t cur;
	cur.get_time();

	/** check if some component need rendering **/
//	if (not _need_render) {
//		for (auto i : _graph_scene) {
//			if (i->need_render(cur)) {
//				_need_render = true;
//				break;
//			}
//		}
//	}

	for (auto &i : _graph_scene) {
		_damaged += i->get_damaged();
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
	if((double)_damaged.area()/(double)_desktop_region.area() > 0.75) {
		back_buffer_state = XdbeUndefined;
	} else {
		back_buffer_state = XdbeCopied;
	}

	/** if we did not copied the back buffer, repair all screen **/
	if(back_buffer_state == XdbeUndefined) {
		_damaged = _desktop_region;
	}

//	_graph_scene.clear();
//	for(auto x: _prepare_render) {
//		vector<ptr<renderable_t>> tmp = x->call(cur);
//		_graph_scene.insert(_graph_scene.end(), tmp.begin(), tmp.end());
//	}

//	printf("yyyyyy %lu\n", _graph_scene.size());

//	/** if no one need rendering, do not render. **/
//	if(not _need_render) {
//		return;
//	}

	_need_render = false;

	_fps_top = (_fps_top + 1) % _FPS_WINDOWS;
	_fps_history[_fps_top] = cur;
	_damaged_area[_fps_top] = _damaged.area();

	cairo_surface_t * _back_buffer = cairo_xlib_surface_create(_cnx->dpy(), composite_back_buffer,
			root_attributes.visual, root_attributes.width,
			root_attributes.height);

	cairo_t * cr = cairo_create(_back_buffer);

	/** clear back buffer **/
//	CHECK_CAIRO(cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE));
//	CHECK_CAIRO(cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0));
//	CHECK_CAIRO(cairo_paint(cr));

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
		}
		_direct_render -= x;
	}

	if (_show_fps) {

//		for (auto &i : _graph_scene) {
//			i->render(cr, region{0, 0, 640, 480});
//		}

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
			pango_printf(cr, 40.0, 40.0, "fps: %.1f", fps);

			cairo_save(cr);
			cairo_identity_matrix(cr);
			cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
			cairo_translate(cr, 40.0, 140.0);

			cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
			cairo_rectangle(cr, 0.0, 0.0, _FPS_WINDOWS*2.0, 200.0);
			cairo_fill(cr);

			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_rectangle(cr, 0.0, 0.0, _FPS_WINDOWS*2.0, 200.0);
			cairo_stroke(cr);


			cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
			cairo_new_path(cr);
			cairo_move_to(cr, 0.0, 100.0);
			cairo_line_to(cr, _FPS_WINDOWS*2.0, 100.0);
			cairo_stroke(cr);


			cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

			cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
			cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
			cairo_new_path(cr);

			double ref = _desktop_region.area();
			double xdmg = _damaged_area[_fps_top];
			cairo_move_to(cr, 0 * 5.0, 200.0 - std::min((xdmg/ref)*100.0, 200.0));

			for(int i = 1; i < _FPS_WINDOWS; ++i) {
				int frm = (_fps_top + i) % _FPS_WINDOWS;
				double xdmg = _damaged_area[frm];
				cairo_line_to(cr, i * 2.0, 200.0 - std::min((xdmg/ref)*100.0, 200.0));
			}
			cairo_stroke(cr);

			cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
			xdmg = _direct_render_area[_fps_top];
			cairo_move_to(cr, 0 * 5.0, 200.0 - std::min((xdmg/ref)*100.0, 200.0));

			for(int i = 1; i < _FPS_WINDOWS; ++i) {
				int frm = (_fps_top + i) % _FPS_WINDOWS;
				double xdmg = _direct_render_area[frm];
				cairo_line_to(cr, i * 2.0, 200.0 - std::min((xdmg/ref)*100.0, 200.0));
			}
			cairo_stroke(cr);
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

void compositor_t::process_event(XCreateWindowEvent const & e) {
	if(e.send_event == True)
		return;

	if (e.window == composite_overlay)
		return;
	if (e.parent != _cnx->root())
		return;
}

void compositor_t::process_event(XReparentEvent const & e) {

}

void compositor_t::process_event(XMapEvent const & e) {
	if (e.send_event == True)
		return;
	if (e.event != _cnx->root())
		return;

	/** don't make composite overlay visible **/
	if (e.window == composite_overlay)
		return;

	composite_surface_manager_t::onmap(_cnx->dpy(), e.window);
}

void compositor_t::process_event(XUnmapEvent const & e) {
	if (e.send_event == True)
		return;
}

void compositor_t::process_event(XDestroyWindowEvent const & e) {
	if (e.send_event == True)
		return;
	composite_surface_manager_t::ondestroy(e.display, e.window);
}

void compositor_t::process_event(XConfigureEvent const & e) {
	if (e.send_event == True)
		return;

	if (e.event == _cnx->root()
			and e.window != _cnx->root()) {
		composite_surface_manager_t::onresize(_cnx->dpy(), e.window, e.width, e.height);
	}
}


void compositor_t::process_event(XCirculateEvent const & e) {
	if(e.send_event == True)
		return;

	if(e.event != _cnx->root())
		return;

}

void compositor_t::process_event(XDamageNotifyEvent const & e) {

//	printf("Damage area %dx%d+%d+%d\n", e.area.width, e.area.height, e.area.x,
//			e.area.y);
//	printf("Damage geometry %dx%d+%d+%d\n", e.geometry.width, e.geometry.height,
//			e.geometry.x, e.geometry.y);

	/** drop damage data **/

	/* create an empty region */
//	XserverRegion region = XFixesCreateRegion(_cnx->dpy(), 0, 0);
//	if (!region)
//		throw std::runtime_error("could not create region");
//	XDamageSubtract(_cnx->dpy(), e.damage, None, region);
//	XFixesDestroyRegion(_cnx->dpy(), region);

	region r = read_damaged_region(e.damage);
	weak_ptr<composite_surface_t> wp = composite_surface_manager_t::get_weak_surface(e.display, e.drawable);
	if (not wp.expired()) {
		wp.lock()->add_damaged(r);
	}
	_need_render = true;

}

region compositor_t::read_damaged_region(Damage d) {

	region result;

	/* create an empty region */
	XserverRegion region = XFixesCreateRegion(_cnx->dpy(), 0, 0);

	if (!region)
		throw std::runtime_error("could not create region");

	/* get damaged region and remove them from damaged status */
	XDamageSubtract(_cnx->dpy(), d, None, region);

	XRectangle * rects;
	int nb_rects;

	/* get all rectangles for the damaged region */
	rects = XFixesFetchRegion(_cnx->dpy(), region, &nb_rects);

	if (rects) {
		for (int i = 0; i < nb_rects; ++i) {
			//printf("rect %dx%d+%d+%d\n", rects[i].width, rects[i].height, rects[i].x, rects[i].y);
			result += rectangle(rects[i]);
		}
		XFree(rects);
	}

	XFixesDestroyRegion(_cnx->dpy(), region);

	return result;
}

void compositor_t::process_event(XEvent const & e) {

//	if (e.xany.type > 0 && e.xany.type < LASTEvent) {
//		printf("#%lu type: %s, send_event: %u, window: %lu\n", e.xany.serial,
//				x_event_name[e.xany.type], e.xany.send_event, e.xany.window);
//	} else {
//		printf("#%lu type: %u, send_event: %u, window: %lu\n", e.xany.serial,
//				e.xany.type, e.xany.send_event, e.xany.window);
//	}


	if (e.type == CirculateNotify) {
		process_event(e.xcirculate);
	} else if (e.type == ConfigureNotify) {
		process_event(e.xconfigure);
	} else if (e.type == CreateNotify) {
		process_event(e.xcreatewindow);
	} else if (e.type == DestroyNotify) {
		process_event(e.xdestroywindow);
	} else if (e.type == MapNotify) {
		process_event(e.xmap);
	} else if (e.type == ReparentNotify) {
		process_event(e.xreparent);
	} else if (e.type == UnmapNotify) {
		process_event(e.xunmap);
	} else if (e.type == damage_event + XDamageNotify) {
		process_event(reinterpret_cast<XDamageNotifyEvent const &>(e));
	} else if (e.type == xshape_event + ShapeNotify) {
		XShapeEvent const & sev = reinterpret_cast<XShapeEvent const &>(e);
	} else if (e.type == xrandr_event + RRNotify) {
		//printf("RRNotify\n");
		XRRNotifyEvent const & ev = reinterpret_cast<XRRNotifyEvent const &>(e);
		if (ev.subtype == RRNotify_CrtcChange) {
			/* re-read root window attribute, in particular the size **/
			update_layout();
			//rebuild_cairo_context();
		}
	} else {
//		printf("Unhandled event\n");
//		if (e.xany.type > 0 && e.xany.type < LASTEvent) {
//			printf("#%lu type: %s, send_event: %u, window: %lu\n",
//					e.xany.serial, x_event_name[e.xany.type], e.xany.send_event,
//					e.xany.window);
//		} else {
//			printf("#%lu type: %u, send_event: %u, window: %lu\n",
//					e.xany.serial, e.xany.type, e.xany.send_event,
//					e.xany.window);
//		}
	}

}

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
			rectangle area(info->x, info->y, info->width, info->height);
			_desktop_region = _desktop_region + area;
		}
		XRRFreeCrtcInfo(info);
	}
	XRRFreeScreenResources(resources);

	printf("layout = %s\n", _desktop_region.to_string().c_str());

	composite_back_buffer = XdbeAllocateBackBufferName(_cnx->dpy(), composite_overlay, XdbeCopied);

}


void compositor_t::process_events() {
	XEvent ev;
	while(XPending(_cnx->dpy())) {
		XNextEvent(_cnx->dpy(), &ev);
		process_event(ev);
	}
}

/**
 * Check event without blocking.
 **/
bool compositor_t::process_check_event() {
	XEvent e;

	int x;
	/** Passive check of events in queue, never flush any thing **/
	if ((x = XEventsQueued(_cnx->dpy(), QueuedAlready)) > 0) {
		/** should not block or flush **/
		XNextEvent(_cnx->dpy(), &e);
		process_event(e);
		return true;
	} else {
		return false;
	}
}

Window compositor_t::get_composite_overlay() {
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
	cairo_identity_matrix(cr);
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

