/*
 * list_visual.cxx
 *
 *  Created on: 18 juil. 2013
 *      Author: bg
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstdio>

int main(int argc, char ** argv) {

	Display * dpy = XOpenDisplay(0);

	int count = 0;
	/* get all visual list */
	XVisualInfo * list;
	list = XGetVisualInfo(dpy, 0, 0, &count);

	for(int i = 0; i < count; ++i) {
		printf("Visual #%d\n", i);
		printf(" visual:         %p\n", list[i].visual);
		printf(" screen:         %d\n", list[i].screen);
		printf(" depth:          %d\n", list[i].depth);

		char const * c_class;
		switch(list[i].c_class) {
		case StaticGray:
			c_class = "StaticGrey";
			break;
		case StaticColor:
			c_class = "StaticColor";
			break;
		case TrueColor:
			c_class = "TrueColor";
			break;
		case GrayScale:
			c_class = "GrayScale";
			break;
		case PseudoColor:
			c_class = "PseudoColor";
			break;
		case DirectColor:
			c_class = "DirectColor";
			break;
		default:
			c_class = "Unknown";
		}

		printf(" class:          %s\n", c_class);

		printf(" red_mask:       0x%016lx\n", list[i].red_mask);
		printf(" green_mask:     0x%016lx\n", list[i].green_mask);
		printf(" blue_mask:      0x%016lx\n", list[i].blue_mask);

		printf(" colormap_size:  %d\n", list[i].colormap_size);
		printf(" bits_per_rgb:   %d\n", list[i].bits_per_rgb);
		printf("\n");

	}

	return 0;
}



