/*
 * renderable_window.hxx
 *
 *  Created on: 23 mars 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_WINDOW_HXX_
#define RENDERABLE_WINDOW_HXX_

#include "renderable.hxx"

namespace page {

#define CHECK_CAIRO(x) do { \
	x;\
	print_cairo_status(cr, __FILE__, __LINE__); \
} while(false)

class renderable_window_t : public renderable_t {
	composite_surface_t * _surf;
	rectangle _position;

public:

	/**
	 * renderable default constructor.
	 */
	renderable_window_t(composite_surface_t * surf, rectangle const & position) :
			_surf(surf), _position(position) {
	}

	/**
	 * Destroy renderable
	 */
	virtual ~renderable_window_t() {

	}

	/**
	 * this is called before the render to update all surface positions
	 **/
	virtual void prepare_render(time_t time) {

	}
	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 */
	virtual void render(cairo_t * cr, region const & area) {

		if (_surf == 0)
			return;

		region::const_iterator i = area.begin();

		while (i != area.end()) {
			rectangle clip = *i
					& rectangle(_position.x, _position.y, _position.w,
							_position.h);

			if (clip.w > 0 && clip.h > 0) {
				CHECK_CAIRO(cairo_save(cr));
				CHECK_CAIRO(cairo_reset_clip(cr));
				CHECK_CAIRO(cairo_identity_matrix(cr));
				CHECK_CAIRO(
						cairo_rectangle(cr, clip.x, clip.y, clip.w, clip.h));
				CHECK_CAIRO(cairo_clip(cr));

				CHECK_CAIRO(
						cairo_set_source_surface(cr, _surf->get_surf(),
								_position.x, _position.y));
				CHECK_CAIRO(cairo_paint(cr));

				CHECK_CAIRO(cairo_restore(cr));
			}
			++i;
		}

	}

	/**
	 * to avoid too much memory copy compositor need to know opac/not opac area
	 **/
	virtual region opac_area() {
		return region();
	}

	virtual region full_area() {
		return region(_position);
	}

};

}

#endif /* RENDERABLE_WINDOW_HXX_ */
