/*
 * page.cxx
 *
 * copyright (2010) Benoit Gschwind
 *
 */

/* According to POSIX.1-2001 */
#include <sys/select.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <X11/keysymdef.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xinerama.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <stdlib.h>
#include <cstring>

#include <sstream>
#include <limits>
#include <stdint.h>
#include <stdexcept>
#include <set>

#include "page.hxx"
#include "box.hxx"
#include "client.hxx"
#include "root.hxx"
#include "floating_window.hxx"

/* ICCCM definition */
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

namespace page {

long int const ClientEventMask = (StructureNotifyMask | PropertyChangeMask);

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

page_t::page_t(int argc, char ** argv) :
		cnx(), rnd(cnx), event_handler(*this) {

	conf = g_key_file_new();

	last_focus_time = 0;

	process_mode = NORMAL_PROCESS;
	cursor_fleur = XCreateFontCursor(cnx.dpy, XC_fleur);

	if (argc < 2) {
		throw std::runtime_error("usage : prg <config_file>");
	}

	if (!g_key_file_load_from_file(conf, argv[1], G_KEY_FILE_NONE, 0)) {
		throw std::runtime_error("could not load config file");
	}

	gchar * theme = g_key_file_get_string(conf, "default", "theme_dir", 0);
	if (theme == 0) {
		throw std::runtime_error("no theme_dir found in config file");
	}

	page_base_dir = theme;
	g_free(theme);

	gchar * sfont = g_key_file_get_string(conf, "default", "font_file", 0);
	if (theme == 0) {
		throw std::runtime_error("no font_file found in config file");
	}

	font = sfont;
	g_free(sfont);

	gchar * sfont_bold = g_key_file_get_string(conf, "default",
			"font_bold_file", 0);
	if (theme == 0) {
		throw std::runtime_error("no font_file found in config file");
	}

	font_bold = sfont_bold;
	g_free(sfont_bold);

	default_window_pop = 0;
	client_focused = 0;
	running = false;

	has_fullscreen_size = false;

	cursor = None;

	rndt = new render_tree_t(rnd.composite_overlay_s, page_base_dir, font,
			font_bold, cnx.root_size.w, cnx.root_size.h);
	rpage = new renderable_page_t(*rndt, split_list, notebook_list);
	rnd.add(rpage);
	rnd.lower(rpage);

}

page_t::~page_t() {

	if (conf) {
		g_key_file_free(conf);
		conf = 0;
	}

}

void page_t::run() {

	printf("root size: %d,%d\n", cnx.root_size.w, cnx.root_size.h);

	int n;
	XineramaScreenInfo * info = XineramaQueryScreens(cnx.dpy, &n);

	default_window_pop = 0;

	if (n < 1) {
		printf("No Xinerama Screen Found\n");

		box_t<int> x(0, 0, cnx.root_size.w, cnx.root_size.h);

		if (!has_fullscreen_size) {
			has_fullscreen_size = true;
			fullscreen_position = x;
		}

		viewport_t * v = new_viewport(x);
		default_window_pop = *(viewport_to_notebooks[v].begin());

	}

	for (int i = 0; i < n; ++i) {
		box_t<int> x(info[i].x_org, info[i].y_org, info[i].width,
				info[i].height);
		if (!has_fullscreen_size) {
			has_fullscreen_size = true;
			fullscreen_position = x;
		}

		viewport_t * v = new_viewport(x);

		if (default_window_pop == 0)
			default_window_pop = *(viewport_to_notebooks[v].begin());
	}

	XFree(info);

	update_net_supported();

	/* update number of desktop */
	int32_t number_of_desktop = 1;
	cnx.change_property(cnx.xroot, cnx.atoms._NET_NUMBER_OF_DESKTOPS,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&number_of_desktop), 1);

	/* define desktop geometry */
	long desktop_geometry[2];
	desktop_geometry[0] = cnx.root_size.w;
	desktop_geometry[1] = cnx.root_size.h;
	cnx.change_property(cnx.xroot, cnx.atoms._NET_DESKTOP_GEOMETRY,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(desktop_geometry), 2);

	/* set viewport */
	long viewport[2] = { 0, 0 };
	cnx.change_property(cnx.xroot, cnx.atoms._NET_DESKTOP_VIEWPORT,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(viewport), 2);

	/* set current desktop */
	long current_desktop = 0;
	cnx.change_property(cnx.xroot, cnx.atoms._NET_CURRENT_DESKTOP,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&current_desktop), 1);

	long showing_desktop = 0;
	cnx.change_property(cnx.xroot, cnx.atoms._NET_SHOWING_DESKTOP,
			cnx.atoms.CARDINAL, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&showing_desktop), 1);

	XIconSize icon_size;
	icon_size.min_width = 16;
	icon_size.min_height = 16;
	icon_size.max_width = 16;
	icon_size.max_height = 16;
	icon_size.width_inc = 1;
	icon_size.height_inc = 1;
	XSetIconSizes(cnx.dpy, cnx.xroot, &icon_size, 1);

	/* setup _NET_ACTIVE_WINDOW */
	set_focus(0);

	/* add page event handler */
	cnx.add_event_handler(&event_handler);

	scan();

	long workarea[4];
	workarea[0] = 0;
	workarea[1] = 0;
	workarea[2] = cnx.root_size.w;
	workarea[3] = cnx.root_size.h;
	cnx.change_property(cnx.xroot, cnx.atoms._NET_WORKAREA, cnx.atoms.CARDINAL,
			32, PropModeReplace, reinterpret_cast<unsigned char*>(workarea), 4);

	rpage->mark_durty();

	rnd.render_flush();
	XSync(cnx.dpy, False);
	XGrabKey(cnx.dpy, XKeysymToKeycode(cnx.dpy, XK_f), Mod4Mask, cnx.xroot,
			True, GrabModeAsync, GrabModeAsync);
	/* quit page */
	XGrabKey(cnx.dpy, XKeysymToKeycode(cnx.dpy, XK_q), Mod4Mask, cnx.xroot,
			True, GrabModeAsync, GrabModeAsync);

	/* print state info */
	XGrabKey(cnx.dpy, XKeysymToKeycode(cnx.dpy, XK_s), Mod4Mask, cnx.xroot,
			True, GrabModeAsync, GrabModeAsync);
	XGrabKey(cnx.dpy, XKeysymToKeycode(cnx.dpy, XK_Tab), Mod1Mask, cnx.xroot,
			True, GrabModeAsync, GrabModeAsync);

	XGrabButton(cnx.dpy, Button1, AnyModifier, cnx.xroot, False,
			ButtonPressMask, GrabModeSync, GrabModeAsync, cnx.xroot, None);



//	struct timeval next_frame;
//	next_frame.tv_sec = 0;
//	next_frame.tv_usec = 1.0 / 60.0 * 1.0e5;
//	fd_set fds_read;
//	fd_set fds_intr;

	update_allocation();
	rnd.add_damage_area(cnx.root_size);
	running = true;
	while (running) {

//		FD_ZERO(&fds_read);
//		FD_SET(cnx.connection_fd, &fds_read);
//		FD_ZERO(&fds_intr);
//		FD_SET(cnx.connection_fd, &fds_intr);
//
//		XFlush(cnx.dpy);
//		int nfd = select(cnx.connection_fd+1, &fds_read, 0, &fds_intr, 0);
//
//		if(nfd != 0){
//			while(cnx.process_check_event()) { }
//				//rnd.render_flush();
//			XSync(cnx.dpy, False);
//		}

//		if(nfd == 0 || next_frame.tv_usec < 1000) {
//			next_frame.tv_sec = 0;
//			next_frame.tv_usec = 1.0/60.0 * 1.0e5;
//
//			rnd.render_flush();
//			XFlush(cnx.dpy);
//		}

		/* process packed events */
		cnx.process_next_event();
		while(cnx.process_check_event())
			continue;
		rpage->render_if_needed();
		rnd.render_flush();

	}
}

void page_t::manage(window_t * w) {
	notebook_clients.insert(w);
	update_client_list();
	insert_window_in_tree(w, 0);
	if(!w->is_hidden()) {
		activate_client(w);
	}
	if(w->is_fullscreen()) {
		fullscreen(w);
	}
	rpage->mark_durty();
	rnd.add_damage_area(cnx.root_size);
}

void page_t::unmanage(window_t * w) {
	if (client_focused == w)
		set_focus(0);

	if(w->is_fullscreen())
		unfullscreen(w);

	remove_window_from_tree(w);
	notebook_clients.erase(w);
	update_client_list();
}

void page_t::scan() {
	printf("call %s\n", __PRETTY_FUNCTION__);
	unsigned int num;
	Window d1, d2, *wins = 0;
	XWindowAttributes wa = { 0 };
	cnx.grab();
	if (XQueryTree(cnx.dpy, cnx.xroot, &d1, &d2, &wins, &num)) {
		for (unsigned i = 0; i < num; ++i) {
			if (!cnx.get_window_attributes(wins[i], &wa))
				continue;
			//print_window_attributes(wins[i], wa);
			window_t * w = new_window(wins[i], wa);
			w->select_input(PropertyChangeMask | StructureNotifyMask);
			w->read_all();

			rnd.add(new_renderable_window(w));

			if (w->is_map()) {
				if (w->get_window_type() == PAGE_NORMAL_WINDOW_TYPE) {
					manage(w);
				}
			} else {
				/* if the window is not map check if previous windows manager has set WM_STATE to iconic
				 * if this is the case, that mean that is a managed window, otherwise it is a WithDrwn window
				 */
				if(w->get_has_wm_state() && w->get_wm_state() == IconicState) {
					if (w->get_window_type() == PAGE_NORMAL_WINDOW_TYPE) {
						manage(w);
					}
				}
			}


			XFlush(cnx.dpy);
		}
		XFree(wins);
	}

	/* scan is ended, start to listen root event */
	XSelectInput(cnx.dpy, cnx.xroot,
			/*ButtonPressMask | ButtonReleaseMask |*/KeyPressMask
					| KeyReleaseMask | SubstructureNotifyMask
					| SubstructureRedirectMask | ExposureMask);
	cnx.ungrab();

	update_client_list();
	printf("return %s\n", __PRETTY_FUNCTION__);
}

void page_t::update_net_supported() {
	std::list<Atom> supported_list;

	supported_list.push_back(cnx.atoms._NET_WM_NAME);
	supported_list.push_back(cnx.atoms._NET_WM_USER_TIME);
	supported_list.push_back(cnx.atoms._NET_CLIENT_LIST);
	supported_list.push_back(cnx.atoms._NET_CLIENT_LIST_STACKING);
	supported_list.push_back(cnx.atoms._NET_WM_STRUT_PARTIAL);
	supported_list.push_back(cnx.atoms._NET_NUMBER_OF_DESKTOPS);
	supported_list.push_back(cnx.atoms._NET_DESKTOP_GEOMETRY);
	supported_list.push_back(cnx.atoms._NET_DESKTOP_VIEWPORT);
	supported_list.push_back(cnx.atoms._NET_CURRENT_DESKTOP);
	supported_list.push_back(cnx.atoms._NET_ACTIVE_WINDOW);
	supported_list.push_back(cnx.atoms._NET_WM_STATE_FULLSCREEN);
	supported_list.push_back(cnx.atoms._NET_WM_STATE_FOCUSED);
	supported_list.push_back(cnx.atoms._NET_WM_STATE_DEMANDS_ATTENTION);
	supported_list.push_back(cnx.atoms._NET_REQUEST_FRAME_EXTENTS);
	supported_list.push_back(cnx.atoms._NET_FRAME_EXTENTS);

	supported_list.push_back(cnx.atoms._NET_WM_ALLOWED_ACTIONS);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_FULLSCREEN);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_ABOVE);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_BELOW);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_CHANGE_DESKTOP);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_CLOSE);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_FULLSCREEN);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_MAXIMIZE_HORZ);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_MAXIMIZE_VERT);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_MINIMIZE);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_MOVE);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_RESIZE);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_SHADE);
	supported_list.push_back(cnx.atoms._NET_WM_ACTION_STICK);

	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_COMBO);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_DESKTOP);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_DIALOG);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_DND);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_DOCK);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_DROPDOWN_MENU);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_MENU);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_NORMAL);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_NOTIFICATION);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_POPUP_MENU);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_SPLASH);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_TOOLBAR);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_TOOLTIP);
	supported_list.push_back(cnx.atoms._NET_WM_WINDOW_TYPE_UTILITY);

	long * list = new long[supported_list.size()];
	int k = 0;
	for (std::list<Atom>::iterator i = supported_list.begin();
			i != supported_list.end(); ++i, ++k) {
		list[k] = *i;
	}

	cnx.change_property(cnx.xroot, cnx.atoms._NET_SUPPORTED, cnx.atoms.ATOM, 32,
			PropModeReplace, reinterpret_cast<unsigned char *>(list), k);

	delete[] list;

}

void page_t::update_client_list() {

	Window * client_list = new Window[notebook_clients.size() + 1];
	Window * client_list_stack = new Window[notebook_clients.size() + 1];

	int k = 0;
	{
		window_set_t::iterator i = notebook_clients.begin();
		while (i != notebook_clients.end()) {
			if ((*i)->get_wm_state() != WithdrawnState) {
				client_list[k] = (*i)->get_xwin();
				++k;
			}
			++i;
		}
	}

	int l = 0;
	window_set_t::iterator x = notebook_clients.begin();
	while (x != notebook_clients.end()) {
		if ((*x)->get_wm_state() != WithdrawnState) {
			client_list_stack[l] = (*x)->get_xwin();
			++l;
		}
		++x;
	}

	cnx.change_property(cnx.xroot, cnx.atoms._NET_CLIENT_LIST_STACKING,
			cnx.atoms.WINDOW, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(client_list_stack), l);
	cnx.change_property(cnx.xroot, cnx.atoms._NET_CLIENT_LIST, cnx.atoms.WINDOW,
			32, PropModeReplace, reinterpret_cast<unsigned char *>(client_list),
			k);

	delete[] client_list;
	delete[] client_list_stack;
}

/* inspired from dwm */
bool page_t::get_text_prop(Window w, Atom atom, std::string & text) {
	//char **list = NULL;
	XTextProperty name;
	cnx.get_text_property(w, &name, atom);
	if (!name.nitems)
		return false;
	text = (char const *) name.value;
	return true;
}

void page_t::process_event(XKeyEvent const & e) {
	int n;
	KeySym * k = XGetKeyboardMapping(cnx.dpy, e.keycode, 1, &n);

	if (k == 0)
		return;
	if (n == 0) {
		XFree(k);
		return;
	}

	printf("key : %x\n", (unsigned) k[0]);

	if (XK_f == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
		if (client_focused != 0) {
			toggle_fullscreen(client_focused);
		}
	}

	if (XK_q == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
		running = false;
	}

	if (XK_s == k[0] && e.type == KeyPress && (e.state & Mod4Mask)) {
		print_state();
	}

	if (XK_Tab == k[0] && e.type == KeyPress && (e.state & Mod1Mask)) {
		/* select next window */
		if (client_focused != 0) {
			window_set_t::iterator x = std::find(notebook_clients.begin(),
					notebook_clients.end(), client_focused);
			if (x != notebook_clients.end()) {
				++x;
				if (x != notebook_clients.end()) {
					//printf("Next = %ld\n", (*x)->get_xwin());
					activate_client(*x);
				} else {
					if (!notebook_clients.empty()) {
						//printf("Next = %ld\n", tmp.front()->get_xwin());
						activate_client(*(notebook_clients.begin()));
					}
				}
			} else {
				if (!notebook_clients.empty()) {
					activate_client(*(notebook_clients.begin()));
				}
			}
		} else {
			if (!notebook_clients.empty()) {
				activate_client(*(notebook_clients.begin()));
			}
		}
	}

	XFree(k);

}

/* Button event make page to grab pointer */
void page_t::process_event_press(XButtonEvent const & e) {
	printf("Xpress event, window = %lu, root = %lu, subwindow = %lu\n",
			e.window, e.root, e.subwindow);
	window_t * c = get_window_t(e.window);
	switch (process_mode) {
	case NORMAL_PROCESS:
		cnx.last_know_time = e.time;
		if (c) {
			/* the hidden focus parameter */
			if (last_focus_time < cnx.last_know_time) {
				c->focus();
				set_focus(c);
				//cnx.raise_window(c->get_xwin());
			}
		} else if (e.window
				== cnx.xroot && e.root == cnx.xroot && e.subwindow == None) {
		/* split and notebook are mutually exclusive */
			if (!check_for_start_split(e)) {
				mode_data_notebook.start_x = e.x;
				mode_data_notebook.start_y = e.y;
				check_for_start_notebook(e);
			}
		}

		if(has_key(xfloating_window_to_floating_window, e.window) && e.subwindow == None) {

			mode_data_floating.f =
					xfloating_window_to_floating_window[e.window];
			mode_data_floating.size = mode_data_floating.f->w->get_size();

			/* click on close button ? */
			if (e.x > mode_data_floating.size.w - 17 && e.y < 24) {
				mode_data_floating.f->w->delete_window(e.time);
			} else {

				mode_data_floating.x_offset = e.x;
				mode_data_floating.y_offset = e.y;
				mode_data_floating.x_root = e.x_root;
				mode_data_floating.y_root = e.y_root;
				mode_data_floating.f =
						xfloating_window_to_floating_window[e.window];
				mode_data_floating.size = mode_data_floating.f->w->get_size();
				box_int_t size = mode_data_floating.f->border->get_size();
				mode_data_floating.size.x += size.x;
				mode_data_floating.size.y += size.y;

				printf("XXXXX size = %s, x: %d, y: %d\n",
						mode_data_floating.size.to_string().c_str(), e.x, e.y);
				if ((e.x > size.w - 10) && (e.y > size.h - 10)) {
					process_mode = FLOATING_RESIZE_PROCESS;
				} else {
					process_mode = FLOATING_GRAB_PROCESS;
				}
				/* Grab Pointer no other client will get mouse event */
				if (XGrabPointer(cnx.dpy, cnx.xroot, False,
						(ButtonPressMask | ButtonReleaseMask
								| PointerMotionMask),
						GrabModeAsync, GrabModeAsync, None, cursor_fleur,
						CurrentTime) != GrabSuccess) {
					/* bad news */
					throw std::runtime_error("fail to grab pointer");
				}
			}

		}

		break;
	case SPLIT_GRAB_PROCESS:
		/* should never happen */
		break;
	case NOTEBOOK_GRAB_PROCESS:
		/* should never happen */
		break;
	case FLOATING_GRAB_PROCESS:
		/* should never happen */
		break;
	case FLOATING_RESIZE_PROCESS:
		break;
	}

	/* AllowEvents, and replay Events */
	XAllowEvents(cnx.dpy, ReplayPointer, e.time);
}

void page_t::process_event_release(XButtonEvent const & e) {

	switch (process_mode) {
	case NORMAL_PROCESS:
		break;
	case SPLIT_GRAB_PROCESS:

		process_mode = NORMAL_PROCESS;
		rnd.remove(mode_data_split.p);
		delete mode_data_split.p;
		XUngrabPointer(cnx.dpy, CurrentTime);
		mode_data_split.split->set_split(mode_data_split.split_ratio);
		rnd.add_damage_area(mode_data_split.split->_allocation);
		rpage->mark_durty();
		break;
	case NOTEBOOK_GRAB_PROCESS:

		process_mode = NORMAL_PROCESS;
		rnd.remove(mode_data_notebook.pn0);
		rnd.remove(mode_data_notebook.pn1);
		delete mode_data_notebook.pn0;
		delete mode_data_notebook.pn1;
		/* ev is button release
		 * so set the hidden focus parameter
		 */
		cnx.last_know_time = e.time;

		XUngrabPointer(cnx.dpy, CurrentTime);


		if (mode_data_notebook.zone == SELECT_TAB && mode_data_notebook.ns != 0
				&& mode_data_notebook.ns != mode_data_notebook.from) {
			remove_window_from_tree(mode_data_notebook.c);
			insert_window_in_tree(mode_data_notebook.c, mode_data_notebook.ns);
		} else if (mode_data_notebook.zone == SELECT_TOP
				&& mode_data_notebook.ns != 0) {
			remove_window_from_tree(mode_data_notebook.c);
			split_top(mode_data_notebook.ns, mode_data_notebook.c);
		} else if (mode_data_notebook.zone == SELECT_LEFT
				&& mode_data_notebook.ns != 0) {
			remove_window_from_tree(mode_data_notebook.c);
			split_left(mode_data_notebook.ns, mode_data_notebook.c);
		} else if (mode_data_notebook.zone == SELECT_BOTTOM
				&& mode_data_notebook.ns != 0) {
			remove_window_from_tree(mode_data_notebook.c);
			split_bottom(mode_data_notebook.ns, mode_data_notebook.c);
		} else if (mode_data_notebook.zone == SELECT_RIGHT
				&& mode_data_notebook.ns != 0) {
			remove_window_from_tree(mode_data_notebook.c);
			split_right(mode_data_notebook.ns, mode_data_notebook.c);
		} else {
			mode_data_notebook.from->set_selected(mode_data_notebook.c);
		}

		/* automaticaly close empty notebook */
		if (mode_data_notebook.from->_clients.empty()
				&& mode_data_notebook.from->_parent != 0) {
			notebook_close(mode_data_notebook.from);
			update_allocation();
		} else {
			rnd.add_damage_area(mode_data_notebook.from->_allocation);
		}

		set_focus(mode_data_notebook.c);
		rpage->mark_durty();
		break;
	case FLOATING_GRAB_PROCESS:
		process_mode = NORMAL_PROCESS;
		XUngrabPointer(cnx.dpy, CurrentTime);
		break;
	case FLOATING_RESIZE_PROCESS:
		process_mode = NORMAL_PROCESS;
		XUngrabPointer(cnx.dpy, CurrentTime);
		break;
	}
}

void page_t::process_event(XMotionEvent const & e) {
	XEvent ev;
	box_int_t old_area;
	box_int_t x;
	static int count = 0;
	count++;
	switch (process_mode) {
	case NORMAL_PROCESS:
		/* should not happen */
		break;
	case SPLIT_GRAB_PROCESS:

		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx.dpy, Button1MotionMask, &ev));

		if (mode_data_split.split->get_split_type() == VERTICAL_SPLIT) {
			mode_data_split.split_ratio = (ev.xmotion.x
					- mode_data_split.split->_allocation.x)
					/ (double) (mode_data_split.split->_allocation.w);
		} else {
			mode_data_split.split_ratio = (ev.xmotion.y
					- mode_data_split.split->_allocation.y)
					/ (double) (mode_data_split.split->_allocation.h);
		}

		if (mode_data_split.split_ratio > 0.95)
			mode_data_split.split_ratio = 0.95;
		if (mode_data_split.split_ratio < 0.05)
			mode_data_split.split_ratio = 0.05;

		/* Render slider with quite complex render method to avoid flickering */
		old_area = mode_data_split.slider_area;
		mode_data_split.split->compute_split_bar_area(mode_data_split.slider_area,
				mode_data_split.split_ratio);

		mode_data_split.p->area = mode_data_split.slider_area;

		rnd.add_damage_area(old_area);
		rnd.add_damage_area(mode_data_split.slider_area);

		break;
	case NOTEBOOK_GRAB_PROCESS:
		{
		/* get lastest know motion event */
		ev.xmotion = e;
		while (XCheckMaskEvent(cnx.dpy, Button1MotionMask, &ev))
			continue;

		/* do not start drag&drop for small move */
		if (ev.xmotion.x_root < mode_data_notebook.start_x - 5
				|| ev.xmotion.x_root > mode_data_notebook.start_x + 5
				|| ev.xmotion.y_root < mode_data_notebook.start_y - 5
				|| ev.xmotion.y_root > mode_data_notebook.start_y + 5
				|| !mode_data_notebook.from->tab_area.is_inside(
						ev.xmotion.x_root, ev.xmotion.y_root)) {
			if (!mode_data_notebook.popup_is_added) {
				rnd.add(mode_data_notebook.pn0);
				rnd.add(mode_data_notebook.pn1);
				mode_data_notebook.popup_is_added = true;
			}
		}

		++count;
		if (mode_data_notebook.popup_is_added && (count % 10) == 0) {
			box_int_t old_area = mode_data_notebook.pn1->get_absolute_extend();
			box_int_t new_area(ev.xmotion.x_root + 10, ev.xmotion.y_root,
					old_area.w, old_area.h);
			mode_data_notebook.pn1->reconfigure(new_area);
			rnd.add_damage_area(old_area);
			rnd.add_damage_area(new_area);
			rpage->mark_durty();
		}


		std::set<notebook_t *>::iterator i = notebook_list.begin();
		while (i != notebook_list.end()) {
			if ((*i)->tab_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("tab\n");
				if (mode_data_notebook.zone != SELECT_TAB
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_TAB;
					mode_data_notebook.ns = (*i);
					update_popup_position(mode_data_notebook.pn0,
							(*i)->tab_area.x, (*i)->tab_area.y,
							(*i)->tab_area.w, (*i)->tab_area.h,
							mode_data_notebook.popup_is_added);
				}
				break;
			} else if ((*i)->right_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("right\n");
				if (mode_data_notebook.zone != SELECT_RIGHT
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_RIGHT;
					mode_data_notebook.ns = (*i);
					update_popup_position(mode_data_notebook.pn0,
							(*i)->popup_right_area.x, (*i)->popup_right_area.y,
							(*i)->popup_right_area.w, (*i)->popup_right_area.h,
							mode_data_notebook.popup_is_added);
				}
				break;
			} else if ((*i)->top_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("top\n");
				if (mode_data_notebook.zone != SELECT_TOP
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_TOP;
					mode_data_notebook.ns = (*i);
					update_popup_position(mode_data_notebook.pn0,
							(*i)->popup_top_area.x, (*i)->popup_top_area.y,
							(*i)->popup_top_area.w, (*i)->popup_top_area.h,
							mode_data_notebook.popup_is_added);
				}
				break;
			} else if ((*i)->bottom_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("bottom\n");
				if (mode_data_notebook.zone != SELECT_BOTTOM
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_BOTTOM;
					mode_data_notebook.ns = (*i);
					update_popup_position(mode_data_notebook.pn0,
							(*i)->popup_bottom_area.x,
							(*i)->popup_bottom_area.y,
							(*i)->popup_bottom_area.w,
							(*i)->popup_bottom_area.h,
							mode_data_notebook.popup_is_added);
				}
				break;
			} else if ((*i)->left_area.is_inside(ev.xmotion.x_root,
					ev.xmotion.y_root)) {
				//printf("left\n");
				if (mode_data_notebook.zone != SELECT_LEFT
						|| mode_data_notebook.ns != (*i)) {
					mode_data_notebook.zone = SELECT_LEFT;
					mode_data_notebook.ns = (*i);
					update_popup_position(mode_data_notebook.pn0,
							(*i)->popup_left_area.x, (*i)->popup_left_area.y,
							(*i)->popup_left_area.w, (*i)->popup_left_area.h,
							mode_data_notebook.popup_is_added);
				}
				break;
			}
			++i;
		}
		}

		break;
	case FLOATING_GRAB_PROCESS:
	{
		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx.dpy, Button1MotionMask, &ev));
		x = mode_data_floating.f->border->get_size();
		rnd.add_damage_area(x);
		x.x = e.x_root - mode_data_floating.x_offset;
		x.y = e.y_root - mode_data_floating.y_offset;
		mode_data_floating.f->border->move_resize(x);

	}
		break;
	case FLOATING_RESIZE_PROCESS:
	{
		/* get lastest know motion event */
		ev.xmotion = e;
		while(XCheckMaskEvent(cnx.dpy, Button1MotionMask, &ev));
		box_int_t x = mode_data_floating.size;
		x.w += e.x_root - mode_data_floating.x_root;
		x.h += e.y_root - mode_data_floating.y_root;

		if(x.h < 1)
			x.h = 1;
		if(x.w < 1)
			x.w = 1;

		printf("XXXXX resize = %s\n", x.to_string().c_str());

		rnd.add_damage_area(mode_data_floating.f->border->get_size());
		mode_data_floating.f->reconfigure(x);

	}
		break;
	}
}

void page_t::process_event(XCirculateEvent const & e) {
	std::map<Window, renderable_window_t *>::iterator x = window_to_renderable_context.find(e.window);
	if(x != window_to_renderable_context.end()) {
		if (e.place == PlaceOnTop) {
			rnd.raise(x->second);
		} else if (e.place == PlaceOnBottom) {
			rnd.lower(x->second);
		}
	}
}

void page_t::process_event(XConfigureEvent const & e) {
	printf("Configure %dx%d+%d+%d above:%lu, event:%lu, window:%lu, send_event: %s \n", e.width,
			e.height, e.x, e.y, e.above, e.event, e.window, (e.send_event == True)?"true":"false");
	if(e.send_event == True)
		return;

	/* apply stacking and resize for render */
	if (e.event == cnx.xroot) {
		std::map<Window, renderable_window_t *>::iterator x =
				window_to_renderable_context.find(e.window);
		if (x != window_to_renderable_context.end()) {

			/* apply stackiong */
			std::map<Window, renderable_window_t *>::iterator above =
					window_to_renderable_context.find(e.above);
			if (above != window_to_renderable_context.end()) {
				rnd.move_above(x->second, above->second);
			} else {
				rnd.lower(x->second);
			}

			/* mark as dirty all area */
			box_int_t area(e.x, e.y, e.width, e.height);
			x->second->reconfigure(area);
		}
	}

	/* track window position and stacking */
	window_t * x = get_window_t(e.window);
	if (x) {
		rnd.add_damage_area(x->get_size());
		box_int_t z(e.x, e.y, e.width, e.height);
		x->process_configure_notify_event(e);
		rnd.add_damage_area(x->get_size());

		/* restack x */
		//window_t * above = get_window_t(e.above);

		/* if window is transient for other window, restack them */
		std::set<window_t *>::const_iterator i = windows_set.begin();
		while (i != windows_set.end()) {
			if ((*i)->transient_for() == e.window) {
				/* update the stacking order for this window, hope that there is not
				 * mutual transient for */
				XWindowChanges cr;
				cr.sibling = e.window;
				cr.stack_mode = Above;
				cnx.configure_window((*i)->get_xwin(), CWSibling | CWStackMode,
						&cr);
			}
			++i;
		}
	}

	{
		std::map<Window, floating_window_t *>::iterator x = xfloating_window_to_floating_window.find(e.window);
		if(x != xfloating_window_to_floating_window.end()) {
			x->second->border->process_configure_notify_event(e);
			rpage->render.render_floating(x->second);
		}
	}

	{
		std::map<Window, floating_window_t *>::iterator x = window_to_floating_window.find(e.window);
		if(x != window_to_floating_window.end()) {
			rpage->render.render_floating(x->second);
		}
	}

}

/* track all created window */
void page_t::process_event(XCreateWindowEvent const & e) {

	/* Is-it fake event? */
	if (e.send_event == True)
		return;
	/* Does it come from root window */
	if (e.parent != cnx.xroot)
		return;

	XWindowAttributes wa;
	if (!cnx.get_window_attributes(e.window, &wa))
		return;

	/* for unkwown reason page get create window event
	 * from an already existing window. */
	window_t * w = get_window_t(e.window);
	if (w == 0) {
		w = new_window(e.window, wa);
		w->select_input(PropertyChangeMask | StructureNotifyMask);
		w->read_all();
		rnd.add(new_renderable_window(w));
		update_transient_for(w);

		printf("size = %s\n", w->get_size().to_string().c_str());

	} else {
		printf("already processed window\n");
	}
}

void page_t::process_event(XDestroyWindowEvent const & e) {

	/* apply destroy for render */
	{
		std::map<Window, renderable_window_t *>::iterator x =
				window_to_renderable_context.find(e.window);
		if (x != window_to_renderable_context.end()) {
			rnd.remove(x->second);
			destroy(x->second);
		}
	}

	window_t * c = get_window_t(e.window);
	if (c) {
		if (has_key(notebook_clients, c)) {
			unmanage(c);
		}

		if(has_key(window_to_floating_window, e.window)) {
			floating_window_t * t = window_to_floating_window[e.window];
			cnx.reparentwindow(t->w->get_xwin(), cnx.xroot, 0, 0);
			XDestroyWindow(cnx.dpy, t->border->get_xwin());
			destroy(t);
		}

		destroy(c);
	}
}

void page_t::process_event(XGravityEvent const & e) {
	/* Ignore it */
}

void page_t::process_event(XMapEvent const & e) {
	if (e.event != cnx.xroot)
		return;

	window_t * x = get_window_t(e.window);
	if (!x)
		return;
	x->map_notify();
	rnd.add_damage_area(x->get_size());
	if (x == client_focused) {
		x->focus();
	}

	if(has_key(window_to_floating_window, e.window))
		return;

	printf("overide_redirect = %s\n", x->override_redirect()? "true" : "false");
	x->print_net_wm_window_type();

	/* Libre Office doesn't generate MapRequest ?
	 */
	if (!has_key(notebook_clients, x) && !x->override_redirect()
			&& !has_key(fullscreen_client_to_viewport, x)
			&& !has_key(window_to_floating_window, e.window)) {

		printf("overide_redirect = %s\n", x->override_redirect()? "true" : "false");
		x->print_net_wm_window_type();

		page_window_type_e type = x->get_window_type();
		if(type == PAGE_NORMAL_WINDOW_TYPE) {
			manage(x);
		} else if (type == PAGE_FLOATING_WINDOW_TYPE) {
			floating_window_t * fw = new_floating_window(x);
			fw->map();
			rpage->render.render_floating(fw);
		}
	}

}

void page_t::process_event(XReparentEvent const & e) {
	/* TODO: track reparent */
	window_t * x = get_window_t(e.window);
	if (x != 0) {
		/* the window is removed from root */
		if (e.parent != cnx.xroot) {
			/* do nothing */
			box_int_t size = x->get_size();
			size.x = e.x;
			size.y = e.y;
			x->notify_move_resize(size);
		}
	} else {
		/* a new top level window */
		if (e.parent == cnx.xroot) {
			XWindowAttributes wa;
			if (cnx.get_window_attributes(e.window, &wa)) {
				//window_t * w = new_window(e.window, wa);
				/* TODO: manage things */
				//insert_window(w);
			}
		}
	}

	/* apply reparent for render */
	{
		std::map<Window, renderable_window_t *>::iterator x =
				window_to_renderable_context.find(e.window);
		if (x != window_to_renderable_context.end()) {
			region_t<int> old = x->second->get_absolute_extend();
			if(e.parent != cnx.xroot) {
				rnd.remove(x->second);
				destroy(x->second);
			}
		} else {
			window_t * w = get_window_t(e.window);
			if(e.parent == cnx.xroot && w != 0) {
				rnd.add(new_renderable_window(w));
			}
		}
	}
}

void page_t::process_event(XUnmapEvent const & e) {
	window_t * x = get_window_t(e.window);
	if (x) {
		x->unmap_notify();
		rnd.add_damage_area(x->get_size());
		event_t ex;
		ex.serial = e.serial;
		ex.type = e.type;
		bool expected_event = cnx.find_pending_event(ex);
		if (expected_event)
			return;

		/* if client is managed */
		if (has_key(notebook_clients, x)) {
			unmanage(x);
		}

		if(has_key(window_to_floating_window, e.window)) {
			floating_window_t * t = window_to_floating_window[e.window];
			cnx.reparentwindow(t->w->get_xwin(), cnx.xroot, 0, 0);
			XDestroyWindow(cnx.dpy, t->border->get_xwin());
			destroy(t);
		}

	}
}

void page_t::process_event(XCirculateRequestEvent const & e) {
	/* will happpen ? */
	window_t * x = get_window_t(e.window);
	if (x) {
		if (e.place == PlaceOnTop) {
			safe_raise_window(x);
		} else if (e.place == PlaceOnBottom) {
			cnx.lower_window(e.window);
		}
	}
}

void page_t::process_event(XConfigureRequestEvent const & e) {
	//printf("ConfigureRequest %dx%d+%d+%d above:%lu, mode:%x, window:%lu \n", e.width,
	//		e.height, e.x, e.y, e.above, e.detail, e.window);

	window_t * c = get_window_t(e.window);

	if(c)
		printf("name = %s\n", c->get_title().c_str());

	printf("send event: %s\n", e.send_event? "true" : "false");

	if(e.value_mask & CWX)
		printf("has x: %d\n", e.x);
	if(e.value_mask & CWY)
		printf("has y: %d\n", e.y);
	if(e.value_mask & CWWidth)
		printf("has width: %d\n", e.width);
	if(e.value_mask & CWHeight)
		printf("has height: %d\n", e.height);

	if(e.value_mask & CWSibling)
		printf("has sibling: %lu\n", e.above);
	if(e.value_mask & CWStackMode)
		printf("has stack mode: %d\n", e.detail);

	if(e.value_mask & CWBorderWidth)
		printf("has border: %d\n", e.border_width);



	if (c) {

		XEvent ev;
		ev.xconfigure.type = ConfigureNotify;
		ev.xconfigure.display = cnx.dpy;
		ev.xconfigure.event = cnx.xroot;
		ev.xconfigure.window = e.window;
		ev.xconfigure.send_event = True;

		/* if ConfigureRequest happen, override redirect is False */
		ev.xconfigure.override_redirect = False;
		ev.xconfigure.border_width = e.border_width;

		ev.xconfigure.above = c->transient_for();

		if (has_key(client_to_notebook, c)) {
			box_int_t size = c->get_size();

			/* send mandatory fake event */
			ev.xconfigure.x = size.x;
			ev.xconfigure.y = size.y;
			ev.xconfigure.width = size.w;
			ev.xconfigure.height = size.h;

			if((e.value_mask & CWStackMode) && !(e.value_mask & CWSibling)) {
				if(e.detail == Above)
					safe_raise_window(c);
			}


		} else if (has_key(window_to_floating_window, e.window)) {

			floating_window_t * f = window_to_floating_window[e.window];

			box_int_t new_size = f->get_position();

			if(e.value_mask & CWX) {
				new_size.x = e.x;
			}

			if(e.value_mask & CWY) {
				new_size.y = e.y;
			}

			if(e.value_mask & CWWidth) {
				new_size.w = e.width;
			}

			if(e.value_mask & CWHeight) {
				new_size.h = e.height;
			}

			f->reconfigure(new_size);

			ev.xconfigure.x = new_size.x;
			ev.xconfigure.y = new_size.y;
			ev.xconfigure.width = new_size.w;
			ev.xconfigure.height = new_size.h;

			printf("configure event %d %d %d %d\n", ev.xconfigure.x, ev.xconfigure.y, ev.xconfigure.width, ev.xconfigure.height);



		} else {

			box_int_t size = c->get_size();

			if(e.value_mask & CWX) {
				size.x = e.x;
			}

			if(e.value_mask & CWY) {
				size.y = e.y;
			}

			if(e.value_mask & CWWidth) {
				size.w = e.width;
			}

			if(e.value_mask & CWHeight) {
				size.h = e.height;
			}

			if(e.value_mask & (CWX | CWY | CWWidth | CWHeight)) {
				c->move_resize(size);
			}

			/* send mandatory fake event */
			ev.xconfigure.x = size.x;
			ev.xconfigure.y = size.y;
			ev.xconfigure.width = size.w;
			ev.xconfigure.height = size.h;

		}

		cnx.send_event(e.window, False, (SubstructureRedirectMask|SubstructureNotifyMask), &ev);

	} else {
		/* if this is an unknown window, move it without condition
		 * this should never happen */
		XWindowChanges wc;
		wc.x = e.x;
		wc.y = e.y;
		wc.height = e.height;
		wc.width = e.width;
		wc.sibling = e.above;
		wc.stack_mode = e.detail;
		wc.border_width = e.border_width;
		cnx.configure_window(e.window, e.value_mask, &wc);
	}
}

void page_t::process_event(XMapRequestEvent const & e) {
	printf("Entering in %s #%p\n", __PRETTY_FUNCTION__, (void *) e.window);
	window_t * x = get_window_t(e.window);
	if(x == 0)
		return;

	/* if we don't map the window, some client try to remap the window, but they are already
	 * managed, so skip them.
	 */
	if (!has_key(notebook_clients, x) && !has_key(window_to_floating_window, e.window)) {
		printf("overide_redirect = %s\n", x->override_redirect()? "true" : "false");
		x->print_net_wm_window_type();

		page_window_type_e type = x->get_window_type();
		if(type == PAGE_NORMAL_WINDOW_TYPE) {
			manage(x);
		} else if (type == PAGE_FLOATING_WINDOW_TYPE) {
			floating_window_t * fw = new_floating_window(x);
			fw->map();
			rpage->render.render_floating(fw);
		} else {
			x->map();
		}
	} else {
		activate_client(x);
	}
}

void page_t::process_event(XPropertyEvent const & e) {
	char * name = cnx.get_atom_name(e.atom);
	printf("Atom Name = \"%s\"\n", name);
	XFree(name);

	cnx.last_know_time = e.time;

	window_t * x = get_window_t(e.window);
	if (!x)
		return;
	if (e.atom == cnx.atoms._NET_WM_USER_TIME) {
		x->read_net_wm_user_time();
		//safe_raise_window(x);
		//if(client_focused == x) {
		//	x->focus();
		//}
	} else if (e.atom == cnx.atoms._NET_WM_NAME) {
		x->read_net_vm_name();
		if(has_key(client_to_notebook, x)) {
			rpage->mark_durty();
			rnd.add_damage_area(client_to_notebook[x]->tab_area);
		}

		if(has_key(window_to_floating_window, x->get_xwin())) {
			rpage->render.render_floating(window_to_floating_window[x->get_xwin()]);
		}

	} else if (e.atom == cnx.atoms.WM_NAME) {
		x->read_vm_name();
		if(has_key(client_to_notebook, x)) {
			rpage->mark_durty();
			rnd.add_damage_area(client_to_notebook[x]->tab_area);
		}

		if(has_key(window_to_floating_window, x->get_xwin())) {
			rpage->render.render_floating(window_to_floating_window[x->get_xwin()]);
		}

	} else if (e.atom == cnx.atoms._NET_WM_STRUT_PARTIAL) {
		if (e.state == PropertyNewValue || e.state == PropertyDelete) {
			x->read_partial_struct();
			update_allocation();
		}
	} else if (e.atom == cnx.atoms._NET_WM_ICON) {
		x->read_icon_data();
		/* TODO: durty notebook */
	} else if (e.atom == cnx.atoms._NET_WM_WINDOW_TYPE) {
		//window_t::page_window_type_e old = x->get_window_type();
		x->read_net_wm_type();
		x->read_transient_for();
		//x->find_window_type();

		/* I do not see something in ICCCM */
		//if(x->get_window_type() == window_t::PAGE_NORMAL_WINDOW_TYPE && old != window_t::PAGE_NORMAL_WINDOW_TYPE) {
		//	manage(x);
		//}
	} else if (e.atom == cnx.atoms.WM_NORMAL_HINTS) {
		x->read_wm_normal_hints();
		if (has_key(client_to_notebook, x)) {
			update_allocation();
		}
	} else if (e.atom == cnx.atoms.WM_PROTOCOLS) {
		x->read_net_wm_protocols();
	} else if (e.atom == cnx.atoms.WM_TRANSIENT_FOR) {
		x->read_transient_for();
		printf("TRANSIENT_FOR = #%ld\n", x->transient_for());

		update_transient_for(x);
		/* ICCCM if transient_for is set for override redirect window, move this window above
		 * the transient one (it's for menus and popup)
		 */
		if (x->override_redirect() && x->transient_for() != None) {
			/* make the request to X server */
			XWindowChanges cr;

			box_int_t s = x->get_size();
			cr.x = s.x;
			cr.y = s.y;
			cr.width = s.w;
			cr.height = s.h;
			cr.sibling = x->transient_for();
			cr.stack_mode = Above;
			cnx.configure_window(x->get_xwin(), CWSibling | CWStackMode, &cr);
		}
	} else if (e.atom == cnx.atoms.WM_HINTS) {
		x->read_wm_hints();

		/* WM_HINTS can change the focus behaviors, so re-focus if needed */
		if(client_focused == x) {
			x->focus();
		}
	} else if (e.atom == cnx.atoms._NET_WM_STATE) {
		x->read_net_wm_state();
	} else if (e.atom == cnx.atoms.WM_STATE) {
		x->read_wm_state();
	}
}

void page_t::process_event(XClientMessageEvent const & e) {
	printf("Entering in %s on %lu\n", __PRETTY_FUNCTION__, e.window);

	//printf("%lu\n", ev->xclient.message_type);
	char * name = cnx.get_atom_name(e.message_type);
	printf("Atom Name = \"%s\"\n", name);
	XFree(name);

	if (e.message_type == cnx.atoms._NET_ACTIVE_WINDOW) {
		printf("request to activate %lu\n", e.window);
		window_t * w = get_window_t(e.window);
		if (w) {
			activate_client(w);
		}
	} else if (e.message_type == cnx.atoms._NET_WM_STATE) {
		window_t * c = get_window_t(e.window);
		if (c) {

			process_net_vm_state_client_messate(c, e.data.l[0], e.data.l[1]);
			process_net_vm_state_client_messate(c, e.data.l[0], e.data.l[2]);

			for (int i = 1; i < 3; ++i) {
				if (std::find(supported_list.begin(), supported_list.end(),
						e.data.l[i]) != supported_list.end()) {
					switch (e.data.l[0]) {
					case _NET_WM_STATE_REMOVE:
						c->unset_net_wm_state(e.data.l[i]);
						break;
					case _NET_WM_STATE_ADD:
						c->set_net_wm_state(e.data.l[i]);
						break;
					case _NET_WM_STATE_TOGGLE:
						c->toggle_net_wm_state(e.data.l[i]);
						break;
					}
				}
			}
		}
	} else if (e.message_type == cnx.atoms.WM_CHANGE_STATE) {
		/* client should send this message to go iconic */
		if (e.data.l[0] == IconicState) {
			window_t * x = get_window_t(e.window);
			if (x) {
				printf("Set to iconic %lu\n", x->get_xwin());
				iconify_client(x);
			}
		}
	} else if (e.message_type == cnx.atoms.PAGE_QUIT) {
		running = false;
	} else if (e.message_type == cnx.atoms.WM_PROTOCOLS) {
		char * name = cnx.get_atom_name(e.data.l[0]);
		printf("PROTOCOL Atom Name = \"%s\"\n", name);
		XFree(name);
	} else if (e.message_type == cnx.atoms._NET_CLOSE_WINDOW) {

		XEvent evx;
		evx.xclient.display = cnx.dpy;
		evx.xclient.type = ClientMessage;
		evx.xclient.format = 32;
		evx.xclient.message_type = cnx.atoms.WM_PROTOCOLS;
		evx.xclient.window = e.window;
		evx.xclient.data.l[0] = cnx.atoms.WM_DELETE_WINDOW;
		evx.xclient.data.l[1] = e.data.l[0];

		cnx.send_event(e.window, False, NoEventMask, &evx);
	} else if (e.message_type == cnx.atoms._NET_REQUEST_FRAME_EXTENTS) {
		if (!viewport_list.empty()) {
			//box_int_t b = default_window_pop->get_new_client_size();
			long int size[4] = { 0, 0, 0, 0 };
			cnx.change_property(e.window, cnx.atoms._NET_FRAME_EXTENTS,
					cnx.atoms.CARDINAL, 32, PropModeReplace,
					reinterpret_cast<unsigned char *>(size), 4);
		}
	}
}

void page_t::process_event(XDamageNotifyEvent const & e) {

	//printf("Damage area %dx%d+%d+%d\n", e.area.width, e.area.height, e.area.x, e.area.y);

	/* create an empty region */
	XserverRegion region = XFixesCreateRegion(cnx.dpy, 0, 0);

	if (!region)
		throw std::runtime_error("could not create region");

	/* get damaged region and remove them from damaged status */
	XDamageSubtract(cnx.dpy, e.damage, None, region);

	window_t * x = get_window_t(e.drawable);
	if (x) {

		XRectangle * rects;
		int nb_rects;

		/* get all rectangles for the damaged region */
		rects = XFixesFetchRegion(cnx.dpy, region, &nb_rects);

		if (rects) {
			for (int i = 0; i < nb_rects; ++i) {
				box_int_t box(rects[i]);
				window_to_renderable_context[x->get_xwin()]->mark_dirty_retangle(box);
				box_int_t _ = x->get_size();
				/* setup absolute damaged area */
				box.x += _.x;
				box.y += _.y;
				if(x->is_visible())
					rnd.add_damage_area(box);
			}
			XFree(rects);
		}

		XFixesDestroyRegion(cnx.dpy, region);
		//rnd.render_flush();
	}

}

void page_t::fullscreen(window_t * c) {
	/* WARNING: Call order is important, change it with caution */

	notebook_t * n = 0;
	viewport_t * v = 0;

	{
		std::map<window_t *, notebook_t *>::iterator _ =
				client_to_notebook.find(c);
		if (_ == client_to_notebook.end())
			return;
		n = _->second;
	}

	{
		std::map<notebook_t *, viewport_t *>::iterator _ =
				notebook_to_viewport.find(n);
		if (_ == notebook_to_viewport.end())
			return;
		v = _->second;
	}

	v->_is_visible = false;
	if (v->fullscreen_client != 0) {
		v->fullscreen_client->unmap();
		v->fullscreen_client->unset_fullscreen();
		insert_window_in_tree(v->fullscreen_client, 0);
		v->fullscreen_client = 0;
		set_focus(0);
	}

	remove_window_from_tree(c);

	/* unmap all notebook window */
	notebook_set_t ns = viewport_to_notebooks[v];
	for(notebook_set_t::iterator i = ns.begin(); i != ns.end(); ++i) {
		(*i)->unmap_all();
	}

	v->fullscreen_client = c;
	fullscreen_client_to_viewport[c] = std::pair<viewport_t *, notebook_t *>(v, n);
	c->set_fullscreen();
	c->normalize();
	c->move_resize(v->raw_aera);
	set_focus(c);
	c->focus();
	//cnx.raise_window(c->get_xwin());

}

void page_t::unfullscreen(window_t * c) {
	/* WARNING: Call order is important, change it with caution */
	std::pair<viewport_t *, notebook_t *> p = get_safe_value(fullscreen_client_to_viewport, c, std::pair<viewport_t *, notebook_t *>(0, 0));
	if(p.first == 0)
		return;

	viewport_t * v = p.first;
	notebook_t * old = p.second;
	if(!has_key(notebook_list, old))
		old = default_window_pop;

	v->fullscreen_client = 0;
	fullscreen_client_to_viewport.erase(c);
	v->_is_visible = true;

	/* map all notebook window */
	notebook_set_t ns = viewport_to_notebooks[v];
	for(notebook_set_t::iterator i = ns.begin(); i != ns.end(); ++i) {
		(*i)->map_all();
	}

	c->unmap();
	c->unset_fullscreen();
	set_focus(0);
	insert_window_in_tree(c, old);
	update_allocation();
	old->activate_client(c);
	rnd.add_damage_area(v->raw_aera);
}

void page_t::toggle_fullscreen(window_t * c) {
	if(c->is_fullscreen())
		unfullscreen(c);
	else
		fullscreen(c);
}

void page_t::print_window_attributes(Window w, XWindowAttributes & wa) {
	printf(">>> Window: #%lu\n", w);
	printf("> size: %dx%d+%d+%d\n", wa.width, wa.height, wa.x, wa.y);
//	printf("> border_width: %d\n", wa.border_width);
//	printf("> depth: %d\n", wa.depth);
//	printf("> visual #%p\n", wa.visual);
//	printf("> root: #%lu\n", wa.root);
	if (wa.c_class == CopyFromParent) {
		printf("> class: CopyFromParent\n");
	} else if (wa.c_class == InputOutput) {
		printf("> class: InputOutput\n");
	} else if (wa.c_class == InputOnly) {
		printf("> class: InputOnly\n");
	} else {
		printf("> class: Unknown\n");
	}

	if (wa.map_state == IsViewable) {
		printf("> map_state: IsViewable\n");
	} else if (wa.map_state == IsUnviewable) {
		printf("> map_state: IsUnviewable\n");
	} else if (wa.map_state == IsUnmapped) {
		printf("> map_state: IsUnmapped\n");
	} else {
		printf("> map_state: Unknown\n");
	}

//	printf("> bit_gravity: %d\n", wa.bit_gravity);
//	printf("> win_gravity: %d\n", wa.win_gravity);
//	printf("> backing_store: %dlx\n", wa.backing_store);
//	printf("> backing_planes: %lx\n", wa.backing_planes);
//	printf("> backing_pixel: %lx\n", wa.backing_pixel);
//	printf("> save_under: %d\n", wa.save_under);
//	printf("> colormap: ?\n");
//	printf("> all_event_masks: %08lx\n", wa.all_event_masks);
//	printf("> your_event_mask: %08lx\n", wa.your_event_mask);
//	printf("> do_not_propagate_mask: %08lx\n", wa.do_not_propagate_mask);
	printf("> override_redirect: %d\n", wa.override_redirect);
//	printf("> screen: %p\n", wa.screen);

}

std::string page_t::safe_get_window_name(Window w) {
	window_t * x = get_window_t(w);
	std::string wname = "unknown window";
	if(x != 0) {
		wname = x->get_title();
	}

	return wname;
}

void page_t::page_event_handler_t::process_event(XEvent const & e) {



	if (e.type == MapNotify) {
		printf("#%08lu %s: event = %lu, win = %lu, name=\"%s\"\n", e.xmap.serial,
				x_event_name[e.type], e.xmap.event, e.xmap.window, page.safe_get_window_name(e.xmap.window).c_str());
	} else if (e.type == DestroyNotify) {
		printf("#%08lu %s: event = %lu, win = %lu, name=\"%s\"\n", e.xdestroywindow.serial,
				x_event_name[e.type], e.xdestroywindow.event,
				e.xdestroywindow.window, page.safe_get_window_name(e.xdestroywindow.window).c_str());
	} else if (e.type == MapRequest) {
		printf("#%08lu %s: parent = %lu, win = %lu, name=\"%s\"\n", e.xmaprequest.serial,
				x_event_name[e.type], e.xmaprequest.parent,
				e.xmaprequest.window, page.safe_get_window_name(e.xmaprequest.window).c_str());
	} else if (e.type == UnmapNotify) {
		printf("#%08lu %s: event = %lu, win = %lu, name=\"%s\"\n", e.xunmap.serial,
				x_event_name[e.type], e.xunmap.event, e.xunmap.window, page.safe_get_window_name(e.xunmap.window).c_str());
	} else if (e.type == CreateNotify) {
		printf("#%08lu %s: parent = %lu, win = %lu, name=\"%s\"\n", e.xcreatewindow.serial,
				x_event_name[e.type], e.xcreatewindow.parent,
				e.xcreatewindow.window, page.safe_get_window_name(e.xcreatewindow.window).c_str());
	} else if (e.type < LASTEvent && e.type > 0) {
		printf("#%08lu %s: win: #%lu, name=\"%s\"\n", e.xany.serial, x_event_name[e.type],
				e.xany.window, page.safe_get_window_name(e.xany.window).c_str());
	}

	fflush(stdout);

//	page.cnx.grab();
//
//	/* remove all destroyed windows */
//	XEvent tmp_ev;
//	while (XCheckTypedEvent(page.cnx.dpy, DestroyNotify, &tmp_ev)) {
//		page.process_event(e.xdestroywindow);
//	}

	if (e.type == ButtonPress) {
		page.process_event_press(e.xbutton);
	} else if (e.type == ButtonRelease) {
		page.process_event_release(e.xbutton);
	} else if (e.type == MotionNotify) {
		page.process_event(e.xmotion);
	} else if (e.type == KeyPress || e.type == KeyRelease) {
		page.process_event(e.xkey);
	} else if (e.type == CirculateNotify) {
		page.process_event(e.xcirculate);
	} else if (e.type == ConfigureNotify) {
		page.process_event(e.xconfigure);
	} else if (e.type == CreateNotify) {
		page.process_event(e.xcreatewindow);
	} else if (e.type == DestroyNotify) {
		page.process_event(e.xdestroywindow);
	} else if (e.type == GravityNotify) {
		page.process_event(e.xgravity);
	} else if (e.type == MapNotify) {
		page.process_event(e.xmap);
	} else if (e.type == ReparentNotify) {
		page.process_event(e.xreparent);
	} else if (e.type == UnmapNotify) {
		page.process_event(e.xunmap);
	} else if (e.type == CirculateRequest) {
		page.process_event(e.xcirculaterequest);
	} else if (e.type == ConfigureRequest) {
		page.process_event(e.xconfigurerequest);
	} else if (e.type == MapRequest) {
		page.process_event(e.xmaprequest);
	} else if (e.type == PropertyNotify) {
		page.process_event(e.xproperty);
	} else if (e.type == ClientMessage) {
		page.process_event(e.xclient);
	} else if (e.type == page.cnx.damage_event + XDamageNotify) {
		page.process_event(reinterpret_cast<XDamageNotifyEvent const &>(e));
	}

	//page.rnd.render_flush();

	//page.cnx.ungrab();

//	if (!page.cnx.is_not_grab()) {
//		fprintf(stderr, "SERVER IS GRAB WHERE IT SHOULDN'T");
//		exit(EXIT_FAILURE);
//	}

}

void page_t::activate_client(window_t * x) {
	if (has_key(client_to_notebook, x)) {
		notebook_t * n = client_to_notebook[x];
		n->activate_client(x);
		rpage->mark_durty();
		rnd.add_damage_area(n->get_absolute_extend());
	}
}

void page_t::insert_window_in_tree(window_t * x, notebook_t * n) {
	assert(x != 0);
	assert(!has_key(client_to_notebook, x));
	if (n == 0)
		n = default_window_pop;
	assert(n != 0);
	client_to_notebook[x] = n;
	n->add_client(x);
	rpage->mark_durty();
	rnd.add_damage_area(n->get_absolute_extend());
}

void page_t::remove_window_from_tree(window_t * x) {
	assert(has_key(client_to_notebook, x));
	notebook_t * n = client_to_notebook[x];
	client_to_notebook.erase(x);
	n->remove_client(x);
	rpage->mark_durty();
	rnd.add_damage_area(n->get_absolute_extend());
}

void page_t::iconify_client(window_t * x) {
	std::set<notebook_t *>::iterator i = notebook_list.begin();
	while (i != notebook_list.end()) {
		(*i)->iconify_client(x);
		++i;
	}
}

/* update viewport and childs allocation */
void page_t::update_allocation() {
	std::set<viewport_t *>::iterator i = viewport_list.begin();
	while (i != viewport_list.end()) {
		fix_allocation(*(*i));
		(*i)->reconfigure();
		++i;
	}
}

void page_t::read_viewport_layout() {
	int n;
	XineramaScreenInfo * info = XineramaQueryScreens(cnx.dpy, &n);

	if (n < 1) {
		throw std::runtime_error("no Xinerama screen Found");
	}

	for (int i = 0; i < n; ++i) {
		box_t<int> x(info[i].x_org, info[i].y_org, info[i].width,
				info[i].height);
		viewport_t * v = new_viewport(x);
	}

	XFree(info);
}

void page_t::set_default_pop(notebook_t * x) {
	default_window_pop = x;
}

void page_t::set_focus(window_t * w) {
	last_focus_time = cnx.last_know_time;
	if (client_focused == w)
		return;

	if (client_focused != 0) {
		client_focused->unset_focused();
	}

	client_focused = w;

	if (client_focused != 0) {
		client_focused->set_focused();
	}

	/* update _NET_ACTIVE_WINDOW */
	Window xw = None;
	if(client_focused != 0)
		xw = client_focused->get_xwin();
	cnx.change_property(cnx.xroot, cnx.atoms._NET_ACTIVE_WINDOW,
			cnx.atoms.WINDOW, 32, PropModeReplace,
			reinterpret_cast<unsigned char *>(&xw), 1);

	if(client_focused != 0)
		safe_raise_window(w);

}

render_context_t & page_t::get_render_context() {
	return rnd;
}

xconnection_t & page_t::get_xconnection() {
	return cnx;
}

window_t * page_t::new_window(Window const w, XWindowAttributes const & wa) {
	assert(!has_key(xwindow_to_window, w));
	window_t * x = new window_t(cnx, w, wa);
	xwindow_to_window[w] = x;
	windows_set.insert(x);
	return x;
}

/* delete window handler */
void page_t::delete_window(window_t * x) {
	assert(has_key(xwindow_to_window, x->get_xwin()));
	xwindow_to_window.erase(x->get_xwin());
	windows_set.erase(x);
	delete x;
}

window_list_t page_t::get_windows() {
	window_list_t tmp;
	std::set<viewport_t *>::iterator i = viewport_list.begin();
	while (i != viewport_list.end()) {
		window_set_t x = (*i)->get_windows();
		tmp.insert(tmp.begin(), x.begin(), x.end());
		++i;
	}
	return tmp;
}

bool page_t::check_for_start_split(XButtonEvent const & e) {
	//printf("call %s\n", __FUNCTION__);
	split_t * x = 0;
	std::set<split_t *>::const_iterator i = split_list.begin();
	while (i != split_list.end()) {
		if ((*i)->get_split_bar_area().is_inside(e.x, e.y)) {
			x = (*i);
			break;
		}
		++i;
	}

	if (x != 0) {
		//printf("starting split\n");
		/* switch process mode */
		process_mode = SPLIT_GRAB_PROCESS;
		mode_data_split.split_ratio = x->get_split_ratio();
		mode_data_split.split = x;
		mode_data_split.slider_area = mode_data_split.split->get_split_bar_area();
		mode_data_split.p = new popup_split_t(mode_data_split.slider_area);
		rnd.add(mode_data_split.p);

		/* Grab Pointer no other client will get mouse event */
		if (XGrabPointer(cnx.dpy, cnx.xroot, False,
				(ButtonPressMask | ButtonReleaseMask | PointerMotionMask),
				GrabModeAsync, GrabModeAsync, None, cursor_fleur,
				CurrentTime) != GrabSuccess) {
			/* bad news */
			throw std::runtime_error("fail to grab pointer");
		}

	}

	//printf("exit %s\n", __FUNCTION__);
	return x != 0;
}

bool page_t::check_for_start_notebook(XButtonEvent const & e) {
	//printf("call %s\n", __FUNCTION__);
	window_t * c = 0;
	std::set<notebook_t *>::const_iterator i = notebook_list.begin();
	while (notebook_list.end() != i) {
		c = (*i)->find_client_tab(e.x, e.y);
		if (c)
			break;
		++i;
	}

	if (c != 0) {

		(*i)->update_close_area();
		if ((*i)->close_client_area.is_inside(e.x, e.y)) {
			c->delete_window(e.time);
		} else {

			//printf("starting notebook\n");
			process_mode = NOTEBOOK_GRAB_PROCESS;
			mode_data_notebook.c = c;
			mode_data_notebook.from = *i;
			mode_data_notebook.ns = 0;
			mode_data_notebook.zone = SELECT_NONE;
			mode_data_notebook.pn0 = new popup_notebook0_t(
					mode_data_notebook.from->tab_area.x,
					mode_data_notebook.from->tab_area.y,
					mode_data_notebook.from->tab_area.w,
					mode_data_notebook.from->tab_area.h);

			mode_data_notebook.name = c->get_title();
			mode_data_notebook.pn1 = new popup_notebook1_t(
					mode_data_notebook.from->_allocation.x,
					mode_data_notebook.from->_allocation.y, rndt->font,
					mode_data_notebook.from->get_icon_surface(c), mode_data_notebook.name);

			mode_data_notebook.popup_is_added = false;

			/* Grab Pointer no other client will get mouse event */
			if (XGrabPointer(cnx.dpy, cnx.xroot, False,
					(ButtonPressMask | ButtonReleaseMask | PointerMotionMask),
					GrabModeAsync, GrabModeAsync, None, cursor_fleur,
					CurrentTime) != GrabSuccess) {
				/* bad news */
				throw std::runtime_error("fail to grab pointer");
			}
		}

	} else {

		std::set<notebook_t *>::const_iterator i = notebook_list.begin();
		notebook_t * x = 0;
		while (notebook_list.end() != i) {
			notebook_t * n = (*i);
			if (n->button_close.is_inside(e.x, e.y)) {
				x = n;
				break;
			} else if (n->button_vsplit.is_inside(e.x, e.y)) {
				x = n;
				break;
			} else if (n->button_hsplit.is_inside(e.x, e.y)) {
				x = n;
				break;
			} else if (n->button_pop.is_inside(e.x, e.y)) {
				x = n;
				break;
			}
			++i;
		}

		if (x != 0) {
			if (x->button_close.is_inside(e.x, e.y)) {
				notebook_close(x);
				rpage->mark_durty();
				rnd.add_damage_area(cnx.root_size);
			} else if (x->button_vsplit.is_inside(e.x, e.y)) {
				split(x, VERTICAL_SPLIT);
				update_allocation();
				rpage->mark_durty();
				rnd.add_damage_area(cnx.root_size);
			} else if (x->button_hsplit.is_inside(e.x, e.y)) {
				split(x, HORIZONTAL_SPLIT);
				update_allocation();
				rpage->mark_durty();
				rnd.add_damage_area(cnx.root_size);
			} else if (x->button_pop.is_inside(e.x, e.y)) {
				default_window_pop = x;
				rnd.add_damage_area(x->tab_area);
				update_allocation();
				rpage->mark_durty();
				rnd.add_damage_area(cnx.root_size);
			}
		}

	}
	//printf("exit %s\n", __FUNCTION__);
	return c != 0;
}

void page_t::split(notebook_t * nbk, split_type_e type) {
	rnd.add_damage_area(nbk->_allocation);
	split_t * split = new_split(type);
	nbk->_parent->replace(nbk, split);
	split->replace(0, nbk);
	notebook_t * n = new_notebook(notebook_to_viewport[nbk]);
	split->replace(0, n);

}

void page_t::split_left(notebook_t * nbk, window_t * c) {
	rnd.add_damage_area(nbk->_allocation);
	split_t * split = new_split(VERTICAL_SPLIT);
	nbk->_parent->replace(nbk, split);
	notebook_t * n = new_notebook(notebook_to_viewport[nbk]);
	split->replace(0, n);
	split->replace(0, nbk);
	insert_window_in_tree(c, n);
}

void page_t::split_right(notebook_t * nbk, window_t * c) {
	rnd.add_damage_area(nbk->_allocation);
	split_t * split = new_split(VERTICAL_SPLIT);
	nbk->_parent->replace(nbk, split);
	notebook_t * n = new_notebook(notebook_to_viewport[nbk]);
	split->replace(0, nbk);
	split->replace(0, n);
	insert_window_in_tree(c, n);
}

void page_t::split_top(notebook_t * nbk, window_t * c) {
	rnd.add_damage_area(nbk->_allocation);
	split_t * split = new_split(HORIZONTAL_SPLIT);
	nbk->_parent->replace(nbk, split);
	notebook_t * n = new_notebook(notebook_to_viewport[nbk]);
	split->replace(0, n);
	split->replace(0, nbk);
	insert_window_in_tree(c, n);
}

void page_t::split_bottom(notebook_t * nbk, window_t * c) {
	rnd.add_damage_area(nbk->_allocation);
	split_t * split = new_split(HORIZONTAL_SPLIT);
	nbk->_parent->replace(nbk, split);
	split->replace(0, nbk);
	notebook_t * n = new_notebook(notebook_to_viewport[nbk]);
	split->replace(0, n);
	insert_window_in_tree(c, n);
}

void page_t::notebook_close(notebook_t * src) {
	/* clean but not fast type checking, use rtti instead ? */
	std::set<split_t *>::iterator i = split_list.find((split_t *)src->_parent);
	if(i == split_list.end())
		return;

	split_t * ths = *i;

	/* if parent is viewport return */
	if(ths == 0)
		return;

	assert(src == ths->get_pack0() || src == ths->get_pack1());

	/* if notebook is default_pop, select another one */
	if (default_window_pop == src) {
		notebook_list.erase(src);
		/* if notebook list is empty we probably did something wrong */
		assert(!notebook_list.empty());
		default_window_pop = *(notebook_list.begin());
		/* put it back temporary since destroy will remove it */
		notebook_list.insert(src);
	}

	tree_t * dst = (src == ths->get_pack0()) ? ths->get_pack1() : ths->get_pack0();

	/* move all windows from src to default_window_pop */
	window_set_t windows = src->get_windows();
	for(window_set_t::iterator i = windows.begin(); i != windows.end(); ++i) {
		remove_window_from_tree((*i));
		insert_window_in_tree((*i), 0);
	}

	assert(ths->_parent != 0);
	/* remove this split from tree */
	ths->_parent->replace(ths, dst);

	/* cleanup */
	destroy(src);
	destroy(ths);
}

void page_t::update_popup_position(popup_notebook0_t * p, int x, int y, int w,
		int h, bool show_popup) {
	box_int_t old_area = p->get_absolute_extend();
	box_int_t new_area(x, y, w, h);
	p->reconfigure(new_area);
	rnd.add_damage_area(old_area);
	rnd.add_damage_area(new_area);
}

void page_t::fix_allocation(viewport_t & v) {
	printf("fix_allocation %dx%d+%d+%d\n", v.raw_aera.w, v.raw_aera.h,
			v.raw_aera.x, v.raw_aera.y);

	/* Partial struct content definition */
	enum {
		X_LEFT = 0,
		X_RIGHT = 1,
		X_TOP = 2,
		X_BOTTOM = 3,
		X_LEFT_START_Y = 4,
		X_LEFT_END_Y = 5,
		X_RIGHT_START_Y = 6,
		X_RIGHT_END_Y = 7,
		X_TOP_START_X = 8,
		X_TOP_END_X = 9,
		X_BOTTOM_START_X = 10,
		X_BOTTOM_END_X = 11,
	};

	v.effective_aera = v.raw_aera;
	int xtop = 0, xleft = 0, xright = 0, xbottom = 0;

	std::set<window_t *>::iterator j = windows_set.begin();
	while (j != windows_set.end()) {
		long const * ps = (*j)->get_partial_struct();
		if (ps) {
			window_t * c = (*j);
			/* this is very crappy, but there is a way to do it better ? */
			if (ps[X_LEFT] >= v.raw_aera.x
					&& ps[X_LEFT] <= v.raw_aera.x + v.raw_aera.w) {
				int top = std::max<int const>(ps[X_LEFT_START_Y], v.raw_aera.y);
				int bottom = std::min<int const>(ps[X_LEFT_END_Y],
						v.raw_aera.y + v.raw_aera.h);
				if (bottom - top > 0) {
					xleft = std::max<int const>(xleft,
							ps[X_LEFT] - v.effective_aera.x);
				}
			}

			if (cnx.root_size.w - ps[X_RIGHT] >= v.raw_aera.x
					&& cnx.root_size.w - ps[X_RIGHT]
							<= v.raw_aera.x + v.raw_aera.w) {
				int top = std::max<int const>(ps[X_RIGHT_START_Y],
						v.raw_aera.y);
				int bottom = std::min<int const>(ps[X_RIGHT_END_Y],
						v.raw_aera.y + v.raw_aera.h);
				if (bottom - top > 0) {
					xright = std::max<int const>(xright,
							(v.raw_aera.x + v.raw_aera.w)
									- (cnx.root_size.w - ps[X_RIGHT]));
				}
			}

			if (ps[X_TOP] >= v.raw_aera.y
					&& ps[X_TOP] <= v.raw_aera.y + v.raw_aera.h) {
				int left = std::max<int const>(ps[X_TOP_START_X], v.raw_aera.x);
				int right = std::min<int const>(ps[X_TOP_END_X],
						v.raw_aera.x + v.raw_aera.w);
				if (right - left > 0) {
					xtop = std::max<int const>(xtop, ps[X_TOP] - v.raw_aera.y);
				}
			}

			if (cnx.root_size.h - ps[X_BOTTOM] >= v.raw_aera.y
					&& cnx.root_size.h - ps[X_BOTTOM]
							<= v.raw_aera.y + v.raw_aera.h) {
				int left = std::max<int const>(ps[X_BOTTOM_START_X],
						v.raw_aera.x);
				int right = std::min<int const>(ps[X_BOTTOM_END_X],
						v.raw_aera.x + v.raw_aera.w);
				if (right - left > 0) {
					xbottom = std::max<int const>(xbottom,
							(v.effective_aera.h + v.effective_aera.y)
									- (cnx.root_size.h - ps[X_BOTTOM]));
				}
			}

		}
		++j;
	}

	v.effective_aera.x += xleft;
	v.effective_aera.w -= xleft + xright;
	v.effective_aera.y += xtop;
	v.effective_aera.h -= xtop + xbottom;

	printf("subarea %dx%d+%d+%d\n", v.effective_aera.w, v.effective_aera.h,
			v.effective_aera.x, v.effective_aera.y);
}

split_t * page_t::new_split(split_type_e type) {
	split_t * x = new split_t(type);
	split_list.insert(x);
	return x;
}

notebook_t * page_t::new_notebook(viewport_t * v) {
	notebook_t * x = new notebook_t();
	notebook_list.insert(x);
	notebook_to_viewport[x] = v;
	viewport_to_notebooks[v].insert(x);
	return x;
}

void page_t::destroy(split_t * x) {
	split_list.erase(x);
	delete x;
}

void page_t::destroy(notebook_t * x) {
	notebook_list.erase(x);
	viewport_to_notebooks[notebook_to_viewport[x]].erase(x);
	notebook_to_viewport.erase(x);
	delete x;
}

void page_t::destroy(window_t * c) {
	if(client_focused == c)
		set_focus(0);
	clear_sibbling_child(c);
	notebook_clients.erase(c);
	delete_window(c);
}

void page_t::unmap_set(window_set_t & set) {
	window_set_t::iterator i = set.begin();
	while (i != set.end()) {
		(*i)->unmap();
		++i;
	}
}

void page_t::map_set(window_set_t & set) {
	window_set_t::iterator i = set.begin();
	while (i != set.end()) {
		(*i)->map();
		++i;
	}
}

void page_t::print_state() {
	window_set_t::iterator i = notebook_clients.begin();
	while (i != notebook_clients.end()) {
		std::string n = (*i)->get_title();
		printf("Know window %lu %s\n", (*i)->get_xwin(), n.c_str());
		++i;
	}

}

viewport_t * page_t::find_viewport(window_t * w) {
	std::map<window_t *, notebook_t *>::iterator x = client_to_notebook.find(w);
	if(x == client_to_notebook.end())
		return 0;
	std::map<notebook_t *, viewport_t *>::iterator y = notebook_to_viewport.find(x->second);
	if(y == notebook_to_viewport.end())
		return 0;
	return y->second;
}

viewport_t * page_t::new_viewport(box_int_t & area) {
	viewport_t * v = new viewport_t(area);
	viewport_list.insert(v);
	notebook_t * n = new_notebook(v);
	v->add_notebook(n);
	return v;
}

void page_t::process_net_vm_state_client_messate(window_t * c, long type, Atom state_properties) {
	if(state_properties == None)
		return;

	char const * action;

	switch(type) {
	case _NET_WM_STATE_REMOVE:
		action = "remove";
		break;
	case _NET_WM_STATE_ADD:
		action = "add";
		break;
	case _NET_WM_STATE_TOGGLE:
		action = "toggle";
		break;
	default:
		action = "unknown";
		break;
	}


	char * name = cnx.get_atom_name(state_properties);
	printf("process wm_state %s %s\n", action, name);
	XFree(name);

	if (state_properties == cnx.atoms._NET_WM_STATE_FULLSCREEN) {
		switch (type) {
		case _NET_WM_STATE_REMOVE:
			printf("SET normal\n");
			if (!c->is_fullscreen())
				break;
			unfullscreen(c);
			break;
		case _NET_WM_STATE_ADD:
			printf("SET fullscreen\n");
			if (c->is_fullscreen())
				break;
			fullscreen(c);
			break;
		case _NET_WM_STATE_TOGGLE:
			printf("SET toggle\n");
			toggle_fullscreen(c);
			break;
		}
		update_allocation();
	} else if (state_properties == cnx.atoms._NET_WM_STATE_HIDDEN) {
		switch(type) {
		case _NET_WM_STATE_REMOVE:
			if(has_key(client_to_notebook, c)) {
				client_to_notebook[c]->activate_client(c);
			} else {
				c->map();
			}
			break;
		case _NET_WM_STATE_ADD:
			if(has_key(client_to_notebook, c)) {
				client_to_notebook[c]->iconify_client(c);
			} else {
				c->unmap();
			}
			break;
		case _NET_WM_STATE_TOGGLE:
			if(has_key(client_to_notebook, c)) {
				if (c->get_wm_state() == IconicState) {
					client_to_notebook[c]->activate_client(c);
				} else {
					client_to_notebook[c]->iconify_client(c);
				}
			} else {
				if(c->is_map()) {
					c->unmap();
				} else {
					c->map();
				}
			}
			break;
		default:
			break;
		}
	}

}

void page_t::update_transient_for(window_t * w) {
	/* remove sibbling_child if needed */
	clear_sibbling_child(w);

	if(w->transient_for() != None) {
		window_t * t = get_window_t(w->transient_for());
		if(t != 0) {
			t->add_sibbling_child(w);
		}
	}
}

void page_t::safe_raise_window(window_t * w) {
	window_set_t raised_window;
	window_list_t raise_list;
	window_list_t raise_next;

	raise_next.push_back(w);
	while (raise_next.size() > 0) {
		raise_list = raise_next;
		raise_next.clear();
		for (window_list_t::iterator i = raise_list.begin();
				i != raise_list.end(); ++i) {
			std::set<window_t *> childs = (*i)->get_sibbling_childs();
			raise_next.insert(raise_next.end(), childs.begin(),
					childs.end());
			cnx.raise_window((*i)->get_xwin());
			if(has_key(window_to_floating_window, (*i)->get_xwin())) {
				cnx.raise_window(window_to_floating_window[(*i)->get_xwin()]->border->get_xwin());
			}
		}
	}

	for (window_set_t::iterator i = windows_set.begin(); windows_set.end() != i;
			++i) {
		if ((*i)->get_window_type() == PAGE_NOTIFY_TYPE) {
			cnx.raise_window((*i)->get_xwin());
		}
	}

}

void page_t::clear_sibbling_child(window_t * w) {
	/* remove sibbling_child if needed */
	for(window_set_t::iterator i = windows_set.begin(); i != windows_set.end(); ++i) {
		(*i)->remove_sibbling_child(w);
	}
}

renderable_window_t * page_t::new_renderable_window(window_t * w) {
	renderable_window_t * x = new renderable_window_t(w);
	window_to_renderable_context[w->get_xwin()] = x;
	return x;
}

void page_t::destroy(renderable_window_t * w) {
	window_to_renderable_context.erase(w->w->get_xwin());
	delete w;
}

floating_window_t * page_t::new_floating_window(window_t * w) {
	Window parent = cnx.create_window(0, 0, 800, 800);
	XWindowAttributes xwa;
	cnx.get_window_attributes(parent, &xwa);
	window_t * border = new_window(parent, xwa);
	rnd.add(new_renderable_window(border));
	border->select_input(ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | PropertyChangeMask | SubstructureRedirectMask);
	border->read_all();

	floating_window_t * fw = new floating_window_t(w, border);
	xfloating_window_to_floating_window[parent] = fw;
	window_to_floating_window[w->get_xwin()] = fw;
	floating_windows.insert(parent);
	return fw;
}

void page_t::destroy(floating_window_t * w) {
	floating_windows.erase(w->border->get_xwin());
	window_to_floating_window.erase(w->w->get_xwin());
	xfloating_window_to_floating_window.erase(w->border->get_xwin());
	delete w;
}

}
