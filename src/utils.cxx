/*
 * utils.cxx
 *
 *  Created on: 23 août 2015
 *      Author: gschwind
 */

#include "utils.hxx"

namespace page {

uint32_t g_log_flags = 0u;

void log(log_module_e module, char const * fmt, ...) {
	if (module == LOG_NONE or (module&g_log_flags)) {
		va_list l;
		va_start(l, fmt);
		vprintf(fmt, l);
		va_end(l);
	}
}

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

uint64_t xcb_sync_system_counter_int64_swap(xcb_sync_int64_t const * v)
{
	return ((uint64_t) v->hi << 32) + (uint64_t) v->lo;
}

int xcb_sync_system_counter_sizeof_item(
		fixed_xcb_sync_systemcounter_t const * item)
{
	unsigned length = item->name_len;
	/* padding apply to packed struct fixed_xcb_sync_systemcounter_t + length */
	return (((sizeof(struct fixed_xcb_sync_systemcounter_t) + length) >> 2) << 2)
			+ 4;
}

char * xcb_sync_system_counter_dup_name(
		fixed_xcb_sync_systemcounter_t const * entry)
{
	char * name = (char*) (entry + 1);
	int length = entry->name_len;
	char * ret = (char*) malloc(sizeof(char) * (length + 1));
	memcpy(ret, name, length);
	ret[length] = 0;
	return ret;
}

bool exists(char const * name)
{
	struct stat buf;
	int status = stat(name, &buf);
	if(status != 0)
		return false;
	return true;
}


}

