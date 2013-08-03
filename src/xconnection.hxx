/*
 * xconnection.hxx
 *
 * copyright (2011) Benoit Gschwind
 *
 */

#ifndef XCONNECTION_HXX_
#define XCONNECTION_HXX_

#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xrandr.h>

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <algorithm>
#include <list>
#include <vector>
#include <stdexcept>
#include <map>

#include "box.hxx"
#include "atoms.hxx"
#include "window.hxx"

//#define cnx_printf(args...) printf(args)

#define cnx_printf(args...) do { } while(false)

using namespace std;

namespace page {

/* Datas to print X errors */
static char const * const x_function_codes[] = { //
				"", //
				"X_CreateWindow", //   1
				"X_ChangeWindowAttributes", //   2
				"X_GetWindowAttributes", //   3
				"X_DestroyWindow", //   4
				"X_DestroySubwindows", //   5
				"X_ChangeSaveSet", //   6
				"X_ReparentWindow", //   7
				"X_MapWindow", //   8
				"X_MapSubwindows", //   9
				"X_UnmapWindow", //  10
				"X_UnmapSubwindows", //  11
				"X_ConfigureWindow", //  12
				"X_CirculateWindow", //  13
				"X_GetGeometry", //  14
				"X_QueryTree", //  15
				"X_InternAtom", //  16
				"X_GetAtomName", //  17
				"X_ChangeProperty", //  18
				"X_DeleteProperty", //  19
				"X_GetProperty", //  20
				"X_ListProperties", //  21
				"X_SetSelectionOwner", //  22
				"X_GetSelectionOwner", //  23
				"X_ConvertSelection", //  24
				"X_SendEvent", //  25
				"X_GrabPointer", //  26
				"X_UngrabPointer", //                27
				"X_GrabButton", //                   28
				"X_UngrabButton", //                 29
				"X_ChangeActivePointerGrab", //      30
				"X_GrabKeyboard", //                 31
				"X_UngrabKeyboard", //               32
				"X_GrabKey", //                      33
				"X_UngrabKey", //                    34
				"X_AllowEvents", //                  35
				"X_GrabServer", //                   36
				"X_UngrabServer", //                 37
				"X_QueryPointer", //                 38
				"X_GetMotionEvents", //              39
				"X_TranslateCoords", //              40
				"X_WarpPointer", //                  41
				"X_SetInputFocus", //                42
				"X_GetInputFocus", //                43
				"X_QueryKeymap", //                  44
				"X_OpenFont", //                     45
				"X_CloseFont", //                    46
				"X_QueryFont", //                    47
				"X_QueryTextExtents", //             48
				"X_ListFonts", //                    49
				"X_ListFontsWithInfo", //    	    50
				"X_SetFontPath", //                  51
				"X_GetFontPath", //                  52
				"X_CreatePixmap", //                 53
				"X_FreePixmap", //                   54
				"X_CreateGC", //                     55
				"X_ChangeGC", //                     56
				"X_CopyGC", //                       57
				"X_SetDashes", //                    58
				"X_SetClipRectangles", //            59
				"X_FreeGC", //                       60
				"X_ClearArea", //                    61
				"X_CopyArea", //                     62
				"X_CopyPlane", //                    63
				"X_PolyPoint", //                    64
				"X_PolyLine", //                     65
				"X_PolySegment", //                  66
				"X_PolyRectangle", //                67
				"X_PolyArc", //                      68
				"X_FillPoly", //                     69
				"X_PolyFillRectangle", //            70
				"X_PolyFillArc", //                  71
				"X_PutImage", //                     72
				"X_GetImage", //                     73
				"X_PolyText8", //                    74
				"X_PolyText16", //                   75
				"X_ImageText8", //                   76
				"X_ImageText16", //                  77
				"X_CreateColormap", //               78
				"X_FreeColormap", //                 79
				"X_CopyColormapAndFree", //          80
				"X_InstallColormap", //              81
				"X_UninstallColormap", //            82
				"X_ListInstalledColormaps", //       83
				"X_AllocColor", //                   84
				"X_AllocNamedColor", //              85
				"X_AllocColorCells", //              86
				"X_AllocColorPlanes", //             87
				"X_FreeColors", //                   88
				"X_StoreColors", //                  89
				"X_StoreNamedColor", //              90
				"X_QueryColors", //                  91
				"X_LookupColor", //                  92
				"X_CreateCursor", //                 93
				"X_CreateGlyphCursor", //            94
				"X_FreeCursor", //                   95
				"X_RecolorCursor", //                96
				"X_QueryBestSize", //                97
				"X_QueryExtension", //               98
				"X_ListExtensions", //               99
				"X_ChangeKeyboardMapping", //        100
				"X_GetKeyboardMapping", //           101
				"X_ChangeKeyboardControl", //        102
				"X_GetKeyboardControl", //           103
				"X_Bell", //                         104
				"X_ChangePointerControl", //         105
				"X_GetPointerControl", //            106
				"X_SetScreenSaver", //               107
				"X_GetScreenSaver", //               108
				"X_ChangeHosts", //                  109
				"X_ListHosts", //                    110
				"X_SetAccessControl", //             111
				"X_SetCloseDownMode", //             112
				"X_KillClient", //                   113
				"X_RotateProperties", //	            114
				"X_ForceScreenSaver", //	            115
				"X_SetPointerMapping", //            116
				"X_GetPointerMapping", //            117
				"X_SetModifierMapping", //	        118
				"X_GetModifierMapping", //	        119
				"X_NoOperation" //                   127

		};

static char const * const xdamage_func[] = { "X_DamageQueryVersion",
		"X_DamageCreate", "X_DamageDestroy", "X_DamageSubtract", "X_DamageAdd" };

static char const * const xcomposite_request_name[] = {
		"X_CompositeQueryVersion", "X_CompositeRedirectWindow",
		"X_CompositeRedirectSubwindows", "X_CompositeUnredirectWindow",
		"X_CompositeUnredirectSubwindows",
		"X_CompositeCreateRegionFromBorderClip", "X_CompositeNameWindowPixmap",
		"X_CompositeGetOverlayWindow", "X_CompositeReleaseOverlayWindow" };

struct event_t {
	unsigned long serial;
	int type;
};

struct xevent_handler_t {
	virtual ~xevent_handler_t() {
	}
	virtual void process_event(XEvent const & e) = 0;
};


/**
 * Structure to handle X connection context.
 */
struct xconnection_t {

private:
	/* that allow error_handler to bind display to connection */
	static std::map<Display *, xconnection_t *> open_connections;
	std::map<int, char const * const *> extension_request_name_map;

	/* main display connection */
	Display * _dpy;

public:

	Display * const & dpy;

	/* main screen */
	int screen;
	/* the root window ID */
	Window xroot;
	/* overlay composite */
	Window composite_overlay;
	/* root window atributes */
	XWindowAttributes root_wa;
	/* size of default root window */
	box_t<int> root_size;

	std::list<event_t> pending;

	int grab_count;

	int (*old_error_handler)(Display * _dpy, XErrorEvent * ev);

	int connection_fd;

	std::list<xevent_handler_t *> event_handler_list;

	map<Atom, string> _atom_to_name;
	map<string, Atom> _name_to_atom;

	/*damage event handler */
	int damage_opcode, damage_event, damage_error;

	/* composite event handler */
	int composite_opcode, composite_event, composite_error;

	/* fixes event handler */
	int fixes_opcode, fixes_event, fixes_error;

	/* xshape extension handler */
	int xshape_opcode, xshape_event, xshape_error;

	/* xrandr extension handler */
	int xrandr_opcode, xrandr_event, xrandr_error;

	map<Window, window_t *> window_cache;

public:

	xconnection_t();

	void grab();
	void ungrab();
	bool is_not_grab();

	template<typename T, unsigned int SIZE>
	T * get_properties(Window win, char const * prop, char const * type, unsigned int *num) {
		int res;
		unsigned char * xdata = 0;
		Atom ret_type;
		int ret_size;
		unsigned long int ret_items, bytes_left;
		T * result = 0;
		T * data;

		res = get_window_property(win, prop, 0L,
				std::numeric_limits<int>::max(), False, type, &ret_type,
				&ret_size, &ret_items, &bytes_left, &xdata);
		if (res == Success) {
			if (bytes_left != 0)
				printf("some bits lefts\n");
			if (ret_size == SIZE && ret_items > 0) {
				result = new T[ret_items];
				data = reinterpret_cast<T*>(xdata);
				for (unsigned int i = 0; i < ret_items; ++i) {
					result[i] = data[i];
					//printf("%d %p\n", data[i], &data[i]);
				}
			}
			if (num)
				*num = ret_items;
			XFree(xdata);
		}
		return result;
	}

	long * get_properties32(Window win, char const * prop, char const * type,
			unsigned int *num) {
		return this->get_properties<long, 32>(win, prop, type, num);
	}

	short * get_properties16(Window win, char const * prop, char const * type,
			unsigned int *num) {
		return this->get_properties<short, 16>(win, prop, type, num);
	}

	char * get_properties8(Window win, char const * prop, char const * type,
			unsigned int *num) {
		return this->get_properties<char, 8>(win, prop, type, num);
	}

	static int error_handler(Display * dpy, XErrorEvent * ev);
	void allow_input_passthrough(Window w);

	void unmap(Window w);
	void reparentwindow(Window w, Window parent, int x, int y);
	void map_window(Window w);
	bool find_pending_event(event_t & e);

	unsigned long int last_serial;

	class serial_filter {
		unsigned long int last_serial;
	public:
		serial_filter(unsigned long int serial);
		bool operator() (event_t e);
	};

	void xnextevent(XEvent * ev);

	/* this fonction come from xcompmgr
	 * it is intend to make page as composite manager */
	bool register_cm(Window w);
	bool register_wm(bool replace, Window w);
	void add_to_save_set(Window w);
	void remove_from_save_set(Window w);
	void select_input(Window w, long int mask);
	void move_resize(Window w, box_int_t const & size);
	void set_window_border_width(Window w, unsigned int width);

	void raise_window(Window w);
	void add_event_handler(xevent_handler_t * func);
	void remove_event_handler(xevent_handler_t * func);
	void process_next_event();
	bool process_check_event();

	int change_property(Window w, char const * property, char const * type, int format,
			int mode, unsigned char const * data, int nelements);

	Status get_window_attributes(Window w,
			XWindowAttributes * window_attributes_return);

	Status get_text_property(Window w, XTextProperty * text_prop_return,
			char const * property);

	int lower_window(Window w);

	int configure_window(Window w, unsigned int value_mask,
			XWindowChanges * values);


	Status send_event(Window w, Bool propagate, long event_mask,
			XEvent* event_send);

	int set_input_focus(Window focus, int revert_to, Time time);

	int get_window_property(Window w, char const * property, long long_offset,
			long long_length, Bool c_delete, char const * req_type,
			Atom* actual_type_return, int* actual_format_return,
			unsigned long* nitems_return, unsigned long* bytes_after_return,
			unsigned char** prop_return);

	XWMHints * get_wm_hints(Window w);

	Window create_window(Visual * visual, int x, int y, unsigned w, unsigned h);

	void fake_configure(Window w, box_int_t location, int border_width);
	string const & get_atom_name(Atom a);

	void init_composite_overlay();

	void add_select_input(Window w, long mask);

	Atom get_atom(char const * name);

	string const & atom_to_name(Atom a);
	Atom const & name_to_atom(char const * name);

	window_t * get_window_t(Window w) {
		map<Window, window_t *>::iterator i = window_cache.find(w);
		if(i != window_cache.end()) {
			return i->second;
		} else {
			window_t * x = new window_t(*this, w);
			window_cache[w] = x;
			return x;
		}
	}

	window_t * destroy_cache(Window w) {
		map<Window, window_t *>::iterator i = window_cache.find(w);
		if(i != window_cache.end()) {
			delete i->second;
			window_cache.erase(i);
		}
	}

	Atom A(char const * name) {
		return get_atom(name);
	}

private:
	char * _get_atom_name(Atom atom);
	static int _return_true(Display * dpy, XEvent * ev, XPointer none);


};

}

#endif /* XCONNECTION_HXX_ */
