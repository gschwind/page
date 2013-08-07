/*
 * window_icon_handler.cxx
 *
 *  Created on: 15 févr. 2013
 *      Author: gschwind
 */

#include <stdint.h>

#include <limits>
#include <cairo.h>
#include <cairo-xlib.h>

#include "window_icon_handler.hxx"

namespace page {

window_icon_handler_t::window_icon_handler_t(xconnection_t * cnx, Window w, unsigned int xsize, unsigned int ysize) {

	_cnx = cnx;
	icon_surf = 0;
	icon.data = 0;
	_px = None;

	vector<long> icon_data;
	/* if window have icon properties */
	if (cnx->read_net_wm_icon(w, &icon_data)) {
		int32_t * icon_data32 = 0;

		icon_t selected;
		std::list<struct icon_t> icons;
		bool has_icon = false;

		/* copy long to 32 bits int, this is needed for 64bits arch (recall: long in 64 bits arch are 64 bits)*/
		icon_data32 = new int32_t[icon_data.size()];
		for (unsigned int i = 0; i < icon_data.size(); ++i)
			icon_data32[i] = icon_data[i];

		/* find all icons and set up an handler */
		unsigned int offset = 0;
		while (offset < icon_data.size()) {
			icon_t tmp;
			tmp.width = icon_data[offset + 0];
			tmp.height = icon_data[offset + 1];
			tmp.data = (unsigned char *) &icon_data32[offset + 2];
			offset += 2 + tmp.width * tmp.height;
			icons.push_back(tmp);
		}

		/* find the smaller icon that is greater than desired size */
		icon_t ic;
		int x = std::numeric_limits<int>::max();
		int y = std::numeric_limits<int>::max();
		/* find an icon */
		std::list<icon_t>::iterator i = icons.begin();
		while (i != icons.end()) {
			if ((*i).width >= (int)xsize and (*i).height >= (int)ysize and x > (*i).width
					and y > (*i).width) {
				x = (*i).width;
				y = (*i).height;
				ic = (*i);
				has_icon = true;
			}
			++i;
		}

		if (has_icon) {
			selected = ic;
		} else {
			/** if no usefull icon are found, just use the last one (often the big one) **/
			if (!icons.empty()) {
				selected = icons.back();
				has_icon = true;
			} else {
				has_icon = false;
			}

		}

		if (has_icon) {

			XVisualInfo vinfo;
			if (XMatchVisualInfo(cnx->dpy, cnx->screen(), 32, TrueColor, &vinfo)
					== 0) {
				throw std::runtime_error(
						"Unable to find valid visual for background windows");
			}

			_px = XCreatePixmap(cnx->dpy, cnx->get_root_window(), xsize,
					ysize, 32);

			icon_surf = cairo_xlib_surface_create(cnx->dpy, _px, vinfo.visual,
					xsize, ysize);

			int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
					selected.width);
			cairo_surface_t * tmp = cairo_image_surface_create_for_data(
					selected.data, CAIRO_FORMAT_ARGB32,
					selected.width, selected.height, stride);

			double x_ratio = xsize / (double)selected.width;
			double y_ratio = ysize / (double)selected.height;

			cairo_t * cr = cairo_create(icon_surf);

			cairo_scale(cr, x_ratio, y_ratio);
			cairo_set_source_surface(cr, tmp, 0.0, 0.0);
			cairo_rectangle(cr, 0, 0, selected.width, selected.height);

			cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BEST);
			cairo_fill(cr);

			cairo_destroy(cr);
			cairo_surface_destroy(tmp);

		} else {
			icon_surf = 0;
		}


//		/* if we found icon we scale it to 16x16, with nearest method */
//		if (has_icon) {
//
//			int target_height;
//			int target_width;
//			double ratio;
//
//			if (selected.width > selected.height) {
//				target_width = 16;
//				target_height = (double) selected.height
//						/ (double) selected.width * 16;
//				ratio = (double) target_width / (double) selected.width;
//			} else {
//				target_height = 16;
//				target_width = (double) selected.width
//						/ (double) selected.height * 16;
//				ratio = (double) target_height / (double) selected.height;
//			}
//
//			int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
//					target_width);
//
////			printf("reformat from %dx%d to %dx%d %f\n", selected.width,
////					selected.height, target_width, target_height, ratio);
//
//			icon.width = target_width;
//			icon.height = target_height;
//			icon.data = (unsigned char *) malloc(stride * target_height);
//
//			for (int j = 0; j < target_height; ++j) {
//				for (int i = 0; i < target_width; ++i) {
//					int x = i / ratio;
//					int y = j / ratio;
//					if (x < selected.width && x >= 0 && y < selected.height
//							&& y >= 0) {
//						((uint32_t *) (icon.data + stride * j))[i] =
//								((uint32_t*) selected.data)[x
//										+ selected.width * y];
//					}
//				}
//			}
//
//			icon_surf = cairo_image_surface_create_for_data(
//					(unsigned char *) icon.data, CAIRO_FORMAT_ARGB32,
//					icon.width, icon.height, stride);
//
//		} else {
//			icon_surf = 0;
//		}

		delete[] icon_data32;

	} else {
		icon_surf = 0;
	}

}

window_icon_handler_t::~window_icon_handler_t() {
	if (icon_surf != 0) {
		cairo_surface_destroy(icon_surf);
		icon_surf = 0;
	}

	if (icon.data != 0) {
		free(icon.data);
		icon.data = 0;
	}

	if(_px != None) {
		XFreePixmap(_cnx->dpy, _px);
	}
}

cairo_surface_t * window_icon_handler_t::get_cairo_surface() {
	return icon_surf;
}

}
