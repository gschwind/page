/*
 * icon_handler.hxx
 *
 *  Created on: 20 sept. 2014
 *      Author: gschwind
 */

#ifndef ICON_HANDLER_HXX_
#define ICON_HANDLER_HXX_

#include <cairo/cairo.h>

#include <limits>

#include "client_base.hxx"

namespace page {

using namespace std;

template<unsigned const WIDTH, unsigned const HEIGHT>
class icon_handler_t {

	cairo_surface_t * icon_surf;

	struct _icon_ref_t {
		int width;
		int height;
		uint32_t const * data;
	};

public:
	icon_handler_t(client_base_t * c) {
		icon_surf = nullptr;

		vector<uint32_t> const * net_wm_icon = c->net_wm_icon();
		/* if window have icon properties */
		if (net_wm_icon != nullptr) {

			vector<_icon_ref_t> icons;

			/* find all icons in net_wm_icon */
			for(unsigned offset = 0; offset+2 < net_wm_icon->size();) {
				_icon_ref_t tmp;
				tmp.width = (*net_wm_icon)[offset + 0];
				tmp.height = (*net_wm_icon)[offset + 1];
				tmp.data = &((*net_wm_icon)[offset + 2]);
				offset += 2 + tmp.width * tmp.height;

				if(offset <= net_wm_icon->size() and tmp.width > 0 and tmp.height > 0)
					icons.push_back(tmp);
				else
					break;
			}

			_icon_ref_t selected;
			selected.width = std::numeric_limits<int>::max();
			selected.height = std::numeric_limits<int>::max();
			selected.data = nullptr;

			/* find the smallest icon that is greater than desired size */
			for(auto &i: icons) {
				if (i.width >= WIDTH and i.height >= HEIGHT
						and selected.width > i.width and selected.height > i.height) {
					selected = i;
				}
			}

			/** look for the greatest icons if no icon already matched **/
			if(selected.data == nullptr) {
				selected.width = std::numeric_limits<int>::min();
				selected.height = std::numeric_limits<int>::min();
				selected.data = nullptr;
				/* find the greatest icon */
				for(auto &i: icons) {
					if (i.width >= selected.width and i.height >= selected.height) {
						selected = i;
					}
				}
			}

			if (selected.data != nullptr) {

				/** convert possibly 64 bit data to 32 bit **/
				std::vector<uint32_t> data(selected.width*selected.height);
				copy(&selected.data[0], &selected.data[selected.width*selected.height], data.begin());

				/** TODO: make X11 surfaces **/
				icon_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
						WIDTH, HEIGHT);

				int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
						selected.width);
				cairo_surface_t * tmp =
						cairo_image_surface_create_for_data(
								reinterpret_cast<unsigned char *>(&data[0]),
								CAIRO_FORMAT_ARGB32, selected.width,
								selected.height, stride);

				double x_ratio = WIDTH / (double)selected.width;
				double y_ratio = HEIGHT / (double)selected.height;

				cairo_t * cr = cairo_create(icon_surf);

				cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
				cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
				cairo_paint(cr);

				cairo_scale(cr, x_ratio, y_ratio);
				cairo_set_source_surface(cr, tmp, 0.0, 0.0);
				cairo_rectangle(cr, 0, 0, selected.width, selected.height);
				cairo_pattern_set_filter(cairo_get_source(cr),
						CAIRO_FILTER_NEAREST);
				cairo_fill(cr);

				warn(cairo_get_reference_count(cr) == 1);
				cairo_destroy(cr);
				warn(cairo_surface_get_reference_count(tmp) == 1);
				cairo_surface_destroy(tmp);

			}
		}
	}

	~icon_handler_t() {
		if (icon_surf != nullptr) {
			warn(cairo_surface_get_reference_count(icon_surf) == 1);
			cairo_surface_destroy(icon_surf);
			icon_surf = nullptr;
		}
	}

	cairo_surface_t * get_cairo_surface() const {
		return icon_surf;
	}



};

using icon16 = icon_handler_t<16,16>;
using icon64 = icon_handler_t<64,64>;


}



#endif /* ICON_HANDLER_HXX_ */
