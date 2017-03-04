/*
 * renderable_surface.hxx
 *
 *  Created on: 23 sept. 2014
 *      Author: gschwind
 */

#ifndef RENDERABLE_NOTEBOOK_FADING_HXX_
#define RENDERABLE_NOTEBOOK_FADING_HXX_

#include <cairo.h>

#include <memory>

#include "tree.hxx"
#include "utils.hxx"
#include "renderable.hxx"
#include "pixmap.hxx"
#include "mainloop.hxx"
#include "page-types.hxx"

namespace page {

using namespace std;

class renderable_notebook_fading_t : public tree_t {
	page_t * _ctx;

	double _ratio;
	rect _location;
	shared_ptr<pixmap_t> _surface;
	region _damaged;
	region _opaque_region;

public:

	renderable_notebook_fading_t(tree_t * ctx, shared_ptr<pixmap_t> surface, int x, int y);
	virtual ~renderable_notebook_fading_t();
	virtual void render(cairo_t * cr, region const & area);
	void set_ratio(double x);
	virtual region get_opaque_region();
	virtual region get_visible_region();
	virtual region get_damaged();
	virtual void render_finished();
	void set_opaque_region(region const & r);
	void move(int x, int y);
	void update_pixmap(shared_ptr<pixmap_t> surface, int x, int y);
	double ratio();
	shared_ptr<pixmap_t> surface();

};



}



#endif /* RENDERABLE_SURFACE_HXX_ */
