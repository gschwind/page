/*
 * render_context.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include <algorithm>
#include <list>
#include "compositor.hxx"
#include "atoms.hxx"

#include "xconnection.hxx"

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

static void _draw_crossed_box(cairo_t * cr, box_int_t const & box, double r, double g,
		double b) {
	return;
	cairo_save(cr);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

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

compositor_t::compositor_t() : _cnx(new xconnection_t()) {

	/* create an invisible window to identify page */
	cm_window = XCreateSimpleWindow(_cnx->dpy, _cnx->get_root_window(), -100, -100, 1, 1, 0,
			0, 0);
	if (cm_window == None) {
		XCloseDisplay(_cnx->dpy);
		throw compositor_fail_t();
	}

	std::string name("page_internal_compositor");
	_cnx->change_property(cm_window, _NET_WM_NAME,
			UTF8_STRING, 8,
			PropModeReplace,
			reinterpret_cast<unsigned char const *>(name.c_str()),
			name.length() + 1);

	long pid = getpid();

	_cnx->change_property(cm_window, _NET_WM_PID, CARDINAL,
			32, (PropModeReplace),
			reinterpret_cast<unsigned char const *>(&pid), 1);

	/* try to register compositor manager */
	if (!_cnx->register_cm(cm_window)) {
		XCloseDisplay(_cnx->dpy);
		throw compositor_fail_t();
	}

	/* initialize composite */
	_cnx->init_composite_overlay();

	fast_region_surf_monitor = 0.0;
	slow_region_surf_monitor = 0.0;

	flush_count = 0;
	clock_gettime(CLOCK_MONOTONIC, &last_tic);

	_cnx->grab();
	XRRSelectInput(_cnx->dpy, _cnx->get_root_window(), RRCrtcChangeNotifyMask);
	update_layout();
	_cnx->ungrab();

	_cnx->add_event_handler(this);

	/** update the windows list **/
	scan();

	//printf("Root = %lu\n", _cnx->get_root_window());
	//printf("Composite = %lu\n", _cnx->composite_overlay);

	/**
	 * Add the composite window to the stack as invisible window to tack a
	 * valid stack. scan() does not report this window.
	 **/
	//printf("Add window %lu\n", _cnx->composite_overlay);
	window_stack.push_back(_cnx->composite_overlay);
	window_data[_cnx->composite_overlay] = new composite_window_t();

	damage_all();
}

compositor_t::~compositor_t() {

	map<Window, composite_window_t *>::iterator i = window_data.begin();
	while(i != window_data.end()) {
		delete i->second;
		++i;
	}

	_cnx->remove_event_handler(this);
};

void compositor_t::add_damage_area(_region_t const & box) {
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

	/**
	 * list content is bottom window to upper window in stack a optimization.
	 **/
	std::list<composite_window_t *> visible;
	for(std::list<Window>::iterator i = window_stack.begin(); i != window_stack.end(); ++i) {
		if(window_data[*i]->is_visible()) {
			visible.push_back(window_data[*i]);
		}
	}

	/**
	 * Asked to cairo community, build/destroy cairo contexts and surfaces
	 * should not hit too much performance.
	 **/

	XWindowAttributes wa;
	XGetWindowAttributes(_cnx->dpy, _cnx->composite_overlay, &wa);

	composite_overlay_s = cairo_xlib_surface_create(_cnx->dpy,
			_cnx->composite_overlay, wa.visual, wa.width, wa.height);
	composite_overlay_cr = cairo_create(composite_overlay_s);

	for (std::list<composite_window_t *>::iterator i = visible.begin();
			i != visible.end(); ++i) {
		(*i)->init_cairo();
	}

	/* clip damage region to visible region */
	pending_damage = pending_damage & _desktop_region;

	/**
	 * Find region where windows are not overlap each other. i.e. region where
	 * only one window will be rendered.
	 * This is often more than 80% of the screen.
	 * This kind of region will be directly rendered.
	 **/
	_region_t region_without_overlapped_window;

	std::list<composite_window_t *>::iterator i = visible.begin();
	while (i != visible.end()) {
		_region_t r = (*i)->get_region();
		std::list<composite_window_t *>::iterator j = visible.begin();
		while (j != visible.end()) {
			if (i != j) {
				r = r - (*j)->get_region();
			}
			++j;
		}
		region_without_overlapped_window += r;
		++i;
	}

	/**
	 * Find region where the windows on top is not a window with alpha.
	 * Small area, often dropdown menu.
	 * This kind of area will be directly rendered.
	 **/
	_region_t region_without_alpha_on_top;

	/**
	 * Walk over all all window from bottom to top one. If window has alpha
	 * channel remove it from region with no alpha, if window do not have alpha
	 * add this window to the area.
	 **/
	i = visible.begin();
	while (i != visible.end()) {
		_region_t r = (*i)->get_region();
		if((*i)->has_alpha()) {
			region_without_alpha_on_top = region_without_alpha_on_top - r;
		} else {
			/* if not has_alpha, add this area */
			region_without_alpha_on_top += r;
		}
		++i;
	}

	_region_t direct_region = region_without_overlapped_window;

	direct_region = direct_region & pending_damage;
	_region_t slow_region = (pending_damage - direct_region) -
			region_without_alpha_on_top;

	/* direct render, area where there is only one window visible */
	{
		cairo_reset_clip(composite_overlay_cr);
		cairo_set_operator(composite_overlay_cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_antialias(composite_overlay_cr, CAIRO_ANTIALIAS_NONE);

		_region_t::const_iterator i = direct_region.begin();
		while (i != direct_region.end()) {
			fast_region_surf_monitor += (*i).w * (*i).h;
			repair_buffer(visible, composite_overlay_cr, *i);
			// for debuging
			_draw_crossed_box(composite_overlay_cr, (*i), 0.0, 1.0, 0.0);
			++i;
		}
	}

	/* directly render area where window on the has no alpha */

	region_without_alpha_on_top = (region_without_alpha_on_top & pending_damage) - direct_region;

	{
		cairo_reset_clip(composite_overlay_cr);
		cairo_set_operator(composite_overlay_cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_antialias(composite_overlay_cr, CAIRO_ANTIALIAS_NONE);

		/* from top to bottom */
		for (std::list<composite_window_t *>::reverse_iterator i = visible.rbegin(); i != visible.rend();
				++i) {
			composite_window_t * r = *i;
			_region_t draw_area = region_without_alpha_on_top & r->get_region();
			if (!draw_area.empty()) {
				for (_region_t::iterator j = draw_area.begin();
						j != draw_area.end(); ++j) {
					if (!(*j).is_null()) {
						r->draw_to(composite_overlay_cr, (*j));

						/* this section show direct rendered screen */
						_draw_crossed_box(composite_overlay_cr, (*j), 0.0, 0.0, 1.0);
					}
				}
			}
			region_without_alpha_on_top = region_without_alpha_on_top - r->get_region();
		}
	}

	/**
	 * To avoid glitch (blinking) I use back buffer to make the composition.
	 * update back buffer, render area with possible transparency
	 **/
	if (!slow_region.empty()) {
		pre_back_buffer_s = cairo_surface_create_similar(composite_overlay_s,
				CAIRO_CONTENT_COLOR, wa.width, wa.height);
		pre_back_buffer_cr = cairo_create(pre_back_buffer_s);

		{

			cairo_reset_clip(pre_back_buffer_cr);
			cairo_set_operator(pre_back_buffer_cr, CAIRO_OPERATOR_OVER);
			_region_t::const_iterator i = slow_region.begin();
			while (i != slow_region.end()) {
				slow_region_surf_monitor += (*i).w * (*i).h;
				repair_buffer(visible, pre_back_buffer_cr, *i);
				++i;
			}
		}

		{
			cairo_surface_flush(pre_back_buffer_s);
			_region_t::const_iterator i = slow_region.begin();
			while (i != slow_region.end()) {
				repair_overlay(*i, pre_back_buffer_s);
				_draw_crossed_box(composite_overlay_cr, (*i), 1.0, 0.0, 0.0);
				++i;
			}
		}
		cairo_destroy(pre_back_buffer_cr);
		cairo_surface_destroy(pre_back_buffer_s);
	}

	pending_damage.clear();

	cairo_destroy(composite_overlay_cr);
	cairo_surface_destroy(composite_overlay_s);

	for (std::list<composite_window_t *>::iterator i = visible.begin();
			i != visible.end(); ++i) {
		(*i)->destroy_cairo();
	}

}


void compositor_t::repair_buffer(std::list<composite_window_t *> & visible, cairo_t * cr,
		_box_t const & area) {
	for (std::list<composite_window_t *>::iterator i = visible.begin(); i != visible.end();
			++i) {
		composite_window_t * r = *i;
		_region_t barea = r->get_region();
		for (_region_t::iterator j = barea.begin();
				j != barea.end(); ++j) {
			_box_t clip = area & (*j);
			if (!clip.is_null()) {
				r->draw_to(cr, clip);
			}
		}
	}
}


void compositor_t::repair_overlay(_box_t const & area, cairo_surface_t * src) {

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
	XWindowAttributes const * wa = _cnx->get_window_attributes(
			_cnx->get_root_window());
	add_damage_area(box_int_t(wa->x, wa->y, wa->width, wa->height));
}


void compositor_t::process_event(XCreateWindowEvent const & e) {
	if(e.parent != _cnx->get_root_window())
		return;

	/** grab to read valid shape values **/
	_cnx->grab();
	register_window(e.window);
	_cnx->ungrab();
}

void compositor_t::process_event(XReparentEvent const & e) {
	if(e.parent == _cnx->get_root_window()) {
		register_window(e.window);
	} else {
		unregister_window(e.window);
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
	unregister_window(e.window);
}

void compositor_t::process_event(XConfigureEvent const & e) {
	if(has_key(window_data, e.window)) {
		composite_window_t * ws = window_data[e.window];
		add_damage_area(ws->get_region());
		ws->update_position(_box_t(e.x, e.y, e.width, e.height));
		add_damage_area(ws->get_region());

		if (e.above == None) {
			window_stack.remove(e.window);
			window_stack.push_front(e.window);
		} else {

			std::list<Window>::iterator x = std::find(window_stack.begin(),
					window_stack.end(), e.above);

			if (x == window_stack.end()) {
				printf("Window not found : %lu for window %lu\n", e.above,
						e.window);
			} else {
				window_stack.remove(e.window);
				window_stack.insert(++x, e.window);
			}

		}

		add_damage_area(window_data[e.window]->get_region());
	}
}

void compositor_t::process_event(XCirculateEvent const & e) {
	if(e.event != _cnx->get_root_window())
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
	XserverRegion region = XFixesCreateRegion(_cnx->dpy, 0, 0);

	if (!region)
		throw std::runtime_error("could not create region");

	/* get damaged region and remove them from damaged status */
	XDamageSubtract(_cnx->dpy, e.damage, None, region);

	if (has_key(window_data, e.drawable)) {

		composite_window_t * ws = window_data[e.drawable];
		XRectangle * rects;
		int nb_rects;

		/* get all rectangles for the damaged region */
		rects = XFixesFetchRegion(_cnx->dpy, region, &nb_rects);

		if (rects) {
			for (int i = 0; i < nb_rects; ++i) {
				_box_t box(rects[i]);
				/* setup absolute damaged area */
				box.x += ws->position().x;
				box.y += ws->position().y;
				if(ws->is_visible())
					add_damage_area(box);
				if(e.more == False)
					render_flush();
			}
			XFree(rects);
		}

		XFixesDestroyRegion(_cnx->dpy, region);
	}

}

void compositor_t::register_window(Window w) {
	if(has_key(window_data, w))
		return;

	//printf("Add window %lu\n", w);
	window_stack.push_back(w);
	window_data[w] = new composite_window_t();

	XWindowAttributes wa;
	if(XGetWindowAttributes(_cnx->dpy, w, &wa) != 0) {
		window_data[w]->update(_cnx->dpy, w, wa);
		window_data[w]->create_damage();
	}
}

void compositor_t::unregister_window(Window w) {
	window_stack.remove(w);
	if(has_key(window_data, w)) {
		window_data[w]->destroy_damage();
		delete window_data[w];
		window_data.erase(w);
	}
}

void compositor_t::scan() {
//	printf("call %s\n", __PRETTY_FUNCTION__);
	unsigned int num;
	Window d1, d2, *wins = 0;

	/** Grab server to avoid missing event **/
	_cnx->grab();

	/**
	 * Start listen root event before anything each event will be stored to
	 * right run later.
	 **/
	XSelectInput(_cnx->dpy, _cnx->get_root_window(), SubstructureNotifyMask);

	window_stack.clear();
	window_data.clear();

	/** read all current root window **/
	if (XQueryTree(_cnx->dpy, _cnx->get_root_window(), &d1, &d2, &wins, &num) != 0) {

		for (unsigned i = 0; i < num; ++i) {
			register_window(wins[i]);
		}

		XFree(wins);
	}

	_cnx->ungrab();

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
	} else if (e.type == _cnx->damage_event + XDamageNotify) {
		process_event(reinterpret_cast<XDamageNotifyEvent const &>(e));
	} else if (e.type == _cnx->xshape_event + ShapeNotify) {
		XShapeEvent const & sev = reinterpret_cast<XShapeEvent const &>(e);
		if(has_key(window_data, sev.window)) {
			window_data[sev.window]->read_shape();
		}
	} else if (e.type == _cnx->xrandr_event + RRNotify) {
		printf("RRNotify\n");
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

	_desktop_region.clear();

	XRRScreenResources * resources = XRRGetScreenResourcesCurrent(_cnx->dpy,
			_cnx->get_root_window());

	for (int i = 0; i < resources->ncrtc; ++i) {
		XRRCrtcInfo * info = XRRGetCrtcInfo(_cnx->dpy, resources,
				resources->crtcs[i]);

		/** if the CRTC has at less one output bound **/
		if(info->noutput > 0) {
			_box_t area(info->x, info->y, info->width, info->height);
			_desktop_region = _desktop_region + area;
		}
		XRRFreeCrtcInfo(info);
	}
	XRRFreeScreenResources(resources);

	printf("layout = %s\n", _desktop_region.to_string().c_str());
}


void compositor_t::process_events() {
	while(_cnx->process_check_event())
		continue;
	render_flush();
}

}

