/*
 * compositor_overlay.hxx
 *
 *  Created on: 9 août 2015
 *      Author: gschwind
 */

#ifndef SRC_COMPOSITOR_OVERLAY_HXX_
#define SRC_COMPOSITOR_OVERLAY_HXX_

#include "config.hxx"

#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <cairo-xcb.h>

#include "page_context.hxx"
#include "region.hxx"
#include "tree.hxx"

namespace page {


struct compositor_overlay_t : public tree_t {
	page_context_t * _ctx;

	PangoFontDescription * _fps_font_desc;
	PangoFontMap * _fps_font_map;
	PangoContext * _fps_context;

	shared_ptr<pixmap_t> _back_surf;
	rect _position;

	bool _has_damage;

public:

	compositor_overlay_t(page_context_t * ctx, rect const & pos);
	~compositor_overlay_t();

	virtual region get_opaque_region();
	virtual region get_visible_region();
	virtual region get_damaged();

	void show();
	void hide();
	void update_layout(time64_t const t);
	void _update_back_buffer();
	virtual void render(cairo_t * cr, region const & area) override;
	void pango_printf(cairo_t * cr, double x, double y,
			char const * fmt, ...);

};



}



#endif /* SRC_COMPOSITOR_OVERLAY_HXX_ */
