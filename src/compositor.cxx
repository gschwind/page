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

static void _draw_crossed_box(cairo_t * cr, box_t<int> const & box, double r, double g,
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

/**
 * Register composite manager. if another one is in place just fail to take
 * the ownership.
 **/
bool compositor_t::register_cm(Window w) {
	Window current_cm;
	Atom a_cm;
	static char net_wm_cm[] = "_NET_WM_CM_Sxx";
	snprintf(net_wm_cm, sizeof(net_wm_cm), "_NET_WM_CM_S%d",
			DefaultScreen(_dpy));
	a_cm = XInternAtom(_dpy, net_wm_cm, False);

	/** read if there is a compositor **/
	current_cm = XGetSelectionOwner(_dpy, a_cm);
	if (current_cm != None) {
		printf("Another composite manager is running\n");
		return false;
	} else {

		/** become the compositor **/
		XSetSelectionOwner(_dpy, a_cm, w, CurrentTime);

		/** check is we realy are the current compositor **/
		if (XGetSelectionOwner(_dpy, a_cm) != w) {
			printf("Could not acquire window manager selection on screen %d\n",
					DefaultScreen(_dpy));
			return false;
		}

		printf("Composite manager is registered on screen %d\n",
				DefaultScreen(_dpy));
		return true;
	}

}

void compositor_t::init_composite_overlay() {
	/* map & passthrough the overlay */
	composite_overlay = XCompositeGetOverlayWindow(_dpy, DefaultRootWindow(_dpy));
	allow_input_passthrough(_dpy, composite_overlay);

	/** Automatically redirect windows, but paint window manually */
	XCompositeRedirectSubwindows(_dpy, DefaultRootWindow(_dpy), CompositeRedirectManual);
}

compositor_t::compositor_t() {

	fade_length = new_timespec(0, 500000000);

	old_error_handler = XSetErrorHandler(error_handler);

	_dpy = XOpenDisplay(0);
	if (_dpy == NULL) {
		throw std::runtime_error("Could not open display");
	} else {
		printf("Open display : Success\n");
	}

	if (!check_composite_extension(_dpy, &composite_opcode, &composite_event,
			&composite_error)) {
		throw std::runtime_error("X Server doesn't support Composite 0.4");
	}

	if (!check_damage_extension(_dpy, &damage_opcode, &damage_event,
			&damage_error)) {
		throw std::runtime_error("Damage extension is not supported");
	}

	if (!check_shape_extension(_dpy, &xshape_opcode, &xshape_event,
			&xshape_error)) {
		throw std::runtime_error(SHAPENAME " extension is not supported");
	}

	if (!check_randr_extension(_dpy, &xrandr_opcode, &xrandr_event,
			&xrandr_error)) {
		throw std::runtime_error(RANDR_NAME " extension is not supported");
	}

	A = atom_handler_t(_dpy);

	/* create an invisible window to identify page */
	cm_window = XCreateSimpleWindow(_dpy, DefaultRootWindow(_dpy), -100,
			-100, 1, 1, 0, 0, 0);
	if (cm_window == None) {
		XCloseDisplay(_dpy);
		throw compositor_fail_t();
	}

	char const * name = "page_internal_compositor";
	XChangeProperty(_dpy, cm_window, A(_NET_WM_NAME), A(UTF8_STRING), 8,
	PropModeReplace, reinterpret_cast<unsigned char const *>(name),
			strlen(name) + 1);

	long pid = getpid();

	XChangeProperty(_dpy, cm_window, A(_NET_WM_PID), A(CARDINAL), 32,
			(PropModeReplace), reinterpret_cast<unsigned char const *>(&pid),
			1);

	/* try to register compositor manager */
	if (!register_cm(cm_window)) {
		XCloseDisplay(_dpy);
		throw compositor_fail_t();
	}



	/* initialize composite */
	init_composite_overlay();

	/** some performance counter **/
	fast_region_surf_monitor = 0.0;
	slow_region_surf_monitor = 0.0;

	flush_count = 0;
	damage_count = 0;
	clock_gettime(CLOCK_MONOTONIC, &last_tic);


	/** update the windows list **/
	scan();


	_front_buffer = 0;
	_bask_buffer = 0;

	XRRSelectInput(_dpy, DefaultRootWindow(_dpy), RRCrtcChangeNotifyMask);
	update_layout();

	/**
	 * Add the composite window to the stack as invisible window to track a
	 * valid stack. scan() currently does not report this window, I add remove
	 * line, in case of scan will report it.
	 **/
	stack_window_place_on_top(composite_overlay);

}

compositor_t::~compositor_t() {

	map<Window, composite_window_t *>::iterator i = window_data.begin();
	while(i != window_data.end()) {
		delete i->second;
		++i;
	}

	XCloseDisplay(_dpy);

};

//void compositor_t::render_flush() {
//	flush_count += 1;
//
//	clock_gettime(CLOCK_MONOTONIC, &curr_tic);
//	if (last_tic.tv_sec + 30 < curr_tic.tv_sec) {
//
//		double t0 = last_tic.tv_sec + last_tic.tv_nsec / 1.0e9;
//		double t1 = curr_tic.tv_sec + curr_tic.tv_nsec / 1.0e9;
//
//		printf("render flush per second : %0.2f\n", flush_count / (t1 - t0));
//		printf("damage per second : %0.2f\n", damage_count / (t1 - t0));
//
//		last_tic = curr_tic;
//		flush_count = 0;
//		damage_count = 0;
//	}
//
//	compute_pending_damage();
//
//	/** this line divide page cpu cost by 4.0 ... O_O it's insane **/
//	if(pending_damage.empty())
//		return;
//
//	/**
//	 * list content is bottom window to upper window in stack a optimization.
//	 **/
//	std::list<composite_window_t *> visible;
//	for (std::list<Window>::iterator i = window_stack.begin();
//			i != window_stack.end(); ++i) {
//		map<Window, composite_window_t *>::iterator x = window_data.find(*i);
//		if (x != window_data.end()) {
//			if (x->second->map_state() != IsUnmapped
//					and x->second->c_class() == InputOutput) {
//				visible.push_back(x->second);
//			}
//		}
//	}
//
//	/**
//	 * Asked to cairo community, build/destroy cairo contexts and surfaces
//	 * should not hit too much performance.
//	 **/
//
//	cairo_t * front_cr = cairo_create(_front_buffer);
//
//	for (std::list<composite_window_t *>::iterator i = visible.begin();
//			i != visible.end(); ++i) {
//		(*i)->update_cairo();
//	}
//
//	/* clip damage region to visible region */
//	pending_damage = pending_damage & _desktop_region;
//
//	/**
//	 * Find region where windows are not overlap each other. i.e. region where
//	 * only one window will be rendered.
//	 * This is often more than 80% of the screen.
//	 * This kind of region will be directly rendered.
//	 **/
//
//	region_t<int> region_without_overlapped_window;
//	for (list<composite_window_t *>::iterator i = visible.begin();
//			i != visible.end(); ++i) {
//		region_t<int> r = (*i)->get_region();
//		for (list<composite_window_t *>::iterator j = visible.begin();
//				j != visible.end(); ++j) {
//			if (i != j) {
//				r -= (*j)->get_region();
//			}
//		}
//		region_without_overlapped_window += r;
//	}
//
//	region_without_overlapped_window = region_without_overlapped_window
//			& pending_damage;
//
//	/**
//	 * Find region where the windows on top is not a window with alpha.
//	 * Small area, often dropdown menu.
//	 * This kind of area will be directly rendered.
//	 **/
//	region_t<int> region_without_alpha_on_top;
//
//	/**
//	 * Walk over all all window from bottom to top one. If window has alpha
//	 * channel remove it from region with no alpha, if window do not have alpha
//	 * add this window to the area.
//	 **/
//	for (list<composite_window_t *>::iterator i = visible.begin();
//			i != visible.end(); ++i) {
//		if ((*i)->has_alpha()) {
//			region_without_alpha_on_top -= (*i)->get_region();
//		} else {
//			/* if not has_alpha, add this area */
//			region_without_alpha_on_top += (*i)->get_region();
//		}
//	}
//
//	region_without_alpha_on_top = region_without_alpha_on_top & pending_damage;
//	region_without_alpha_on_top -= region_without_overlapped_window;
//
//
//	region_t<int> slow_region = pending_damage;
//	slow_region -= region_without_overlapped_window;
//	slow_region -= region_without_alpha_on_top;
//
//
//	/* direct render, area where there is only one window visible */
//	{
//		cairo_reset_clip(front_cr);
//		cairo_set_operator(front_cr, CAIRO_OPERATOR_SOURCE);
//		cairo_set_antialias(front_cr, CAIRO_ANTIALIAS_NONE);
//
//		region_t<int>::const_iterator i = region_without_overlapped_window.begin();
//		while (i != region_without_overlapped_window.end()) {
//			fast_region_surf_monitor += i->w * i->h;
//			repair_buffer(visible, front_cr, *i);
//			// for debuging
//			_draw_crossed_box(front_cr, (*i), 0.0, 1.0, 0.0);
//			++i;
//		}
//	}
//
//	/* directly render area where window on the has no alpha */
//
//	{
//		cairo_reset_clip(front_cr);
//		cairo_set_operator(front_cr, CAIRO_OPERATOR_SOURCE);
//		cairo_set_antialias(front_cr, CAIRO_ANTIALIAS_NONE);
//
//		/* from top to bottom */
//		for (list<composite_window_t *>::reverse_iterator i =
//				visible.rbegin(); i != visible.rend(); ++i) {
//			composite_window_t * r = *i;
//			region_t<int> draw_area = region_without_alpha_on_top & r->get_region();
//			if (!draw_area.empty()) {
//				for (region_t<int>::const_iterator j = draw_area.begin();
//						j != draw_area.end(); ++j) {
//					if (!j->is_null()) {
//						r->draw_to(front_cr, *j);
//						/* this section show direct rendered screen */
//						_draw_crossed_box(front_cr, *j, 0.0, 0.0, 1.0);
//					}
//				}
//			}
//			region_without_alpha_on_top -= r->get_region();
//		}
//	}
//
//	/**
//	 * To avoid glitch (blinking) I use back buffer to make the composition.
//	 * update back buffer, render area with possible transparency
//	 **/
//	if (!slow_region.empty()) {
//
//		cairo_t * back_cr = cairo_create(_bask_buffer);
//
//		{
//
//			cairo_reset_clip(back_cr);
//			cairo_set_operator(back_cr, CAIRO_OPERATOR_OVER);
//			region_t<int>::const_iterator i = slow_region.begin();
//			while (i != slow_region.end()) {
//				slow_region_surf_monitor += (*i).w * (*i).h;
//				repair_buffer(visible, back_cr, *i);
//				++i;
//			}
//		}
//
//		{
//			cairo_surface_flush(_bask_buffer);
//			region_t<int>::const_iterator i = slow_region.begin();
//			while (i != slow_region.end()) {
//				repair_overlay(front_cr, *i, _bask_buffer);
//				//_draw_crossed_box(front_cr, (*i), 1.0, 0.0, 0.0);
//				++i;
//			}
//		}
//		cairo_destroy(back_cr);
//
//	}
//
//	pending_damage.clear();
//
//	cairo_destroy(front_cr);
//
////	for (std::list<composite_window_t *>::iterator i = visible.begin();
////			i != visible.end(); ++i) {
////		(*i)->destroy_cairo();
////	}
//
//}

void compositor_t::repair_area_region(region_t<int> const & repair) {

	region_t<int> pending_damage = repair;

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
			if (x->second->map_state() != IsUnmapped
					and x->second->c_class() == InputOutput) {
				visible.push_back(x->second);
			}
		}
	}

	//printf("SIZE = %d\n", visible.size());

	/**
	 * Asked to cairo community, build/destroy cairo contexts and surfaces
	 * should not hit too much performance.
	 **/

	cairo_t * front_cr = cairo_create(_front_buffer);

	for (std::list<composite_window_t *>::iterator i = visible.begin();
			i != visible.end(); ++i) {
		(*i)->update_cairo();

	}

	/* clip damage region to visible region */
	pending_damage = pending_damage & _desktop_region;

	/**
	 * Find region where windows are not overlap each other. i.e. region where
	 * only one window will be rendered.
	 * This is often more than 80% of the screen.
	 * This kind of region will be directly rendered.
	 **/

	region_t<int> region_without_overlapped_window;
	for (list<composite_window_t *>::iterator i = visible.begin();
			i != visible.end(); ++i) {
		region_t<int> r = (*i)->get_region();
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
	region_t<int> region_without_alpha_on_top;

	/**
	 * Walk over all all window from bottom to top one. If window has alpha
	 * channel remove it from region with no alpha, if window do not have alpha
	 * add this window to the area.
	 **/
	for (list<composite_window_t *>::iterator i = visible.begin();
			i != visible.end(); ++i) {
		if ((*i)->has_alpha() or (*i)->fade_in_step < 1.0) {
			region_without_alpha_on_top -= (*i)->get_region();
		} else {
			/* if not has_alpha, add this area */
			region_without_alpha_on_top += (*i)->get_region();
		}
	}

	region_without_alpha_on_top = region_without_alpha_on_top & pending_damage;
	region_without_alpha_on_top -= region_without_overlapped_window;


	region_t<int> slow_region = pending_damage;
	slow_region -= region_without_overlapped_window;
	slow_region -= region_without_alpha_on_top;


	/* direct render, area where there is only one window visible */
	{
		cairo_reset_clip(front_cr);
		cairo_set_operator(front_cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_antialias(front_cr, CAIRO_ANTIALIAS_NONE);

		region_t<int>::const_iterator i = region_without_overlapped_window.begin();
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
			region_t<int> draw_area = region_without_alpha_on_top & r->get_region();
			if (!draw_area.empty()) {
				for (region_t<int>::const_iterator j = draw_area.begin();
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
			region_t<int>::const_iterator i = slow_region.begin();
			while (i != slow_region.end()) {
				slow_region_surf_monitor += (*i).w * (*i).h;
				repair_buffer(visible, back_cr, *i);
				++i;
			}
		}

		{
			cairo_surface_flush(_bask_buffer);
			region_t<int>::const_iterator i = slow_region.begin();
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

//	for (std::list<composite_window_t *>::iterator i = visible.begin();
//			i != visible.end(); ++i) {
//		(*i)->destroy_cairo();
//	}

}


void compositor_t::repair_buffer(std::list<composite_window_t *> & visible,
		cairo_t * cr, box_t<int> const & area) {
	for (list<composite_window_t *>::iterator i = visible.begin();
			i != visible.end(); ++i) {
		composite_window_t * r = *i;
		region_t<int> const & barea = r->get_region();
		for (region_t<int>::const_iterator j = barea.begin(); j != barea.end();
				++j) {
			box_t<int> clip = area & (*j);
			if (!clip.is_null()) {
				r->draw_to(cr, clip);
			}
		}
	}
}


void compositor_t::repair_overlay(cairo_t * cr, box_t<int> const & area,
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


void compositor_t::process_event(XCreateWindowEvent const & e) {
	if (e.window == composite_overlay)
		return;
	if (e.parent != DefaultRootWindow(_dpy))
		return;

	XWindowAttributes wa;
	if (XGetWindowAttributes(_dpy, e.window, &wa)) {
		window_data[e.window] = new composite_window_t(_dpy, e.window, &wa,
				window_stack.empty() ? None : window_stack.back());
	}
	stack_window_place_on_top(e.window);
}

void compositor_t::process_event(XReparentEvent const & e) {
	if (e.parent == DefaultRootWindow(_dpy)) {
		/** will be followed by map **/
		XWindowAttributes wa;
		if (XGetWindowAttributes(_dpy, e.window, &wa)) {
			window_data[e.window] = new composite_window_t(_dpy, e.window, &wa,
					window_stack.empty() ? None : window_stack.back());
			repair_area_region(window_data[e.window]->position());
		}
		stack_window_place_on_top(e.window);
	} else {
		stack_window_remove(e.window);
	}
}

void compositor_t::process_event(XMapEvent const & e) {
	if (e.send_event)
		return;
	if (e.event != DefaultRootWindow(_dpy))
		return;

	/** don't make composite overlay visible **/
	if (e.window == composite_overlay)
		return;

	map<Window, composite_window_t *>::iterator x = window_data.find(e.window);
	if (x != window_data.end()) {
		x->second->update_map_state(IsViewable);
		x->second->fade_in_step = 0.0;
		x->second->fade_start = curr_tic;
		repair_area_region(x->second->position());
	}
}

void compositor_t::process_event(XUnmapEvent const & e) {
	map<Window, composite_window_t *>::iterator x = window_data.find(e.window);
	if (x != window_data.end()) {
		x->second->update_map_state(IsUnmapped);
		x->second->fade_in_step = 0.0;
		x->second->fade_start = new_timespec(0, 0);
		repair_area_region(x->second->position());
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

	if (e.event == DefaultRootWindow(_dpy)
			&& e.window != DefaultRootWindow(_dpy)) {

		stack_window_update(e.window, e.above);

		map<Window, composite_window_t *>::iterator x = window_data.find(
				e.window);
		if (x != window_data.end()) {
			region_t<int> r = x->second->position();
			x->second->update_position(e);
			r += x->second->position();
			if (x->second->map_state() != IsUnmapped) {
				repair_area_region(r);
			}
		}
	}
}


void compositor_t::process_event(XCirculateEvent const & e) {
	if(e.event != DefaultRootWindow(_dpy))
		return;

	if(e.place == PlaceOnTop) {
		stack_window_place_on_top(e.window);
	} else {
		stack_window_place_on_bottom(e.window);
	}

	if(has_key(window_data, e.window)) {
		window_data[e.window]->moved();

		if (window_data[e.window]->map_state() != IsUnmapped)
			repair_area_region(window_data[e.window]->position());
	}



}

void compositor_t::process_event(XDamageNotifyEvent const & e) {

//	printf("Damage area %dx%d+%d+%d\n", e.area.width, e.area.height, e.area.x,
//			e.area.y);
//	printf("Damage geometry %dx%d+%d+%d\n", e.geometry.width, e.geometry.height,
//			e.geometry.x, e.geometry.y);

	++damage_count;

	region_t<int> r = read_damaged_region(e.damage);

	if (e.drawable == composite_overlay
			|| e.drawable == DefaultRootWindow(_dpy))
		return;

	map<Window, composite_window_t *>::iterator x = window_data.find(
			e.drawable);
	if (x == window_data.end())
		return;

	//x->second->add_damaged_region(r);

	r.translate(x->second->position().x, x->second->position().y);
	repair_area_region(r);

}

region_t<int> compositor_t::read_damaged_region(Damage d) {

	region_t<int> result;

	/* create an empty region */
	XserverRegion region = XFixesCreateRegion(_dpy, 0, 0);

	if (!region)
		throw std::runtime_error("could not create region");

	/* get damaged region and remove them from damaged status */
	XDamageSubtract(_dpy, d, None, region);

	XRectangle * rects;
	int nb_rects;

	/* get all rectangles for the damaged region */
	rects = XFixesFetchRegion(_dpy, region, &nb_rects);

	if (rects) {
		for (int i = 0; i < nb_rects; ++i) {
			//printf("rect %dx%d+%d+%d\n", rects[i].width, rects[i].height, rects[i].x, rects[i].y);
			result += box_int_t(rects[i]);
		}
		XFree(rects);
	}

	XFixesDestroyRegion(_dpy, region);

	return result;
}

void compositor_t::scan() {

	unsigned int num = 0;
	Window d1, d2, *wins = 0;

	/**
	 * Start listen root event before anything each event will be stored to
	 * right run later.
	 **/
	XSelectInput(_dpy, DefaultRootWindow(_dpy), SubstructureNotifyMask);

	window_stack.clear();
	window_data.clear();

	/** read all current root window **/
	if (XQueryTree(_dpy, DefaultRootWindow(_dpy), &d1, &d2, &wins, &num)
			!= 0) {

		if (num > 0)
			window_stack.insert(window_stack.end(), &wins[0], &wins[num]);

		for (unsigned i = 0; i < num; ++i) {
			if(wins[i] == composite_overlay)
				continue;
			XWindowAttributes wa;
			if (XGetWindowAttributes(_dpy, wins[i], &wa)) {
				window_data[wins[i]] = new composite_window_t(_dpy, wins[i],
						&wa, i == 0 ? None : wins[i - 1]);
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
	} else if (e.type == damage_event + XDamageNotify) {
		process_event(reinterpret_cast<XDamageNotifyEvent const &>(e));
	} else if (e.type == xshape_event + ShapeNotify) {
		XShapeEvent const & sev = reinterpret_cast<XShapeEvent const &>(e);
		if(has_key(window_data, sev.window)) {
			window_data[sev.window]->update_shape();
		}
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

	XGetWindowAttributes(_dpy, DefaultRootWindow(_dpy), &root_attributes);

	destroy_cairo();
	init_cairo();

	_desktop_region.clear();

	XRRScreenResources * resources = XRRGetScreenResourcesCurrent(_dpy,
			DefaultRootWindow(_dpy));

	for (int i = 0; i < resources->ncrtc; ++i) {
		XRRCrtcInfo * info = XRRGetCrtcInfo(_dpy, resources,
				resources->crtcs[i]);

		/** if the CRTC has at less one output bound **/
		if(info->noutput > 0) {
			box_t<int> area(info->x, info->y, info->width, info->height);
			_desktop_region = _desktop_region + area;
		}
		XRRFreeCrtcInfo(info);
	}
	XRRFreeScreenResources(resources);

	printf("layout = %s\n", _desktop_region.to_string().c_str());
}


void compositor_t::process_events() {

	clock_gettime(CLOCK_MONOTONIC, &curr_tic);

	XEvent ev;
	while(XPending(_dpy)) {
		XNextEvent(_dpy, &ev);
		process_event(ev);
	}


	for (map<Window, composite_window_t *>::iterator i = window_data.begin();
			i != window_data.end(); ++i) {
		if(i->second->map_state() == IsUnmapped)
			continue;

		if (i->second->fade_start + fade_length > curr_tic) {
			struct timespec diff = pdiff(curr_tic, i->second->fade_start);
			double alpha = (diff.tv_sec + diff.tv_nsec / 1.0e9)
					/ (fade_length.tv_sec + fade_length.tv_nsec / 1.0e9);
			i->second->fade_in_step = alpha;
			repair_area_region(i->second->get_region());
		} else if (i->second->fade_in_step < 1.0) {
			i->second->fade_in_step = 1.0;
			repair_area_region(i->second->get_region());
		}
	}

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

//void compositor_t::save_state() {
//
//	compositor_window_state.clear();
//
//	for (map<Window, composite_window_t *>::iterator i = window_data.begin();
//			i != window_data.end(); ++i) {
//		compositor_window_state[i->first] = i->second->get_region();
//	}
//
//
//}

/**
 * Compute damaged area from last render_flush
 **/
//void compositor_t::compute_pending_damage() {
//
//	for (map<Window, composite_window_t *>::iterator i = window_data.begin();
//			i != window_data.end(); ++i) {
//
//		composite_window_t * cw = i->second;
//
//		if (cw->c_class() != InputOutput) {
//			continue;
//		}
//
//		if (cw->old_map_state() == IsUnmapped) {
//			if(cw->map_state() == IsUnmapped) {
//				continue;
//			} else {
//				/** was mapped **/
//				pending_damage += cw->get_region();
//			}
//		} else {
//			if(cw->map_state() == IsUnmapped) {
//				/** was unmapped **/
//				pending_damage += cw->old_region();
//			} else {
//				if (cw->has_moved()) {
//					/**
//					 * if moved or re-stacked, add previous location and current
//					 * location as damaged.
//					 **/
//					pending_damage += cw->old_region();
//					pending_damage += cw->get_region();
//				} else {
//					/**
//					 * What ever is happen, add damaged area
//					 **/
//					pending_damage += cw->get_damaged_region();
//				}
//			}
//		}
//
//		/**
//		 * reset damaged, and moved flags
//		 **/
//		i->second->clear_state();
//
//	}
//
//}


/**
 * Check event without blocking.
 **/
bool compositor_t::process_check_event() {
	XEvent e;

	int x;
	/** Passive check of events in queue, never flush any thing **/
	if ((x = XEventsQueued(_dpy, QueuedAlready)) > 0) {
		/** should not block or flush **/
		XNextEvent(_dpy, &e);
		process_event(e);
		return true;
	} else {
		return false;
	}
}

void compositor_t::destroy_cairo() {
	if(_front_buffer != 0) {
		cairo_surface_destroy(_front_buffer);
		_front_buffer = 0;
	}

	if(_bask_buffer != 0) {
		cairo_surface_destroy(_bask_buffer);
		_bask_buffer = 0;
	}
}

void compositor_t::init_cairo() {
	_front_buffer = cairo_xlib_surface_create(_dpy, composite_overlay,
			root_attributes.visual, root_attributes.width,
			root_attributes.height);

	_bask_buffer = cairo_surface_create_similar(_front_buffer,
			CAIRO_CONTENT_COLOR, root_attributes.width, root_attributes.height);

}

}

