/*
 * Copyright (2017) Benoit Gschwind
 *
 * icon_handler.cxx is part of page-compositor.
 *
 * page-compositor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * page-compositor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with page-compositor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "icon_handler.hxx"

#include <vector>

#include "client_managed.hxx"

namespace page {

using namespace std;

template<unsigned const WIDTH, unsigned const HEIGHT>
icon_handler_t<WIDTH, HEIGHT>::icon_handler_t(client_managed_t * c)
{
	icon_surf = nullptr;

	auto net_wm_icon = c->get<p_net_wm_icon>();
	/* if window have icon properties */
	if (net_wm_icon != nullptr) {

		vector<_icon_ref_t> icons;

		/* find all icons in net_wm_icon */
		for (unsigned offset = 0; offset + 2 < net_wm_icon->size();) {
			_icon_ref_t tmp;
			tmp.width = (*net_wm_icon)[offset + 0];
			tmp.height = (*net_wm_icon)[offset + 1];
			tmp.data = &((*net_wm_icon)[offset + 2]);
			offset += 2 + tmp.width * tmp.height;

			if (offset <= net_wm_icon->size() and tmp.width > 0
					and tmp.height > 0)
				icons.push_back(tmp);
			else
				break;
		}

		_icon_ref_t selected;
		selected.width = std::numeric_limits<int>::max();
		selected.height = std::numeric_limits<int>::max();
		selected.data = nullptr;

		/* find the smallest icon that is greater than desired size */
		for (auto &i : icons) {
			if (i.width >= WIDTH and i.height >= HEIGHT
					and selected.width > i.width
					and selected.height > i.height) {
				selected = i;
			}
		}

		/** look for the greatest icons if no icon already matched **/
		if (selected.data == nullptr) {
			selected.width = std::numeric_limits<int>::min();
			selected.height = std::numeric_limits<int>::min();
			selected.data = nullptr;
			/* find the greatest icon */
			for (auto &i : icons) {
				if (i.width >= selected.width and i.height >= selected.height) {
					selected = i;
				}
			}
		}

		if (selected.data != nullptr) {

			/** convert possibly 64 bit data to 32 bit **/
			std::vector<uint32_t> data(selected.width * selected.height);
			copy(&selected.data[0],
					&selected.data[selected.width * selected.height],
					data.begin());

			/** TODO: make X11 surfaces **/
			icon_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH,
					HEIGHT);

			int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
					selected.width);
			cairo_surface_t * tmp = cairo_image_surface_create_for_data(
					reinterpret_cast<unsigned char *>(&data[0]),
					CAIRO_FORMAT_ARGB32, selected.width, selected.height,
					stride);

			double x_ratio = WIDTH / (double) selected.width;
			double y_ratio = HEIGHT / (double) selected.height;

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

template<unsigned const WIDTH, unsigned const HEIGHT>
icon_handler_t<WIDTH, HEIGHT>::~icon_handler_t()
{
	if (icon_surf != nullptr) {
		warn(cairo_surface_get_reference_count(icon_surf) == 1);
		cairo_surface_destroy(icon_surf);
		icon_surf = nullptr;
	}
}

template<unsigned const WIDTH, unsigned const HEIGHT>
cairo_surface_t * icon_handler_t<WIDTH, HEIGHT>::get_cairo_surface() const
{
	return icon_surf;
}

template struct icon_handler_t<16, 16> ;
template struct icon_handler_t<64, 64> ;

using icon16 = icon_handler_t<16,16>;
using icon64 = icon_handler_t<64,64>;

}

