/*
 * render_context.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include <algorithm>
#include <list>
#include "compositor.hxx"

char const * x_event_name[LASTEvent] = { 0, 0, "KeyPress", "KeyRelease",
		"ButtonPress", "ButtonRelease", "MotionNotify", "EnterNotify",
		"LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose",
		"GraphicsExpose", "NoExpose", "VisibilityNotify", "CreateNotify",
		"DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
		"ReparentNotify", "ConfigureNotify", "ConfigureRequest",
		"GravityNotify", "ResizeRequest", "CirculateNotify", "CirculateRequest",
		"PropertyNotify", "SelectionClear", "SelectionRequest",
		"SelectionNotify", "ColormapNotify", "ClientMessage", "MappingNotify",
		"GenericEvent" };

namespace page {

template<typename T, typename _>
inline bool has_key(std::map<T, _> const & x, T const & key) {
	typename std::map<T, _>::const_iterator i = x.find(key);
	return i != x.end();
}

template<typename T>
inline bool has_key(std::set<T> const & x, T const & key) {
	typename std::set<T>::const_iterator i = x.find(key);
	return i != x.end();
}

template<typename T>
inline bool has_key(std::list<T> const & x, T const & key) {
	typename std::list<T>::const_iterator i = std::find(x.begin(), x.end(), key);
	return i != x.end();
}

template<typename _0, typename _1>
inline _1 get_safe_value(std::map<_0, _1> & map, _0 key, _1 def) {
	typename std::map<_0, _1>::iterator i = map.find(key);
	if(i != map.end())
		return i->second;
	else
		return def;
}


static void _draw_crossed_box(cairo_t * cr, box_int_t const & box, double r, double g,
		double b) {

	return;

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
}

compositor_t::compositor_t() : _cnx() {

	/* create an invisile window to identify page */
	cm_window = XCreateSimpleWindow(_cnx.dpy, _cnx.xroot, -100, -100, 1, 1, 0,
			0, 0);
	if (cm_window == None) {
		XCloseDisplay(_cnx.dpy);
		throw compositor_fail_t();
	}

	std::string name("page_internal_compositor");
	_cnx.change_property(cm_window, _cnx.atoms._NET_WM_NAME,
			_cnx.atoms.UTF8_STRING, 8,
			PropModeReplace,
			reinterpret_cast<unsigned char const *>(name.c_str()),
			name.length() + 1);

	/* try to register compositor manager */
	if (!_cnx.register_cm(cm_window)) {
		XCloseDisplay(_cnx.dpy);
		throw compositor_fail_t();
	}

	/* initialize composite */
	_cnx.init_composite_overlay();

	fast_region_surf = 0.0;
	slow_region_surf = 0.0;

	flush_count = 0;
	clock_gettime(CLOCK_MONOTONIC, &last_tic);

	composite_overlay_s = 0;
	composite_overlay_cr = 0;

	pre_back_buffer_s = 0;
	pre_back_buffer_cr = 0;

	rebuild_cairo_context();

	_cnx.grab();
	XRRSelectInput(_cnx.dpy, _cnx.xroot, RRCrtcChangeNotifyMask);
	update_layout();
	_cnx.ungrab();

	_cnx.add_event_handler(this);

	/** update the windows list **/
	scan();

	XDamageCreate(_cnx.dpy, _cnx.xroot, XDamageReportRawRectangles);

}

void compositor_t::rebuild_cairo_context() {

	if (composite_overlay_cr != 0)
		cairo_destroy(composite_overlay_cr);
	if (composite_overlay_s != 0)
		cairo_surface_destroy(composite_overlay_s);

	composite_overlay_s = cairo_xlib_surface_create(_cnx.dpy,
			_cnx.composite_overlay, _cnx.root_wa.visual, _cnx.root_wa.width,
			_cnx.root_wa.height);
	composite_overlay_cr = cairo_create(composite_overlay_s);

	/* clean up surface */
	cairo_set_operator(composite_overlay_cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(composite_overlay_cr, 0.0, 0.0, 0.0, 1.0);
	cairo_reset_clip(composite_overlay_cr);
	cairo_paint(composite_overlay_cr);

	if (pre_back_buffer_cr != 0)
		cairo_destroy(pre_back_buffer_cr);
	if (pre_back_buffer_s != 0)
		cairo_surface_destroy(pre_back_buffer_s);

	pre_back_buffer_s = cairo_surface_create_similar(composite_overlay_s,
			CAIRO_CONTENT_COLOR, _cnx.root_wa.width, _cnx.root_wa.height);
	pre_back_buffer_cr = cairo_create(pre_back_buffer_s);

	/* clean up surface */
	cairo_set_operator(pre_back_buffer_cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(pre_back_buffer_cr, 0.0, 0.0, 0.0, 1.0);
	cairo_reset_clip(pre_back_buffer_cr);
	cairo_paint(pre_back_buffer_cr);

}

void compositor_t::draw_box(box_int_t box, double r, double g, double b) {
	cairo_set_source_rgb(composite_overlay_cr, r, g, b);
	cairo_set_line_width(composite_overlay_cr, 1.0);
	cairo_rectangle(composite_overlay_cr, box.x + 0.5, box.y + 0.5, box.w - 1.0, box.h - 1.0);
	cairo_stroke(composite_overlay_cr);
	cairo_surface_flush(composite_overlay_s);
}

void compositor_t::add_damage_area(region_t<int> const & box) {
	//printf("Add %s\n", box.to_string().c_str());
	pending_damage = pending_damage + box;
}

void compositor_t::render_flush() {
	flush_count += 1;

	clock_gettime(CLOCK_MONOTONIC, &curr_tic);
	if(last_tic.tv_sec + 30 < curr_tic.tv_sec) {

		double t0 = last_tic.tv_sec + last_tic.tv_nsec / 1.0e9;
		double t1 = curr_tic.tv_sec + curr_tic.tv_nsec / 1.0e9;

		printf("render flush per second : %0.2f\n", flush_count / (t1 - t0));

		last_tic = curr_tic;
		flush_count = 0;
	}

	/* list content is bottom window to upper window in stack */
	/* a small optimization, because visible are often few */
	std::list<renderable_window_t *> visible;
	for(std::list<Window>::iterator i = window_stack.begin(); i != window_stack.end(); ++i) {
		if(window_data[*i]->is_visible()) {
			visible.push_back(window_data[*i]);
		}
	}

//	for (std::list<renderable_window_t *>::iterator i = visible.begin();
//			i != visible.end(); ++i) {
//		(*i)->init_cairo();
//	}

	/* clip damage region to visible region */
	pending_damage = pending_damage & _desktop_region;

	/**
	 * Find region where windows are not overlap each other. i.e. region where
	 * only one window will be rendered.
	 * This is often more than 80% of the screen.
	 * This kind of region will be directly rendered.
	 **/
	region_t<int> region_with_not_overlapped_window;

	std::list<renderable_window_t *>::iterator i = visible.begin();
	while (i != visible.end()) {
		region_t<int> r = (*i)->get_region();
		std::list<renderable_window_t *>::iterator j = visible.begin();
		while (j != visible.end()) {
			if (i != j) {
				r = r - (*j)->get_region();
			}
			++j;
		}
		region_with_not_overlapped_window = region_with_not_overlapped_window + r;
		++i;
	}

	/**
	 * Find region where the windows on top is not a window with alpha.
	 * Small area, often dropdown menu.
	 * This kind of area will be directly rendered.
	 **/
	region_t<int> region_with_not_alpha_on_top;

	/**
	 * Walk over all all window from bottom to top one. If window has alpha
	 * channel remove it from region with no alpha, if window do not have alpha
	 * add this window to the area.
	 **/
	i = visible.begin();
	while (i != visible.end()) {
		region_t<int> r = (*i)->get_region();
		if((*i)->has_alpha()) {
			region_with_not_alpha_on_top = region_with_not_alpha_on_top - r;
		} else {
			/* if not has_alpha, add this area */
			region_with_not_alpha_on_top = region_with_not_alpha_on_top + r;
		}
		++i;
	}

	region_t<int> direct_region = region_with_not_overlapped_window;

	direct_region = direct_region & pending_damage;
	region_t<int> slow_region = (pending_damage - direct_region) -
			region_with_not_alpha_on_top;

	/* direct render, area where there is only one window visible */
	{
		cairo_reset_clip(composite_overlay_cr);
		cairo_set_operator(composite_overlay_cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_antialias(composite_overlay_cr, CAIRO_ANTIALIAS_NONE);

		region_t<int>::box_list_t::const_iterator i = direct_region.begin();
		while (i != direct_region.end()) {
			fast_region_surf += (*i).w * (*i).h;
			repair_buffer(visible, composite_overlay_cr, *i);
			// for debuging
			_draw_crossed_box(composite_overlay_cr, (*i), 0.0, 1.0, 0.0);
			++i;
		}
	}

	/* directly render area where window on the has no alpha */

	region_with_not_alpha_on_top = (region_with_not_alpha_on_top & pending_damage) - direct_region;

	{
		cairo_reset_clip(composite_overlay_cr);
		cairo_set_operator(composite_overlay_cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_antialias(composite_overlay_cr, CAIRO_ANTIALIAS_NONE);

		/* from top to bottom */
		for (std::list<renderable_window_t *>::reverse_iterator i = visible.rbegin(); i != visible.rend();
				++i) {
			renderable_window_t * r = *i;
			region_t<int> draw_area = region_with_not_alpha_on_top & r->get_region();
			if (!draw_area.empty()) {
				for (region_t<int>::box_list_t::iterator j = draw_area.begin();
						j != draw_area.end(); ++j) {
					if (!(*j).is_null()) {
						r->repair1(composite_overlay_cr, (*j));

						/* this section show direct rendered screen */
						_draw_crossed_box(composite_overlay_cr, (*j), 0.0, 0.0, 1.0);
					}
				}
			}
			region_with_not_alpha_on_top = region_with_not_alpha_on_top - r->get_region();
		}
	}

	/**
	 * To avoid glitch (blinking) I use back buffer to make the composition.
	 * update back buffer, render area with possible transparency
	 **/
	{
		cairo_reset_clip(pre_back_buffer_cr);
		cairo_set_operator(pre_back_buffer_cr, CAIRO_OPERATOR_OVER);
		region_t<int>::box_list_t::const_iterator i = slow_region.begin();
		while (i != slow_region.end()) {
			slow_region_surf += (*i).w * (*i).h;
			repair_buffer(visible, pre_back_buffer_cr, *i);
			++i;
		}
	}

	{
		cairo_surface_flush(pre_back_buffer_s);
		region_t<int>::box_list_t::const_iterator i = slow_region.begin();
		while (i != slow_region.end()) {
			repair_overlay(*i, pre_back_buffer_s);
			_draw_crossed_box(composite_overlay_cr, (*i), 1.0, 0.0, 0.0);
			++i;
		}
	}
	pending_damage.clear();

//	for (std::list<renderable_window_t *>::iterator i = visible.begin();
//			i != visible.end(); ++i) {
//		(*i)->destroy_cairo();
//	}

}


void compositor_t::repair_buffer(std::list<renderable_window_t *> & visible, cairo_t * cr,
		box_int_t const & area) {
	for (std::list<renderable_window_t *>::iterator i = visible.begin(); i != visible.end();
			++i) {
		renderable_window_t * r = *i;
		region_t<int>::box_list_t barea = r->get_region();
		for (region_t<int>::box_list_t::iterator j = barea.begin();
				j != barea.end(); ++j) {
			box_int_t clip = area & (*j);
			if (!clip.is_null()) {
				r->repair1(cr, clip);
			}
		}
	}
}


void compositor_t::repair_overlay(box_int_t const & area, cairo_surface_t * src) {

	cairo_reset_clip(composite_overlay_cr);
	cairo_identity_matrix(composite_overlay_cr);
	cairo_set_operator(composite_overlay_cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(composite_overlay_cr, src, 0, 0);
	cairo_rectangle(composite_overlay_cr, area.x, area.y, area.w, area.h);
	cairo_fill(composite_overlay_cr);

	/* for debug purpose */
	if (false) {
		static int color = 0;
		switch (color % 3) {
		case 0:
			cairo_set_source_rgb(composite_overlay_cr, 1.0, 0.0, 0.0);
			break;
		case 1:
			cairo_set_source_rgb(composite_overlay_cr, 1.0, 1.0, 0.0);
			break;
		case 2:
			cairo_set_source_rgb(composite_overlay_cr, 1.0, 0.0, 1.0);
			break;
		}
		//++color;
		cairo_set_line_width(composite_overlay_cr, 1.0);
		cairo_rectangle(composite_overlay_cr, area.x + 0.5, area.y + 0.5, area.w - 1.0, area.h - 1.0);
		cairo_clip(composite_overlay_cr);
		cairo_paint_with_alpha(composite_overlay_cr, 0.3);
		cairo_reset_clip(composite_overlay_cr);
		//cairo_stroke(composite_overlay_cr);
	}

	// Never flush composite_overlay_s because this surface is never a source surface.
	//cairo_surface_flush(composite_overlay_s);

}


void compositor_t::damage_all() {
	add_damage_area(_cnx.root_size);
}


void compositor_t::process_event(XCreateWindowEvent const & e) {
	if(e.parent != _cnx.xroot)
		return;

	_cnx.grab();
	try_add_window(e.window);
	_cnx.ungrab();
}

void compositor_t::process_event(XReparentEvent const & e) {
	if(e.parent == _cnx.xroot) {
		try_add_window(e.window);
	} else {
		try_remove_window(e.window);
	}
}

void compositor_t::process_event(XMapEvent const & e) {
	if(has_key(window_data, e.window)) {
		window_data[e.window]->update_map_state(true);
		add_damage_area(window_data[e.window]->get_region());
	}
}

void compositor_t::process_event(XUnmapEvent const & e) {
	if(has_key(window_data, e.window)) {
		window_data[e.window]->update_map_state(false);
		add_damage_area(window_data[e.window]->get_region());
	}
}

void compositor_t::process_event(XDestroyWindowEvent const & e) {
	//printf("Destroy EVENT\n");
	try_remove_window(e.window);
}

void compositor_t::process_event(XConfigureEvent const & e) {
	if(has_key(window_data, e.window)) {
		renderable_window_t * ws = window_data[e.window];
		add_damage_area(ws->get_region());
		ws->update_position(box_int_t(e.x, e.y, e.width, e.height));
		add_damage_area(ws->get_region());
		window_stack.remove(e.window);
		std::list<Window>::iterator x = std::find(window_stack.begin(), window_stack.end(), e.above);
		if(x == window_stack.end()) {
			window_stack.push_front(e.window);
		} else {
			window_stack.insert(++x, e.window);
		}

		add_damage_area(window_data[e.window]->get_region());
	}
}

void compositor_t::process_event(XCirculateEvent const & e) {
	if(e.event != _cnx.xroot)
		return;
	if(has_key(window_data, e.window)) {
		window_stack.remove(e.window);
		if(e.place == PlaceOnTop) {
			window_stack.push_back(e.window);
		} else {
			window_stack.push_front(e.window);
		}
	}
}

void compositor_t::process_event(XDamageNotifyEvent const & e) {
	//printf("Damage area %dx%d+%d+%d\n", e.area.width, e.area.height, e.area.x, e.area.y);

	/* create an empty region */
	XserverRegion region = XFixesCreateRegion(_cnx.dpy, 0, 0);

	if (!region)
		throw std::runtime_error("could not create region");

	/* get damaged region and remove them from damaged status */
	XDamageSubtract(_cnx.dpy, e.damage, None, region);

	if (has_key(window_data, e.drawable)) {

		renderable_window_t * ws = window_data[e.drawable];
		XRectangle * rects;
		int nb_rects;

		/* get all rectangles for the damaged region */
		rects = XFixesFetchRegion(_cnx.dpy, region, &nb_rects);

		if (rects) {
			for (int i = 0; i < nb_rects; ++i) {
				box_int_t box(rects[i]);
				/* setup absolute damaged area */
				box.x += ws->position.x;
				box.y += ws->position.y;
				if(ws->is_visible())
					add_damage_area(box);
				if(e.more == False)
					render_flush();
			}
			XFree(rects);
		}

		XFixesDestroyRegion(_cnx.dpy, region);
	}

}

renderable_window_t * compositor_t::find_window(Window w) {
	return 0;
}

void compositor_t::try_add_window(Window w) {
	if(has_key(window_data, w))
		return;
	window_stack.push_back(w);
	window_data[w] = new renderable_window_t();

	XWindowAttributes wa;
	if(XGetWindowAttributes(_cnx.dpy, w, &wa) != 0) {
		window_data[w]->update(_cnx.dpy, w, wa);
	}
}

void compositor_t::try_remove_window(Window w) {
	window_stack.remove(w);
	if(window_data[w] != 0) {
		delete window_data[w];
	}
	window_data.erase(w);
}

void compositor_t::scan() {
//	printf("call %s\n", __PRETTY_FUNCTION__);
	unsigned int num;
	Window d1, d2, *wins = 0;

	/** Grab server to avoid missing event **/
	_cnx.grab();

	/**
	 * Start listen root event before anything each event will be stored to
	 * right run later.
	 **/
	XSelectInput(_cnx.dpy, _cnx.xroot, SubstructureNotifyMask);

	window_stack.clear();
	window_data.clear();

	/** read all current root window **/
	if (XQueryTree(_cnx.dpy, _cnx.xroot, &d1, &d2, &wins, &num) != 0) {

		for (unsigned i = 0; i < num; ++i) {
			try_add_window(wins[i]);
		}

		XFree(wins);
	}

	_cnx.ungrab();

//	printf("return %s\n", __PRETTY_FUNCTION__);
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
	} else if (e.type == _cnx.damage_event + XDamageNotify) {
		process_event(reinterpret_cast<XDamageNotifyEvent const &>(e));
	} else if (e.type == _cnx.xinerama_event) {
		printf("a xinerama event\n");
	} else if (e.type == _cnx.xshape_event + ShapeNotify) {
		XShapeEvent const & sev = reinterpret_cast<XShapeEvent const &>(e);
		if(has_key(window_data, sev.window)) {
			window_data[sev.window]->read_shape();
		}
	} else if (e.type == _cnx.xrandr_event + RRNotify || e.type == RRScreenChangeNotify) {
		printf("RRNotify\n");
		XRRNotifyEvent const & ev = reinterpret_cast<XRRNotifyEvent const &>(e);
		if (ev.subtype == RRNotify_CrtcChange) {
			/* re-read root window attribute, in particular the size **/
			XGetWindowAttributes(_cnx.dpy, _cnx.xroot, &_cnx.root_wa);
			_cnx.root_size.w = _cnx.root_wa.width;
			_cnx.root_size.h = _cnx.root_wa.height;
			rebuild_cairo_context();
			update_layout();
		}
	} else {
		printf("Unhandled event\n");
		if (e.xany.type > 0 && e.xany.type < LASTEvent) {
			printf("#%lu type: %s, send_event: %u, window: %lu\n",
					e.xany.serial, x_event_name[e.xany.type], e.xany.send_event,
					e.xany.window);
		} else {
			printf("#%lu type: %u, send_event: %u, window: %lu\n",
					e.xany.serial, e.xany.type, e.xany.send_event,
					e.xany.window);
		}
	}

}

void compositor_t::update_layout() {

	_desktop_region.clear();

	XRRScreenResources * resources = XRRGetScreenResourcesCurrent(_cnx.dpy,
			_cnx.xroot);

	for (int i = 0; i < resources->ncrtc; ++i) {
		XRRCrtcInfo * info = XRRGetCrtcInfo(_cnx.dpy, resources,
				resources->crtcs[i]);

		/** if the CRTC has at less one output bound **/
		if(info->noutput > 0) {
			box_t<int> area(info->x, info->y, info->width, info->height);
			_desktop_region = _desktop_region + area;
		}
		XRRFreeCrtcInfo(info);
	}
	XRRFreeScreenResources(resources);

	//printf("layout = %s\n", _desktop_region.to_string().c_str());
}

}

