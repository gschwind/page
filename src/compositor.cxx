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

namespace page {

static void _draw_crossed_box(cairo_t * cr, box_int_t const & box, double r, double g,
		double b) {

	return;

	cairo_save(cr);
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

compositor_t::compositor_t() : _cnx(new xconnection_t()) {

	/* create an invisible window to identify page */
	cm_window = XCreateSimpleWindow(_cnx->dpy, _cnx->get_root_window(), -100,
			-100, 1, 1, 0, 0, 0);
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
	damage_count = 0;
	clock_gettime(CLOCK_MONOTONIC, &last_tic);

	XRRSelectInput(_cnx->dpy, _cnx->get_root_window(), RRCrtcChangeNotifyMask);
	update_layout();

	_cnx->add_event_handler(this);

	/** update the windows list **/
	scan();

	//printf("Root = %lu\n", _cnx->get_root_window());
	//printf("Composite = %lu\n", _cnx->composite_overlay);

	/**
	 * Add the composite window to the stack as invisible window to track a
	 * valid stack. scan() currently does not report this window, I add remove
	 * line, in case of scan will report it.
	 **/

	stack_window_place_on_top(_cnx->composite_overlay);

	p_window_attribute_t wa = _cnx->get_window_attributes(_cnx->get_root_window());

	_front_buffer = cairo_xlib_surface_create(_cnx->dpy,
			_cnx->composite_overlay, wa->visual, wa->width, wa->height);
	_bask_buffer = cairo_surface_create_similar(_front_buffer,
			CAIRO_CONTENT_COLOR, wa->width, wa->height);



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

//void compositor_t::add_damage_area(_region_t const & box) {
//	//printf("Add damage %s\n", box.to_string().c_str());
//	pending_damage = pending_damage + box;
//}

void compositor_t::render_flush() {
	flush_count += 1;

	clock_gettime(CLOCK_MONOTONIC, &curr_tic);
	if (last_tic.tv_sec + 30 < curr_tic.tv_sec) {

		double t0 = last_tic.tv_sec + last_tic.tv_nsec / 1.0e9;
		double t1 = curr_tic.tv_sec + curr_tic.tv_nsec / 1.0e9;

		printf("render flush per second : %0.2f\n", flush_count / (t1 - t0));
		printf("damage per second : %0.2f\n", damage_count / (t1 - t0));

		last_tic = curr_tic;
		flush_count = 0;
		damage_count = 0;
	}

	compute_pending_damage();

	/** this line divide page cpu cost by 4.0 ... O_O it's insane **/
	if(pending_damage.empty())
		return;

	/**
	 * list content is bottom window to upper window in stack a optimization.
	 **/
	std::list<composite_window_t *> visible;
	for (std::list<Window>::iterator i = window_stack.begin();
			i != window_stack.end(); ++i) {
		map<Window, composite_window_t *>::iterator x = window_data.find(*i);
		if (x != window_data.end()) {
			visible.push_back(x->second);
		}
	}

	/**
	 * Asked to cairo community, build/destroy cairo contexts and surfaces
	 * should not hit too much performance.
	 **/

	cairo_t * front_cr = cairo_create(_front_buffer);

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
	for (list<composite_window_t *>::iterator i = visible.begin();
			i != visible.end(); ++i) {
		_region_t r = (*i)->get_region();
		for (list<composite_window_t *>::iterator j = visible.begin();
				j != visible.end(); ++j) {
			if (i != j) {
				r -= (*j)->get_region();
			}
		}
		region_without_overlapped_window += r;
	}

	region_without_overlapped_window = region_without_overlapped_window
			& pending_damage;

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
	for (list<composite_window_t *>::iterator i = visible.begin();
			i != visible.end(); ++i) {
		if ((*i)->has_alpha()) {
			region_without_alpha_on_top -= (*i)->get_region();
		} else {
			/* if not has_alpha, add this area */
			region_without_alpha_on_top += (*i)->get_region();
		}
	}

	region_without_alpha_on_top = region_without_alpha_on_top & pending_damage;
	region_without_alpha_on_top -= region_without_overlapped_window;


	_region_t slow_region = pending_damage;
	slow_region -= region_without_overlapped_window;
	slow_region -= region_without_alpha_on_top;


	/* direct render, area where there is only one window visible */
	{
		cairo_reset_clip(front_cr);
		cairo_set_operator(front_cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_antialias(front_cr, CAIRO_ANTIALIAS_NONE);

		_region_t::const_iterator i = region_without_overlapped_window.begin();
		while (i != region_without_overlapped_window.end()) {
			fast_region_surf_monitor += i->w * i->h;
			repair_buffer(visible, front_cr, *i);
			// for debuging
			_draw_crossed_box(front_cr, (*i), 0.0, 1.0, 0.0);
			++i;
		}
	}

	/* directly render area where window on the has no alpha */

	{
		cairo_reset_clip(front_cr);
		cairo_set_operator(front_cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_antialias(front_cr, CAIRO_ANTIALIAS_NONE);

		/* from top to bottom */
		for (list<composite_window_t *>::reverse_iterator i =
				visible.rbegin(); i != visible.rend(); ++i) {
			composite_window_t * r = *i;
			_region_t draw_area = region_without_alpha_on_top & r->get_region();
			if (!draw_area.empty()) {
				for (_region_t::const_iterator j = draw_area.begin();
						j != draw_area.end(); ++j) {
					if (!j->is_null()) {
						r->draw_to(front_cr, *j);
						/* this section show direct rendered screen */
						_draw_crossed_box(front_cr, *j, 0.0, 0.0, 1.0);
					}
				}
			}
			region_without_alpha_on_top -= r->get_region();
		}
	}

	/**
	 * To avoid glitch (blinking) I use back buffer to make the composition.
	 * update back buffer, render area with possible transparency
	 **/
	if (!slow_region.empty()) {

		cairo_t * back_cr = cairo_create(_bask_buffer);

		{

			cairo_reset_clip(back_cr);
			cairo_set_operator(back_cr, CAIRO_OPERATOR_OVER);
			_region_t::const_iterator i = slow_region.begin();
			while (i != slow_region.end()) {
				slow_region_surf_monitor += (*i).w * (*i).h;
				repair_buffer(visible, back_cr, *i);
				++i;
			}
		}

		{
			cairo_surface_flush(_bask_buffer);
			_region_t::const_iterator i = slow_region.begin();
			while (i != slow_region.end()) {
				repair_overlay(front_cr, *i, _bask_buffer);
				//_draw_crossed_box(front_cr, (*i), 1.0, 0.0, 0.0);
				++i;
			}
		}
		cairo_destroy(back_cr);

	}

	pending_damage.clear();

	cairo_destroy(front_cr);

	for (std::list<composite_window_t *>::iterator i = visible.begin();
			i != visible.end(); ++i) {
		(*i)->destroy_cairo();
	}

}


void compositor_t::repair_buffer(std::list<composite_window_t *> & visible,
		cairo_t * cr, _box_t const & area) {
	for (list<composite_window_t *>::iterator i = visible.begin();
			i != visible.end(); ++i) {
		composite_window_t * r = *i;
		_region_t const & barea = r->get_region();
		for (_region_t::const_iterator j = barea.begin(); j != barea.end();
				++j) {
			_box_t clip = area & (*j);
			if (!clip.is_null()) {
				r->draw_to(cr, clip);
			}
		}
	}
}


void compositor_t::repair_overlay(cairo_t * cr, _box_t const & area,
		cairo_surface_t * src) {

	cairo_reset_clip(cr);
	cairo_identity_matrix(cr);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_surface(cr, src, 0, 0);
	cairo_rectangle(cr, area.x, area.y, area.w, area.h);
	cairo_fill(cr);

	/* for debug purpose */
	if (false) {
		static int color = 0;
		switch (color % 3) {
		case 0:
			cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
			break;
		case 1:
			cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
			break;
		case 2:
			cairo_set_source_rgb(cr, 1.0, 0.0, 1.0);
			break;
		}
		//++color;
		cairo_set_line_width(cr, 1.0);
		cairo_rectangle(cr, area.x + 0.5, area.y + 0.5, area.w - 1.0,
				area.h - 1.0);
		cairo_clip(cr);
		cairo_paint_with_alpha(cr, 0.3);
		cairo_reset_clip(cr);
		//cairo_stroke(composite_overlay_cr);
	}

	// Never flush composite_overlay_s because this surface is never a source surface.
	//cairo_surface_flush(composite_overlay_s);

}


void compositor_t::damage_all() {
	p_window_attribute_t wa = _cnx->get_window_attributes(
			_cnx->get_root_window());
	pending_damage += (box_int_t(wa->x, wa->y, wa->width, wa->height));
}


void compositor_t::process_event(XCreateWindowEvent const & e) {
	if (e.window == _cnx->composite_overlay)
		return;
	if(e.parent != _cnx->get_root_window())
		return;
	stack_window_place_on_top(e.window);
}

void compositor_t::process_event(XReparentEvent const & e) {
	if(e.parent == _cnx->get_root_window()) {
		/** will be followed by map **/
		stack_window_place_on_top(e.window);
	} else {
		stack_window_remove(e.window);
	}
}

void compositor_t::process_event(XMapEvent const & e) {
	if(e.send_event)
		return;
	if(e.event != _cnx->get_root_window())
		return;

	/** don't make composite overlay visible **/
	if (e.window == _cnx->composite_overlay)
		return;

	p_window_attribute_t wa = _cnx->get_window_attributes(e.window);
	if (wa->is_valid) {
		if (wa->c_class == InputOutput) {
			window_data[e.window] = new composite_window_t(_cnx, e.window, wa);
		}
	}
}

void compositor_t::process_event(XUnmapEvent const & e) {
	map<Window, composite_window_t *>::iterator x = window_data.find(e.window);
	if (x != window_data.end()) {
		delete x->second;
		window_data.erase(x);
	}
}

void compositor_t::process_event(XDestroyWindowEvent const & e) {
	map<Window, composite_window_t *>::iterator x = window_data.find(e.window);
	if (x != window_data.end()) {
		delete x->second;
		window_data.erase(x);
	}

	stack_window_remove(e.window);
}

void compositor_t::process_event(XConfigureEvent const & e) {
	if (e.send_event)
		return;

	if (e.event == _cnx->get_root_window()
			&& e.window != _cnx->get_root_window()) {

		stack_window_update(e.window, e.above);

		map<Window, composite_window_t *>::iterator x = window_data.find(
				e.window);
		if (x != window_data.end()) {
			x->second->moved();
			//x->second->update_shape();
			//printf("XXXXX %s %s\n", old.to_string().c_str(), cur.to_string().c_str());
		}
	}
}


void compositor_t::process_event(XCirculateEvent const & e) {
	if(e.event != _cnx->get_root_window())
		return;

	if(e.place == PlaceOnTop) {
		stack_window_place_on_top(e.window);
	} else {
		stack_window_place_on_bottom(e.window);
	}

	if(has_key(window_data, e.window)) {
		window_data[e.window]->moved();
	}

}

void compositor_t::process_event(XDamageNotifyEvent const & e) {

//	printf("Damage area %dx%d+%d+%d\n", e.area.width, e.area.height, e.area.x,
//			e.area.y);
//	printf("Damage geometry %dx%d+%d+%d\n", e.geometry.width, e.geometry.height,
//			e.geometry.x, e.geometry.y);

	++damage_count;

	region_t<int> r = read_damaged_region(e.damage);

	if (e.drawable == _cnx->composite_overlay
			|| e.drawable == _cnx->get_root_window())
		return;

	map<Window, composite_window_t *>::iterator x = window_data.find(
			e.drawable);
	if (x == window_data.end())
		return;

	x->second->add_damaged_region(r);

}

region_t<int> compositor_t::read_damaged_region(Damage d) {

	region_t<int> result;

	/* create an empty region */
	XserverRegion region = XFixesCreateRegion(_cnx->dpy, 0, 0);

	if (!region)
		throw std::runtime_error("could not create region");

	/* get damaged region and remove them from damaged status */
	XDamageSubtract(_cnx->dpy, d, None, region);

	XRectangle * rects;
	int nb_rects;

	/* get all rectangles for the damaged region */
	rects = XFixesFetchRegion(_cnx->dpy, region, &nb_rects);

	if (rects) {
		for (int i = 0; i < nb_rects; ++i) {
			//printf("rect %dx%d+%d+%d\n", rects[i].width, rects[i].height, rects[i].x, rects[i].y);
			result += box_int_t(rects[i]);
		}
		XFree(rects);
	}

	XFixesDestroyRegion(_cnx->dpy, region);

	return result;
}

void compositor_t::scan() {

	unsigned int num = 0;
	Window d1, d2, *wins = 0;


	/**
	 * Start listen root event before anything each event will be stored to
	 * right run later.
	 **/
	XSelectInput(_cnx->dpy, _cnx->get_root_window(), SubstructureNotifyMask);

	window_stack.clear();
	window_data.clear();

	/** read all current root window **/
	if (XQueryTree(_cnx->dpy, _cnx->get_root_window(), &d1, &d2, &wins, &num)
			!= 0) {

		if (num > 0)
			window_stack.insert(window_stack.end(), &wins[0], &wins[num]);

		for (unsigned i = 0; i < num; ++i) {
			p_window_attribute_t wa = _cnx->get_window_attributes(wins[i]);
			if (wa->is_valid and wins[i] != _cnx->composite_overlay
					and wins[i] != _cnx->get_root_window()) {
				if (wa->c_class == InputOutput && wa->map_state != IsUnmapped) {
					window_data[wins[i]] = new composite_window_t(_cnx, wins[i],
							wa);
				}
			}
		}

		XFree(wins);
	}

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
			window_data[sev.window]->update_shape();
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
	save_state();
	while(_cnx->process_check_event())
		continue;
	render_flush();
}

void compositor_t::stack_window_update(Window window, Window above) {
	if (above == None) {
		window_stack.remove(window);
		window_stack.push_front(window);
	} else {

		std::list<Window>::iterator x = find(window_stack.begin(),
				window_stack.end(), above);

		if (x == window_stack.end()) {
			printf("Window not found : %lu for window %lu\n", above, window);
		} else {
			window_stack.remove(window);
			window_stack.insert(++x, window);
		}
	}
}

void compositor_t::stack_window_place_on_top(Window w) {
	window_stack.remove(w);
	window_stack.push_back(w);
}

void compositor_t::stack_window_place_on_bottom(Window w) {
	window_stack.remove(w);
	window_stack.push_front(w);
}

void compositor_t::stack_window_remove(Window w) {
	window_stack.remove(w);
}

void compositor_t::save_state() {

	compositor_window_state.clear();

	for (map<Window, composite_window_t *>::iterator i = window_data.begin();
			i != window_data.end(); ++i) {
		compositor_window_state[i->first] = i->second->get_region();
	}


}

/**
 * Compute damaged area from last render_flush
 **/
void compositor_t::compute_pending_damage() {

	for (map<Window, composite_window_t *>::iterator i = window_data.begin();
			i != window_data.end(); ++i) {

		map<Window, region_t<int> >::iterator x = compositor_window_state.find(
				i->first);

		/**
		 * if moved or re-stacked, add previous location and current
		 * location as damaged.
		 **/
		if (i->second->has_moved()) {

			if (x != compositor_window_state.end()) {
				pending_damage += x->second;
			}

			i->second->update_shape();
			pending_damage += i->second->get_region();
		}

		/**
		 * What ever is happen, add damaged area
		 **/
		pending_damage += i->second->get_damaged_region();

		if (x != compositor_window_state.end()) {
			compositor_window_state.erase(x);
		}

		/**
		 * reset damaged, and moved flags
		 **/
		i->second->clear_state();

	}

	/**
	 * if compositor state is not empty, that mean this window where
	 * unmapped. repair corresponding area.
	 **/
	for (map<Window, region_t<int> >::iterator x =
			compositor_window_state.begin(); x != compositor_window_state.end();
			++x) {
		pending_damage += x->second;
	}

	compositor_window_state.clear();

}

}

