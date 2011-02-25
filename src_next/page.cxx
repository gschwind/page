/*
 * page.cxx
 *
 *  Created on: 23 févr. 2011
 *      Author: gschwind
 */

#include "page.hxx"

namespace page_next {

char const * x_event_name[LASTEvent] = { 0, 0, "KeyPress", "KeyRelease",
		"ButtonPress", "ButtonRelease", "MotionNotify", "EnterNotify",
		"LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose",
		"GraphicsExpose", "NoExpose", "VisibilityNotify", "CreateNotify",
		"DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
		"ReparentNotify", "ConfigureNotify", "ConfigureRequest",
		"GravityNotify", "ResizeRequest", "CirculateNotify",
		"CirculateRequest", "PropertyNotify", "SelectionClear",
		"SelectionRequest", "SelectionNotify", "ColormapNotify",
		"ClientMessage", "MappingNotify", "GenericEvent" };

root_t::root_t() {
	XSetWindowAttributes swa;
	this->dpy = XOpenDisplay(0);
	this->screen = DefaultScreen(dpy);
	this->xroot = DefaultRootWindow(dpy);
	assert(dpy);
	// Create the window
	this->x_main_window = XCreateWindow(this->dpy, this->xroot, 0, 0, 800, 600,
			0, DefaultDepth(this->dpy, this->screen), InputOutput,
			DefaultVisual(this->dpy, this->screen), 0, &swa);
	XSelectInput(this->dpy, this->x_main_window, StructureNotifyMask
			| ButtonPressMask);
	XMapWindow(this->dpy, this->x_main_window);

	box_t allocation;
	allocation.x = 0;
	allocation.y = 0;
	allocation.width = 800;
	allocation.height = 600;

	vpan = new tree_t(dpy, x_main_window);
	vpan->mutate_to_split();
	vpan->add_notebook(new client_t("Asdl;kfj ;lafsdja;sdf"));
	vpan->add_notebook(new client_t("bsdl;kfj ;lafsdja;sdf"));
	vpan->add_notebook(new client_t("Csdl;kfj ;lafsdja;sdf"));
	vpan->add_notebook(new client_t("Dsdl;kfj ;lafsdja;sdf"));
	vpan->add_notebook(new client_t("Esdl;kfj ;lafsdja;sdf"));
	vpan->add_notebook(new client_t("Fsdl;kfj ;lafsdja;sdf"));
	vpan->add_notebook(new client_t("Gsdl;kfj ;lafsdja;sdf"));
	vpan->add_notebook(new client_t("Hsdl;kfj ;lafsdja;sdf"));
	vpan->update_allocation(allocation);

}

void root_t::run() {
	this->running = 1;
	while (running) {
		XEvent e;
		XNextEvent(this->dpy, &e);
		printf("Event %s\n", x_event_name[e.type]);
		if (e.type == MapNotify || e.type == Expose) {
			this->render();
		} else if (e.type == ButtonPress) {
			vpan->process_button_press_event(&e);
			render();
		}
	}
}

void root_t::render(cairo_t * cr) {
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_rectangle(cr, 0, 0, wa.width, wa.height);
	cairo_fill(cr);
	vpan->render(cr);
}

void root_t::render() {
	cairo_t * cr = get_cairo();
	render(cr);
	cairo_destroy(cr);
}

}
