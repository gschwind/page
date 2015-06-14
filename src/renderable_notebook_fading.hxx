/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_NOTEBOOK_FADING_HXX_
#define RENDERABLE_NOTEBOOK_FADING_HXX_

#include <cairo/cairo.h>

#include <memory>

#include "utils.hxx"
#include "renderable.hxx"
#include "pixmap.hxx"

namespace page {

class renderable_notebook_fading_t : public renderable_t {

	double ratio;

	i_rect old_client_area;
	std::shared_ptr<pixmap_t> old_surface;

	i_rect client_area;
	i_rect client_pos;
	std::shared_ptr<pixmap_t> client_surf;

public:

	renderable_notebook_fading_t(std::shared_ptr<pixmap_t> old_surface, i_rect old_client_area) :
		old_client_area(old_client_area),
		old_surface(old_surface),
		ratio(0.5) {

	}

	virtual ~renderable_notebook_fading_t() {

	}

	/**
	 * draw the area of a renderable to the destination surface
	 * @param cr the destination surface context
	 * @param area the area to redraw
	 **/
	virtual void render(cairo_t * cr, region const & area) {

		cairo_surface_t * target = cairo_get_target(cr);



		cairo_save(cr);
		if (client_surf != nullptr) {
			cairo_clip(cr, client_pos);
			cairo_set_source_surface(cr, client_surf->get_cairo_surface(), client_pos.x, client_pos.y);
			cairo_mask_surface(cr, client_surf->get_cairo_surface(), client_pos.x, client_pos.y);
		}
		cairo_restore(cr);

		cairo_save(cr);
		for (auto &cl : area) {
			/* client old surface if necessary */
			i_rect tmp_pos;
			tmp_pos.x = client_area.x;
			tmp_pos.y = client_area.y;
			tmp_pos.w = std::min(old_client_area.w, client_area.w);
			tmp_pos.h = std::min(old_client_area.h, client_area.h);
			cairo_clip(cr, tmp_pos);
			cairo_pattern_t * p0 = cairo_pattern_create_rgba(1.0, 1.0, 1.0, 1.0-ratio);
			cairo_set_source_surface(cr, old_surface->get_cairo_surface(), tmp_pos.x, tmp_pos.y);
			cairo_mask(cr, p0);
			cairo_pattern_destroy(p0);
		}
		cairo_restore(cr);

	}

	/**
	 * Derived class must return opaque region for this object,
	 * If unknown it's safe to leave this empty.
	 **/
	virtual region get_opaque_region() {
		return region();
	}

	/**
	 * Derived class must return visible region,
	 * If unknow the whole screen can be returned, but draw will be called each time.
	 **/
	virtual region get_visible_region() {
		return region(old_client_area);
	}

	virtual region get_damaged() {
		return region(old_client_area);
	}

	void set_ratio(double x) {
		ratio = std::min(std::max(0.0, x), 1.0);
	}

	void update_client_area(i_rect const & pos) {
		client_area = pos;
	}

	void update_client(std::shared_ptr<pixmap_t> const & surf, i_rect const & pos) {
		client_pos = pos;
		client_surf = surf;
	}

};



}



#endif /* RENDERABLE_SURFACE_HXX_ */
