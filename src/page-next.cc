/*
 * page-next.cc
 *
 *  Created on: 20 févr. 2011
 *      Author: gschwind
 */

#include <X11/X.h>
#include <X11/cursorfont.h>
#include <assert.h>
#include <unistd.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include <stdio.h>

#include <list>
#include <string>
#include <iostream>

namespace page {

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

struct box_t {
	int x, y;
	int width, height;
};

class page_widget_t {
protected:
	page_widget_t * _parent;
public:
	page_widget_t(page_widget_t * parent) {
		_parent = parent;
	}
	virtual void update_allocation(box_t & alloc) = 0;
	virtual void render(cairo_t * cr) = 0;
	virtual Window get_window() = 0;
	virtual Display * get_dpy() = 0;
	virtual cairo_t * get_cairo() = 0;
	page_widget_t * get_parent() {
		return _parent;
	}
};

class vpan_t: public page_widget_t {
	box_t allocation, pack0_alloc, pack1_alloc;
	double split;
	Cursor cursor;
	page_widget_t * pack0;
	page_widget_t * pack1;
	vpan_t(vpan_t const &);
	vpan_t &operator=(vpan_t const &);
public:
	vpan_t(page_widget_t * parent, page_widget_t * pack0, page_widget_t * pack1) :
		page_widget_t(parent->get_parent()) {

		this->pack0 = pack0;
		this->pack1 = pack1;
		split = 0.7;
		cursor = XCreateFontCursor(this->get_dpy(), XC_fleur);
	}
	void update_allocation(box_t & alloc) {
		allocation = alloc;
		pack0_alloc.x = allocation.x;
		pack0_alloc.y = allocation.y;
		pack0_alloc.width = allocation.width * split - 2;
		pack0_alloc.height = allocation.height;
		pack1_alloc.x = allocation.x + allocation.width * split + 2;
		pack1_alloc.y = allocation.y;
		pack1_alloc.width = allocation.width - allocation.width * split - 5;
		pack1_alloc.height = allocation.height;
		this->pack0->update_allocation(pack0_alloc);
		this->pack1->update_allocation(pack1_alloc);
	}

	void render(cairo_t * cr) {
		cairo_rectangle(cr, allocation.x, allocation.y, allocation.width,
				allocation.height);
		cairo_clip(cr);
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_rectangle(cr, allocation.x + (allocation.width * split) - 2,
				allocation.y, 4, allocation.height);
		cairo_fill(cr);
		pack0->render(cr);
		pack1->render(cr);

	}

	void process_button_press_event(XEvent const * e) {
		if (e->xbutton.y > allocation.y && e->xbutton.y < allocation.y
				+ allocation.height) {
			if (allocation.x + (allocation.width * split) - 2 < e->xbutton.x
					&& allocation.x + (allocation.width * split) + 2
							> e->xbutton.x) {
				movemouse(e->xbutton.x, e->xbutton.y);

			}
		}
	}

	void movemouse(int x, int y) {
		XEvent ev;
		cairo_t * cr;

		if (XGrabPointer(this->get_dpy(), this->get_window(), False,
				(ButtonPressMask | ButtonReleaseMask | PointerMotionMask),
				GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime)
				!= GrabSuccess)
			return;
		do {
			XMaskEvent(this->get_dpy(), (ButtonPressMask | ButtonReleaseMask
					|PointerMotionMask) | ExposureMask
					|SubstructureRedirectMask, &ev);
			switch (ev.type) {
			case ConfigureRequest:
			case Expose:
			case MapRequest:
				cr = this->get_cairo();
				cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
				cairo_rectangle(cr, allocation.x, allocation.y,
						allocation.width, allocation.height);
				cairo_fill(cr);
				this->render(cr);
				cairo_destroy(cr);
				break;
			case MotionNotify:
				split = (ev.xmotion.x - allocation.x)
						/ (double) (allocation.width);
				this->update_allocation(allocation);
				cr = this->get_cairo();
				cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
				cairo_rectangle(cr, allocation.x, allocation.y,
						allocation.width, allocation.height);
				cairo_fill(cr);
				this->render(cr);
				cairo_destroy(cr);
				break;
			}
		} while (ev.type != ButtonRelease);
		XUngrabPointer(this->get_dpy(), CurrentTime);
	}

	Window get_window() {
		return _parent->get_window();
	}

	Display * get_dpy() {
		return _parent->get_dpy();
	}

	cairo_t * get_cairo() {
		return _parent->get_cairo();
	}

};

class notebook_tmp {
public:
	std::string name;
	notebook_tmp(Display * dpy, Window parent, std::string name) :
		name(name) {

	}

	void render(cairo_t * cr, int selected) {
		cairo_surface_t * surf;
		if (selected) {
			cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
		} else {
			cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
		}
		cairo_paint(cr);
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(cr, 13);
		cairo_move_to(cr, 2, 13);
		cairo_show_text(cr, this->name.c_str());
	}
};

class client_t {
	std::string name;
public:
	client_t(char const * name) :
		name(name) {
	}
	inline std::string const & get_name() {
		return name;
	}
};

class notebook_t: public page_widget_t {
public:
	static std::list<notebook_t *> note_link;
private:
	Cursor cursor;
	cairo_surface_t * close_img;
	cairo_surface_t * vsplit_img;
	cairo_surface_t * hsplit_img;

	box_t allocation;
	int selected;
	std::list<client_t *> clients;

	notebook_t(notebook_t const &);
	notebook_t &operator=(notebook_t const &);
public:
	notebook_t(page_widget_t * parent) :
		page_widget_t(parent->get_parent()) {
		this->close_img = cairo_image_surface_create_from_png(
				"/home/gschwind/page/data/close.png");
		this->hsplit_img = cairo_image_surface_create_from_png(
				"/home/gschwind/page/data/hsplit.png");
		this->vsplit_img = cairo_image_surface_create_from_png(
				"/home/gschwind/page/data/vsplit.png");

		notebook_t::note_link.push_back(this);
		cursor = XCreateFontCursor(this->get_dpy(), XC_fleur);

	}
	void add_notebook(client_t * c) {
		clients.push_back(c);
	}
	void select_notebook(int n) {
		if (n >= 0 && n < clients.size())
			selected = n;
		else
			selected = 0;
	}

	void render(cairo_t * cr) {

		cairo_rectangle(cr, allocation.x, allocation.y, allocation.width,
				allocation.height);
		cairo_clip(cr);
		cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
		cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(cr, 13);
		std::list<client_t *>::iterator i;
		int offset = allocation.x;
		int length = (allocation.width - 17 * 3) / clients.size();
		int s = 0;
		for (i = clients.begin(); i != clients.end(); ++i, ++s) {
			cairo_save(cr);
			cairo_translate(cr, offset, allocation.y);
			cairo_rectangle(cr, 0, 0, length - 1, 19);
			cairo_clip(cr);
			offset += length;
			if (selected == s) {
				cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
			} else {
				cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
			}
			cairo_paint(cr);
			cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
			cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
					CAIRO_FONT_WEIGHT_BOLD);
			cairo_set_font_size(cr, 13);
			cairo_move_to(cr, 2, 13);
			cairo_show_text(cr, (*i)->get_name().c_str());
			cairo_restore(cr);
		}

		cairo_save(cr);
		cairo_translate(cr, allocation.x + allocation.width - 16.0,
				allocation.y + 1.0);
		cairo_set_source_surface(cr, this->close_img, 0.0, 0.0);
		cairo_paint(cr);
		cairo_translate(cr, -17.0, 0.0);
		cairo_set_source_surface(cr, this->vsplit_img, 0.0, 0.0);
		cairo_paint(cr);
		cairo_translate(cr, -17.0, 0.0);
		cairo_set_source_surface(cr, this->hsplit_img, 0.0, 0.0);
		cairo_paint(cr);
		cairo_restore(cr);
		cairo_reset_clip(cr);
	}

	int is_selected(int x, int y) {
		if (allocation.x < x && allocation.x + allocation.width > x
				&& allocation.y < y && allocation.y + allocation.height > y) {
			return 1;
		}
		return 0;
	}

	void process_button_press_event(XEvent const * e) {
		if ((e->xbutton.y - allocation.y) < 20) {
			int s = (e->xbutton.x - allocation.x)
					/ ((allocation.width - 17 * 3) / clients.size());
			if (s >= 0 && s < clients.size()) {
				selected = s;
				printf("select %d\n", s);
				movemouse(s);
			}

			if(allocation.x + allocation.width - 17 < e->xbutton.x && allocation.x + allocation.width > e->xbutton.x){
				/* TODO: close */
			}

			if(allocation.x + allocation.width - 34 < e->xbutton.x && allocation.x + allocation.width - 17 > e->xbutton.x){
				/* TODO: vsplit */
			}

			if(allocation.x + allocation.width - 51 < e->xbutton.x && allocation.x + allocation.width - 34 > e->xbutton.x){
				/* TODO: hsplit */
			}

		}
	}

	void movemouse(int s) {
		XEvent ev;
		cairo_t * cr;

		if (XGrabPointer(this->get_dpy(), this->get_window(), False,
				(ButtonPressMask | ButtonReleaseMask | PointerMotionMask),
				GrabModeAsync, GrabModeAsync, None, cursor, CurrentTime)
				!= GrabSuccess)
			return;
		do {
			XMaskEvent(this->get_dpy(), (ButtonPressMask | ButtonReleaseMask
					|PointerMotionMask) | ExposureMask
					|SubstructureRedirectMask, &ev);
			switch (ev.type) {
			case ConfigureRequest:
			case Expose:
			case MapRequest:
				cr = this->get_cairo();
				cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
				cairo_rectangle(cr, allocation.x, allocation.y,
						allocation.width, allocation.height);
				cairo_fill(cr);
				this->render(cr);
				cairo_destroy(cr);
				break;
			case MotionNotify:
				break;
			}
		} while (ev.type != ButtonRelease);
		XUngrabPointer(this->get_dpy(), CurrentTime);

		std::list<notebook_t *>::iterator i;
		for(i = notebook_t::note_link.begin(); i != notebook_t::note_link.end(); ++i) {
			if((*i)->is_selected(ev.xbutton.x, ev.xbutton.y))
				break;
		}

		if(i != notebook_t::note_link.end()) {
			std::list<client_t *>::iterator c = clients.begin();
			std::advance(c,s);
			client_t * move = *(c);
			clients.remove(move);
			(*i)->add_notebook(move);
		}

		cr = this->get_cairo();
		this->render(cr);
		cairo_destroy(cr);

	}

	void update_allocation(box_t & alloc) {
		allocation = alloc;
	}

	Window get_window() {
		return _parent->get_window();
	}

	Display * get_dpy() {
		return _parent->get_dpy();
	}

	cairo_t * get_cairo() {
		return _parent->get_cairo();
	}
};

std::list<notebook_t *> notebook_t::note_link;

class main: public page_widget_t {
	notebook_t *note0, *note1;
	vpan_t *vpan;

	Cursor cursor;
	XWindowAttributes wa;
	int running;
	int selected;
	Display *dpy;
	int screen;
	Window xroot;
	/* size of default root window */
	int sw, sh, sx, sy;
	int start_x, end_x;
	int start_y, end_y;

	std::list<notebook_tmp *> list;

	Window x_main_window;

	main(main const &);
	main &operator=(main const &);
public:
	main();
	void render(cairo_t * cr);
	void render();
	void run();

	Window get_window() {
		return x_main_window;
	}

	Display * get_dpy() {
		return dpy;
	}

	cairo_t * get_cairo() {
		cairo_surface_t * surf;
		XGetWindowAttributes(this->dpy, this->x_main_window, &(this->wa));
		surf = cairo_xlib_surface_create(this->dpy, this->x_main_window,
				this->wa.visual, this->wa.width, this->wa.height);
		cairo_t * cr = cairo_create(surf);
		return cr;
	}

	void update_allocation(box_t & alloc) {
	}

};

main::main() :
	page_widget_t(this) {
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
	note0 = new notebook_t(this);
	note0->add_notebook(new client_t("Asdl;kfj ;lafsdja;sdf"));
	note0->add_notebook(new client_t("bsdl;kfj ;lafsdja;sdf"));
	note0->add_notebook(new client_t("Csdl;kfj ;lafsdja;sdf"));
	note0->add_notebook(new client_t("Dsdl;kfj ;lafsdja;sdf"));
	note0->add_notebook(new client_t("Esdl;kfj ;lafsdja;sdf"));
	note0->add_notebook(new client_t("Fsdl;kfj ;lafsdja;sdf"));
	note0->add_notebook(new client_t("Gsdl;kfj ;lafsdja;sdf"));
	note0->add_notebook(new client_t("Hsdl;kfj ;lafsdja;sdf"));

	note1 = new notebook_t(this);
	note1->add_notebook(new client_t("Asdl;kfj ;lafsdja;sdf"));
	note1->add_notebook(new client_t("bsdl;kfj ;lafsdja;sdf"));
	note1->add_notebook(new client_t("Csdl;kfj ;lafsdja;sdf"));
	note1->add_notebook(new client_t("Dsdl;kfj ;lafsdja;sdf"));
	note1->add_notebook(new client_t("Esdl;kfj ;lafsdja;sdf"));
	note1->add_notebook(new client_t("Fsdl;kfj ;lafsdja;sdf"));
	note1->add_notebook(new client_t("Gsdl;kfj ;lafsdja;sdf"));
	note1->add_notebook(new client_t("Hsdl;kfj ;lafsdja;sdf"));

	vpan = new vpan_t(this, note0, note1);
	vpan->update_allocation(allocation);

}

void main::run() {
	this->running = 1;
	while (running) {
		XEvent e;
		XNextEvent(this->dpy, &e);
		printf("Event %s\n", x_event_name[e.type]);
		if (e.type == MapNotify || e.type == Expose) {
			this->render();
		} else if (e.type == ButtonPress) {
			note0->process_button_press_event(&e);
			note1->process_button_press_event(&e);
			vpan->process_button_press_event(&e);
			render();
		}
	}
}

void main::render(cairo_t * cr) {
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_rectangle(cr, 0, 0, wa.width, wa.height);
	cairo_fill(cr);
	vpan->render(cr);
}

void main::render() {
	cairo_t * cr = get_cairo();
	render(cr);
	cairo_destroy(cr);
}

}

int main(int argc, char * * argv) {
	page::main * m = new page::main();
	m->run();
}
