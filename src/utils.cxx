/*
 * utils.cxx
 *
 *  Created on: 23 août 2015
 *      Author: gschwind
 */

#include "utils.hxx"

namespace page {

/**
 * Draw rectangle with all corner rounded
 **/
void cairo_rectangle_arc_corner(cairo_t * cr, double x, double y, double w, double h, double radius, uint8_t corner_mask) {

	if(radius * 2 > w) {
		radius = w / 2.0;
	}

	if(radius * 2 > h) {
		radius = h / 2.0;
	}

	cairo_new_path(cr);
	cairo_move_to(cr, x, y + h - radius);
	cairo_line_to(cr, x, y + radius);
	/* top-left */
	if(corner_mask&CAIRO_CORNER_TOPLEFT) {
		cairo_arc(cr, x + radius, y + radius, radius, 2.0 * M_PI_2, 3.0 * M_PI_2);
	} else {
		cairo_line_to(cr, x, y);
	}
	cairo_line_to(cr, x + w - radius, y);
	/* top-right */
	if(corner_mask&CAIRO_CORNER_TOPRIGHT) {
		cairo_arc(cr, x + w - radius, y + radius, radius, 3.0 * M_PI_2, 4.0 * M_PI_2);
	} else {
		cairo_line_to(cr, x + w, y);
	}
	cairo_line_to(cr, x + w, y + h - radius);
	/* bot-right */
	if(corner_mask&CAIRO_CORNER_BOTRIGHT) {
		cairo_arc(cr, x + w - radius, y + h - radius, radius, 0.0 * M_PI_2, 1.0 * M_PI_2);
	} else {
		cairo_line_to(cr, x + w, y + h);
	}
	cairo_line_to(cr, x + radius, y + h);
	/* bot-left */
	if(corner_mask&CAIRO_CORNER_BOTLEFT) {
		cairo_arc(cr, x + radius, y + h - radius, radius, 1.0 * M_PI_2, 2.0 * M_PI_2);
	} else {
		cairo_line_to(cr, x, y + h);
	}
	cairo_close_path(cr);

}

void cairo_rectangle_arc_corner(cairo_t * cr, rect const & position, double radius, uint8_t corner_mask) {
	cairo_rectangle_arc_corner(cr, position.x, position.y, position.w, position.h, radius, corner_mask);
}

}

