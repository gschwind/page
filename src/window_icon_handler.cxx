/*
 * window_icon_handler.cxx
 *
 *  Created on: 15 févr. 2013
 *      Author: gschwind
 */

#include <stdint.h>
#include "window_icon_handler.hxx"

namespace page {

window_icon_handler_t::window_icon_handler_t(window_t * w) {

	icon_surf = 0;
	icon.data = 0;

	/* get icon properties */
	int icon_data_size = 0;
	long const * icon_data = 0;

	w->get_icon_data(icon_data, icon_data_size);

	/* if window have icon properties */
	if (icon_data != 0) {
		int32_t * icon_data32 = 0;

		icon_t selected;
		std::list<struct icon_t> icons;
		bool has_icon = false;

		/* copy long to 32 bits int, this is needed for 64bits arch (recall: long in 64 bits arch are 64 bits)*/
		icon_data32 = new int32_t[icon_data_size];
		for (unsigned int i = 0; i < icon_data_size; ++i)
			icon_data32[i] = icon_data[i];

		/* find all icons */
		unsigned int offset = 0;
		while (offset < icon_data_size) {
			icon_t tmp;
			tmp.width = icon_data[offset + 0];
			tmp.height = icon_data[offset + 1];
			tmp.data = (unsigned char *) &icon_data32[offset + 2];
			offset += 2 + tmp.width * tmp.height;
			icons.push_back(tmp);
		}

		/* find then greater and nearest icon to show (16x16) */
		icon_t ic;
		int x = 0;
		/* find an icon */
		std::list<icon_t>::iterator i = icons.begin();
		while (i != icons.end()) {
			if ((*i).width <= 16 && x < (*i).width) {
				x = (*i).width;
				ic = (*i);
				has_icon = true;
			}
			++i;
		}

		if (has_icon) {
			selected = ic;
		} else {
			if (!icons.empty()) {
				selected = icons.front();
				has_icon = true;
			} else {
				has_icon = false;
			}

		}

		/* if we found icon we scale it to 16x16, with nearest method */
		if (has_icon) {

			int target_height;
			int target_width;
			double ratio;

			if (selected.width > selected.height) {
				target_width = 16;
				target_height = (double) selected.height
						/ (double) selected.width * 16;
				ratio = (double) target_width / (double) selected.width;
			} else {
				target_height = 16;
				target_width = (double) selected.width
						/ (double) selected.height * 16;
				ratio = (double) target_height / (double) selected.height;
			}

			int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
					target_width);

//			printf("reformat from %dx%d to %dx%d %f\n", selected.width,
//					selected.height, target_width, target_height, ratio);

			icon.width = target_width;
			icon.height = target_height;
			icon.data = (unsigned char *) malloc(stride * target_height);

			for (int j = 0; j < target_height; ++j) {
				for (int i = 0; i < target_width; ++i) {
					int x = i / ratio;
					int y = j / ratio;
					if (x < selected.width && x >= 0 && y < selected.height
							&& y >= 0) {
						((uint32_t *) (icon.data + stride * j))[i] =
								((uint32_t*) selected.data)[x
										+ selected.width * y];
					}
				}
			}

			icon_surf = cairo_image_surface_create_for_data(
					(unsigned char *) icon.data, CAIRO_FORMAT_ARGB32,
					icon.width, icon.height, stride);

		} else {
			icon_surf = 0;
		}

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
}

cairo_surface_t * window_icon_handler_t::get_cairo_surface() {
	return icon_surf;
}

}
