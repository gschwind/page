#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <list>
#include <map>
#include <string>
#include <pango/pangoxft.h>
#include <pango/pango.h>

#include <glib-object.h>

#include "gtk_xwindow_handler.h"

struct client {
	std::string name;
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, oldbw;
	unsigned int tags;
	Bool isfixed, isfloating, isurgent, oldstate;
	Window xwin;
	GdkWindow * gwin;
	GtkNotebook * contener;
	GtkXWindowHandler * widget;

	void resize(int x, int y, int w, int h);
	void update_layout();
};

GdkFilterReturn tabwm_process_map_request(XEvent * e);
GdkFilterReturn tabwm_process_destroy(XEvent * e);

struct global_t {
	Display * dpy;
	int screen;
	Window root;
	int sw, sh;
	std::list<client *> clients;
	std::map<Window, client *> window_to_client;

	XFontSet font_set;
	XFontStruct * xfont;
	GC font_gc;

	Window test_window;
};

struct tab_bar {
	int w, h;
	int x, y;
};

global_t global;

void client::update_layout() {
	XConfigureEvent ce;
	ce.type = ConfigureNotify;
	ce.display = global.dpy;
	ce.event = xwin;
	ce.window = xwin;
	ce.x = x;
	ce.y = y;
	ce.width = w;
	ce.height = h;
	ce.border_width = bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(global.dpy, xwin, False, StructureNotifyMask, (XEvent *) &ce);
}

unsigned long get_color(const char * colstr) {
	Colormap cmap = DefaultColormap(global.dpy, global.screen);
	XColor color;
	if (!XAllocNamedColor(global.dpy, cmap, colstr, &color, &color)) {
		printf("error, cannot allocate color '%s'\n", colstr);
		exit(0);
	}
	return color.pixel;
}

void manage(Window w, XWindowAttributes * wa);
void map_request(XEvent * e);
void configure_notify(XEvent * e);

enum {
	WMProtocols, WMDelete, WMState, WMLast
};

void (*handler[LASTEvent])(XEvent *);

GdkFilterReturn (*gdk_handler[LASTEvent])(XEvent *);

char const * event_name[LASTEvent] = { 0, 0, "KeyPress", "KeyRelease",
		"ButtonPress", "ButtonRelease", "MotionNotify", "EnterNotify",
		"LeaveNotify", "FocusIn", "FocusOut", "KeymapNotify", "Expose",
		"GraphicsExpose", "NoExpose", "VisibilityNotify", "CreateNotify",
		"DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
		"ReparentNotify", "ConfigureNotify", "ConfigureRequest",
		"GravityNotify", "ResizeRequest", "CirculateNotify",
		"CirculateRequest", "PropertyNotify", "SelectionClear",
		"SelectionRequest", "SelectionNotify", "ColormapNotify",
		"ClientMessage", "MappingNotify", "GenericEvent" };

char const * gdk_event_name[] = { "GDK_NOTHING", "GDK_DELETE", "GDK_DESTROY",
		"GDK_EXPOSE", "GDK_MOTION_NOTIFY", "GDK_BUTTON_PRESS",
		"GDK_2BUTTON_PRESS", "GDK_3BUTTON_PRESS", "GDK_BUTTON_RELEASE",
		"GDK_KEY_PRESS", "GDK_KEY_RELEASE", "GDK_ENTER_NOTIFY",
		"GDK_LEAVE_NOTIFY", "GDK_FOCUS_CHANGE", "GDK_CONFIGURE", "GDK_MAP",
		"GDK_UNMAP", "GDK_PROPERTY_NOTIFY", "GDK_SELECTION_CLEAR",
		"GDK_SELECTION_REQUEST", "GDK_SELECTION_NOTIFY", "GDK_PROXIMITY_IN",
		"GDK_PROXIMITY_OUT", "GDK_DRAG_ENTER", "GDK_DRAG_LEAVE",
		"GDK_DRAG_MOTION", "GDK_DRAG_STATUS", "GDK_DROP_START",
		"GDK_DROP_FINISHED", "GDK_CLIENT_EVENT", "GDK_VISIBILITY_NOTIFY",
		"GDK_NO_EXPOSE", "GDK_SCROLL", "GDK_WINDOW_STATE", "GDK_SETTING",
		"GDK_OWNER_CHANGE", "GDK_GRAB_BROKEN", "GDK_DAMAGE", "GDK_EVENT_LAST" };

gchar const * get_gdk_event_name(GdkEventType ev_type) {
	return gdk_event_name[ev_type + 1];
}

client * window_to_client(Window w) {
	std::map<Window, client *>::iterator i = global.window_to_client.find(w);
	if (i != global.window_to_client.end())
		return (*i).second;
	else
		return 0;
}

bool get_root_ptr(int * x, int * y) {
	int di;
	unsigned int dui;
	Window dummy;
	return XQueryPointer(global.dpy, global.root, &dummy, &dummy, x, y, &di,
			&di, &dui);
}

void client::resize(int x, int y, int w, int h) {
	printf("resize to %dx%d+%d+%d\n", x, y, w, h);
	XWindowChanges wc;
	oldx = x;
	this->x = wc.x = x;
	oldy = y;
	this->y = wc.y = y;
	oldw = w;
	this->w = wc.width = w;
	oldh = h;
	this->h = wc.height = h;
	wc.border_width = 0;
	XConfigureWindow(global.dpy, xwin, CWX | CWY | CWWidth | CWHeight
			|CWBorderWidth, &wc);
	update_layout();
	XSync(global.dpy, False);
}

void unfocus(client *c, Bool setfocus) {
}

void init_hander() {
	for (gint i = 0; i < LASTEvent; ++i) {
		gdk_handler[i] = NULL;
	}

	gdk_handler[MapRequest] = tabwm_process_map_request;
	gdk_handler[DestroyNotify] = tabwm_process_destroy;
}

bool get_text_prop(Window w, Atom atom, std::string & s) {
	char * * list = 0;
	int n;
	XTextProperty name;
	XGetTextProperty(global.dpy, w, &name, atom);
	if (!name.nitems)
		return false;
	if (name.encoding == XA_STRING)
		s = ((char *) name.value);
	else {
		if (XmbTextPropertyToTextList(global.dpy, &name, &list, &n) >= Success
				&& n > 0 && *list) {
			s = *list;
			XFreeStringList(list);
		}
	}
	XFree(name.value);
	return true;
}

void process_event(void) {
	XEvent ev;
	/* main event loop */
	XSync(global.dpy, False);
	while (!XNextEvent(global.dpy, &ev)) {
		if (handler[ev.type])
			handler[ev.type](&ev); /* call handler */

	}
}

void configure_notify(XEvent * e) {
	printf("Configure Notify %d %d\n", (int) e->xconfigure.window,
			(int) global.test_window);
	if (e->xconfigure.window == global.test_window) {
		XDrawString(global.dpy, global.test_window, global.font_gc, 0, 0,
				"hello world", 1);
		XDrawLine(global.dpy, global.test_window, global.font_gc, 10, 10, 20,
				20);
		XFlush(global.dpy);
	}
}

GdkDisplay * dpy;
GdkScreen * scn;
GdkWindow * root;
gint sw, sh;
GdkWindow * main_win;

/* this function is call on each event, it can filter, or translate event to another */
static GdkFilterReturn event_callback(GdkXEvent * xevent, GdkEvent * event,
		gpointer data) {
	XEvent * e = (XEvent *) (xevent);
	if (e->type != MotionNotify)
		printf("process event %d %d : %s\n", (int) e->xany.window, (int)e->xany.serial,
				event_name[e->type]);
	if (gdk_handler[e->type])
		return gdk_handler[e->type](e); /* call handler */

	return GDK_FILTER_CONTINUE;
}

static GMainLoop * main_loop = NULL;
GtkWidget * notebook;
GtkWidget * win;

/* this function is call when a client want map a new window
 * the evant is on root window, e->xmaprequest.window is the window that request the map */
GdkFilterReturn tabwm_process_map_request(XEvent * e) {
	printf("call %s\n", __FUNCTION__);
	if (e->xmaprequest.window == GDK_WINDOW_XID(gtk_widget_get_window(win))) {
		/* just map, do noting else */
		XMapWindow(e->xmaprequest.display, e->xmaprequest.window);
		printf("try mapping myself\n");
		return GDK_FILTER_REMOVE;
	}
	XWindowAttributes wa;
	if (!XGetWindowAttributes(e->xmaprequest.display, e->xmaprequest.window,
			&wa)) {
		/* cannot get attribute, just ignore this request */
		return GDK_FILTER_REMOVE;
	}

	/* manage the window but do not map it
	 * Map will be made on widget show
	 */
	manage(e->xmaprequest.window, &wa);
	return GDK_FILTER_REMOVE;

}

client * find_client(Window w) {

	std::list<client *>::iterator i = global.clients.begin();
	while (i != global.clients.end()) {
		if ((*i)->xwin == w)
			return *i;
		++i;
	}

	return 0;
}

GdkFilterReturn tabwm_process_destroy(XEvent * e) {
	printf("call %s\n", __FUNCTION__);
	client * c = 0;
	if (c = find_client(e->xdestroywindow.window)) {
		gtk_notebook_remove_page((c->contener), gtk_notebook_page_num(
				c->contener, GTK_WIDGET(c->widget)));
		global.clients.remove(c);
		delete c;
	}
	gtk_widget_draw(GTK_WIDGET(notebook), NULL);
	return GDK_FILTER_REMOVE;
}

void manage(Window w, XWindowAttributes * wa) {
	printf("call %s\n", __FUNCTION__);
	if (w == GDK_WINDOW_XID(gtk_widget_get_window(win))) {
		printf("try manage myself\n");
		return;
	}
	client * c, *t = 0;
	Window trans = None;
	XWindowChanges wc;

	c = new client;

	c->xwin = w;
	c->gwin = gdk_window_foreign_new(w);

	printf("Manage %d %dx%d+%d+%d\n", (gint) w, wa->width, wa->height, wa->x,
			wa->y);

	gchar * title = g_strdup_printf("%d", (gint) w);
	GtkWidget * label = gtk_label_new(title);
	GtkWidget * content = gtk_xwindow_handler_new();
	gtk_xwindow_handler_set_gwindow(GTK_XWINDOW_HANDLER(content), c->gwin);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), content, label);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), content, TRUE);
	gtk_widget_show_all(GTK_WIDGET(notebook));
	gtk_widget_draw(GTK_WIDGET(notebook), NULL);
	c->contener = GTK_NOTEBOOK(notebook);
	c->widget = GTK_XWINDOW_HANDLER(content);
	global.clients.push_back(c);

}

void scan() {
	printf("call %s\n", __FUNCTION__);
	unsigned int i, num;
	Window d1, d2, *wins = 0;
	XWindowAttributes wa;

	/* ask for child of current root window */
	if (XQueryTree(gdk_x11_display_get_xdisplay(dpy), GDK_WINDOW_XID(root),
			&d1, &d2, &wins, &num)) {
		for (i = 0; i < num; ++i) {
			/* try to get Window Atribute */
			if (!XGetWindowAttributes(gdk_x11_display_get_xdisplay(dpy),
					wins[i], &wa))
				continue;
			if (wa.map_state == IsViewable)
				manage(wins[i], &wa);
		}
	}

	if (wins)
		XFree(wins);
}

#if 0
static void
sigterm_handler (int signum)
{
  if (sigterm_pipe_fds[1] >= 0)
    {
      int dummy;

      dummy = write (sigterm_pipe_fds[1], "", 1);
      close (sigterm_pipe_fds[1]);
      sigterm_pipe_fds[1] = -1;
    }
}

#endif

int main(int argc, char * * argv) {
	gdk_init(&argc, &argv);
	gtk_init(&argc, &argv);
	//global.dpy = XOpenDisplay(0);
	dpy = gdk_display_open(NULL);
	//global.screen = DefaultScreen(global.dpy);
	scn = gdk_display_get_default_screen(dpy);
	//global.root = RootWindow(global.dpy, global.screen);
	root = gdk_screen_get_root_window(scn);
	//global.sw = DisplayWidth(global.dpy, global.screen);
	//global.sh = DisplayHeight(global.dpy, global.screen);
	gdk_window_get_geometry(root, NULL, NULL, &sw, &sh, NULL);
	printf("display size %d %d\n", sw, sh);

	gint x, y, width, height, depth;

	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(win), sw, sh);

	notebook = gtk_notebook_new();
	GtkWidget * label0 = gtk_label_new("hello world 0");
	GtkWidget * label1 = gtk_label_new("hello world 1");
	GtkWidget * label2 = gtk_label_new("hello world 2");
	GtkWidget * tab_label0 = gtk_label_new("label 0");
	GtkWidget * tab_label1 = gtk_label_new("label 1");
	GtkWidget * tab_label2 = gtk_label_new("label 2");

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), label0, tab_label0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), label1, tab_label1);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), label2, tab_label2);
	gtk_container_add(GTK_CONTAINER(win), notebook);

	scan();
	gtk_widget_show_all(GTK_WIDGET(win));

	main_win = gdk_window_get_toplevel(gtk_widget_get_root_window(win));
	printf("my main window %p\n", main_win);

	gint n_screen = gdk_display_get_n_screens(dpy);
	printf("Found %d screen(s)\n", n_screen);

	init_hander();
	/* redirect map/unmap, liten structure modifiction */
	XSelectInput(gdk_x11_display_get_xdisplay(dpy), GDK_WINDOW_XID(root),
			SubstructureRedirectMask | SubstructureNotifyMask);
	/* get all unhandled event */
	gdk_window_add_filter(NULL, event_callback, NULL);

	gtk_widget_draw(GTK_WIDGET(win), NULL);

	main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(main_loop);

}

