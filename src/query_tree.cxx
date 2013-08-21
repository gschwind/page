/*
 * query_tree.cxx
 *
 * copyright (2012) Benoit Gschwind
 *
 */

#include <X11/Xutil.h>

#include <X11/Xlib.h>
#include <stdexcept>
#include <cstdio>
#include <string>
#include <limits>

Atom WM_STATE;
Atom WM_NAME;
Atom _NET_WM_WINDOW_TYPE;
Atom ATOM;

template<typename T, unsigned int SIZE>
T * get_properties(Display * dpy, Window win, Atom prop, Atom type, unsigned int *num) {
	bool ret = false;
	int res;
	unsigned char * xdata = 0;
	Atom ret_type;
	int ret_size;
	unsigned long int ret_items, bytes_left;
	T * result = 0;
	T * data;

	res = XGetWindowProperty(dpy, win, prop, 0L,
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

void print_net_wm_window_type(Display * dpy, Window w) {
	unsigned int num;
	long * val = get_properties<long, 32>(dpy, w, _NET_WM_WINDOW_TYPE, ATOM,
			&num);
	if (val) {
		/* use the first value that we know about in the array */
		for (unsigned i = 0; i < num; ++i) {
			char * name = XGetAtomName(dpy, val[i]);
			printf("_NET_WM_WINDOW_TYPE = \"%s\"\n", name);
			XFree(name);
		}
		delete[] val;
	}
}

void print_window_attributes(Window w, XWindowAttributes & wa) {
	return;
	printf(">>> Window: #%lu\n", w);
	printf("> size: %dx%d+%d+%d\n", wa.width, wa.height, wa.x, wa.y);
	printf("> border_width: %d\n", wa.border_width);
	//printf("> depth: %d\n", wa.depth);
	//printf("> visual #%p\n", wa.visual);
	printf("> root: #%lu\n", wa.root);
	printf("> class: %s\n",
			(wa.c_class == InputOutput) ? "InputOutput" : "InputOnly");
	//printf("> bit_gravity: %d\n", wa.bit_gravity);
	//printf("> win_gravity: %d\n", wa.win_gravity);
	//printf("> backing_store: %dlx\n", wa.backing_store);
	//printf("> backing_planes: %lx\n", wa.backing_planes);
	//printf("> backing_pixel: %lx\n", wa.backing_pixel);
	//printf("> save_under: %d\n", wa.save_under);
	//printf("> colormap: ?\n");
	//printf("> all_event_masks: %08lx\n", wa.all_event_masks);
	//printf("> your_event_mask: %08lx\n", wa.your_event_mask);
	//printf("> do_not_propagate_mask: %08lx\n", wa.do_not_propagate_mask);
	printf("> override_redirect: %d\n", wa.override_redirect);
	//printf("> screen: %p\n", wa.screen);

	char const * state = 0;
	switch (wa.map_state) {
	case IsUnmapped:
		state = "IsUnmapped";
		break;
	case IsUnviewable:
		state = "IsUnviewable";
		break;
	case IsViewable:
		state = "IsViewable";
		break;
	default:
		state = "Unknow";
		break;
	}

	printf("> map_status: %s\n", state);

}

std::string update_vm_name(Display * dpy, Window w) {
	char **list = NULL;
	XTextProperty name;
	XGetTextProperty(dpy, w, &name, WM_NAME);
	if (!name.nitems) {
		XFree(name.value);
		return std::string("NotSet");
	}
	std::string wm_name = (char const *) name.value;
	XFree(name.value);
	return wm_name;
}

long get_window_state(Display * dpy, Window w);

void scan() {
	XWindowAttributes root_wa;
	printf("call %s\n", __PRETTY_FUNCTION__);

	Display * dpy = XOpenDisplay(0);

	WM_STATE = XInternAtom(dpy, "WM_STATE", False);
	WM_NAME = XInternAtom(dpy, "WM_NAME", False);
	_NET_WM_WINDOW_TYPE = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	ATOM = XInternAtom(dpy, "ATOM", False);

	if (dpy == NULL) {
		throw std::runtime_error("Could not open display");
	} else {
		printf("Open display : Success\n");
	}

	int screen = DefaultScreen(dpy);
	Window xroot = DefaultRootWindow(dpy);
	if (!XGetWindowAttributes(dpy, xroot, &root_wa)) {
		throw std::runtime_error("Cannot get root window attributes");
	} else {
		printf("Get root windows attribute Success\n");
	}

	unsigned int num;
	Window d1, d2, *wins = 0;
	XWindowAttributes wa;
	if (XQueryTree(dpy, xroot, &d1, &d2, &wins, &num)) {
		for (unsigned i = 0; i < num; ++i) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			print_window_attributes(wins[i], wa);
			print_net_wm_window_type(dpy, wins[i]);

			long toto = get_window_state(dpy, wins[i]);
			switch (toto) {
			case NormalState:
				printf("> WM_STATE: Normal\n");
				break;
			case IconicState:
				printf("> WM_STATE: IconicState\n");
				break;
			case WithdrawnState:
				printf("> WM_STATE: WithdrawnState\n");
				break;
			default:
				printf("> WM_STATE: NotSET\n");
				break;
			}

			std::string name = update_vm_name(dpy, wins[i]);
			printf("> name: %s\n", name.c_str());

			if (wa.c_class == InputOnly) {
				printf("> SET Ignore\n");
			} else {
				if (toto == IconicState || toto == NormalState) {
					printf("> SET Auto\n");
				} else if (toto == WithdrawnState) {
					printf("> SET Withdrawn\n");
				} else {

					if (wa.override_redirect && wa.map_state != IsUnmapped) {
						printf("> SET POPUP\n");
					} else if (wa.map_state == IsUnmapped
							&& wa.override_redirect == 0) {
						printf("> SET Withdrawn\n");
					} else if (wa.override_redirect
							&& wa.map_state == IsUnmapped) {
						printf("> SET Ignore\n");
					} else if (!wa.override_redirect
							&& wa.map_state != IsUnmapped) {
						printf("> SET Auto\n");
					} else {
						printf("> OUPSSSSS\n");

					}
				}

			}

			printf("\n\n");

		}

		XFree(wins);
	}
}

long get_window_state(Display * dpy, Window w) {
	int format;
	long result = -1;
	long * p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, WM_STATE, 0L, 2L, False, WM_STATE, &real,
			&format, &n, &extra, (unsigned char **) &p) != Success)
		return -1;

	if (n != 0 && format == 32 && real == WM_STATE) {
		result = p[0];
	} else {
		printf("Error in WM_STATE %lu %d %lu\n", n, format, real);
		return -1;
	}
	XFree(p);
	return result;
}

int main(int argc, char **argv) {
	scan();
}
